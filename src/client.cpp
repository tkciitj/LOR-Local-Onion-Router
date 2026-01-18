#include<iostream>
#include<winsock2.h>
#include<ws2tcpip.h>
#include<fstream>
//#include"../include/crypto.h"
#include"../include/aes_gcm.h"

using namespace std;

#pragma comment(lib, "ws2_32.lib")
#define SERVER_PORT 5000
#define RELAY_PORT 5001
#define BUFFER_SIZE 4096
//#define XOR_KEY 0x5A

const uint8_t AES_KEY[32] = {
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
    0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,
    0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20
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
    relayAddr.sin_port=htons(RELAY_PORT);
    relayAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // localhost loopback address

    //connecting to server
    connect(sock, (struct sockaddr*)&relayAddr, sizeof(relayAddr));

    //openening file in binary mode 
    ifstream file("test.txt", ios::binary);
    file.seekg(0, ios::end);
    uint64_t fileSZ = file.tellg();
    file.seekg(0, ios::beg);

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

    AESGCMBlock enc = aes_gcm_encrypt(plaintext, AES_KEY);
    uint32_t ivSz  = enc.iv.size();
    uint32_t tagSz = enc.tag.size();
    uint32_t ctSz  = enc.ciphertext.size();

    if (send_all(sock, (char*)&ivSz,  sizeof(ivSz))  != sizeof(ivSz))  return 1;
    if (send_all(sock, (char*)&tagSz, sizeof(tagSz)) != sizeof(tagSz)) return 1;
    if (send_all(sock, (char*)&ctSz,  sizeof(ctSz))  != sizeof(ctSz))  return 1;

    if (send_all(sock, (char*)enc.iv.data(),  ivSz)  != ivSz)  return 1;
    if (send_all(sock, (char*)enc.tag.data(), tagSz) != tagSz) return 1;
    if (send_all(sock, (char*)enc.ciphertext.data(), ctSz) != ctSz) return 1;

    cout<<"Encrypted File sent successfully."<<endl;
    //closing the socket and file
    file.close();
    closesocket(sock);
    WSACleanup();

    return 0;
}