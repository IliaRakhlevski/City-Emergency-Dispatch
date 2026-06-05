#ifndef VEHICLE_TEMPLATES_H
#define VEHICLE_TEMPLATES_H

#include "vehicle.h"

/**
 * @brief Predefined vehicle pools for all departments.
 *
 * Defines the vehicles available in the simulation,
 * including their identifiers, assigned departments,
 * initial status, and display names.
 */

static Vehicle_t ambulanceVehicles[] =
{
    { 1, EVENT_AMBULANCE, VEHICLE_AVAILABLE, "Ambulance_1" },
    { 2, EVENT_AMBULANCE, VEHICLE_AVAILABLE, "Ambulance_2" }
};

static Vehicle_t policeVehicles[] =
{
    { 3, EVENT_POLICE, VEHICLE_AVAILABLE, "Police_1" },
    { 4, EVENT_POLICE, VEHICLE_AVAILABLE, "Police_2" }
};

static Vehicle_t fireVehicles[] =
{
    { 5, EVENT_FIRE, VEHICLE_AVAILABLE, "Fire_1" },
    { 6, EVENT_FIRE, VEHICLE_AVAILABLE, "Fire_2" }
};

static Vehicle_t maintenanceVehicles[] =
{
    { 7, EVENT_MAINTENANCE, VEHICLE_AVAILABLE, "Maintenance_1" },
    { 8, EVENT_MAINTENANCE, VEHICLE_AVAILABLE, "Maintenance_2" }
};

static Vehicle_t wasteVehicles[] =
{
    { 9,  EVENT_WASTE, VEHICLE_AVAILABLE, "Waste_1" },
    { 10, EVENT_WASTE, VEHICLE_AVAILABLE, "Waste_2" }
};

static Vehicle_t electricVehicles[] =
{
    { 11, EVENT_ELECTRIC, VEHICLE_AVAILABLE, "Electric_1" },
    { 12, EVENT_ELECTRIC, VEHICLE_AVAILABLE, "Electric_2" }
};

#define AMBULANCE_VEHICLE_COUNT   (sizeof(ambulanceVehicles)   / sizeof(ambulanceVehicles[0]))
#define POLICE_VEHICLE_COUNT      (sizeof(policeVehicles)      / sizeof(policeVehicles[0]))
#define FIRE_VEHICLE_COUNT        (sizeof(fireVehicles)        / sizeof(fireVehicles[0]))
#define MAINTENANCE_VEHICLE_COUNT (sizeof(maintenanceVehicles) / sizeof(maintenanceVehicles[0]))
#define WASTE_VEHICLE_COUNT       (sizeof(wasteVehicles)       / sizeof(wasteVehicles[0]))
#define ELECTRIC_VEHICLE_COUNT    (sizeof(electricVehicles)    / sizeof(electricVehicles[0]))


#endif