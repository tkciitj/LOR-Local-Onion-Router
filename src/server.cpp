#include<cstdint>
#include<iostream>
#include<winsock2.h>
#include<ws2tcpip.h>
#include<fstream>
#include"../include/event_logger.h"

using namespace std;

#pragma comment(lib, "ws2_32.lib")
#define SERVER_PORT 5000
#define BUFFER_SIZE 4096

int main(){
    WSADATA wsa;
    SOCKET serverSocket, clientSocket; //DESCRIPTOR FOR checking SOCKET connection
    struct sockaddr_in serverAddr, clientAddr;
    int clientSize=sizeof(clientAddr);

    // initializing winsocket
    WSAStartup(MAKEWORD(2,2), &wsa);
    //creating the socket
    serverSocket=socket(AF_INET, SOCK_STREAM, 0); // SOCK_STREAM for TCP conn and SOCK_DGRAM for UDP conn
    // binding the port and IP addr in the socket
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_port=htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr=INADDR_ANY; // to accept connections from any IP addr
    bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    // listening for connections
    listen(serverSocket, 1);
    cout<<"Server is listening on port "<<SERVER_PORT<<endl;
    // accepting the connection from client
    clientSocket=accept(serverSocket, (struct sockaddr*)&clientAddr, &clientSize);
    cout<<"Connection accepted from "<<inet_ntoa(clientAddr.sin_addr)<<":"<<ntohs(clientAddr.sin_port)<<endl;
    log_event("server", "connection_accepted");
    //receive file size
    uint64_t fileSZ;
    int r = recv(clientSocket, (char*)&fileSZ, sizeof(fileSZ), 0);
    if (r != sizeof(fileSZ)) {
        std::cerr << "[SERVER] Failed to receive file size\n";
        return 1;
    }

    //recieve file data
    ofstream outFile("received_file", ios::binary);
    char buffer[BUFFER_SIZE]; //buffer to hold the filedata
    uint64_t totalReceived=0;

    while(totalReceived < fileSZ){
        int bytesReceived=recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) {
            std::cerr << "[SERVER] Connection closed early\n";
            break;
        }
        outFile.write(buffer, bytesReceived);
        totalReceived+=bytesReceived;
    }  //while loop to check if all data is received and writes to file
    if (totalReceived != fileSZ) {
        std::cerr << "[SERVER] Incomplete file received ("
                << totalReceived << "/" << fileSZ << " bytes)\n";
    } else {
        std::cout << "[SERVER] File received successfully.\n";
    }
    log_event("server", "file_received", std::to_string(fileSZ) + " bytes");


    //closing the sockets
    outFile.close();
    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();

    return 0;

}