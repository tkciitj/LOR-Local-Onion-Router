#include<winsock2.h>
#include<ws2tcpip.h>
#include<iostream>
//#include"../include/crypto.h"
#include"../include/aes_gcm.h"
#include"../include/event_logger.h"

#pragma comment(lib, "ws2_32.lib")

#define RELAY_PORT1 5001
#define RELAY_PORT2 5002
#define RELAY_PORT3 5003
#define BUFFER_SIZE 4096
#define SERVER_PORT 5000
//#define XOR_KEY 0x5A

//outer layer
// const uint8_t KEY_RELAY1[32] = {
//     0x10,0x21,0x32,0x43,0x54,0x65,0x76,0x87,
//     0x98,0xA9,0xBA,0xCB,0xDC,0xED,0xFE,0x0F,
//     0x1E,0x2D,0x3C,0x4B,0x5A,0x69,0x78,0x87,
//     0x96,0xA5,0xB4,0xC3,0xD2,0xE1,0xF0,0x11
// };

// middle layer
const uint8_t KEY_RELAY2[32] = {
    0x22,0x44,0x66,0x88,0xAA,0xCC,0xEE,0x00,
    0x11,0x33,0x55,0x77,0x99,0xBB,0xDD,0xFF,
    0xF1,0xE2,0xD3,0xC4,0xB5,0xA6,0x97,0x88,
    0x79,0x6A,0x5B,0x4C,0x3D,0x2E,0x1F,0x00
};

// // innermost layer
// const uint8_t KEY_RELAY3[32] = {
//     0xA1,0xB2,0xC3,0xD4,0xE5,0xF6,0x07,0x18,
//     0x29,0x3A,0x4B,0x5C,0x6D,0x7E,0x8F,0x90,
//     0x9F,0x8E,0x7D,0x6C,0x5B,0x4A,0x39,0x28,
//     0x17,0x06,0xF5,0xE4,0xD3,0xC2,0xB1,0xA0
// };
using namespace std;

int send_all(SOCKET sock, const char* buf, int len) {
    int total = 0;
    while (total < len) {
        int s = send(sock, buf + total, len - total, 0);
        if (s <= 0) return s;   // error or connection closed
        total += s;
    }
    return total; // bytes sent == len
}

int recv_all(SOCKET sock, char* buf, int len) {
    int total = 0;
    while (total < len) {
        int r = recv(sock, buf + total, len - total, 0);
        if (r <= 0) return r;
        total += r;
    }
    return total;
}

AESGCMBlock deserialize_block(const std::vector<uint8_t>& blob) {
    AESGCMBlock blk;
    size_t offset = 0;

    uint32_t ivSz  = *(uint32_t*)(blob.data() + offset); offset += 4;
    uint32_t tagSz = *(uint32_t*)(blob.data() + offset); offset += 4;
    uint32_t ctSz  = *(uint32_t*)(blob.data() + offset); offset += 4;

    blk.iv.assign(blob.begin() + offset, blob.begin() + offset + ivSz);
    offset += ivSz;

    blk.tag.assign(blob.begin() + offset, blob.begin() + offset + tagSz);
    offset += tagSz;

    blk.ciphertext.assign(blob.begin() + offset, blob.begin() + offset + ctSz);

    return blk;
}


int main(){
    WSADATA wsa;
    SOCKET relayListenSocket, inSocket, outSocket;
    sockaddr_in relayAddr, serverAddr, clientAddr;
    int clientSize=sizeof(clientAddr);

    //initializing winsocket
    WSAStartup(MAKEWORD(2,2), &wsa);
    //creating the relay listening socket (relay as server)
    relayListenSocket=socket(AF_INET, SOCK_STREAM, 0);

    relayAddr.sin_family=AF_INET;
    relayAddr.sin_port=htons(RELAY_PORT2);
    relayAddr.sin_addr.s_addr=INADDR_ANY;

    //binding the socket
    bind(relayListenSocket, (struct sockaddr*)&relayAddr, sizeof(relayAddr));
    listen(relayListenSocket, 1);
    cout<<"Relay2 node listening on port "<<RELAY_PORT2<<endl;

    //accepting incoming connection from client to forward the data
    inSocket=accept(relayListenSocket, (struct sockaddr*)&clientAddr, &clientSize);
    cout<<"Connection accepted from relay1 "<<inet_ntoa(clientAddr.sin_addr)<<":"<<ntohs(clientAddr.sin_port)<<endl;
    log_event("relay2", "connection_accepted", "");
    //creating outgoing socket to connect to server
    outSocket=socket(AF_INET, SOCK_STREAM, 0);

    serverAddr.sin_family=AF_INET;
    serverAddr.sin_port=htons(RELAY_PORT3);  //htons to correclty order the bytes
    serverAddr.sin_addr.s_addr=inet_addr("127.0.0.1");
    connect(outSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)); //connect func used if relay acts as client otherwise bind if as server
    cout<<"[RELAY2] Connected to RELAY3 on port "<<RELAY_PORT3<<"\n";

    uint32_t blobSz;
    if (recv_all(inSocket, (char*)&blobSz, sizeof(blobSz)) != sizeof(blobSz)) {
        std::cerr << "[RELAY2] Failed to receive blob size\n";
        return 1;
    }

    std::vector<uint8_t> blob(blobSz);
    if (recv_all(inSocket, (char*)blob.data(), blobSz) != (int)blobSz) {
        std::cerr << "[RELAY2] Failed to receive blob\n";
        return 1;
    }
    log_event("relay2", "decrypt_start","");
    AESGCMBlock blk = deserialize_block(blob);
    std::vector<uint8_t> plaintext;
    if (!aes_gcm_decrypt(blk, KEY_RELAY2, plaintext)) {
        std::cerr << "[RELAY2] Auth failed. Dropping connection.\n";
        log_event("relay2", "auth_failed", "");
        closesocket(inSocket);
        closesocket(outSocket);
        return 1;
    }

    std::cout << "[RELAY2] Authentication successful.\n";
    log_event("relay2", "auth_success", "");
    log_event("relay2", "forwarding_to_next", "");
    uint32_t nextBlobSz = plaintext.size();
    if (send_all(outSocket, (char*)&nextBlobSz, sizeof(nextBlobSz)) != sizeof(nextBlobSz))
        return 1;

    if (send_all(outSocket, (char*)plaintext.data(), plaintext.size())
            != (int)plaintext.size())
        return 1;

    std::cout << "[RELAY2] Layer peeled & forwarded to relay-3\n";


    // forwarding data between inSocket and outSocket
//uint64_t fileSZ;
//int ret = recv(inSocket, (char*)&fileSZ, sizeof(fileSZ), 0);
//if (ret != sizeof(fileSZ)) {
    //std::cerr << "[RELAY] Failed to receive file size\n";
    //return 1;
//}

// send(outSocket, (char*)&fileSZ, sizeof(fileSZ), 0);
// std::cout << "[RELAY] Forwarding file of size " << fileSZ << " bytes\n";

// // forwarding file data
// char buffer[BUFFER_SIZE];
// uint64_t totalForwarded = 0;

// while (totalForwarded < fileSZ) {
//     int bytesReceived = recv(inSocket, buffer, BUFFER_SIZE, 0);

//     //  handling disconnects and errors
//     if (bytesReceived <= 0) {
//         std::cerr << "[RELAY] Connection closed or recv error\n";
//         break;
//     }

//     AESGCMBlock blk;
//     uint32_t ivSz, tagSz, ctSz;

//     recv(inSocket, (char*)&ivSz,  sizeof(ivSz),  0);
//     recv(inSocket, (char*)&tagSz, sizeof(tagSz), 0);
//     recv(inSocket, (char*)&ctSz,  sizeof(ctSz),  0);

//     blk.iv.resize(ivSz);
//     blk.tag.resize(tagSz);
//     blk.ciphertext.resize(ctSz);

//     recv(inSocket, (char*)blk.iv.data(), ivSz, 0);
//     recv(inSocket, (char*)blk.tag.data(), tagSz,0);
//     recv(inSocket, (char*)blk.ciphertext.data(), ctSz,0);

//     vector<uint8_t> plaintext;
// if (!aes_gcm_decrypt(blk, AES_KEY, plaintext)) {
//     std::cerr << "[RELAY] Auth failed. Dropping packet.\n";
//     return 0;
// }

//     // forward plaintext
//     send(outSocket, (char*)plaintext.data(), plaintext.size(), 0);

//     // decrypt exactly the bytes received
//     //XORCrypt(buffer, bytesReceived, XOR_KEY);

//     // ensure ALL decrypted bytes are forwarded
//     // int totalSent = 0;
//     // while (totalSent < bytesReceived) {
//     //     int sent = send(outSocket,
//     //                     buffer + totalSent,
//     //                     bytesReceived - totalSent,
//     //                     0);
//     //     if (sent <= 0) {
//     //         std::cerr << "[RELAY] Send error\n";
//     //         break;
//     //     }
//     //     totalSent += sent;
//     // }

//     //totalForwarded += bytesReceived;
// }

    //cleaning up
    closesocket(inSocket);
    closesocket(outSocket);
    closesocket(relayListenSocket);
    WSACleanup();

    return 0;
}