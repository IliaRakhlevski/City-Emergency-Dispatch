#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include <time.h>
#include "event.h"

/**
 * @brief Event message transmitted between server and client.
 *
 * Contains all information required to dispatch and
 * process an event.
 */
typedef struct
{
    uint32_t eventId;          /**< Unique event identifier. */
    EventType_t type;          /**< Event type. */
    Priority_t priority;       /**< Event priority level. */
    char location[64];         /**< Event location. */
    char description[256];     /**< Event description. */
    time_t timestampStart;     /**< Event creation timestamp. */
    uint8_t retryCount;        /**< Number of processing attempts. */

} EventMessage_t;

/**
 * @brief Event completion message.
 *
 * Sent by the client to report the final outcome
 * of event processing.
 */
typedef struct
{
    uint32_t eventId;          /**< Processed event identifier. */
    char handledBy[32];        /**< Vehicle that handled the event. */
    time_t timestampEnd;       /**< Event completion timestamp. */
    EventStatus_t status;      /**< Final event status. */

} CompletionMessage_t;

/**
 * @brief Event acknowledgment message.
 *
 * Sent by the client to confirm successful receipt
 * of an event message.
 */
typedef struct
{
    uint32_t eventId;       /**< Acknowledged event identifier. */

} AckMessage_t;

/**
 * @brief Supported UDP message types.
 */
typedef enum
{
    MESSAGE_EVENT,       /**< Event dispatch message. */
    MESSAGE_ACK,         /**< Event acknowledgment message. */
    MESSAGE_COMPLETION   /**< Event completion message. */

} MessageType_t;

/**
 * @brief Generic UDP protocol message.
 *
 * Wraps all supported message types exchanged between
 * the server and client using a tagged union.
 */
typedef struct
{
    MessageType_t type;     /**< Payload type identifier. */

    union
    {
        EventMessage_t event;           /**< Event message payload. */
        AckMessage_t ack;               /**< Acknowledgment payload. */
        CompletionMessage_t completion; /**< Completion payload. */
        
    } payload;

} UdpMessage_t;

#endif