#include "strings.h"

/**
 * @brief Converts an event type to its string representation.
 *
 * @param type Event type.
 *
 * @return Pointer to the corresponding string.
 */
const char* EventTypeToString(EventType_t type)
{
    switch (type)
    {
        case EVENT_AMBULANCE:   return "Ambulance";
        case EVENT_POLICE:      return "Police";
        case EVENT_FIRE:        return "Fire";
        case EVENT_MAINTENANCE: return "Maintenance";
        case EVENT_WASTE:       return "Waste";
        case EVENT_ELECTRIC:    return "Electric";

        default:                return "Unknown";
    }
}

/**
 * @brief Converts a priority value to its string representation.
 *
 * @param priority Event priority level.
 *
 * @return Pointer to the corresponding string.
 */
const char* PriorityToString(Priority_t priority)
{
    switch (priority)
    {
        case PRIORITY_LOW:    return "Low";
        case PRIORITY_MEDIUM: return "Medium";
        case PRIORITY_HIGH:   return "High";

        default:              return "Unknown";
    }
}

/**
 * @brief Converts a vehicle status to its string representation.
 *
 * @param status Vehicle status.
 *
 * @return Pointer to the corresponding string.
 */
const char* VehicleStatusToString(VehicleStatus_t status)
{
    switch (status)
    {
        case VEHICLE_AVAILABLE: return "Available";
        case VEHICLE_BUSY:      return "Busy";
        case VEHICLE_BREAK:     return "Break";

        default:                return "Unknown";
    }
}

/**
 * @brief Converts an event status to its string representation.
 *
 * @param status Event status.
 *
 * @return Pointer to the corresponding string.
 */
const char* EventStatusToString(EventStatus_t status)
{
    switch (status)
    {
        case STATUS_PENDING:    return "Pending";
        case STATUS_ASSIGNED:   return "Assigned";
        case STATUS_COMPLETED:  return "Completed";
        case STATUS_FAILED:     return "Failed";
        case STATUS_IN_PROGRESS: return "In Progress";
        case STATUS_CANCELLED:   return "Cancelled";

        default:                return "Unknown";
    }
}

/**
 * @brief Converts a message type to its string representation.
 *
 * @param type UDP message type.
 *
 * @return Pointer to the corresponding string.
 */
const char* MessageTypeToString(MessageType_t type)
{
    switch (type)
    {
        case MESSAGE_EVENT:
            return "Event";

        case MESSAGE_ACK:
            return "ACK";

        case MESSAGE_COMPLETION:
            return "Completion";

        default:
            return "Unknown";
    }
}

/**
 * @brief Converts a department type to its string representation.
 *
 * @param type Department type.
 *
 * @return Pointer to the corresponding string.
 */
const char* DepartmentTypeToString(DepartmentType_t type)
{
    switch (type)
    {
        case DEPARTMENT_AMBULANCE:
            return "Ambulance";

        case DEPARTMENT_POLICE:
            return "Police";

        case DEPARTMENT_FIRE:
            return "Fire";

        case DEPARTMENT_MAINTENANCE:
            return "Maintenance";

        case DEPARTMENT_WASTE:
            return "Waste";

        case DEPARTMENT_ELECTRIC:
            return "Electric";

        default:
            return "Unknown";
    }
}