#ifndef STRINGS_H
#define STRINGS_H

#include "event.h"
#include "vehicle.h"
#include "message.h"
#include "department.h"

const char* EventTypeToString(EventType_t type);
const char* PriorityToString(Priority_t priority);
const char* EventStatusToString(EventStatus_t status);
const char* VehicleStatusToString(VehicleStatus_t status);
const char* MessageTypeToString(MessageType_t type);
const char* DepartmentTypeToString(DepartmentType_t type);

#endif