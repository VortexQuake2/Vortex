#define DEFER_MAX_CALLS 32

typedef void (*deferred_function_t) (void* args);

typedef struct
{
	deferred_function_t function;
	void* data;
} defer_t;

typedef struct
{
	defer_t deferrals[DEFER_MAX_CALLS];
	int deferral_count;
} deferrals_t;

// should only be called in main thread
void defer_global_init();
void defer_global_close();

void defer_init(deferrals_t* defers);

// if not in a multithreaded context, this does the right thing and calls the function immediately.
qboolean defer_add(deferrals_t* defers, defer_t func);

// should only be called in main thread
void defer_run(deferrals_t* defers);
