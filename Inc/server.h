#ifndef SERVER_H
#define SERVER_H


#define SERVER_TX_QUEUE_LENGTH 10
#define EVENT_GENERATOR_STACK_SIZE 1000
#define SERVER_UDP_TX_STACK_SIZE 1000
#define SERVER_UDP_RX_STACK_SIZE 1000
#define ACK_TIMEOUT_MS 500


void RunServer(void);

void ServerEventGeneratorTask(void *pvParameters);
void ServerUdpTxTask(void *pvParameters);
void ServerUdpRxTask(void *pvParameters);

#endif