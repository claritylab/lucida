#include <stdio.h>
#include <sbthread.h>
#include <err.h>

int
worker_main(sbthread_t *th)
{
	sbmsgq_t *msgq;
	void *data;
	size_t len;

	msgq = sbthread_msgq(th);
	while ((data = sbmsgq_wait(msgq, &len, -1, -1)) != NULL) {
		int msg = *(int *)data;
		E_INFO("Got message: %d\n", msg);
		if (msg == 32)
			break;
	}

	return 0;
}

int
main(int argc, char *argv[])
{
	sbthread_t *worker;
	int i;
	
	worker = sbthread_start(NULL, worker_main, NULL);
	for (i = 0; i <= 32; ++i) {
		int ii[128];
		E_INFO("Sending message: %d\n", i);
		ii[0] = i;
		if (sbthread_send(worker, sizeof(ii), &ii) < 0) {
			E_ERROR("sbthread_send failed\n");
			return 1;
		}
	}
	sbthread_free(worker);

	return 0;
}
