#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winSock2.h>
#include <Windows.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <process.h>
#include <stdatomic.h>

#pragma comment(lib, "Ws2_32.lib")

#define BUFLEN 512
#define PORT 27015
#define ADDRESS "127.0.0.1" //localhost

DWORD WINAPI sendThreadFunc(LPVOID lpParam);

int main () {

    printf("Hello World!\n");

    int res;

    // INITIALIZATION ============================================================
    
    WSADATA wsaData;
    res = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (res) {
        printf("WSAStartup failed: %d\n", res);
        return 1;
    }
    // ===========================================================================

    // SETUP CLIENT SOCKET =======================================================
    
    // contruct server
    SOCKET client;
    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client == INVALID_SOCKET) {
        printf("socket Contruction Failure: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    //connect to address
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ADDRESS);
    address.sin_port = htons(PORT);
    res = connect(client, (struct sockaddr*)&address, sizeof(address));
    if (res == SOCKET_ERROR) {
        printf("Connection Failure: %d\n", WSAGetLastError());
        closesocket(client);
        WSACleanup();
        return 1;
    } else if (client == INVALID_SOCKET) {
        printf("Connection Failure: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }



    // ===========================================================================

    // MAIN LOOP =================================================================

    // start send thread
    DWORD thrdId;
    HANDLE sendThread = CreateThread(
        NULL,
        0, 
        sendThreadFunc, 
        (LPVOID)client, 
        0, 
        &thrdId
        );
        if (sendThread) {
            printf("Thread started: %d\n", thrdId);
        } else {
            printf("Thread failed to start: %d\n", GetLastError());
        }
        char recvbuf[BUFLEN];

        //start recieve loop
        do {
            res = recv(client, recvbuf, BUFLEN, 0);
            recvbuf[res] = '\0';
            if (res > 0) {
                printf("Recieved: %s\n",res, recvbuf);
            } else if (!res) {
                printf("Connection closed\n");
            } else {
                printf("recv failed: %d\n", WSAGetLastError());
            }
        } while (res > 0);

        

    // ===========================================================================

    // CLEANUP ===================================================================
    
    // ===========================================================================

    return 0;
}



