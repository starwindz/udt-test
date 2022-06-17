#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>

#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <iostream>

#include "udt.h"
#include "cc.h"
#include "test_util.h"

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("invalid parameter\n");
        printf("usesage: udt-test [option]\n");
        printf("option: 0(server), 1(client)\n");
        return 0;
    }
    int prog_mode;
    if (strcmp(argv[1], "0") == 0)
        prog_mode = 0;  // server
    else if (strcmp(argv[1], "1") == 0)
        prog_mode = 1;  // client
    else
        return 0;

    // init
    UDTUpDown _udt_;

    addrinfo hints, * local, * peer;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    bool first_packet_received = false;
    printf("init var done\n");

    // start local and remote
    // -- local
    char local_port[32];
    if (prog_mode == 0)
        strcpy_s(local_port, "27885");
    else
        strcpy_s(local_port, "27886");

    if (0 != getaddrinfo(NULL, local_port, &hints, &local)) { // local_port
        printf("illegal port number or port is busy.\n");
        return 0;
    }

    UDTSOCKET client = UDT::socket(local->ai_family, local->ai_socktype, local->ai_protocol);
    freeaddrinfo(local);
    printf("init local done\n");

    // -- remote
    char remote_port[32];
    if (prog_mode == 0)
        strcpy_s(remote_port, "27886");
    else
        strcpy_s(remote_port, "27885");

    getaddrinfo("127.0.0.1", remote_port, &hints, &peer);
    UDT::connect(client, peer->ai_addr, peer->ai_addrlen);
    freeaddrinfo(peer);
    printf("init remote done\n");

    // misc vars
    char szMessage[256] = { 0, }, szSendMessage[256] = { 0, };
    memset(szMessage, 0, sizeof(szMessage));
    memset(szSendMessage, 0, sizeof(szSendMessage));
    int s_cnt = 0;
    printf("press '1' to send a packet\n");

    // on_draw loop
    while (true) {
        sprintf_s(szSendMessage, "%s: %d", "I am Player 1", s_cnt);

        // polling rudp process
        memset(szMessage, 0, sizeof(szMessage));

        // simulating 30fps in on_draw loop of swos menu
        Sleep(33);

        // send packet
        if (_kbhit())
            if (_getch() == '1')
                if (UDT::sendmsg(client, szSendMessage, strlen(szSendMessage))) s_cnt++;

        // receive packet
        if (UDT::recvmsg(client, szMessage, strlen(szMessage)) > 0) {
            printf("packet received from Player 2: %s\n", szMessage);

            // if first packet received, send back packet to remote
            if (first_packet_received == false) {
                printf("first_packet_received\n");
                UDT::sendmsg(client, "first_packet_received", strlen("first_packet_received") + 1);
                first_packet_received = true;
            }
        }
    }

    // close     
    UDT::close(client);

    return 0;
}
