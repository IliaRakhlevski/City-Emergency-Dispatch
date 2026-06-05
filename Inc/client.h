#ifndef CLIENT_H
#define CLIENT_H

#define CLIENT_RX_QUEUE_LENGTH 10
#define DEPARTMENT_QUEUE_LENGTH 10
#define CLIENT_UDP_RX_STACK_SIZE 1000
#define CLIENT_DISPATCHER_STACK_SIZE 1000
#define VEHICLE_TASK_STACK_SIZE 1000
#define EVENT_PROCESS_FAILURE_PERCENT 10
#define EVENT_PROCESS_CANCEL_PERCENT 2
#define SHIFT_MANAGER_STACK_SIZE 1000
#define MAX_VEHICLES_PER_DEPARTMENT 2
#define SHIFT_MANAGER_PERIOD_MS 10000
#define DEPARTMENT_OVERLOAD_THRESHOLD 7
#define MAX_EVENT_RETRIES 3
#define FAULT_MANAGER_STACK_SIZE 1000
#define SHIFT_MANAGER_INTERRUPT_PERCENT 2

/**
 * @brief Parameters passed to ShiftManagerTask.
 *
 * Contains a pointer to the department managed by the task
 * and the corresponding department type.
 */
typedef struct
{
    Department_t* department;
    DepartmentType_t departmentType;

} ShiftManagerParams_t;

void RunClient(void);
void ClientUdpRxTask(void *pvParameters);
void ClientDispatcherTask(void *pvParameters);
void VehicleTask(void *pvParameters);
void ShiftManagerTask(void *pvParameters);
void FaultManagerTask(void *pvParameters);

#endif