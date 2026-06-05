#ifndef DATABASE_H
#define DATABASE_H

#include  "event.h"
#include "message.h"

int DatabaseInit(void);
void DatabaseClose(void);
int DatabaseInsertEvent(const Event_t* event);
int DatabaseUpdateCompletion(const CompletionMessage_t* completion);

#endif