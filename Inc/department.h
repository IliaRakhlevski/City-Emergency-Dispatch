#ifndef DEPARTMENT_H
#define DEPARTMENT_H

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
#include "event.h"
#include "vehicle.h"


#define VEHICLE_BREAK_BIT(index)   (1U << (index))


/**
 * @brief Represents a city service department.
 *
 * Stores department resources, including vehicles,
 * priority event queues, synchronization objects,
 * and vehicle monitoring infrastructure.
 */
typedef struct
{
    EventType_t type;                             /**< Department event type. */
    Vehicle_t* vehicles;                          /**< Array of department vehicles. */
    uint32_t vehicleCount;                        /**< Number of vehicles in the department. */
    QueueHandle_t priorityQueues[PRIORITY_COUNT]; /**< Department priority queues. */
    SemaphoreHandle_t vehicleMutex;               /**< Protects vehicle state access. */
    EventGroupHandle_t vehicleEventGroup;         /**< Vehicle monitoring event group. */

} Department_t;

/**
 * @brief Supported city service departments.
 */
typedef enum
{
    DEPARTMENT_AMBULANCE = 0,
    DEPARTMENT_POLICE,
    DEPARTMENT_FIRE,
    DEPARTMENT_MAINTENANCE,
    DEPARTMENT_WASTE,
    DEPARTMENT_ELECTRIC,
    DEPARTMENT_COUNT

} DepartmentType_t;

#endif