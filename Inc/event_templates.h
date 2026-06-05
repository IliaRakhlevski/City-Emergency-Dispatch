#ifndef EVENT_GEN_TEMPLATES_H
#define EVENT_GEN_TEMPLATES_H

#include "event_generator.h"

/**
 * @brief Predefined event templates used by the event generator.
 *
 * Each department provides a set of event descriptions
 * and priority levels used to generate random simulation events.
 */

static const EventTemplate_t ambulanceEvents[] =
{
    { "Severe traffic accident",     PRIORITY_HIGH   },
    { "Heart attack patient",        PRIORITY_HIGH   },
    { "Minor accident",              PRIORITY_MEDIUM },
    { "Minor injury",                PRIORITY_LOW    },
    { "Emergency in public building",PRIORITY_HIGH   }
};

static const EventTemplate_t policeEvents[] =
{
    { "Illegal gathering",           PRIORITY_MEDIUM },
    { "Home burglary",               PRIORITY_HIGH   },
    { "Traffic violation",           PRIORITY_LOW    },
    { "Violence incident",           PRIORITY_HIGH   },
    { "City emergency assistance",   PRIORITY_MEDIUM }
};

static const EventTemplate_t fireEvents[] =
{
    { "Restaurant fire",             PRIORITY_HIGH   },
    { "Residential house fire",      PRIORITY_HIGH   },
    { "Vehicle fire",                PRIORITY_MEDIUM },
    { "Suspicious smoke",            PRIORITY_MEDIUM },
    { "Open field fire",             PRIORITY_LOW    }
};

static const EventTemplate_t maintenanceEvents[] =
{
    { "Sidewalk repair",                     PRIORITY_MEDIUM },
    { "Water pipe leak",                     PRIORITY_HIGH   },
    { "Routine public building maintenance", PRIORITY_LOW    },
    { "Dangerous sewer openings",            PRIORITY_MEDIUM },
    { "Roof leak issue",                     PRIORITY_HIGH   }
};

static const EventTemplate_t wasteEvents[] =
{
    { "Full neighborhood bins",                  PRIORITY_MEDIUM },
    { "Hazardous waste collection",              PRIORITY_HIGH   },
    { "Regular bin collection",                  PRIORITY_LOW    },
    { "Large public waste removal",              PRIORITY_HIGH   },
    { "Uncollected paper bins from commerce",    PRIORITY_MEDIUM }
};

static const EventTemplate_t electricEvents[] =
{
    { "Streetlight failure",             PRIORITY_LOW    },
    { "Neighborhood power outage",       PRIORITY_HIGH   },
    { "Power off in public building",    PRIORITY_MEDIUM },
    { "Overload in power network",       PRIORITY_HIGH   },
    { "Traffic light signaling failure", PRIORITY_MEDIUM }
};

static const char* locations[] =
{
    "Herzliya",
    "Tel Aviv",
    "Raanana",
    "Kfar Saba",
    "Netanya",
    "Petah Tikva",
    "Ramat Gan",
    "Holon"
};


#endif