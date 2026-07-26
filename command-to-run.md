& "V:/tools/qemu/app/qemu-system-x86_64.exe" `
>>   -machine q35 `
>>   -m 256M `
>>   -cpu qemu64 `
>>   -vga std `
>>   -display sdl,show-cursor=on `
>>   -serial stdio `
>>   -monitor none `
>>   -drive if=pflash,format=raw,readonly=on,file="V:/tools/qemu/app/share/edk2-x86_64-code.fd" `
>>   -drive if=pflash,format=raw,file="V:/vex_os/build/out/ovmf-vars.fd" `
>>   -drive format=raw,file=fat:rw:"V:/vex_os/build/out/efi_root"