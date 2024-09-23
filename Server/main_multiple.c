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
#define MAX_CLIENTS 30
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

    // setup for multiple connections
    char multiple = !0;
    res = setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &multiple, sizeof(multiple));
    if (res < 0) {
        printf("Multiple client setup failed: %d\n", WSAGetLastError());
        closesocket(listener);
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

    // Main Loop ==============================================================
    
    //variables
    fd_set socketSet; //set of active clients
    SOCKET clients[MAX_CLIENTS]; //array of clients
    SOCKET sd, max_sd; //current socket
    int curNoClients = 0; //info about the array
    struct sockaddr_in clientAddr;
    int clientAddrlen;
    char quit = 0;

    char recvbuf[BUFLEN];


    char *welcome = "Welcome to the server";
    int welcomeLength = strlen(welcome);
    char *goodbye = "Goodbye!";
    int goodbyeLength = strlen(goodbye);

    //clear a client array
    memset(clients, 0, MAX_CLIENTS * sizeof(SOCKET));

    while(!quit) {
        //clear the set
        FD_ZERO(&socketSet);

        //add listener socket
        FD_SET(listener, &socketSet);

        for (int i=0; i < MAX_CLIENTS;i++){
            //socket
            sd = clients[i];
            if (sd > 0) {
                //add an actie client to the set
                FD_SET(sd, &socketSet);
            }
            if (sd > max_sd){
                //update max_sd
                max_sd = sd;
            }
        }

        int activity = select(max_sd + 1, &socketSet, NULL, NULL, NULL);
        if (activity < 0) {
            continue;
        }

        //determine which sockets have activity
        if (FD_ISSET(listener, &socketSet)){

            // accept connection
            sd = accept(listener, NULL, NULL);
            if (sd == INVALID_SOCKET) {
                printf("ERROR accepting failed: %d\n", WSAGetLastError());
                closesocket(listener);
            }

            //get client info
            getpeername(sd, (struct sockaddr*)&clientAddr, &clientAddrlen);
            printf(
                "Client connected at %s:%d\n",
                sd,
                inet_ntoa(clientAddr.sin_addr),
                ntohs(clientAddr.sin_port)
                );

            //send welcome message
            sendRes = send(sd, welcome, welcomeLength, 0);
            if (sendRes != welcomeLength) {
                printf("send failed: %d\n", WSAGetLastError());
                shutdown(sd, SD_BOTH);
                closesocket(sd);
            }

            //add new socket to the array
            if (curNoClients >= MAX_CLIENTS) {
                printf("Too many clients\n");
                shutdown(sd, SD_BOTH);
                closesocket(sd);
            } else {
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i] == 0) {
                        clients[i] = sd;
                        printf("Added to the list at index %d\n", i);
                        curNoClients++;
                        break;
                    }
                }
            }

            for (int i = 0; i <MAX_CLIENTS;i++) {
                if (!clients[i]){
                    continue;
                }
                sd = clients[i];

                if (FD_ISSET(sd, &socketSet)){
                    //get message
                    res = recv(sd, recvbuf, BUFLEN, 0);
                    if (res > 0 ) {
                        //print message
                        recvbuf[res] = '\0';
                        printf("Message(%d): %s\n", res, recvbuf);

                        //test if quit command
                        if (!memcmp(recvbuf, "/quit", 5 * sizeof(char))) {
                            quit = !0; //quit true
                            printf("Client has disconnected\n");
                            break;
                        }

                        //echo message
                        sendRes = send(sd, recvbuf, res, 0);
                        if (sendRes  == SOCKET_ERROR) {
                            printf("ECHO failed: %d\n", WSAGetLastError());
                            shutdown(sd, SD_BOTH);
                            closesocket(sd);
                            clients[i] = 0;
                            curNoClients--;
                        }
                    } else {
                        //get client information
                        getpeername(sd, (struct sockaddr*)&clientAddr, &clientAddrlen);
                        printf(
                            "Client disconnected at %s:%d\n",
                            sd,
                            inet_ntoa(clientAddr.sin_addr),
                            ntohs(clientAddr.sin_port)
                            );
                        shutdown(sd, SD_BOTH);
                        closesocket(sd);
                        clients[i] = 0;
                        curNoClients--;
                    }
                }
            }
        }
    }

    //=========================================================================

    // cleanup=======================================================================

        // disconnect all clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != 0) {
            //active clients
            sendRes = send(clients[i], goodbye, goodbyeLength, 0);

            shutdown(clients[i], SD_BOTH);
            closesocket(clients[i]);
            clients[i] = 0;
        }
    }
    
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