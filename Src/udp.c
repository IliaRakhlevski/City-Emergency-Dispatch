#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "udp.h"

/* UDP socket used by the server to send and receive messages */
static int g_serverSocket = -1;

/* UDP socket used by the client to send and receive messages */
static int g_clientSocket = -1;

/* Mutex used to synchronize UDP transmissions from multiple client tasks */
static SemaphoreHandle_t g_clientUdpMutex = NULL;

/**
 * @brief Initializes the server UDP communication subsystem.
 *
 * Creates and configures the server UDP socket,
 * binds it to the server port, and prepares the
 * communication channel for client interaction.
 *
 * @return 0 on success, -1 on failure.
 */
int ServerUdpInit(void)
{
    struct sockaddr_in serverAddr;

    g_serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_serverSocket < 0)
    {
        perror("Server socket");
        return -1;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);

    if (bind(g_serverSocket,
             (struct sockaddr*)&serverAddr,
             sizeof(serverAddr)) < 0)
    {
        perror("Server bind");
        close(g_serverSocket);
        g_serverSocket = -1;
        return -1;
    }

    printf("\nServer UDP initialized on port %d\n", SERVER_PORT);
    return 0;
}

/**
 * @brief Initializes the client UDP communication subsystem.
 *
 * Creates and configures the client UDP socket,
 * binds it to the client port, and prepares the
 * communication channel for server interaction.
 *
 * @return 0 on success, -1 on failure.
 */
int ClientUdpInit(void)
{
    struct sockaddr_in clientAddr;

    g_clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_clientSocket < 0)
    {
        perror("Client socket");
        return -1;
    }

    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = INADDR_ANY;
    clientAddr.sin_port = htons(CLIENT_PORT);

    if (bind(g_clientSocket,
             (struct sockaddr*)&clientAddr,
             sizeof(clientAddr)) < 0)
    {
        perror("Client bind");
        close(g_clientSocket);
        g_clientSocket = -1;
        return -1;
    }

    g_clientUdpMutex = xSemaphoreCreateMutex();

    if (g_clientUdpMutex == NULL)
    {
        printf("Client UDP mutex creation failed\n");
        close(g_clientSocket);
        g_clientSocket = -1;
        return -1;
    }

    printf("\nClient UDP initialized on port %d\n", CLIENT_PORT);
    return 0;
}

/**
 * @brief Closes the server UDP communication subsystem.
 *
 * Releases network resources and closes the
 * server UDP socket.
 */
void ServerUdpClose(void)
{
    if (g_serverSocket >= 0)
    {
        close(g_serverSocket);
        g_serverSocket = -1;
    }

    if (g_clientUdpMutex != NULL)
    {
        vSemaphoreDelete(g_clientUdpMutex);
        g_clientUdpMutex = NULL;
    }
}

/**
 * @brief Closes the client UDP communication subsystem.
 *
 * Releases network resources and closes the
 * client UDP socket.
 */
void ClientUdpClose(void)
{
    if (g_clientSocket >= 0)
    {
        close(g_clientSocket);
        g_clientSocket = -1;
    }
}

/**
 * @brief Sends a UDP message from the server to the client.
 *
 * Transmits a protocol message to the client using the
 * server UDP socket.
 *
 * @param message Pointer to the message to be transmitted.
 *
 * @return 0 on success, -1 on failure.
 */
int ServerUdpSendMessage(const UdpMessage_t* message)
{
    struct sockaddr_in clientAddr;

    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(CLIENT_PORT);
    inet_pton(AF_INET, LOCALHOST_IP, &clientAddr.sin_addr);

    int sent = sendto(g_serverSocket,
                      message,
                      sizeof(UdpMessage_t),
                      0,
                      (struct sockaddr*)&clientAddr,
                      sizeof(clientAddr));

    if (sent < 0)
    {
        perror("ServerSendEvent sendto");
        return -1;
    }

    return 0;
}

/**
 * @brief Receives a UDP message from the server.
 *
 * Waits for and receives a protocol message using
 * the client UDP socket.
 *
 * @param message Pointer to the destination message buffer.
 *
 * @return 0 on success, -1 on failure.
 */
int ClientUdpReceiveMessage(UdpMessage_t* message)
{
    int received;

    do
    {
        received = recvfrom(g_clientSocket,
                            message,
                            sizeof(UdpMessage_t),
                            0,
                            NULL,
                            NULL);
    }
    while (received < 0 && errno == EINTR);

    if (received < 0)
    {
        perror("ClientReceiveEvent recvfrom");
        return -1;
    }

    if (received != sizeof(UdpMessage_t))
    {
        printf("Invalid event message size\n");
        return -1;
    }

    return 0;
}

/**
 * @brief Sends a UDP message from the client to the server.
 *
 * Transmits a protocol message to the server using the
 * client UDP socket.
 *
 * @param message Pointer to the message to be transmitted.
 *
 * @return 0 on success, -1 on failure.
 */
int ClientUdpSendMessage(const UdpMessage_t* message)
{
    struct sockaddr_in serverAddr;

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, LOCALHOST_IP, &serverAddr.sin_addr) <= 0)
    {
        perror("ClientUdpSendCompletion inet_pton");
        return -1;
    }

    if (xSemaphoreTake(g_clientUdpMutex, portMAX_DELAY) != pdTRUE)
    {
        return -1;
    }

    int sent = sendto(g_clientSocket,
                      message,
                      sizeof(UdpMessage_t),
                      0,
                      (struct sockaddr*)&serverAddr,
                      sizeof(serverAddr));

    xSemaphoreGive(g_clientUdpMutex);

    if (sent < 0)
    {
        perror("ClientUdpSendCompletion sendto");
        return -1;
    }

    return 0;
}

/**
 * @brief Receives a UDP message from the client.
 *
 * Waits for and receives a protocol message using
 * the server UDP socket.
 *
 * @param message Pointer to the destination message buffer.
 *
 * @return 0 on success, -1 on failure.
 */
int ServerUdpReceiveMessage(UdpMessage_t* message)
{
    int received;

    do
    {
        received = recvfrom(g_serverSocket,
                            message,
                            sizeof(UdpMessage_t),
                            0,
                            NULL,
                            NULL);
    }
    while (received < 0 && errno == EINTR);

    if (received < 0)
    {
        perror("ServerUdpReceiveCompletion recvfrom");
        return -1;
    }

    if (received != sizeof(UdpMessage_t))
    {
        printf("Invalid completion message size\n");
        return -1;
    }

    return 0;
}