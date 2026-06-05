#include <stdio.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "client.h"
#include "udp.h"
#include "event.h"
#include "message.h"
#include "department.h"
#include "vehicle_templates.h"
#include "strings.h"
#include "project_config.h"


/* Department infrastructure */
static Department_t g_departments[DEPARTMENT_COUNT];

/* Vehicle worker tasks */
static TaskHandle_t g_vehicleTasks[DEPARTMENT_COUNT][MAX_VEHICLES_PER_DEPARTMENT];

/* Department shift manager tasks */
static TaskHandle_t g_shiftManagerTasks[DEPARTMENT_COUNT];

/* Fault management resources */
static QueueHandle_t g_faultQueue = NULL;
static TaskHandle_t g_faultManagerTask = NULL;

/* UDP communication resources */
static QueueHandle_t g_clientRxQueue = NULL;
static TaskHandle_t g_clientUdpTxTask = NULL;
static TaskHandle_t g_clientUdpRxTask = NULL;

/* Event dispatcher task */
static TaskHandle_t g_clientDispatcherTask = NULL;

/* Shift manager parameters */
static ShiftManagerParams_t g_shiftManagerParams[DEPARTMENT_COUNT];


static uint32_t g_clientEventsReceived = 0;   /**< Total number of events received from the server. */
static uint32_t g_clientEventsDispatched = 0; /**< Total number of event dispatch operations. */
static uint32_t g_clientCompleted = 0;        /**< Total number of successfully completed events. */
static uint32_t g_clientFailed = 0;           /**< Total number of failed events. */
static uint32_t g_clientCancelled = 0;        /**< Total number of cancelled events. */
static uint32_t g_clientRetries = 0;          /**< Total number of event retry attempts. */


/**
 * @brief Displays client execution statistics.
 *
 * Prints accumulated runtime statistics collected
 * during client operation, including event processing,
 * retries, failures, and pending events.
 */
static void PrintClientStatistics(void)
{
    uint32_t pending =
        g_clientEventsReceived -
        (g_clientCompleted +
         g_clientFailed +
         g_clientCancelled);

    printf("\n=== Client Statistics ===\n\n");

    printf("Received       : %u\n", g_clientEventsReceived);
    printf("Dispatches     : %u\n", g_clientEventsDispatched);
    printf("Completed      : %u\n", g_clientCompleted);
    printf("Failed         : %u\n", g_clientFailed);
    printf("Cancelled      : %u\n", g_clientCancelled);
    printf("Retries        : %u\n", g_clientRetries);
    printf("Pending        : %u\n", pending);
    printf("\n=========================\n\n");
}

/**
 * @brief Handles application termination signal.
 *
 * Performs client shutdown operations, prints runtime
 * statistics, releases allocated resources, and closes
 * network connections before terminating the process.
 *
 * @param signal Received system signal number.
 */
static void ClientSignalHandler(int signal)
{
    (void)signal;

    printf("\n\nClient stopping...\n");
    PrintClientStatistics();
    ClientUdpClose();

    exit(0);
}

/**
 * @brief Cleans up all department resources.
 *
 * Deletes department queues, synchronization objects,
 * and releases any resources allocated for department
 * management during client initialization.
 */
static void CleanupDepartments(void)
{
    for (int i = 0; i < DEPARTMENT_COUNT; i++)
    {
        for (int p = 0; p < PRIORITY_COUNT; p++)
        {
            if (g_departments[i].priorityQueues[p] != NULL)
            {
                vQueueDelete(g_departments[i].priorityQueues[p]);
                g_departments[i].priorityQueues[p] = NULL;
            }
        }
        if (g_departments[i].vehicleMutex != NULL)
        {
            vSemaphoreDelete(g_departments[i].vehicleMutex);
            g_departments[i].vehicleMutex = NULL;
        }

        if (g_departments[i].vehicleEventGroup != NULL)
        {
            vEventGroupDelete(g_departments[i].vehicleEventGroup);
            g_departments[i].vehicleEventGroup = NULL;
        }
    }
}

/**
 * @brief Initializes all service departments.
 *
 * Creates department queues, synchronization objects,
 * and initializes department-specific resources required
 * for event processing.
 *
 * @return 0 on success, -1 on failure.
 */
static int InitDepartments(void)
{
    g_departments[EVENT_AMBULANCE].type = EVENT_AMBULANCE;
    g_departments[EVENT_AMBULANCE].vehicles = ambulanceVehicles;
    g_departments[EVENT_AMBULANCE].vehicleCount = AMBULANCE_VEHICLE_COUNT;

    g_departments[EVENT_POLICE].type = EVENT_POLICE;
    g_departments[EVENT_POLICE].vehicles = policeVehicles;
    g_departments[EVENT_POLICE].vehicleCount = POLICE_VEHICLE_COUNT;

    g_departments[EVENT_FIRE].type = EVENT_FIRE;
    g_departments[EVENT_FIRE].vehicles = fireVehicles;
    g_departments[EVENT_FIRE].vehicleCount = FIRE_VEHICLE_COUNT;

    g_departments[EVENT_MAINTENANCE].type = EVENT_MAINTENANCE;
    g_departments[EVENT_MAINTENANCE].vehicles = maintenanceVehicles;
    g_departments[EVENT_MAINTENANCE].vehicleCount = MAINTENANCE_VEHICLE_COUNT;

    g_departments[EVENT_WASTE].type = EVENT_WASTE;
    g_departments[EVENT_WASTE].vehicles = wasteVehicles;
    g_departments[EVENT_WASTE].vehicleCount = WASTE_VEHICLE_COUNT;

    g_departments[EVENT_ELECTRIC].type = EVENT_ELECTRIC;
    g_departments[EVENT_ELECTRIC].vehicles = electricVehicles;
    g_departments[EVENT_ELECTRIC].vehicleCount = ELECTRIC_VEHICLE_COUNT;

    /* Initialize department resources */
    for (int d = 0; d < DEPARTMENT_COUNT; d++)
    {
        /* Create priority queues */
        for (int p = 0; p < PRIORITY_COUNT; p++)
        {
            g_departments[d].priorityQueues[p] = xQueueCreate(DEPARTMENT_QUEUE_LENGTH, sizeof(EventMessage_t));

            if (g_departments[d].priorityQueues[p] == NULL)
            {
                CleanupDepartments();
                return -1;
            }
        }

        /* Create vehicle synchronization mutex */
        g_departments[d].vehicleMutex = xSemaphoreCreateMutex();

        if (g_departments[d].vehicleMutex == NULL)
        {
            printf("\nFailed to create vehicle mutex\n");
            CleanupDepartments();
            return -1;
        }

        /* Create vehicle notification event group */
        g_departments[d].vehicleEventGroup = xEventGroupCreate();

        if (g_departments[d].vehicleEventGroup == NULL)
        {
            printf("\nFailed to create vehicle event group\n");
            vSemaphoreDelete(g_departments[d].vehicleMutex);
            CleanupDepartments();
            return -1;
        }

        /* Department initialization completed */
        printf("\nDepartment %s initialized\n", DepartmentTypeToString(d));
    }

    return 0;
}

/**
 * @brief Cleans up vehicle tasks.
 *
 * Deletes all vehicle tasks for every department and
 * resets task handles used to manage vehicle execution.
 */
static void CleanupVehicleTasks(void)
{
    for (int d = 0; d < DEPARTMENT_COUNT; d++)
    {
        for (int i = 0; i < MAX_VEHICLES_PER_DEPARTMENT; i++)
        {
            if (g_vehicleTasks[d][i] != NULL)
            {
                vTaskDelete(g_vehicleTasks[d][i]);
                g_vehicleTasks[d][i] = NULL;
            }
        }
    }
}

/**
 * @brief Creates all vehicle tasks.
 *
 * Creates vehicle tasks for each department and assigns
 * the corresponding Vehicle_t structures used during
 * event processing.
 *
 * @return 0 on success, -1 on failure.
 */
static int CreateVehicleTasks(void)
{
    for (int d = 0; d < DEPARTMENT_COUNT; d++)
    {
        for (uint32_t i = 0; i < g_departments[d].vehicleCount; i++)
        {
            if (xTaskCreate(VehicleTask,
                            g_departments[d].vehicles[i].name,
                            VEHICLE_TASK_STACK_SIZE,
                            &g_departments[d].vehicles[i],
                            TASK_PRIORITY_VEHICLE,
                            &g_vehicleTasks[d][i]) != pdPASS)
            {
                printf("\nFailed to create vehicle task %s\n", g_departments[d].vehicles[i].name);
                return -1;
            }
        }
    }

    return 0;
}

/**
 * @brief Cleans up all shift manager tasks.
 *
 * Deletes shift manager tasks created for each department
 * and resets the corresponding task handles.
 */
static void CleanupShiftManagerTasks(void)
{
    for (int d = 0; d < DEPARTMENT_COUNT; d++)
    {
        if (g_shiftManagerTasks[d] != NULL)
        {
            vTaskDelete(g_shiftManagerTasks[d]);
            g_shiftManagerTasks[d] = NULL;
        }
    }
}

/**
 * @brief Creates shift manager tasks for all departments.
 *
 * Creates a dedicated shift manager task for each department
 * and initializes the task parameters required for vehicle
 * monitoring and break management.
 *
 * @return 0 on success, -1 on failure.
 */
static int CreateShiftManagerTasks(void)
{
    for (int d = 0; d < DEPARTMENT_COUNT; d++)
    {
        g_shiftManagerParams[d].department = &g_departments[d];
        g_shiftManagerParams[d].departmentType = (DepartmentType_t)d;

        if (xTaskCreate(ShiftManagerTask,
                        "ShiftManager",
                        SHIFT_MANAGER_STACK_SIZE,
                        &g_shiftManagerParams[d],
                        TASK_PRIORITY_MANAGER,
                        &g_shiftManagerTasks[d]) != pdPASS)
        {
            printf("\nFailed to create ShiftManager for %s\n", DepartmentTypeToString((DepartmentType_t)d));
            return -1;
        }
    }

    return 0;
}

/**
 * @brief Creates the fault manager task.
 *
 * Creates a dedicated task responsible for handling failed
 * events, managing retry attempts, and reporting final
 * failures when the retry limit is reached.
 *
 * @return 0 on success, -1 on failure.
 */
static int CreateFaultManagerTask(void)
{
    if (xTaskCreate(FaultManagerTask,
                    "FaultManager",
                    FAULT_MANAGER_STACK_SIZE,
                    NULL,
                    TASK_PRIORITY_MANAGER,
                    &g_faultManagerTask) != pdPASS)
    {
        printf("\nFailed to create FaultManager task\n");
        return -1;
    }

    return 0;
}

/**
 * @brief Cleans up the fault manager task.
 *
 * Deletes the fault manager task and resets the
 * corresponding task handle.
 */
static void CleanupFaultManagerTask(void)
{
    if (g_faultManagerTask != NULL)
    {
        vTaskDelete(g_faultManagerTask);
        g_faultManagerTask = NULL;
    }
}

/**
 * @brief Starts the client application.
 *
 * Initializes client resources, communication channels,
 * department infrastructure, worker tasks, and enters
 * the FreeRTOS scheduler to begin event processing.
 */
void RunClient(void)
{
    signal(SIGINT, ClientSignalHandler);

    printf("\nClient mode started\n");

    if (ClientUdpInit() != 0)
    {
        printf("\nFailed to initialize UDP client\n");
        return;
    }

    g_clientRxQueue = xQueueCreate(CLIENT_RX_QUEUE_LENGTH,
                               sizeof(EventMessage_t));

    if (g_clientRxQueue == NULL)
    {
        printf("\nFailed to create client RX queue\n");
        ClientUdpClose();
        return;
    }

    g_faultQueue = xQueueCreate(CLIENT_RX_QUEUE_LENGTH,
                            sizeof(EventMessage_t));

    if (g_faultQueue == NULL)
    {
        printf("\nFailed to create fault queue\n");
        vQueueDelete(g_clientRxQueue);
        ClientUdpClose();
        return;
    }

    if (CreateFaultManagerTask() != 0)
    {
        vQueueDelete(g_faultQueue);
        vQueueDelete(g_clientRxQueue);
        ClientUdpClose();
        return;
    }

    if (InitDepartments() != 0)
    {
        printf("\nFailed to initialize departments\n");
        vQueueDelete(g_faultQueue);
        vQueueDelete(g_clientRxQueue);
        CleanupFaultManagerTask();
        ClientUdpClose();
        return;
    }

    if(CreateVehicleTasks()!= 0)
    {
        printf("\nFailed to initialize vehicles\n");
        vQueueDelete(g_faultQueue);
        vQueueDelete(g_clientRxQueue);
        CleanupFaultManagerTask();
        CleanupVehicleTasks();
        CleanupDepartments();
        ClientUdpClose();
        return;
    }

    if(CreateShiftManagerTasks() != 0)
    {
        printf("\nFailed to initialize Shift managers tasks\n");
        CleanupShiftManagerTasks();
        CleanupVehicleTasks();
        CleanupFaultManagerTask();
        CleanupDepartments();
        vQueueDelete(g_faultQueue);
        vQueueDelete(g_clientRxQueue);
        ClientUdpClose();
        return;
    }

    if(xTaskCreate( ClientUdpRxTask, "Client UDP RX", CLIENT_UDP_RX_STACK_SIZE, NULL, TASK_PRIORITY_UDP_RX, &g_clientUdpRxTask ) != pdPASS)
    {
        printf("\nFailed to create client UDP RX task\n");
        CleanupShiftManagerTasks();
        CleanupVehicleTasks();
        CleanupFaultManagerTask();
        CleanupDepartments();
        vQueueDelete(g_faultQueue);
        vQueueDelete(g_clientRxQueue);
        ClientUdpClose();
        return;
    }

    if(xTaskCreate(ClientDispatcherTask, "Dispatcher", CLIENT_DISPATCHER_STACK_SIZE, NULL, TASK_PRIORITY_DISPATCHER, &g_clientDispatcherTask) != pdPASS)
    {
        printf("\nFailed to create client Dispatcher task\n");
        if (g_clientUdpRxTask != NULL)
        {
            vTaskDelete(g_clientUdpRxTask);
            g_clientUdpRxTask = NULL;
        }
        CleanupShiftManagerTasks();
        CleanupVehicleTasks();
        CleanupFaultManagerTask();
        CleanupDepartments();
        vQueueDelete(g_faultQueue);
        vQueueDelete(g_clientRxQueue);
        ClientUdpClose();
        return;
    }

    vTaskStartScheduler();

    printf("\nClient scheduler failed!\n");
}

/**
 * @brief Receives and processes incoming UDP messages.
 *
 * Waits for UDP messages from the server, sends
 * acknowledgments for received events, and forwards
 * event messages to the client event queue for
 * further dispatching.
 *
 * @param pvParameters Pointer to task parameters (unused).
 */
void ClientUdpRxTask(void *pvParameters)
{
    (void)pvParameters;

    UdpMessage_t udpMessage;

    while (1)
    {
        if (ClientUdpReceiveMessage(&udpMessage) == 0)
        {
            if (udpMessage.type != MESSAGE_EVENT)
            {
                printf("\nUnexpected message type %s\n", MessageTypeToString(udpMessage.type));
                continue;
            }

            g_clientEventsReceived++;

            EventMessage_t* event = &udpMessage.payload.event;

            printf("\nUDP Rx Task: Event %u [%s] (%s) %s - %s\n", event->eventId, EventTypeToString(event->type), 
                                                        PriorityToString(event->priority), event->location, event->description);

            AckMessage_t ack;
            ack.eventId = event->eventId;

            UdpMessage_t ackMessage;
            ackMessage.type = MESSAGE_ACK;
            ackMessage.payload.ack = ack;

            ClientUdpSendMessage(&ackMessage);

            xQueueSend(g_clientRxQueue, event, portMAX_DELAY);
        }
    }
}

/**
 * @brief Maps an event type to the corresponding department.
 *
 * Determines which department is responsible for handling
 * the specified event type and returns the associated
 * department identifier.
 *
 * @param type Event type.
 *
 * @return Department responsible for the event.
 */
static DepartmentType_t GetDepartmentByEventType(EventType_t type)
{
    switch (type)
    {
        case EVENT_AMBULANCE:
            return DEPARTMENT_AMBULANCE;

        case EVENT_POLICE:
            return DEPARTMENT_POLICE;

        case EVENT_FIRE:
            return DEPARTMENT_FIRE;

        case EVENT_MAINTENANCE:
            return DEPARTMENT_MAINTENANCE;

        case EVENT_WASTE:
            return DEPARTMENT_WASTE;

        case EVENT_ELECTRIC:
            return DEPARTMENT_ELECTRIC;

        default:
            return DEPARTMENT_COUNT;
    }
}

/**
 * @brief Dispatches events to department priority queues.
 *
 * Receives events from the client event queue, determines
 * the responsible department, and forwards each event to
 * the appropriate department queue based on its priority.
 *
 * @param pvParameters Pointer to task parameters (unused).
 */
void ClientDispatcherTask(void *pvParameters)
{
    (void)pvParameters;

    EventMessage_t message;

    printf("\nClientDispatcherTask started\n");

    while (1)
    {
        /* Wait for incoming events */
        if (xQueueReceive(g_clientRxQueue,
                          &message,
                          portMAX_DELAY) == pdPASS)
        {
            DepartmentType_t departmentType = GetDepartmentByEventType(message.type);

            /* Validate event parameters */
            if (departmentType >= DEPARTMENT_COUNT ||
                message.priority >= PRIORITY_COUNT)
            {
                printf("\nDispatcher: Invalid event type or priority\n");
                continue;
            }

            /* Select the target department queue */
            QueueHandle_t q = g_departments[departmentType].priorityQueues[message.priority];

            /* Dispatch the event to the department */
            if (xQueueSend(q, &message, pdMS_TO_TICKS(100)) != pdPASS)
            {
                printf("\nDispatcher: Department queue full\n");
            }
            else
            {
                /* Update dispatch statistics */
                g_clientEventsDispatched++;

                printf("\nDispatcher: Dispatching event %u -> %s [%s]\n",
                        message.eventId,
                        EventTypeToString(message.type),
                        PriorityToString(message.priority));
            }
        }
    }
}

/**
 * @brief Returns the simulated processing time for an event.
 *
 * Determines the processing duration associated with the
 * specified event type and returns the corresponding time
 * in seconds.
 *
 * @param type Event type.
 *
 * @return Processing time in seconds.
 */
static int GetProcessingTimeSec(EventType_t type)
{
    switch (type)
    {
        case EVENT_AMBULANCE:
            return (rand() % 3) + 2;  // 2-4 sec

        case EVENT_FIRE:
            return (rand() % 5) + 4;  // 4-8 sec

        case EVENT_POLICE:
            return (rand() % 4) + 3;  // 3-6 sec

        case EVENT_MAINTENANCE:
            return (rand() % 6) + 5;  // 5-10 sec

        case EVENT_WASTE:
            return (rand() % 4) + 2;  // 2-5 sec

        case EVENT_ELECTRIC:
            return (rand() % 7) + 4;  // 4-10 sec

        default:
            return 3;
    }
}

/**
 * @brief Retrieves the highest-priority available event.
 *
 * Checks the department priority queues in priority order
 * (High, Medium, Low) and retrieves the first available
 * event for processing.
 *
 * @param department Pointer to the department structure.
 * @param message Pointer to the destination event message.
 *
 * @return 1 if an event was received, 0 otherwise.
 */
static int ReceiveDepartmentEvent(Department_t* department, EventMessage_t* message)
{
    if (xQueueReceive(department->priorityQueues[PRIORITY_HIGH],
                      message,
                      pdMS_TO_TICKS(100)) == pdPASS)
        return PRIORITY_HIGH;

    if (xQueueReceive(department->priorityQueues[PRIORITY_MEDIUM],
                      message,
                      pdMS_TO_TICKS(100)) == pdPASS)
        return PRIORITY_MEDIUM;

    if (xQueueReceive(department->priorityQueues[PRIORITY_LOW],
                      message,
                      pdMS_TO_TICKS(100)) == pdPASS)
        return PRIORITY_LOW;

    return -1;
}

/**
 * @brief Sends an event completion message to the server.
 *
 * Creates a completion message containing the event
 * identifier, processing vehicle information, completion
 * timestamp, and final event status, then transmits it
 * to the server via UDP.
 *
 * @param vehicle Pointer to the vehicle that handled the event.
 * @param eventMessage Pointer to the processed event message.
 * @param status Final event status.
 */
static void SendCompletionMessage(const Vehicle_t* vehicle, const EventMessage_t* eventMessage, EventStatus_t status)
{
    CompletionMessage_t completion;
    UdpMessage_t udpMessage;

    completion.eventId = eventMessage->eventId;
    strcpy(completion.handledBy, vehicle->name);
    completion.timestampEnd = time(NULL);
    completion.status = status;

    udpMessage.type = MESSAGE_COMPLETION;
    udpMessage.payload.completion = completion;

    if (ClientUdpSendMessage(&udpMessage) != 0)
    {
        printf("\nSend Completion Message: Failed to send completion for event %u\n", eventMessage->eventId);
    }
}

/**
 * @brief Retrieves the current vehicle status.
 *
 * Safely reads the vehicle status using the department
 * mutex to ensure synchronized access between vehicle
 * and shift manager tasks.
 *
 * @param department Pointer to the department structure.
 * @param vehicle Pointer to the vehicle structure.
 *
 * @return Current vehicle status.
 */
static VehicleStatus_t GetVehicleStatus(Department_t* department, Vehicle_t* vehicle)
{
    VehicleStatus_t status;

    xSemaphoreTake(department->vehicleMutex, portMAX_DELAY);
    status = vehicle->status;
    xSemaphoreGive(department->vehicleMutex);

    return status;
}

/**
 * @brief Updates the vehicle status.
 *
 * Safely modifies the vehicle status using the department
 * mutex to ensure synchronized access between vehicle
 * and shift manager tasks.
 *
 * @param department Pointer to the department structure.
 * @param vehicle Pointer to the vehicle structure.
 * @param status New vehicle status.
 */
static void SetVehicleStatus(Department_t* department, Vehicle_t* vehicle, VehicleStatus_t status)
{
    xSemaphoreTake(department->vehicleMutex, portMAX_DELAY);
    vehicle->status = status;
    xSemaphoreGive(department->vehicleMutex);
}

/**
 * @brief Places a vehicle into break mode.
 *
 * Sets the vehicle status to VEHICLE_BREAK and updates
 * the corresponding event group bit used for vehicle
 * break monitoring.
 *
 * @param department Pointer to the department structure.
 * @param vehicle Pointer to the vehicle structure.
 * @param vehicleIndex Index of the vehicle within the department.
 */
static void SetVehicleBreak(Department_t* department, Vehicle_t* vehicle, uint32_t vehicleIndex)
{
    SetVehicleStatus(department, vehicle, VEHICLE_BREAK);
    xEventGroupSetBits(department->vehicleEventGroup, VEHICLE_BREAK_BIT(vehicleIndex));
}

/**
 * @brief Returns a vehicle from break mode to active duty.
 *
 * Restores the vehicle status to VEHICLE_AVAILABLE and
 * clears the corresponding event group bit used for
 * vehicle break monitoring.
 *
 * @param department Pointer to the department structure.
 * @param vehicle Pointer to the vehicle structure.
 * @param vehicleIndex Index of the vehicle within the department.
 */
static void ClearVehicleBreak(Department_t* department, Vehicle_t* vehicle, uint32_t vehicleIndex)
{
    SetVehicleStatus(department, vehicle, VEHICLE_AVAILABLE);
    xEventGroupClearBits(department->vehicleEventGroup, VEHICLE_BREAK_BIT(vehicleIndex));
}

/**
 * @brief Processes an assigned event.
 *
 * Simulates event handling by the specified vehicle,
 * updates vehicle status during processing, and returns
 * the final outcome of the operation.
 *
 * @param department Pointer to the department structure.
 * @param vehicle Pointer to the processing vehicle.
 * @param message Pointer to the event message.
 *
 * @return Event processing result.
 */
static EventStatus_t ProcessEvent(Department_t* department, Vehicle_t* vehicle, const EventMessage_t* message)
{
    int processingTime = GetProcessingTimeSec(vehicle->department);

    printf("\nProcessEvent: %s processing event %u (%d sec)\n", vehicle->name, message->eventId, processingTime);

    for (int sec = 0; sec < processingTime; sec++)
    {
        /* Abort processing if the vehicle was taken out of service */
        if (GetVehicleStatus(department, vehicle) == VEHICLE_BREAK)
        {
            return STATUS_CANCELLED;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    /* Simulate processing failure */
    if ((rand() % 100) < EVENT_PROCESS_FAILURE_PERCENT)
    {
        return STATUS_FAILED;
    }

    return STATUS_COMPLETED;
}

/**
 * @brief Forwards a failed event to the fault manager.
 *
 * Places the failed event into the fault manager queue
 * for retry handling and failure recovery processing.
 *
 * @param message Pointer to the failed event message.
 */
static void SendFailedEventToFaultManager(const EventMessage_t* message)
{
    if (xQueueSend(g_faultQueue, message, pdMS_TO_TICKS(100)) != pdPASS)
    {
        printf("\nFaultManager: Fault queue full, event %u lost\n", message->eventId);
    }
}

/**
 * @brief Reports a final event failure to the server.
 *
 * Creates and sends a completion message with
 * STATUS_FAILED after the fault manager determines
 * that the retry limit has been reached.
 *
 * @param eventMessage Pointer to the failed event message.
 */
static void SendFailureCompletion(const EventMessage_t* eventMessage)
{
    CompletionMessage_t completion;
    UdpMessage_t udpMessage;

    completion.eventId = eventMessage->eventId;

    strcpy(completion.handledBy, "FaultManager");

    completion.timestampEnd = time(NULL);
    completion.status = STATUS_FAILED;

    udpMessage.type = MESSAGE_COMPLETION;
    udpMessage.payload.completion = completion;

    ClientUdpSendMessage(&udpMessage);
}

/**
 * @brief Sets the current event assigned to a vehicle.
 *
 * Updates current event information using the department
 * mutex to synchronize access between vehicle and shift
 * manager tasks.
 *
 * @param department Pointer to the department structure.
 * @param vehicle Pointer to the vehicle structure.
 * @param eventMessage Pointer to the assigned event message.
 */
static void SetVehicleCurrentEvent(Department_t* department,
                                   Vehicle_t* vehicle,
                                   const EventMessage_t* eventMessage)
{
    if (xSemaphoreTake(department->vehicleMutex, portMAX_DELAY) == pdTRUE)
    {
        vehicle->currentEventId = eventMessage->eventId;
        vehicle->currentPriority = eventMessage->priority;

        xSemaphoreGive(department->vehicleMutex);
    }
}

/**
 * @brief Clears the current event assigned to a vehicle.
 *
 * Resets current event information after processing is
 * completed, failed, or cancelled.
 *
 * @param department Pointer to the department structure.
 * @param vehicle Pointer to the vehicle structure.
 */
static void ClearVehicleCurrentEvent(Department_t* department,
                                     Vehicle_t* vehicle)
{
    if (xSemaphoreTake(department->vehicleMutex, portMAX_DELAY) == pdTRUE)
    {
        vehicle->currentEventId = 0;
        vehicle->currentPriority = PRIORITY_COUNT;

        xSemaphoreGive(department->vehicleMutex);
    }
}

/**
 * @brief Checks whether a busy vehicle can be interrupted.
 *
 * Determines whether the vehicle is currently processing
 * a lower-priority event while high-priority events are
 * waiting in the department queue.
 *
 * @param department Pointer to the department structure.
 * @param vehicle Pointer to the vehicle structure.
 *
 * @return 1 if the vehicle should be interrupted, 0 otherwise.
 */
static int ShouldInterruptVehicle(Department_t* department, Vehicle_t* vehicle)
{
    int shouldInterrupt = 0;

    uint32_t highCount = uxQueueMessagesWaiting(department->priorityQueues[PRIORITY_HIGH]);

    if (xSemaphoreTake(department->vehicleMutex, portMAX_DELAY) == pdTRUE)
    {
        if (highCount > 0 && vehicle->currentPriority < PRIORITY_HIGH)
        {
            shouldInterrupt = 1;
        }

        xSemaphoreGive(department->vehicleMutex);
    }

    return shouldInterrupt;
}

/**
 * @brief Main vehicle task responsible for event processing.
 *
 * Continuously retrieves events from the assigned department
 * priority queues, processes them, and reports the final
 * outcome. Failed events are forwarded to the fault manager,
 * while completed and cancelled events are reported directly
 * to the server.
 *
 * @param pvParameters Pointer to the associated Vehicle_t structure.
 */
void VehicleTask(void *pvParameters)
{
    Vehicle_t* vehicle = (Vehicle_t*)pvParameters;

    EventMessage_t message;

    printf("\nVehicle task started: %s\n", vehicle->name);

    while (1)
    {
        Department_t* department = &g_departments[vehicle->department];

        if (GetVehicleStatus(department, vehicle) != VEHICLE_AVAILABLE)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        /* Wait for a new event assignment */
        int priority = ReceiveDepartmentEvent(department, &message);

        if (priority >= 0)
        {
            printf("\nVehicle: %s picked %s event %u\n", vehicle->name, PriorityToString(priority), message.eventId);

            SetVehicleCurrentEvent(department, vehicle, &message);

            /* Mark vehicle as busy before processing the event */
            SetVehicleStatus(department, vehicle, VEHICLE_BUSY);

            /* Process the assigned event */
            EventStatus_t result = ProcessEvent(department, vehicle, &message);

            /* Update client statistics based on processing result */
            switch (result)
            {
                case STATUS_COMPLETED:
                    g_clientCompleted++;
                    break;

                case STATUS_CANCELLED:
                    g_clientCancelled++;
                    break;

                default:
                    break;
            }

            /* Return the vehicle to the available state */
            SetVehicleStatus(department, vehicle, VEHICLE_AVAILABLE);

            ClearVehicleCurrentEvent(department, vehicle);

            printf("\nVehicle: %s finished event %u with status %s\n", vehicle->name, message.eventId, EventStatusToString(result));

            if (result == STATUS_FAILED)
            {
                /* Failed events are forwarded to the Fault Manager for retry handling */
                SendFailedEventToFaultManager(&message);
            }
            else
            {
                /* Report final event status to the server */
                SendCompletionMessage(vehicle, &message, result);
            }
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

/**
 * @brief Monitors vehicle break status within a department.
 *
 * Checks the department event group and reports vehicles
 * currently marked as being on break. Used by the shift
 * manager for department status monitoring.
 *
 * @param department Pointer to the department structure.
 * @param departmentType Department type identifier.
 */
static void MonitorDepartmentVehicles(Department_t* department, DepartmentType_t departmentType)
{
    EventBits_t bits = xEventGroupGetBits(department->vehicleEventGroup);

    if (bits == 0)
        return;

    for (uint32_t i = 0; i < department->vehicleCount; i++)
    {
        if (bits & VEHICLE_BREAK_BIT(i))
        {
            printf("\nShiftManager: Departament - %s, Vehicle - %s[BREAK] \n", DepartmentTypeToString(departmentType), department->vehicles[i].name);
        }
    }
}

/**
 * @brief Checks a department for queue overload conditions.
 *
 * Monitors the number of pending events in the department
 * priority queues and reports overload situations when
 * queue utilization exceeds the configured threshold.
 *
 * @param department Pointer to the department structure.
 * @param departmentType Department type identifier.
 */
static void CheckDepartmentOverload(Department_t* department, DepartmentType_t departmentType)
{
    for (int p = 0; p < PRIORITY_COUNT; p++)
    {
        UBaseType_t count = uxQueueMessagesWaiting(department->priorityQueues[p]);

        if (count >= DEPARTMENT_OVERLOAD_THRESHOLD)
        {
            printf("\nShiftManager: OVERLOAD - %s department %s queue has %u events\n",
                   DepartmentTypeToString(departmentType),
                   PriorityToString((Priority_t)p),
                   (unsigned)count);
        }
    }
}

/**
 * @brief Counts vehicles with the specified status.
 *
 * Iterates through all department vehicles and
 * returns the number of vehicles currently in
 * the requested state.
 *
 * @param department Pointer to the department.
 * @param status Vehicle status to count.
 *
 * @return Number of matching vehicles.
 */
static uint32_t CountVehiclesByStatus(Department_t* department, VehicleStatus_t status)
{
    uint32_t count = 0;

    for (uint32_t i = 0; i < department->vehicleCount; i++)
    {
        if (GetVehicleStatus(department, &department->vehicles[i]) == status)
        {
            count++;
        }
    }

    return count;
}

/**
 * @brief Counts pending events in department queues.
 *
 * @param department Pointer to the department.
 *
 * @return Number of pending events.
 */
static uint32_t CountPendingDepartmentEvents(Department_t* department)
{
    uint32_t count = 0;

    for (int p = 0; p < PRIORITY_COUNT; p++)
    {
        count += uxQueueMessagesWaiting(department->priorityQueues[p]);
    }

    return count;
}

/**
 * @brief Manages vehicle availability within a department.
 *
 * Periodically monitors department activity, detects queue
 * overload conditions, and controls vehicle break schedules.
 * Vehicles may be temporarily placed on break and later
 * returned to service based on their current availability.
 *
 * @param pvParameters Pointer to ShiftManagerParams_t structure.
 */
void ShiftManagerTask(void *pvParameters)
{
    ShiftManagerParams_t* params = (ShiftManagerParams_t*)pvParameters;

    Department_t* department = params->department;
    DepartmentType_t departmentType = params->departmentType;

    printf("\nShiftManager started for %s\n", DepartmentTypeToString(departmentType));

    while (1)
    {
        /* Monitor department state */
        MonitorDepartmentVehicles(department, departmentType);
        CheckDepartmentOverload(department, departmentType);

        /* Calculate current department capacity */
        uint32_t pendingEvents = CountPendingDepartmentEvents(department);
        uint32_t availableCount = CountVehiclesByStatus(department, VEHICLE_AVAILABLE);

        /* Check vehicle availability */
        for (uint32_t i = 0; i < department->vehicleCount; i++)
        {
            Vehicle_t* vehicle = &department->vehicles[i];

            VehicleStatus_t status = GetVehicleStatus(department, vehicle);

            /* Manage vehicle breaks */
            if (status == VEHICLE_AVAILABLE)
            {
                /* Keep at least one vehicle available for dispatch */
                if (availableCount > 1 && (rand() % 100) < 5)
                {
                    SetVehicleBreak(department, vehicle, i);

                    printf("\nShiftManager: %s sent to break\n", vehicle->name);
                }
            }
            /* Return vehicles from break when work is waiting */
            else if (status == VEHICLE_BREAK)
            {
                if (pendingEvents > 0)
                {
                    ClearVehicleBreak(department, vehicle, i);

                    printf("\nShiftManager: %s returned to duty\n", vehicle->name);

                    pendingEvents--;
                }
            }
            /* Interrupt busy vehicles when high-priority work is waiting */
            else if (status == VEHICLE_BUSY)
            {
                uint32_t highCount = uxQueueMessagesWaiting(department->priorityQueues[PRIORITY_HIGH]);
                if (ShouldInterruptVehicle(department, vehicle))
                {
                    SetVehicleBreak(department, vehicle, i);

                    printf("\nShiftManager: %s interrupted event %u for high priority event\n", vehicle->name, vehicle->currentEventId);
                }
            }
        }
        /* Wait for the next monitoring cycle */
        vTaskDelay(pdMS_TO_TICKS(SHIFT_MANAGER_PERIOD_MS));
    }
}

/**
 * @brief Handles failed events and manages retry attempts.
 *
 * Continuously processes events received from the fault
 * manager queue, requeues failed events for another
 * processing attempt, and reports final failures when
 * the maximum retry limit is reached.
 *
 * @param pvParameters Pointer to task parameters (unused).
 */
void FaultManagerTask(void *pvParameters)
{
    (void)pvParameters;

    EventMessage_t message;

    printf("\nFaultManagerTask started\n");

    while (1)
    {
        /* Wait for a failed event */
        if (xQueueReceive(g_faultQueue, &message, portMAX_DELAY) == pdPASS)
        {
            /* Retry the event if attempts remain */
            if (message.retryCount < MAX_EVENT_RETRIES)
            {
                message.retryCount++;
                g_clientRetries++;

                printf("\nFaultManager: requeue failed event %u, retry %u/%u\n", message.eventId, message.retryCount, MAX_EVENT_RETRIES);

                /* Return the event to the dispatcher queue */
                if (xQueueSend(g_clientRxQueue, &message, pdMS_TO_TICKS(100)) != pdPASS)
                {
                    printf("\nFaultManager: failed to requeue event %u\n", message.eventId);
                }
            }
            else
            {
                /* Report permanent failure */
                g_clientFailed++;
                printf("\nFaultManager: retry limit reached for event %u\n", message.eventId);
                SendFailureCompletion(&message);
            }
        }
    }
}