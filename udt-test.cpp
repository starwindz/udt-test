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

int main(int argc, char** argv)
{
    // set parameters
    if (argc != 2) {
        printf("invalid parameter\n");
        printf("usesage: udt-test [option]\n");
        printf("option: 0(host), 1(client)\n");
        return 0;
    }
    int prog_mode;
    if (strcmp(argv[1], "0") == 0)
        prog_mode = 0;  // host
    else if (strcmp(argv[1], "1") == 0)
        prog_mode = 1;  // client
    else
        return 0;

    // define vars
    addrinfo hints, *local, *peer;
    UDTSOCKET remote;
    char local_port[32];
    char remote_port[32];
    bool rendezVous = true;
    int timeout = 1;
    int s_cnt = 0;
    char szSendMessage[256] = { 0, }, szReceiveMessage[256] = { 0, };

    // init var
    UDTUpDown _udt_;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    bool first_packet_received = false;
    printf("init var: ok\n");

    // start local and remote
    // -- set local port
    if (prog_mode == 0)
        strcpy_s(local_port, "27885");
    else
        strcpy_s(local_port, "27886");

    if (0 != getaddrinfo(NULL, local_port, &hints, &local)) {
        printf("illegal local port number or port is busy.\n");
        return 0;
    }
    printf("set local port: ok\n");

    // -- create remote socket and bind local-remote
    remote = UDT::socket(local->ai_family, local->ai_socktype, local->ai_protocol);
    if (UDT::ERROR == UDT::setsockopt(remote, 0, UDT_RENDEZVOUS, &rendezVous, sizeof(bool))) {
        return 0;
    }
    if (UDT::ERROR == UDT::setsockopt(remote, 0, UDT_RCVTIMEO, &timeout, sizeof(int))) {
        return 0;
    }
    if (UDT::ERROR == UDT::bind(remote, local->ai_addr, local->ai_addrlen)) {
        return 0;
    }
    freeaddrinfo(local);
    printf("create remote socket and bind local-remote: ok\n");

    // -- set remote ip and port
    if (prog_mode == 0)
        strcpy_s(remote_port, "27886");
    else
        strcpy_s(remote_port, "27885");

    if (0 != getaddrinfo("127.0.0.1", remote_port, &hints, &peer)) {
        printf("illegal remote port number or port is busy.\n");
        return 0;
    }
    printf("set remote ip and port: ok\n");

    // -- connect local-remote
    if (UDT::ERROR == UDT::connect(remote, peer->ai_addr, peer->ai_addrlen)) {
        std::cout << "connect: " << UDT::getlasterror().getErrorMessage() << std::endl;
        return 0;
    }
    freeaddrinfo(peer);
    printf("connect local-remote: ok\n");

    // misc init
    memset(szSendMessage, 0, sizeof(szSendMessage));
    memset(szReceiveMessage, 0, sizeof(szReceiveMessage));
    printf("press '1' to send a packet\n");   

    // on_draw loop
    bool loop = true;
    while (loop) { 
        // create message
        sprintf_s(szSendMessage, "%s: %d", "I am Player 1", s_cnt);

        // simulating 30fps in on_draw loop of swos menu
        Sleep(33);

        // send packet
        if (_kbhit()) {
            if (_getch() == '1') {
                if (auto len = UDT::sendmsg(remote, szSendMessage, strlen(szSendMessage), -1, true)) {
                  //  printf("sent a message of length %d", len);
                    s_cnt++;
                }
            }
            else if (_getch() == 'q') {
                loop = false;
            }
        }
  
        // receive packet
        memset(szReceiveMessage, 0, sizeof(szReceiveMessage));
        auto recVal = UDT::recvmsg(remote, szReceiveMessage, sizeof(szReceiveMessage));
        if (recVal > 0) {
            printf("packet received from Player 2: %s\n", szReceiveMessage);

            // if first packet received, send back packet to remote
            if (first_packet_received == false) {
                printf("first_packet_received\n");
                UDT::sendmsg(remote, "first_packet_received", strlen("first_packet_received"), -1, true);
                first_packet_received = true;
            }
        }

        if (UDT::ERROR == recVal) {
            auto err = UDT::getlasterror();
            if (err.getErrorCode() == 6003) {
                // std::cout << "recvmsg timeout, no worries: " << std::endl;
            }
            else {
                std::cout << "recvmsg error: " << UDT::getlasterror().getErrorMessage() << std::endl;
                return 0;
            }
        }
    }

    // close     
    UDT::close(remote);

    return 0;
}