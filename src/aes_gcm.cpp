#include "../include/aes_gcm.h"
#include <openssl/evp.h>   //has high level cryptographic functions //envelope interface
#include <openssl/rand.h>

AESGCMBlock aes_gcm_encrypt(
    const std:: vector<uint8_t>& plaintext,
    const uint8_t key[32]
) {
    AESGCMBlock out;
    out.iv.resize(12);
    out.tag.resize(16);
    out.ciphertext.resize(plaintext.size());

    RAND_bytes(out.iv.data(), out.iv.size()); //key generation with random IV

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();  //creates and initializes a cipher context
    int len=0;

    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, out.iv.size(), nullptr);
    EVP_EncryptInit_ex(ctx, nullptr, nullptr, key, out.iv.data());

    EVP_EncryptUpdate(ctx, out.ciphertext.data(), &len, plaintext.data(), plaintext.size()); // encryption //len holds number of bytes written
    EVP_EncryptFinal_ex(ctx, NULL, &len);

    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, out.tag.size(), out.tag.data());  //extracts the tag
    EVP_CIPHER_CTX_free(ctx); //frees the cipher context

    return out; //returning the encrypted block of file chunks
}

bool aes_gcm_decrypt(
    const AESGCMBlock& block, //encrypted block having iv, ciphertext and tag for authentication
    const uint8_t key[32],   //same symm key
    std::vector<uint8_t>& plaintext_out
) {
    plaintext_out.resize(block.ciphertext.size());

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    int len=0;
    int ret=0;

    EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, block.iv.size(), nullptr);
    EVP_DecryptInit_ex(ctx, nullptr, nullptr, key, block.iv.data());

    EVP_DecryptUpdate(ctx, plaintext_out.data(), &len, block.ciphertext.data(), block.ciphertext.size());  //actual decryption

    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, block.tag.size(), (void*)block.tag.data());  //supplies the tag for verification

    ret = EVP_DecryptFinal_ex(ctx, nullptr, &len);

    EVP_CIPHER_CTX_free(ctx);

    return ret > 0; //authentication successful if ret>0
}


