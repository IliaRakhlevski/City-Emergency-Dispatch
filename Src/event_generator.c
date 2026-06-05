#include <string.h>  
#include <stdlib.h> 
#include <time.h>  
#include "event_generator.h"
#include "event_templates.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

static uint32_t g_nextEventId = 1;

/**
 * @brief Generates a unique event identifier.
 *
 * Produces a sequential identifier used to uniquely
 * distinguish events throughout their lifecycle in
 * the simulation.
 *
 * @return Generated event identifier.
 */
static uint32_t GenerateEventId(void)
{
    return g_nextEventId++;
}

/**
 * @brief Generates a random simulation event.
 *
 * Creates an event with a unique identifier, random
 * department type, location, description, priority,
 * and creation timestamp using predefined event
 * templates.
 *
 * @return Generated event structure.
 */
Event_t CreateRandomEvent(void)
{
    Event_t event;

    event.type = rand() % EVENT_TYPE_COUNT;

    const EventTemplate_t* template = NULL;

    switch (event.type)
    {
        case EVENT_AMBULANCE:
            template = &ambulanceEvents[rand() % ARRAY_SIZE(ambulanceEvents)];
            break;

        case EVENT_POLICE:
            template = &policeEvents[rand() % ARRAY_SIZE(policeEvents)];
            break;

        case EVENT_FIRE:
            template = &fireEvents[rand() % ARRAY_SIZE(fireEvents)];
            break;

        case EVENT_MAINTENANCE:
            template = &maintenanceEvents[rand() % ARRAY_SIZE(maintenanceEvents)];
            break;

        case EVENT_WASTE:
            template = &wasteEvents[rand() % ARRAY_SIZE(wasteEvents)];
            break;

        case EVENT_ELECTRIC:
            template = &electricEvents[rand() % ARRAY_SIZE(electricEvents)];
            break;

        default:
            template = &ambulanceEvents[0];
            break;
    }

    event.eventId = GenerateEventId();
    strcpy(event.description, template->description);
    event.priority = template->priority;
    event.timestampStart = time(NULL);
    event.timestampEnd = 0;
    event.status = STATUS_PENDING;
    strcpy(event.location, locations[rand() % ARRAY_SIZE(locations)]);

    return event;
}

