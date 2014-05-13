#include <stdio.h>
#include <sbthread.h>
#include <err.h>

int
worker_main(sbthread_t *th)
{
	sbevent_t *cond;

	cond = sbthread_arg(th);

	/* Get the first signal. */
	sbevent_wait(cond, -1, -1);
	E_INFO("Got signal\n");

	/* Now wait a while and exit. */
	sbevent_wait(cond, 1, 500*1000*1000);
	return 0;
}

int
main(int argc, char *argv[])
{
	sbthread_t *worker;
	sbevent_t *cond;

	cond = sbevent_init();
	worker = sbthread_start(NULL, worker_main, cond);

	E_INFO("Signalling condition\n");
	sbevent_signal(cond);

	E_INFO("Waiting (about 1.5 sec) for thread termination\n");
	sbthread_free(worker);
	sbevent_free(cond);
	return 0;
}
