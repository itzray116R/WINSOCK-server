#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winSock2.h>
#include <Windows.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

#define BUFLEN 512
#define PORT 27015
#define ADDRESS "127.0.0.1" //aka localhost

int main() {
    printf("Hello World!\n");

    int res, sendRes;

    //INITIalization------------------------------------------------------------------
    WSADATA wsaData; //configuration Data
    res = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (res != 0) {
        printf("WSAStartup failed: %d\n", res);
        return 1;
    }
    //--------------------------------------------------------------------------------


    // SETUP SERVER ==================================================================

    //contruct socket
    SOCKET listener;
    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET) {
        printf("socket failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    
    //bind socket
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ADDRESS);
    address.sin_port = htons(PORT);
    res = bind(listener, (struct sockaddr*)&address, sizeof(address));
    if (res == SOCKET_ERROR) {
        printf("bind failed: %d\n", WSAGetLastError());
        closesocket(listener);
        WSACleanup();
        return 1;
    }

    //set as a listener
    res = listen(listener, SOMAXCONN);
    if (res == SOCKET_ERROR) {
        printf("listen failed: %d\n", WSAGetLastError());
        closesocket(listener);
        WSACleanup();
        return 1;
    }
    //===============================================================================

    printf("Accepting on %s:%d\n", ADDRESS, PORT);

    // HANDLE A CLIENT ==============================================================
    
    //accept a client
    SOCKET client;
    struct sockaddr_in clientaddr;
    int clientaddr_len = sizeof(clientaddr);
    client = accept(listener, NULL, NULL);
    if (client == INVALID_SOCKET) {
        printf("accept failed: %d\n", WSAGetLastError());
        closesocket(listener);
        WSACleanup();
        return 1;
    }

    //get client information
    getpeername(client, (struct sockaddr*)&clientaddr, &clientaddr_len);
    printf("client connected: %s:%d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

    //send welcome message
    char *welcome = "Welcome to the server!";
    sendRes = send(client, welcome, strlen(welcome), 0);
    if (sendRes != strlen(welcome)) {
        printf("send failed: %d\n", WSAGetLastError());
        shutdown(client, SD_BOTH);
        closesocket(client);
    }

    //receive MESSAGES
    char recvbuff[BUFLEN];

    do {
        res = recv(client, recvbuff, BUFLEN, 0);
        if (res > 0) {

            recvbuff[res] = '\0';

            printf("Message(%d): %s\n",res, recvbuff);

            if (!memcmp(recvbuff, "/quit", 5* sizeof(char))) {
                printf("Client has disconnected\n");
                break;
            }
            sendRes = send(client, recvbuff, res, 0);
            if (sendRes != res) {
                printf("send failed: %d\n", WSAGetLastError());
                shutdown(client, SD_BOTH);
                closesocket(client);
                break;
                
            }
        }
        else if (res == 0) {
            printf("Connection closed\n");
        }
        else {
            printf("recv failed: %d\n", WSAGetLastError());
            shutdown(client, SD_BOTH);
            closesocket(client);
            break;
        }
    } while (res > 0);

    //shutdown client
    res = shutdown(client, SD_BOTH);
    if (res == SOCKET_ERROR) {
        printf("shutdown failed: %d\n", WSAGetLastError());
    }
    closesocket(client);

    // cleanup=======================================================================
    //shutdown server socket
    closesocket(listener);

    //cleanup WSA
    res = WSACleanup();
    if (res != 0) {
        printf("WSACleanup failed: %d\n", res);
        return 1;
    }
    //===============================================================================

    printf("shutting down.\nGood night.\n");

    return 0;
}