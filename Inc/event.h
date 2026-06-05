#ifndef EVENT_H
#define EVENT_H

#include <stdint.h>
#include <time.h>

/**
 * @brief Supported event categories.
 */
typedef enum
{
    EVENT_AMBULANCE,     /**< Medical emergency event. */
    EVENT_POLICE,        /**< Police incident event. */
    EVENT_FIRE,          /**< Fire emergency event. */
    EVENT_MAINTENANCE,   /**< Municipal maintenance event. */
    EVENT_WASTE,         /**< Waste management event. */
    EVENT_ELECTRIC,      /**< Electrical infrastructure event. */
    EVENT_TYPE_COUNT     /**< Total number of event types. */

} EventType_t;


/**
 * @brief Event priority levels.
 */
typedef enum
{
    PRIORITY_LOW = 0,    /**< Low priority event. */
    PRIORITY_MEDIUM,     /**< Medium priority event. */
    PRIORITY_HIGH,       /**< High priority event. */
    PRIORITY_COUNT       /**< Total number of priority levels. */

} Priority_t;


/**
 * @brief Event processing status.
 */
typedef enum
{
    STATUS_PENDING,      /**< Event created but not yet assigned. */
    STATUS_ASSIGNED,     /**< Event assigned to a vehicle. */
    STATUS_IN_PROGRESS,  /**< Event currently being processed. */
    STATUS_COMPLETED,    /**< Event successfully completed. */
    STATUS_FAILED,       /**< Event processing failed. */
    STATUS_CANCELLED     /**< Event processing was cancelled. */

} EventStatus_t;


/**
 * @brief Represents a simulation event.
 *
 * Stores all information associated with an event
 * throughout its lifecycle, from creation to final
 * completion.
 */
typedef struct
{
    uint32_t eventId;         /**< Unique event identifier. */
    EventType_t type;         /**< Event type. */
    Priority_t priority;      /**< Event priority level. */
    EventStatus_t status;     /**< Current event status. */
    char location[64];        /**< Event location. */
    char description[256];    /**< Event description. */
    time_t timestampStart;    /**< Event creation timestamp. */
    time_t timestampEnd;      /**< Event completion timestamp. */
    
} Event_t;


# endif
