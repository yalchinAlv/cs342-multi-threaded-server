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
#include <pthread.h>
#include <signal.h>
#include "common.h"

void init_shared_data(struct shared_data* sh_data) {

	for (int i = 0; i < NUM_OF_CLIENTS; i++) {
		sh_data->queue_state[i] = 0;
		sh_data->result_queue[i].in = 0;
		sh_data->result_queue[i].out = 0;
		sh_data->result_queue[i].count = 0;
		for (int k = 0; k < BUFSIZE; k++) {
			sh_data->result_queue[i].buf[k] = 0;
		}
	}

	sh_data->request_queue.in = 0;
	sh_data->request_queue.out = 0;
	sh_data->request_queue.count = 0;
}

void print_arr(int arr[], int index) {
	printf("%d: ", index);
	for (int i = 0; i < BUFSIZE; i++) {
		printf("%d, ", arr[i]);
	}
	printf("\n");
}

struct thread_arg {
	char keyword[KEYWORD_SIZE];
	int index;
	char input_file_name[128];
	//char shm_name[128];
	struct shared_data *sh_data;
	char sem_name[128];
};

void *handle_request(void *a) {
	// line number to be sent to client
	int line_number = 1;

	// assign the argument
	struct thread_arg *arg = (struct thread_arg*) a;

	// open semaphores
	// for testing purposes only
	char rsq_sem_mutex[150];
	char rsq_sem_full[150];
	char rsq_sem_empty[150];

	strcpy(rsq_sem_mutex, arg->sem_name);
	strcpy(rsq_sem_full, arg->sem_name);
	strcpy(rsq_sem_empty, arg->sem_name);

	sprintf(rsq_sem_mutex, "%s%s%d", rsq_sem_mutex, RSQ_SEM_MUTEX, arg->index);
	sprintf(rsq_sem_full, "%s%s%d", rsq_sem_full, RSQ_SEM_FULL, arg->index);
	sprintf(rsq_sem_empty, "%s%s%d", rsq_sem_empty, RSQ_SEM_EMPTY, arg->index);


	sem_t *rsq_sem_mutex_t = sem_open(rsq_sem_mutex, O_RDWR);
	if (rsq_sem_mutex_t == SEM_FAILED) {
		perror("can not create semaphore\n");
		exit (1);
	}
	// printf("sem %s created\n", rsq_sem_mutex);

	sem_t *rsq_sem_full_t = sem_open(rsq_sem_full, O_RDWR);
	if (rsq_sem_full_t == SEM_FAILED) {
		perror("can not create semaphore\n");
		exit (1);
	}
	//printf("sem %s created\n", rsq_sem_full);


	sem_t *rsq_sem_empty_t = sem_open(rsq_sem_empty, O_RDWR);
	if (rsq_sem_empty_t == SEM_FAILED) {
		perror("can not create semaphore\n");
		exit (1);
	}
	//printf("sem %s created\n", rsq_sem_empty);

	// open the file
	FILE *fp;

    char line[LINE_SIZE];

    fp = fopen(arg->input_file_name,"r");
    if(!fp)
	{
        perror("could not find the file");
        exit(0);
	}

	// read the lines untill the end of the file
	while ( fgets ( line, LINE_SIZE, fp ) != NULL ) /* read a line */
    {	
    	// if the keyword is found in this line
    	// send the keyword to result queue
    	if(strstr(line, arg->keyword)) {

			// if (arg->index == 0) {
			// 	printf("in thread %d waiting for sem empty that\n", arg->index);
			// }
		    sem_wait(rsq_sem_empty_t);

		 //    if (arg->index == 0) {
			// 	printf("in thread %d waiting for mutex that\n", arg->index);
			// }
			sem_wait(rsq_sem_mutex_t);

			// if (arg->index == 0) {
			// 	printf("in thread %d done waiting for mutex that\n", arg->index);
			// }
			// printf("sending to %d\n", arg->index);
			arg->sh_data->result_queue[arg->index].buf[arg->sh_data->result_queue[arg->index].in] = line_number;
			arg->sh_data->result_queue[arg->index].in = (arg->sh_data->result_queue[arg->index].in + 1) % BUFSIZE;
			// print_arr(arg->sh_data->result_queue[arg->index].buf, arg->index);

			// if (arg->index == 0) {
			// 	printf("in thread %d posting mutex\n", arg->index);
			// }
			sem_post(rsq_sem_mutex_t);

			// if (arg->index == 0) {
			// 	printf("in thread %d posted mutex\n", arg->index);
			// }
	 		sem_post(rsq_sem_full_t);

			// if (arg->index == 0) {
			// 	printf("in thread %d posted full\n", arg->index);
			// }
    	}

    	line_number++;
    }

 //    if (arg->index == 0) {
	// 	printf("in thread %d waiting for sem empty that is -1\n", arg->index);
	// }
    sem_wait(rsq_sem_empty_t);

 //    if (arg->index == 0) {
	// 	printf("in thread %d waiting for mutex that is -1\n", arg->index);
	// }
	sem_wait(rsq_sem_mutex_t);

	// if (arg->index == 0) {
	// 	printf("in thread %d done waiting for mutex that is -1\n", arg->index);
	// }
	arg->sh_data->result_queue[arg->index].buf[arg->sh_data->result_queue[arg->index].in] = -1;
	arg->sh_data->result_queue[arg->index].in = (arg->sh_data->result_queue[arg->index].in + 1) % BUFSIZE;

	sem_post(rsq_sem_mutex_t);
	sem_post(rsq_sem_full_t);

    // close the file
    // printf("closing file for thread %d\n", arg->index);
    fclose ( fp );
    free(a);
}

char shm_name[128];
char qs_sem_mutex[150];
char rqq_sem_mutex[150];
char rqq_sem_full[150];


void handler(int dummy) {
	// unlink and close semaphores
	sem_unlink(qs_sem_mutex);
	sem_unlink(rqq_sem_mutex);
	sem_unlink(rqq_sem_full);
	
	// clean up shared memory
	shm_unlink(shm_name);

	exit(0);
}

int main(int argc, char **argv) {

	// signal handler for Ctrl+C
	signal(SIGINT, handler);
	
	// printf("%d\n", SEM_FAILED);
	int fd;
	struct stat sbuf;
	void *shared_mem;
	struct shared_data *sh_data;
	char input_file_name[128];

	// char shm_name[128];
	// char qs_sem_mutex[150];
	// char rqq_sem_mutex[150];
	// char rqq_sem_full[150];

	sem_t *qs_sem_mutex_t;
	sem_t *rqq_sem_mutex_t;
	sem_t *rqq_sem_full_t;

	// pthread_t threads[NUM_OF_CLIENTS];

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
	if (qs_sem_mutex_t == SEM_FAILED) {
		perror("can not create semaphore\n");
		exit (1);
	}

	rqq_sem_mutex_t = sem_open(rqq_sem_mutex, O_RDWR | O_CREAT, 0660, 1);
	if (rqq_sem_mutex_t == SEM_FAILED) {
		perror("can not create semaphore\n");
		exit (1);
	}

	rqq_sem_full_t = sem_open(rqq_sem_full, O_RDWR | O_CREAT, 0660, 0);
	if (rqq_sem_full_t == SEM_FAILED) {
		perror("can not create semaphore\n");
		exit (1);
	}

	// handle clients
	while(1) {
		
		sem_wait(rqq_sem_full_t);
		sem_wait(rqq_sem_mutex_t);
		
		// get request
		struct request r = sh_data->request_queue.buf[sh_data->request_queue.out];
		sh_data->request_queue.out = (sh_data->request_queue.out + 1) % NUM_OF_CLIENTS;
		
		sem_post(rqq_sem_mutex_t);

		/* create an argument for the thread */
		struct thread_arg *arg = malloc(sizeof(struct thread_arg));

		strcpy(arg->keyword, r.keyword);
		strcpy(arg->input_file_name, input_file_name);
		strcpy(arg->sem_name, argv[3]);
		arg->index = r.index;
		arg->sh_data = sh_data;

		pthread_t pid;

		/* create a thread which handles the request */
		if(pthread_create(&pid, NULL, handle_request, (void *)arg)) {
			fprintf(stderr, "Error creating thread\n");
			exit(1);
		}
	}
}
