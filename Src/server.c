#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "server.h"
#include "udp.h"
#include "event.h"
#include "message.h"
#include "event_generator.h"
#include "strings.h"
#include "project_config.h"
#include "database.h"


/* UDP communication queue and tasks */
static QueueHandle_t g_serverUdpTxQueue = NULL;
static TaskHandle_t g_serverUdpTxTask = NULL;
static TaskHandle_t g_serverUdpRxTask = NULL;

/* Event generation task */
static TaskHandle_t g_serverEventGeneratorTask = NULL;


static uint32_t g_serverEventsGenerated = 0;      /**< Total events generated. */
static uint32_t g_serverAcksReceived = 0;         /**< ACK messages received. */
static uint32_t g_serverCompletionsReceived = 0;  /**< Completion messages received. */
static uint32_t g_serverCompleted = 0;            /**< Successfully completed events. */
static uint32_t g_serverFailed = 0;               /**< Failed events. */
static uint32_t g_serverCancelled = 0;            /**< Cancelled events. */


/**
 * @brief Displays server runtime statistics.
 *
 * Prints accumulated statistics collected during
 * server execution, including generated events,
 * acknowledgments, completions, failures, and
 * pending events.
 */
static void PrintServerStatistics(void)
{
    uint32_t pending = g_serverEventsGenerated - g_serverCompletionsReceived;

    printf("\n=== Server Statistics ===\n\n");
    printf("Generated   : %u\n", g_serverEventsGenerated);
    printf("ACKs        : %u\n", g_serverAcksReceived);
    printf("Completions : %u\n", g_serverCompletionsReceived);
    printf("Completed   : %u\n", g_serverCompleted);
    printf("Failed      : %u\n", g_serverFailed);
    printf("Cancelled   : %u\n", g_serverCancelled);
    printf("Pending     : %u\n", pending);
    printf("\n=========================\n");
}

/**
 * @brief Handles server termination requests.
 *
 * Processes application shutdown signals, prints
 * server runtime statistics, releases resources,
 * closes the database and network connections,
 * and terminates the application.
 *
 * @param signal Received system signal number.
 */
static void ServerSignalHandler(int signal)
{
    (void)signal;

    printf("\n\nServer stopping...\n");

    PrintServerStatistics();
    DatabaseClose();
    ServerUdpClose();

    printf("\n");

    exit(0);
}

/**
 * @brief Starts the server application.
 *
 * Initializes the database subsystem, network
 * communication, server tasks, and starts the
 * FreeRTOS scheduler to begin event generation
 * and processing.
 */
void RunServer(void)
{
    signal(SIGINT, ServerSignalHandler);

    printf("\nServer mode started\n");

    if (ServerUdpInit() != 0)
    {
        printf("\nFailed to initialize UDP server\n");
        return;
    }

    if (DatabaseInit() != 0)
    {
        printf("\nFailed to initialize SQLite3 database\n");
        ServerUdpClose();
        return;
    }

    g_serverUdpTxQueue = xQueueCreate(SERVER_TX_QUEUE_LENGTH, sizeof(EventMessage_t));

    if (g_serverUdpTxQueue == NULL)
    {   
        printf("\nFailed to create queue\n");
        ServerUdpClose();
        return;
    }

    if(xTaskCreate( ServerEventGeneratorTask, "Event Generator", EVENT_GENERATOR_STACK_SIZE, NULL, TASK_PRIORITY_EVENT_GENERATOR, &g_serverEventGeneratorTask ) != pdPASS)
    {
        printf("\nFailed to create EventGenerator task\n");
        vQueueDelete(g_serverUdpTxQueue);
        ServerUdpClose();
        return;
    }

    if(xTaskCreate( ServerUdpTxTask, "Server UDP TX", SERVER_UDP_TX_STACK_SIZE, NULL, TASK_PRIORITY_UDP_TX, &g_serverUdpTxTask ) != pdPASS)
    {
        printf("\nFailed to create client UDP TX task\n");
        if(g_serverEventGeneratorTask != NULL)
            vTaskDelete(g_serverEventGeneratorTask);
        vQueueDelete(g_serverUdpTxQueue);
        ServerUdpClose();
        return;
    }

    if(xTaskCreate( ServerUdpRxTask, "Server UDP RX", SERVER_UDP_RX_STACK_SIZE, NULL, TASK_PRIORITY_UDP_RX, &g_serverUdpRxTask ) != pdPASS)
    {
        printf("\nFailed to create server UDP RX task\n");
        if(g_serverEventGeneratorTask != NULL)
            vTaskDelete(g_serverEventGeneratorTask);
        if(g_serverUdpTxTask != NULL)
            vTaskDelete(g_serverUdpTxTask);
        vQueueDelete(g_serverUdpTxQueue);
        ServerUdpClose();
        return;
    }

    vTaskStartScheduler();

    printf("\nScheduler failed!\n");
}

/**
 * @brief Generates simulation events and queues them for transmission.
 *
 * Periodically creates random events, stores them in the
 * database, converts them to network messages, and places
 * them into the server transmission queue for delivery
 * to the client.
 *
 * @param pvParameters Pointer to task parameters (unused).
 */
void ServerEventGeneratorTask(void *pvParameters)
{
    (void)pvParameters;

    Event_t event;
    EventMessage_t message;

    printf("\nServerEventGeneratorTask started\n");

    while (1)
    {
        /* Generate a new event */
        event = CreateRandomEvent();

        /* Update generation statistics */
        g_serverEventsGenerated++;

        /* Store the event in the database */
        DatabaseInsertEvent(&event);

        /* Build the network event message */
        message.eventId = event.eventId;
        message.type = event.type;
        message.priority = event.priority;
        strcpy(message.location, event.location);
        strcpy(message.description, event.description);
        message.timestampStart = event.timestampStart;
        message.retryCount = 0;

        /* Queue the event for UDP transmission */
        if (xQueueSend(g_serverUdpTxQueue, &message, pdMS_TO_TICKS(100)) != pdPASS)
        {
            printf("\nQueue full!\n");
        }

        /* Simulate random event arrival intervals */
        int delaySec = (rand() % 5) + 1;
        vTaskDelay(pdMS_TO_TICKS(delaySec * 1000));
    }
}

/**
 * @brief Transmits event messages to the client via UDP.
 *
 * Retrieves event messages from the server transmission
 * queue, encapsulates them in UDP protocol messages,
 * and sends them to the client.
 *
 * @param pvParameters Pointer to task parameters (unused).
 */
void ServerUdpTxTask(void *pvParameters)
{
    (void)pvParameters;

    EventMessage_t eventMessage;
    UdpMessage_t udpMessage;

    printf("\nServerUdpTxTask started\n");

    while(1)
    {
        if(xQueueReceive(g_serverUdpTxQueue,
                        &eventMessage,
                        portMAX_DELAY) == pdPASS)
        {
            udpMessage.type = MESSAGE_EVENT;
            udpMessage.payload.event = eventMessage;

            printf("\nSending event %u [%s] (%s) %s - %s\n",
                    eventMessage.eventId,
                    EventTypeToString(eventMessage.type),
                    PriorityToString(eventMessage.priority),
                    eventMessage.location,
                    eventMessage.description);

            ServerUdpSendMessage(&udpMessage);
        }
    }
}

/**
 * @brief Receives and processes incoming UDP messages.
 *
 * Listens for acknowledgment and completion messages
 * from the client, updates server statistics, and
 * records event completion information in the database.
 *
 * @param pvParameters Pointer to task parameters (unused).
 */
void ServerUdpRxTask(void *pvParameters)
{
    (void)pvParameters;

    UdpMessage_t udpMessage;

    printf("\nServerUdpRxTask started\n");

    while (1)
    {
        /* Process incoming UDP messages */
        if (ServerUdpReceiveMessage(&udpMessage) == 0)
        {
            /* Handle the received message */
            switch (udpMessage.type)
            {
                case MESSAGE_ACK:
                    printf("\nACK received: event %u\n", udpMessage.payload.ack.eventId);
                     /* Update acknowledgment statistics */
                    g_serverAcksReceived++;
                    break;

                case MESSAGE_COMPLETION:
                    printf("\nCompletion received: event %u, handled by %s, status %s\n",
                                udpMessage.payload.completion.eventId,
                                udpMessage.payload.completion.handledBy,
                                EventStatusToString(udpMessage.payload.completion.status));

                    /* Update completion statistics */
                    g_serverCompletionsReceived++;

                    /* Update completion status counters */
                    switch (udpMessage.payload.completion.status)
                    {
                        case STATUS_COMPLETED:
                            g_serverCompleted++;
                            break;

                        case STATUS_FAILED:
                            g_serverFailed++;
                            break;

                        case STATUS_CANCELLED:
                            g_serverCancelled++;
                            break;

                        default:
                            break;
                    }

                    /* Update the event record in the database */
                    DatabaseUpdateCompletion(&udpMessage.payload.completion);
                    break;

                default:
                    printf("\nServer received unknown message type %s\n", MessageTypeToString(udpMessage.type));
                    break;
            }
        }
    }
}