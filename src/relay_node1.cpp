#include<winsock2.h>
#include<ws2tcpip.h>
#include<iostream>
//#include"../include/crypto.h"
#include"../include/aes_gcm.h"

#pragma comment(lib, "ws2_32.lib")

#define RELAY_PORT 5001
#define BUFFER_SIZE 4096
#define SERVER_PORT 5000
//#define XOR_KEY 0x5A

const uint8_t AES_KEY[32] = {
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
    0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,
    0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20
};
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
    relayAddr.sin_port=htons(RELAY_PORT);
    relayAddr.sin_addr.s_addr=INADDR_ANY;

    //binding the socket
    bind(relayListenSocket, (struct sockaddr*)&relayAddr, sizeof(relayAddr));
    listen(relayListenSocket, 1);
    cout<<"Relay node listening on port "<<RELAY_PORT<<endl;

    //accepting incoming connection from client to forward the data
    inSocket=accept(relayListenSocket, (struct sockaddr*)&clientAddr, &clientSize);
    cout<<"Connection accepted from client "<<inet_ntoa(clientAddr.sin_addr)<<":"<<ntohs(clientAddr.sin_port)<<endl;

    //creating outgoing socket to connect to server
    outSocket=socket(AF_INET, SOCK_STREAM, 0);

    serverAddr.sin_family=AF_INET;
    serverAddr.sin_port=htons(SERVER_PORT);  //htons to correclty order the bytes
    serverAddr.sin_addr.s_addr=inet_addr("127.0.0.1");
    connect(outSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)); //connect func used if relay acts as client otherwise bind if as server
    cout<<"[RELAY] Connected to server on port "<<SERVER_PORT<<"\n";

    uint32_t ivSz, tagSz, ctSz;

    // receive sizes
    recv_all(inSocket, (char*)&ivSz, sizeof(ivSz));
    recv_all(inSocket, (char*)&tagSz, sizeof(tagSz));
    recv_all(inSocket, (char*)&ctSz, sizeof(ctSz));

    AESGCMBlock blk;
    blk.iv.resize(ivSz);
    blk.tag.resize(tagSz);
    blk.ciphertext.resize(ctSz);

    // receive actual encrypted data
    recv_all(inSocket, (char*)blk.iv.data(), ivSz);
    recv_all(inSocket, (char*)blk.tag.data(), tagSz);
    recv_all(inSocket, (char*)blk.ciphertext.data(), ctSz);

    std::vector<uint8_t> plaintext;
    if (!aes_gcm_decrypt(blk, AES_KEY, plaintext)) {
        std::cerr << "[RELAY] Auth failed. Dropping connection.\n";
        closesocket(inSocket);
        closesocket(outSocket);
        return 1;
    }

    std::cout << "[RELAY] Authentication successful.\n";

    uint64_t fileSZ = plaintext.size();
    if (send_all(outSocket, (char*)&fileSZ, sizeof(fileSZ)) != sizeof(fileSZ)) return 1;
    if (send_all(outSocket, (char*)plaintext.data(), plaintext.size())
        != (int)plaintext.size()) return 1;

    std::cout << "[RELAY] First onion layer removed & forwarded.\n";

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