#include "../q_shared.h"
#include "defer.h"

#include <pthread.h>

static qboolean defer_system_initialized = false;
static pthread_mutex_t defer_mutex;


void defer_global_init()
{
	if (defer_system_initialized)
		return;

	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);

	pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_RECURSIVE);

	pthread_mutex_init(&defer_mutex, &attr);

	pthread_mutexattr_destroy(&attr);

	defer_system_initialized = true;
}

void defer_global_close()
{
	pthread_mutex_destroy(&defer_mutex);
}

void defer_init(deferrals_t* defers)
{
	memset(defers, 0, sizeof (deferrals_t));
}

qboolean defer_add(deferrals_t* defers, defer_t func)
{
	pthread_mutex_lock(&defer_mutex);
	if (defers->deferral_count + 1 < DEFER_MAX_CALLS)
	{
		defers->deferrals[defers->deferral_count++] = func;
		pthread_mutex_unlock(&defer_mutex);
		return true;
	}

	pthread_mutex_unlock(&defer_mutex);
	return false;
}

void defer_run(deferrals_t* defers)
{
	pthread_mutex_lock(&defer_mutex);
	for (int i = 0; i < defers->deferral_count; i++)
	{
		defers->deferrals[i].function(defers->deferrals[i].data);
		vrx_free(defers->deferrals[i].data);
	}

	defers->deferral_count = 0;
	pthread_mutex_unlock(&defer_mutex);
}
