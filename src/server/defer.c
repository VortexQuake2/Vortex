#include "../q_shared.h"
#include "defer.h"

#ifndef GDS_NOMULTITHREADING
#include <pthread.h>

static qboolean defer_system_initialized = false;
static pthread_mutex_t defer_mutex;

#endif


void defer_global_init()
{
#ifndef GDS_NOMULTITHREADING
	if (defer_system_initialized)
		return;

	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);

	pthread_mutexattr_setkind_np(&attr, PTHREAD_MUTEX_RECURSIVE_NP);

	pthread_mutex_init(&defer_mutex, &attr);

	pthread_mutexattr_destroy(&attr);

	defer_system_initialized = true;
#endif
}

void defer_global_close()
{
#ifndef GDS_NOMULTITHREADING
	pthread_mutex_destroy(&defer_mutex);
#endif	
}

void defer_init(deferrals_t* defers)
{
	memset(defers, 0, sizeof (deferrals_t));
}

qboolean defer_add(deferrals_t* defers, defer_t func)
{
#ifndef GDS_NOMULTITHREADING
	pthread_mutex_lock(&defer_mutex);
	if (defers->deferral_count + 1 < DEFER_MAX_CALLS)
	{
		defers->deferrals[defers->deferral_count++] = func;
		pthread_mutex_unlock(&defer_mutex);
		return true;
	}

	pthread_mutex_unlock(&defer_mutex);
#else // just run it right away...
	func.function(func.data);
#endif

	return false;
}

void defer_run(deferrals_t* defers)
{
#ifndef GDS_NOMULTITHREADING
	pthread_mutex_lock(&defer_mutex);
	for (int i = 0; i < defers->deferral_count; i++)
	{
		defers->deferrals[i].function(defers->deferrals[i].data);
	}

	defers->deferral_count = 0;
	pthread_mutex_unlock(&defer_mutex);
#endif
}
