#include<iostream>
#include<winsock2.h>
#include<ws2tcpip.h>
#include<fstream>

using namespace std;

#pragma comment(lib, "ws2_32.lib")
#define SERVER_PORT 5000
#define BUFFER_SIZE 4096

int main(){
    WSADATA wsa;
    SOCKET sock;
    sockaddr_in serverAddr;

    // initializing winsock
    WSAStartup(MAKEWORD(2,2), &wsa);

    //creating the socket
    sock=socket(AF_INET, SOCK_STREAM, 0); // SOCK_STREAM for TCP conn

    //defining server address
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_port=htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // localhost loopback address

    //connecting to server
    connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    //openening file in binary mode 
    ifstream file("test.txt", ios::binary);
    file.seekg(0, ios::end);
    uint64_t fileSZ = file.tellg();
    file.seekg(0, ios::beg);

    //sending file size first
    send(sock, (char*)&fileSZ, sizeof(fileSZ), 0); // 0 flag for no special options
    //sending file data
    char buffer[BUFFER_SIZE];
    while(!file.eof()){
        file.read(buffer, BUFFER_SIZE);
        streamsize bytesRead = file.gcount();
        send(sock, buffer, bytesRead, 0);
    }

    cout<<"File sent successfully."<<endl;
    //closing the socket and file
    file.close();
    closesocket(sock);
    WSACleanup();

    return 0;
}