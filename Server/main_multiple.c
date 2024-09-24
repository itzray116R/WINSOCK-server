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
#define MAX_CLIENTS 10

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
    fd_set socketSet;                   //set of active clients
    SOCKET clients[MAX_CLIENTS];        //array of clients
    int curNoClients = 0;               // active slots in the array
    SOCKET sd, max_sd;                  // placeholders
    struct sockaddr_in clientAddr;
    int clientAddrlen;
    char running = !0;  //server state

    char recvbuf[BUFLEN];

    char *welcome = "Welcome to the server :)\n";
    int welcomeLength = strlen(welcome);
    char *full = "Sorry The server is full :(\n";
    int fullLength = strlen(full);
    char *goodbye = "Goodbye! \n";
    int goodbyeLength = strlen(goodbye);

    //clear a client array
    memset(clients, 0, MAX_CLIENTS * sizeof(SOCKET));

    while(running) {

        //clear the Socket set
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

        //determine which listener have activity
        if (FD_ISSET(listener, &socketSet)){

            // accept connection
            sd = accept(listener, NULL, NULL);
            if (sd == INVALID_SOCKET) {
                printf("ERROR accepting failed: %d\n", WSAGetLastError());
            }

            //get client info
            getpeername(sd, (struct sockaddr*)&clientAddr, &clientAddrlen);
            printf(
                "Client connected at %s:%d\n",
                sd,
                inet_ntoa(clientAddr.sin_addr),
                ntohs(clientAddr.sin_port)
                );


            //add new socket to the array
            if (curNoClients >= MAX_CLIENTS) {
                printf("Full\n");
                sendRes = send(sd, full, fullLength, 0);
                if (sendRes != fullLength) {
                    printf("Error sending: %d\n", WSAGetLastError());
                }


                shutdown(sd, SD_BOTH);
                closesocket(sd);
            } else {
                //scan through list
                int i;
                for (i = 0; i < MAX_CLIENTS; i++) {
                    if (!clients[i]) {
                        clients[i] = sd;
                        printf("Added to the list at index %d\n", i);
                        curNoClients++;
                        break;
                    }
                }

                //send welcome message
                sendRes = send(sd, welcome, welcomeLength, 0);
                if (sendRes != welcomeLength) {
                    printf("send failed: %d\n", WSAGetLastError());
                    shutdown(sd, SD_BOTH);
                    closesocket(sd);
                    clients[i] = 0;
                    curNoClients--;
                }
            }
        }

        //iterate through the clients
        for (int i = 0; i <MAX_CLIENTS;i++) {
            if (!clients[i]){
                continue;
            }
            sd = clients[i];
            // determine if client has activity
            if (FD_ISSET(sd, &socketSet)){
                //get message
                res = recv(sd, recvbuf, BUFLEN, 0);
                if (res > 0 ) {
                    //print message
                    recvbuf[res] = '\0';
                    printf("Recieved (%d): %s\n", res, recvbuf);
                    //test if quit command
                    if (!memcmp(recvbuf, "/quit", 5 * sizeof(char))) {
                        running = 0; //running false
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