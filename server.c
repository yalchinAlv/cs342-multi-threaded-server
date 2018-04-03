#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include "common.h"

void init_shared_data(struct shared_data* sh_data) {

	for (int i = 0; i < NUM_OF_CLIENTS; i++) {
		sh_data->queue_state[i] = 0;
		sh_data->result_queue[i].in = 0;
		sh_data->result_queue[i].out = 0;
		sh_data->result_queue[i].count = 0;
	}

	sh_data->request_queue.in = 0;
	sh_data->request_queue.out = 0;
	sh_data->request_queue.count = 0;
}

struct thread_arg {
	char keyword[KEYWORD_SIZE];
	int index;
	char input_file_name[128];
	char shm_name[128];
};

static void *handle_request(void *arg) {

}

int main(int argc, char **argv) {

	int fd;
	struct stat sbuf;
	void *shared_mem;
	struct shared_data *sh_data;
	char input_file_name[128];

	char shm_name[128];
	char qs_sem_mutex[150];
	char rqq_sem_mutex[150];
	char rqq_sem_full[150];

	sem_t *qs_sem_mutex_t;
	sem_t *rqq_sem_mutex_t;
	sem_t *rqq_sem_full_t;

	if (argc != 4) {
		printf("usage: server <shm_name> <input_file_name> <sem_name>");
		exit(1);
	}

	strcpy(shm_name, argv[1]);
	strcpy(input_file_name, argv[2]);

	// construct names for semaphores
	strcpy(qs_sem_mutex, argv[3]);
	strcpy(rqq_sem_mutex, argv[3]);
	strcpy(rqq_sem_full, argv[3]);

	strcat(qs_sem_mutex, QS_SEM_MUTEX);
	strcat(rqq_sem_mutex, RQQ_SEM_MUTEX);
	strcat(rqq_sem_full, RQQ_SEM_FULL);

	// clean up shared memory
	shm_unlink(shm_name);

	// create shared memory
	fd = shm_open(shm_name, O_RDWR | O_CREAT, 0660);
	if (fd < 0) {
		perror("can not create shared memory\n");
		exit (1);
	}

	// set size of the shared memory
	ftruncate(fd, sizeof(struct shared_data));
	fstat(fd, &sbuf);
	
	// map the shared memory
	shared_mem = mmap(NULL, sbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (shared_mem < 0) {
		perror("can not map shm\n");
		exit (1);
	}
	close(fd);

	sh_data = (struct shared_data *) shared_mem;

	// initialize the shared data
	init_shared_data(sh_data);

	// clean up semaphores with same names
	sem_unlink(qs_sem_mutex);
	sem_unlink(rqq_sem_mutex);
	sem_unlink(rqq_sem_full);

	// create and initialize semaphores
	qs_sem_mutex_t = sem_open(qs_sem_mutex, O_RDWR | O_CREAT, 0660, 1);
	if (qs_sem_mutex_t < 0) {
		perror("can not create semaphore\n");
		exit (1);
	}

	rqq_sem_mutex_t = sem_open(rqq_sem_mutex, O_RDWR | O_CREAT, 0660, 1);
	if (rqq_sem_mutex_t < 0) {
		perror("can not create semaphore\n");
		exit (1);
	}

	rqq_sem_full_t = sem_open(rqq_sem_full, O_RDWR | O_CREAT, 0660, 0);
	if (rqq_sem_full_t < 0) {
		perror("can not create semaphore\n");
		exit (1);
	}

	// handle clients
	for (int i = 0; i < 5; i++) {
		
		printf("-1\n");
		sem_wait(rqq_sem_full_t);
		printf("0\n");
		sem_wait(rqq_sem_mutex_t);
		
		printf("1\n");
		// get request
		struct request r = sh_data->request_queue.buf[sh_data->request_queue.out];
		printf("2\n");
		sh_data->request_queue.out = (sh_data->request_queue.out + 1) % NUM_OF_CLIENTS;
		printf("3\n");
		sem_post(rqq_sem_mutex_t);

		printf("4\n");

		// for testing purposes only
		char rsq_sem_mutex[150];
 		char rsq_sem_full[150];
 		char rsq_sem_empty[150];

 		strcpy(rsq_sem_mutex, argv[3]);
 		strcpy(rsq_sem_full, argv[3]);
 		strcpy(rsq_sem_empty, argv[3]);

 		sprintf(rsq_sem_mutex, "%s%s%d", rsq_sem_mutex, RSQ_SEM_MUTEX, r.index);
 		sprintf(rsq_sem_full, "%s%s%d", rsq_sem_full, RSQ_SEM_FULL r.index);
 		sprintf(rsq_sem_empty, "%s%s%d", rsq_sem_empty, RSQ_SEM_EMPTY r.index);

		sem_t *rsq_sem_mutex_t = sem_open(rsq_sem_mutex, O_RDWR);
 		if (rsq_sem_mutex_t < 0) {
 			perror("can not create semaphore\n");
 			exit (1);
 		}

 		sem_t *rsq_sem_full_t = sem_open(rsq_sem_full, O_RDWR);
 		if (rsq_sem_full_t < 0) {
 			perror("can not create semaphore\n");
 			exit (1);
 		}

 		sem_t *rsq_sem_empty_t = sem_open(rsq_sem_empty, O_RDWR);
 		if (rsq_sem_empty_t < 0) {
 			perror("can not create semaphore\n");
 			exit (1);
 		}

		for (int k = 0; k < 3; k++) {
			sem_wait(rsq_sem_empty_t);
			sem_wait(rsq_sem_mutex_t);

			sh_data->result_queue[r.index].buf[sh_data->result_queue[r.index].in] = k;
			sh_data->result_queue[r.index].in = (sh_data->result_queue[r.index].in + 1) % BUFSIZE;

			sem_post(rsq_sem_mutex_t);
 			sem_post(rsq_sem_full_t);
		}
		sem_wait(rsq_sem_empty_t);
		sem_wait(rsq_sem_mutex_t);

		sh_data->result_queue[r.index].buf[sh_data->result_queue[r.index].in] = -1;
		sh_data->result_queue[r.index].in = (sh_data->result_queue[r.index].in + 1) % BUFSIZE;

		sem_post(rsq_sem_mutex_t);
		sem_post(rsq_sem_full_t);

	}
}