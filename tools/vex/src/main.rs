use ed25519_dalek::{Signature, Signer, SigningKey, Verifier, VerifyingKey};
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::env;
use std::fs;
use std::io::Write;
use std::path::{Path, PathBuf};
use std::process::{Command, Stdio};

const PACKAGE_MAGIC: &[u8; 8] = b"VEXPKG2\0";
const PACKAGE_FORMAT_VERSION: u32 = 2;
const SIGNING_CONTEXT: &[u8; 8] = b"VEXSIG1\0";
const SIGNATURE_LEN: usize = 64;

#[derive(Clone, Debug, Deserialize, Serialize)]
struct Manifest {
    name: String,
    version: String,
    abi_version: u32,
    entrypoint: String,
    permissions: Vec<String>,
    required_capabilities: Vec<String>,
}

#[derive(Debug, Serialize)]
struct InspectOutput {
    format_version: u32,
    manifest_hash_sha256: String,
    payload_hash_sha256: String,
    signature_hex: String,
    public_key_hex: String,
    manifest: Manifest,
}

#[derive(Clone)]
struct PackageFile {
    manifest: Manifest,
    manifest_bytes: Vec<u8>,
    payload: Vec<u8>,
    signature: [u8; SIGNATURE_LEN],
    payload_hash: [u8; 32],
    public_key: [u8; 32],
}

fn main() {
    if let Err(error) = dispatch() {
        eprintln!("{error}");
        std::process::exit(1);
    }
}

fn dispatch() -> Result<(), String> {
    let args: Vec<String> = env::args().collect();
    let command = args
        .get(1)
        .ok_or("usage: vex <build|run|package|verify|inspect> ...")?;

    match command.as_str() {
        "build" => build_command(args.get(2).cloned()),
        "run" => run_command(args.get(2).cloned()),
        "package" => package_command(&args[2..]),
        "verify" => verify_command(&args[2..]),
        "inspect" => inspect_command(&args[2..]),
        _ => Err("unknown command".to_string()),
    }
}

fn build_command(build_dir: Option<String>) -> Result<(), String> {
    let dir = build_dir.unwrap_or_else(|| "build".to_string());
    run_process(
        Command::new("cmake")
            .arg("-S")
            .arg(".")
            .arg("-B")
            .arg(&dir),
    )?;
    run_process(Command::new("cmake").arg("--build").arg(&dir))
}

fn run_command(build_dir: Option<String>) -> Result<(), String> {
    let dir = build_dir.unwrap_or_else(|| "build".to_string());
    run_process(Command::new("cmake").arg("--build").arg(&dir).arg("--target").arg("run-qemu"))
}

fn package_command(args: &[String]) -> Result<(), String> {
    let manifest_path = required_path_arg(args, "--manifest")?;
    let binary_path = required_path_arg(args, "--binary")?;
    let output_path = required_path_arg(args, "--out")?;
    let signing_key_path = required_path_arg(args, "--signing-key")?;

    let manifest = parse_manifest(&manifest_path)?;
    let manifest_bytes =
        serde_json::to_vec(&manifest).map_err(|e| format!("manifest canonicalize: {e}"))?;
    let payload = fs::read(&binary_path).map_err(|e| format!("binary read: {e}"))?;
    let payload_hash = sha256_array(&payload);

    let signing_key = read_signing_key(&signing_key_path)?;
    let verifying_key = signing_key.verifying_key();
    let signing_message = build_signing_message(&manifest_bytes, &payload);
    let signature = signing_key.sign(&signing_message).to_bytes();

    let mut output_file =
        fs::File::create(&output_path).map_err(|e| format!("output create: {e}"))?;
    output_file
        .write_all(PACKAGE_MAGIC)
        .map_err(|e| format!("write magic: {e}"))?;
    output_file
        .write_all(&PACKAGE_FORMAT_VERSION.to_le_bytes())
        .map_err(|e| format!("write version: {e}"))?;
    output_file
        .write_all(&(manifest_bytes.len() as u32).to_le_bytes())
        .map_err(|e| format!("write manifest size: {e}"))?;
    output_file
        .write_all(&(payload.len() as u64).to_le_bytes())
        .map_err(|e| format!("write payload size: {e}"))?;
    output_file
        .write_all(&(SIGNATURE_LEN as u32).to_le_bytes())
        .map_err(|e| format!("write signature size: {e}"))?;
    output_file
        .write_all(&payload_hash)
        .map_err(|e| format!("write payload hash: {e}"))?;
    output_file
        .write_all(&verifying_key.to_bytes())
        .map_err(|e| format!("write public key: {e}"))?;
    output_file
        .write_all(&manifest_bytes)
        .map_err(|e| format!("write manifest: {e}"))?;
    output_file
        .write_all(&payload)
        .map_err(|e| format!("write payload: {e}"))?;
    output_file
        .write_all(&signature)
        .map_err(|e| format!("write signature: {e}"))?;
    Ok(())
}

fn verify_command(args: &[String]) -> Result<(), String> {
    let package_path = required_path_arg(args, "--package")?;
    let package = parse_package(&package_path)?;
    validate_package(&package)
}

fn inspect_command(args: &[String]) -> Result<(), String> {
    let package_path = required_path_arg(args, "--package")?;
    let package = parse_package(&package_path)?;
    validate_package(&package)?;

    let result = InspectOutput {
        format_version: PACKAGE_FORMAT_VERSION,
        manifest_hash_sha256: hex::encode(sha256_array(&package.manifest_bytes)),
        payload_hash_sha256: hex::encode(package.payload_hash),
        signature_hex: hex::encode(package.signature),
        public_key_hex: hex::encode(package.public_key),
        manifest: package.manifest,
    };

    let stdout = serde_json::to_string_pretty(&result).map_err(|e| format!("inspect json: {e}"))?;
    println!("{stdout}");
    Ok(())
}

fn required_path_arg(args: &[String], name: &str) -> Result<PathBuf, String> {
    let mut index = 0usize;
    while index + 1 < args.len() {
        if args[index] == name {
            return Ok(PathBuf::from(&args[index + 1]));
        }
        index += 1;
    }
    Err(format!("missing {name}"))
}

fn parse_manifest(path: &Path) -> Result<Manifest, String> {
    let manifest_bytes = fs::read(path).map_err(|e| format!("manifest read: {e}"))?;
    let manifest: Manifest =
        serde_json::from_slice(&manifest_bytes).map_err(|e| format!("manifest parse: {e}"))?;
    validate_manifest(&manifest)?;
    Ok(manifest)
}

fn validate_manifest(manifest: &Manifest) -> Result<(), String> {
    if manifest.name.trim().is_empty() {
        return Err("manifest.name is empty".to_string());
    }
    if manifest.version.trim().is_empty() {
        return Err("manifest.version is empty".to_string());
    }
    if manifest.abi_version == 0 {
        return Err("manifest.abi_version must be non-zero".to_string());
    }
    if manifest.entrypoint.trim().is_empty() {
        return Err("manifest.entrypoint is empty".to_string());
    }
    if manifest.permissions.is_empty() {
        return Err("manifest.permissions is empty".to_string());
    }
    if manifest.required_capabilities.is_empty() {
        return Err("manifest.required_capabilities is empty".to_string());
    }
    Ok(())
}

fn read_signing_key(path: &Path) -> Result<SigningKey, String> {
    let raw = fs::read_to_string(path).map_err(|e| format!("signing key read: {e}"))?;
    let trimmed = raw.trim().trim_start_matches('\u{feff}');
    let key_bytes = hex::decode(trimmed).map_err(|e| format!("signing key hex: {e}"))?;
    if key_bytes.len() != 32 {
        return Err("signing key must be 32-byte hex seed".to_string());
    }

    let mut seed = [0u8; 32];
    seed.copy_from_slice(&key_bytes);
    Ok(SigningKey::from_bytes(&seed))
}

fn parse_package(path: &Path) -> Result<PackageFile, String> {
    let bytes = fs::read(path).map_err(|e| format!("package read: {e}"))?;
    let minimum_len = 8 + 4 + 4 + 8 + 4 + 32 + 32 + SIGNATURE_LEN;
    if bytes.len() < minimum_len {
        return Err("package too small".to_string());
    }

    if &bytes[0..8] != PACKAGE_MAGIC {
        return Err("bad package magic".to_string());
    }

    let format_version = read_u32(&bytes, 8)?;
    if format_version != PACKAGE_FORMAT_VERSION {
        return Err(format!("unsupported package version {format_version}"));
    }

    let manifest_len = read_u32(&bytes, 12)? as usize;
    let payload_len = read_u64(&bytes, 16)? as usize;
    let signature_len = read_u32(&bytes, 24)? as usize;
    if signature_len != SIGNATURE_LEN {
        return Err("unexpected signature length".to_string());
    }

    let mut payload_hash = [0u8; 32];
    payload_hash.copy_from_slice(slice_range(&bytes, 28, 32)?);

    let mut public_key = [0u8; 32];
    public_key.copy_from_slice(slice_range(&bytes, 60, 32)?);

    let manifest_offset = 92usize;
    let payload_offset = manifest_offset + manifest_len;
    let signature_offset = payload_offset + payload_len;
    let total_len = signature_offset + SIGNATURE_LEN;
    if bytes.len() != total_len {
        return Err("package length does not match header".to_string());
    }

    let manifest_bytes = slice_range(&bytes, manifest_offset, manifest_len)?.to_vec();
    let payload = slice_range(&bytes, payload_offset, payload_len)?.to_vec();

    let mut signature = [0u8; SIGNATURE_LEN];
    signature.copy_from_slice(slice_range(&bytes, signature_offset, SIGNATURE_LEN)?);

    let manifest: Manifest =
        serde_json::from_slice(&manifest_bytes).map_err(|e| format!("manifest decode: {e}"))?;
    validate_manifest(&manifest)?;

    Ok(PackageFile {
        manifest,
        manifest_bytes,
        payload,
        signature,
        payload_hash,
        public_key,
    })
}

fn validate_package(package: &PackageFile) -> Result<(), String> {
    let actual_payload_hash = sha256_array(&package.payload);
    if actual_payload_hash != package.payload_hash {
        return Err("payload hash mismatch".to_string());
    }

    let verifying_key = VerifyingKey::from_bytes(&package.public_key)
        .map_err(|e| format!("public key decode: {e}"))?;
    let signature = Signature::from_bytes(&package.signature);
    let signing_message = build_signing_message(&package.manifest_bytes, &package.payload);
    verifying_key
        .verify(&signing_message, &signature)
        .map_err(|e| format!("signature verify: {e}"))?;
    Ok(())
}

fn build_signing_message(manifest_bytes: &[u8], payload: &[u8]) -> Vec<u8> {
    let manifest_hash = sha256_array(manifest_bytes);
    let payload_hash = sha256_array(payload);

    let mut message = Vec::with_capacity(8 + 4 + 4 + 8 + 32 + 32);
    message.extend_from_slice(SIGNING_CONTEXT);
    message.extend_from_slice(&PACKAGE_FORMAT_VERSION.to_le_bytes());
    message.extend_from_slice(&(manifest_bytes.len() as u32).to_le_bytes());
    message.extend_from_slice(&(payload.len() as u64).to_le_bytes());
    message.extend_from_slice(&manifest_hash);
    message.extend_from_slice(&payload_hash);
    message
}

fn sha256_array(bytes: &[u8]) -> [u8; 32] {
    let digest = Sha256::digest(bytes);
    let mut out = [0u8; 32];
    out.copy_from_slice(&digest);
    out
}

fn read_u32(bytes: &[u8], offset: usize) -> Result<u32, String> {
    let mut raw = [0u8; 4];
    raw.copy_from_slice(slice_range(bytes, offset, 4)?);
    Ok(u32::from_le_bytes(raw))
}

fn read_u64(bytes: &[u8], offset: usize) -> Result<u64, String> {
    let mut raw = [0u8; 8];
    raw.copy_from_slice(slice_range(bytes, offset, 8)?);
    Ok(u64::from_le_bytes(raw))
}

fn slice_range(bytes: &[u8], offset: usize, len: usize) -> Result<&[u8], String> {
    let end = offset
        .checked_add(len)
        .ok_or("package offset overflow".to_string())?;
    bytes
        .get(offset..end)
        .ok_or("package truncated".to_string())
}

fn run_process(command: &mut Command) -> Result<(), String> {
    let status = command
        .stdin(Stdio::null())
        .stdout(Stdio::inherit())
        .stderr(Stdio::inherit())
        .status()
        .map_err(|e| format!("process start failed: {e}"))?;

    if !status.success() {
        return Err(format!("process failed with status {status}"));
    }
    Ok(())
}
