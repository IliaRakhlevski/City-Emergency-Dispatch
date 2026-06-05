#ifndef EVENT_GEN_H
#define EVENT_GEN_H

#include "event.h"

/**
 * @brief Event generation template.
 *
 * Defines an event description and its associated
 * priority level used by the random event generator.
 */
typedef struct
{
    const char* description;
    Priority_t  priority;

} EventTemplate_t;


Event_t CreateRandomEvent(void);

#endif