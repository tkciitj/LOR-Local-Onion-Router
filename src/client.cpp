#include<iostream>
#include<winsock2.h>
#include<ws2tcpip.h>
#include<fstream>
#include"../include/crypto.h"

using namespace std;

#pragma comment(lib, "ws2_32.lib")
#define SERVER_PORT 5000
#define RELAY_PORT 5001
#define BUFFER_SIZE 4096
#define XOR_KEY 0x5A


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
    send(sock, (char*)&fileSZ, sizeof(fileSZ), 0); // 0 flag for no special options
    //sending file data
    char buffer[BUFFER_SIZE];

while(file.read(buffer, BUFFER_SIZE) || file.gcount()>0) {
    int bytesRead = static_cast<int>(file.gcount());

    // Encrypting only the bytes read
    XORCrypt(buffer, bytesRead, XOR_KEY);

    // Ensuring ALL bytes are sent
    int totalSent=0;
    while (totalSent<bytesRead) {
        int sent=send(sock,
                        buffer + totalSent,
                        bytesRead - totalSent,
                        0);
        if (sent<=0) {
            std::cerr<<"Send failed\n";
            return 1;
        }
        totalSent+=sent;
    }
}


    cout<<"Encrypted File sent successfully."<<endl;
    //closing the socket and file
    file.close();
    closesocket(sock);
    WSACleanup();

    return 0;
}