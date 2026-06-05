#ifndef UDP_H
#define UDP_H

#include "message.h"

#define SERVER_PORT 5000
#define CLIENT_PORT 5001
#define LOCALHOST_IP "127.0.0.1"

int ServerUdpInit(void);
int ClientUdpInit(void);

int ServerUdpSendMessage(const UdpMessage_t* message);
int ServerUdpReceiveMessage(UdpMessage_t* message);

int ClientUdpReceiveMessage(UdpMessage_t* message);
int ClientUdpSendMessage(const UdpMessage_t* message);

void ServerUdpClose(void);
void ClientUdpClose(void);

#endif

