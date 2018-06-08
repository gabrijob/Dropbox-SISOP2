#ifndef SYNC_THREADS_HEADER
#define SYNC_THREADS_HEADER

#include "dropboxUtil.h"
#include "dropboxClient.h"

void *answer_pending(void* user);
void *watcher(void*user);

#endif