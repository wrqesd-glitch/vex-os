#ifndef VEX_BOOT_TWEETNACL_H
#define VEX_BOOT_TWEETNACL_H

int crypto_sign_ed25519_tweet_open(
    unsigned char* message,
    unsigned long long* message_length,
    const unsigned char* signed_message,
    unsigned long long signed_length,
    const unsigned char* public_key
);

#endif
