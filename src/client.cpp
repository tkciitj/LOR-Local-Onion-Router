#include<iostream>
#include<winsock2.h>
#include<ws2tcpip.h>
#include<fstream>
//#include"../include/crypto.h"
#include"../include/aes_gcm.h"
#include"../include/event_logger.h"

using namespace std;

#pragma comment(lib, "ws2_32.lib")
#define RELAY_PORT1 5001
#define RELAY_PORT2 5002
#define RELAY_PORT3 5003
#define BUFFER_SIZE 4096
#define SERVER_PORT 5000
//#define XOR_KEY 0x5A

//outer layer
const uint8_t KEY_RELAY1[32] = {
    0x10,0x21,0x32,0x43,0x54,0x65,0x76,0x87,
    0x98,0xA9,0xBA,0xCB,0xDC,0xED,0xFE,0x0F,
    0x1E,0x2D,0x3C,0x4B,0x5A,0x69,0x78,0x87,
    0x96,0xA5,0xB4,0xC3,0xD2,0xE1,0xF0,0x11
};

// middle layer
const uint8_t KEY_RELAY2[32] = {
    0x22,0x44,0x66,0x88,0xAA,0xCC,0xEE,0x00,
    0x11,0x33,0x55,0x77,0x99,0xBB,0xDD,0xFF,
    0xF1,0xE2,0xD3,0xC4,0xB5,0xA6,0x97,0x88,
    0x79,0x6A,0x5B,0x4C,0x3D,0x2E,0x1F,0x00
};

// innermost layer
const uint8_t KEY_RELAY3[32] = {
    0xA1,0xB2,0xC3,0xD4,0xE5,0xF6,0x07,0x18,
    0x29,0x3A,0x4B,0x5C,0x6D,0x7E,0x8F,0x90,
    0x9F,0x8E,0x7D,0x6C,0x5B,0x4A,0x39,0x28,
    0x17,0x06,0xF5,0xE4,0xD3,0xC2,0xB1,0xA0
};

int send_all(SOCKET sock, const char* buf, int len) {
    int total = 0;
    while (total < len) {
        int s = send(sock, buf + total, len - total, 0);
        if (s <= 0) return s;   // error or connection closed
        total += s;
    }
    return total; // bytes sent == len
}

std::vector<uint8_t> serialize_block(const AESGCMBlock& b) {
    std::vector<uint8_t> out;

    uint32_t ivSz = b.iv.size();
    uint32_t tagSz = b.tag.size();
    uint32_t ctSz = b.ciphertext.size();

    out.insert(out.end(), (uint8_t*)&ivSz, (uint8_t*)&ivSz + sizeof(ivSz));
    out.insert(out.end(), (uint8_t*)&tagSz, (uint8_t*)&tagSz + sizeof(tagSz));
    out.insert(out.end(), (uint8_t*)&ctSz, (uint8_t*)&ctSz + sizeof(ctSz));

    out.insert(out.end(), b.iv.begin(), b.iv.end());
    out.insert(out.end(), b.tag.begin(), b.tag.end());
    out.insert(out.end(), b.ciphertext.begin(), b.ciphertext.end());

    return out;
}


int main(){
    WSADATA wsa;
    SOCKET sock;
    sockaddr_in relayAddr;

    // initializing winsock
    WSAStartup(MAKEWORD(2,2), &wsa);

    //creating the socket
    sock=socket(AF_INET, SOCK_STREAM, 0); // SOCK_STREAM for TCP conn

    //defining server address
    relayAddr.sin_family=AF_INET;
    relayAddr.sin_port=htons(RELAY_PORT1);
    relayAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // localhost loopback address

    //connecting to server
    connect(sock, (struct sockaddr*)&relayAddr, sizeof(relayAddr));

    //openening file in binary mode 
    ifstream file("test.txt", ios::binary);
    file.seekg(0, ios::end);
    uint64_t fileSZ = file.tellg();
    file.seekg(0, ios::beg);

    log_event("client", "file_loaded");
    //sending file size first
    //send(sock, (char*)&fileSZ, sizeof(fileSZ), 0); // 0 flag for no special options
    //sending file data
    char buffer[BUFFER_SIZE];

// while(file.read(buffer, BUFFER_SIZE) || file.gcount()>0) {
//     int bytesRead = static_cast<int>(file.gcount());
//     // Encrypting the read bytes using AES-GCM
//     std::vector<uint8_t> plaintext(buffer, buffer + bytesRead);
//     AESGCMBlock enc = aes_gcm_encrypt(plaintext, AES_KEY);

//     uint32_t ivSz  = enc.iv.size();
//     uint32_t tagSz = enc.tag.size();
//     uint32_t ctSz  = enc.ciphertext.size();

//     send(sock, (char*)&ivSz,  sizeof(ivSz),  0);
//     send(sock, (char*)&tagSz, sizeof(tagSz), 0);
//     send(sock, (char*)&ctSz,  sizeof(ctSz),  0);

//     send(sock, (char*)enc.iv.data(),  ivSz,  0);
//     send(sock, (char*)enc.tag.data(), tagSz, 0);
//     send(sock, (char*)enc.ciphertext.data(), ctSz, 0);


//     // Encrypting only the bytes read
//     //XORCrypt(buffer, bytesRead, XOR_KEY);

//     // Ensuring ALL bytes are sent
//     // int totalSent=0;
//     // while (totalSent<bytesRead) {
//     //     int sent=send(sock,
//     //                     buffer + totalSent,
//     //                     bytesRead - totalSent,
//     //                     0);
//     //     if (sent<=0) {
//     //         std::cerr<<"Send failed\n";
//     //         return 1;
//     //     }
//     //     totalSent+=sent;
//     // }
// }
    std::vector<uint8_t> plaintext(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    // for innermost layer i.e. relay node 3
    AESGCMBlock L3 = aes_gcm_encrypt(plaintext, KEY_RELAY3);
    std::vector<uint8_t> blob3 = serialize_block(L3);

    // for middle layer i.e. relay node 2
    AESGCMBlock L2 = aes_gcm_encrypt(blob3, KEY_RELAY2);
    std::vector<uint8_t> blob2 = serialize_block(L2);

    // for outermost layer i.e. relay node 1
    AESGCMBlock L1 = aes_gcm_encrypt(blob2, KEY_RELAY1);
    std::vector<uint8_t> onion = serialize_block(L1);

    log_event("client", "onion_encryption_start");
    uint32_t onionSz = onion.size();
    send_all(sock, (char*)&onionSz, sizeof(onionSz));
    send_all(sock, (char*)onion.data(), onionSz);

    cout<<"Encrypted Onion sent successfully."<<endl;
    log_event("client", "encrypted_blob_sent");
    //closing the socket and file
    file.close();
    closesocket(sock);
    WSACleanup();

    return 0;
}