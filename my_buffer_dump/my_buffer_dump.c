#ifdef FRONTEND
#undef FRONTEND
#endif

/*
 * pgxs (19devel) may pull in internal headers and define FRONTEND.
 * Extensions MUST undefine it to enable PG_MODULE_MAGIC.
 */
#include "postgres.h"
#include "fmgr.h"

PG_MODULE_MAGIC;

#include "storage/buf_internals.h"
#include "storage/bufmgr.h"
#include "storage/lwlock.h"
#include "storage/proc.h"
#include "postmaster/bgworker.h"
#include "miscadmin.h"
#include "utils/guc.h"
#include "utils/timestamp.h"

#include <unistd.h>

static int buffer_dump_interval = 5; /* GUC parameter N seconds */

static void dump_buffer_states_to_file(void);

/* Registering bg worker */
void _PG_init(void);

void buffer_dump_worker_main(Datum main_arg);

void
_PG_init(void)
{
	BackgroundWorker worker;

	DefineCustomIntVariable(
		"buffer_dump.interval",
		"Interval between buffer cache dumps (seconds)",
		NULL,
		&buffer_dump_interval,
		5,	/* default */
		1,	/* min */
		3600, /* max */
		PGC_POSTMASTER,
		0,
		NULL, NULL, NULL
	);

	MemSet(&worker, 0, sizeof(worker));
	strlcpy(worker.bgw_name,
        "buffer cache state dumper",
        BGW_MAXLEN);
	worker.bgw_flags =
		BGWORKER_SHMEM_ACCESS |
		BGWORKER_BACKEND_DATABASE_CONNECTION;
	worker.bgw_start_time = BgWorkerStart_ConsistentState;
	worker.bgw_restart_time = 1;
	strlcpy(worker.bgw_library_name, "my_buffer_dump", BGW_MAXLEN);
	strlcpy(worker.bgw_function_name, "buffer_dump_worker_main", BGW_MAXLEN);

	RegisterBackgroundWorker(&worker);
}

/* Main worker func */
void
buffer_dump_worker_main(Datum main_arg)
{
	(void) main_arg;
	BackgroundWorkerUnblockSignals();
	BackgroundWorkerInitializeConnection(NULL, NULL, 0);

	while (true)
	{
		dump_buffer_states_to_file();
		pg_usleep(buffer_dump_interval * 1000000L);
	}
}

/* Dump function */
static void
dump_buffer_states_to_file(void)
{
	/* Adding timestamp */
	char tsbuf[64];

	TimestampTz now = GetCurrentTimestamp();
	const char *ts = timestamptz_to_str(now);

	FILE *fp = fopen("/tmp/buffer_states.log", "a");
	if (!fp)
    	return;

	fprintf(fp, "=== Buffer dump at %s ===\n", ts);
	/* Writing all buffers states */
	 for (int i = 0; i < NBuffers; i++)
	{
		BufferDesc *buf = GetBufferDescriptor(i);
		uint32 state = pg_atomic_read_u32(&buf->state);

		int refcount = BUF_STATE_GET_REFCOUNT(state);
		int usage = BUF_STATE_GET_USAGECOUNT(state);
		bool dirty = (state & BM_DIRTY) != 0;
		bool valid = (state & BM_VALID) != 0;

		fprintf(fp,
			"buf=%d valid=%d dirty=%d pin=%d usage=%d\n",
			i, valid, dirty, refcount, usage);
	}

	fclose(fp);
}

