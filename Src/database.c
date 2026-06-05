#include <stdio.h>
#include <sqlite3.h>
#include "database.h"

static sqlite3* g_database = NULL;

/**
 * @brief Removes all records from the events table.
 *
 * Clears previously stored event data while preserving
 * the database structure and table definitions. Used to
 * start a new simulation run with an empty database.
 *
 * @return 0 on success, -1 on failure.
 */
static int ClearEventsTable(void)
{
    char* errorMessage = NULL;

    if (sqlite3_exec(g_database,
                     "DELETE FROM events;",
                     NULL,
                     NULL,
                     &errorMessage) != SQLITE_OK)
    {
        printf("\nDatabase: clear table failed: %s\n", errorMessage);
        sqlite3_free(errorMessage);
        return -1;
    }
    printf("\nDatabase: events table cleared\n");

    return 0;
}

/**
 * @brief Creates the events table if it does not exist.
 *
 * Creates the database table used to store event
 * information, including event details, processing
 * status, timestamps, and assigned vehicles.
 *
 * @return 0 on success, -1 on failure.
 */
static int CreateEventsTable(void)
{
    const char* sql =
        "CREATE TABLE IF NOT EXISTS events ("
        "event_id INTEGER PRIMARY KEY,"
        "type INTEGER,"
        "priority INTEGER,"
        "location TEXT,"
        "description TEXT,"
        "timestamp_start INTEGER,"
        "timestamp_end INTEGER,"
        "status INTEGER,"
        "handled_by TEXT"
        ");";

    char* errorMessage = NULL;

    if (sqlite3_exec(g_database,
                     sql,
                     NULL,
                     NULL,
                     &errorMessage) != SQLITE_OK)
    {
        printf("\nDatabase: create table failed: %s\n",
               errorMessage);

        sqlite3_free(errorMessage);
        return -1;
    }

    return 0;
}

/**
 * @brief Initializes the database subsystem.
 *
 * Opens the SQLite database, creates the required
 * database tables, and prepares the database for
 * event storage during simulation execution.
 *
 * @return 0 on success, -1 on failure.
 */
int DatabaseInit(void)
{
    if (sqlite3_open("events.db", &g_database) != SQLITE_OK)
    {
        printf("\nDatabase: failed to open events.db: %s\n", sqlite3_errmsg(g_database));
        return -1;
    }

    if (CreateEventsTable() != 0)
    {
        sqlite3_close(g_database);
        g_database = NULL;
        return -1;
    }

    if (ClearEventsTable() != 0)
    {
        sqlite3_close(g_database);
        g_database = NULL;
        return -1;
    }

    printf("\nDatabase: events.db opened\n");
    return 0;
}

/**
 * @brief Closes the database connection.
 *
 * Releases database resources and closes the active
 * SQLite connection before application termination.
 */
void DatabaseClose(void)
{
    if (g_database != NULL)
    {
        sqlite3_close(g_database);
        g_database = NULL;
        printf("\nDatabase: closed\n");
    }
}

/**
 * @brief Stores a newly generated event in the database.
 *
 * Inserts event information into the events table,
 * including event details, priority, location,
 * description, and initial processing status.
 *
 * @param event Pointer to the event structure.
 *
 * @return 0 on success, -1 on failure.
 */
int DatabaseInsertEvent(const Event_t* event)
{
    const char* sql =
        "INSERT INTO events "
        "(event_id, type, priority, location, description, "
        "timestamp_start, timestamp_end, status, handled_by) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(g_database, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        printf("Database: prepare insert failed: %s\n",
               sqlite3_errmsg(g_database));
        return -1;
    }

    sqlite3_bind_int(stmt, 1, event->eventId);
    sqlite3_bind_int(stmt, 2, event->type);
    sqlite3_bind_int(stmt, 3, event->priority);
    sqlite3_bind_text(stmt, 4, event->location, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, event->description, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 6, event->timestampStart);
    sqlite3_bind_int64(stmt, 7, event->timestampEnd);
    sqlite3_bind_int(stmt, 8, event->status);
    sqlite3_bind_text(stmt, 9, "", -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        printf("Database: insert event %u failed: %s\n",
               event->eventId,
               sqlite3_errmsg(g_database));

        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);

    return 0;
}

/**
 * @brief Updates an event record with completion information.
 *
 * Updates the corresponding event entry in the database
 * with the final processing status, completion timestamp,
 * and the identifier of the vehicle that handled the event.
 *
 * @param completion Pointer to the completion message.
 *
 * @return 0 on success, -1 on failure.
 */
int DatabaseUpdateCompletion(const CompletionMessage_t* completion)
{
    const char* sql =
        "UPDATE events "
        "SET timestamp_end = ?, status = ?, handled_by = ? "
        "WHERE event_id = ?;";

    sqlite3_stmt* stmt = NULL;

    if (sqlite3_prepare_v2(g_database, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        printf("Database: prepare update failed: %s\n",
               sqlite3_errmsg(g_database));
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, completion->timestampEnd);
    sqlite3_bind_int(stmt, 2, completion->status);
    sqlite3_bind_text(stmt, 3, completion->handledBy, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, completion->eventId);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        printf("Database: update event %u failed: %s\n",
               completion->eventId,
               sqlite3_errmsg(g_database));

        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);

    return 0;
}

