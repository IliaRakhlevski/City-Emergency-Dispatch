#ifndef VEHICLE_H
#define VEHICLE_H

#include <stdint.h>
#include "event.h"

/**
 * @brief Vehicle availability states.
 */
typedef enum
{
    VEHICLE_AVAILABLE, /**< Vehicle is available for assignment. */
    VEHICLE_BUSY,      /**< Vehicle is processing an event. */
    VEHICLE_BREAK      /**< Vehicle is temporarily out of service. */

} VehicleStatus_t;


/**
 * @brief Represents a department vehicle.
 *
 * Stores vehicle identification, department assignment,
 * current status, and display name used during event
 * processing and monitoring.
 */
typedef struct
{
    uint32_t vehicleId;        /**< Unique vehicle identifier. */
    EventType_t department;    /**< Assigned department type. */
    VehicleStatus_t status;    /**< Current vehicle status. */
    char name[32];             /**< Vehicle display name. */

    uint32_t currentEventId;    /**< Event currently processed by the vehicle. */
    Priority_t currentPriority; /**< Priority of the currently processed event. */

} Vehicle_t;

#endif
