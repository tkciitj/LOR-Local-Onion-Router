#pragma once
#include <vector>
#include <cstdint>

//using namespace std;

struct AESGCMBlock {
    std::vector<uint8_t> iv;  //initialization vector  // 12 bytes
    std::vector<uint8_t> ciphertext;
    std::vector<uint8_t> tag; //authentication tag  //16 bytes
};

AESGCMBlock aes_gcm_encrypt(
    const std:: vector<uint8_t>& plaintext,
    const uint8_t key[32]
);

bool aes_gcm_decrypt(
    const AESGCMBlock& block,
    const uint8_t key[32],
    std::vector<uint8_t>& plaintext_out
);

