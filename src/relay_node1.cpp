#include<winsock2.h>
#include<ws2tcpip.h>
#include<iostream>
#include"../include/crypto.h"

#pragma comment(lib, "ws2_32.lib")

#define RELAY_PORT 5001
#define BUFFER_SIZE 4096
#define SERVER_PORT 5000
#define XOR_KEY 0x5A
using namespace std;

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

    // forwarding data between inSocket and outSocket
uint64_t fileSZ;
int ret = recv(inSocket, (char*)&fileSZ, sizeof(fileSZ), 0);
if (ret != sizeof(fileSZ)) {
    std::cerr << "[RELAY] Failed to receive file size\n";
    return 1;
}

send(outSocket, (char*)&fileSZ, sizeof(fileSZ), 0);
std::cout << "[RELAY] Forwarding file of size " << fileSZ << " bytes\n";

// forwarding file data
char buffer[BUFFER_SIZE];
uint64_t totalForwarded = 0;

while (totalForwarded < fileSZ) {
    int bytesReceived = recv(inSocket, buffer, BUFFER_SIZE, 0);

    //  handling disconnects and errors
    if (bytesReceived <= 0) {
        std::cerr << "[RELAY] Connection closed or recv error\n";
        break;
    }

    // decrypt exactly the bytes received
    XORCrypt(buffer, bytesReceived, XOR_KEY);

    // ensure ALL decrypted bytes are forwarded
    int totalSent = 0;
    while (totalSent < bytesReceived) {
        int sent = send(outSocket,
                        buffer + totalSent,
                        bytesReceived - totalSent,
                        0);
        if (sent <= 0) {
            std::cerr << "[RELAY] Send error\n";
            break;
        }
        totalSent += sent;
    }

    totalForwarded += bytesReceived;
}

std::cout << "[RELAY] Decrypted the first layer & File forwarded successfully." << std::endl;


    //cleaning up
    closesocket(inSocket);
    closesocket(outSocket);
    closesocket(relayListenSocket);
    WSACleanup();

    return 0;
}