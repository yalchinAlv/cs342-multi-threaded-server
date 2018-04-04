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

int main(int argc, char **argv) {

	/***CHECK IF ENOUGH ARGUMENTS START***/
	if( argc != 4 ) {
      printf("Invalid arguments. Exiting...");
      exit(1);
  	}
  	/***CHECK IF ENOUGH ARGUMENTS END***/

  	/* assign arguments */
  	char* shm_name = argv[1];
  	char* keyword = argv[2];
  	char* sem_name = argv[3];

  	/* properties */
  	int fd;
  	struct stat sbuf; /* info about shm */
  	void *shm_start;
  	struct shared_data *sdp;
  	char qs_sem_mutex[150];
  	char rqq_sem_mutex[150];
  	char rqq_sem_full[150];
  	sem_t *qs_sem_mutex_t;
  	sem_t *rqq_sem_mutex_t;
  	sem_t *rqq_sem_full_t;

  	strcpy(qs_sem_mutex, argv[3]);
  	strcpy(rqq_sem_mutex, argv[3]);
  	strcpy(rqq_sem_full, argv[3]);

  	strcat(qs_sem_mutex, QS_SEM_MUTEX);
  	strcat(rqq_sem_mutex, RQQ_SEM_MUTEX);
  	strcat(rqq_sem_full, RQQ_SEM_FULL);

  	/* open the shared memory */
  	fd = shm_open(shm_name, O_RDWR, 0600);
  	if (fd < 0) {
  		perror ("can't open shm\n");
  		exit(1);
  	}
  	//printf("shm open success, fd = %d\n", fd);

  	fstat(fd, &sbuf);
  	//printf("shm_size=%d\n", (int) sbuf.st_size);

  	/* map the shared memory */
  	shm_start = mmap(NULL, sbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  	if( shm_start < 0) {
  		perror("can't map the shared memory\n");
  		exit(1);
  	}
  	//printf("mapped shm; start_address=%u\n", (unsigned int) shm_start);
  	close(fd);

  	/* create the shared memory segment's struct */
  	sdp = (struct shared_data *) shm_start;

	/* open the semaphores;  						  */
 	/* they should already be created and initialized */
 	qs_sem_mutex_t = sem_open(qs_sem_mutex, O_RDWR);
 	
 	if (qs_sem_mutex_t < 0) {
 		perror("can not open semaphore\n");
 		exit(1);
 	}
 	//printf("sem %s opened\n", qs_sem_mutex);


 	rqq_sem_mutex_t = sem_open(rqq_sem_mutex, O_RDWR);
 	
 	if (rqq_sem_mutex_t < 0) {
 		perror("can not open semaphore\n");
 		exit(1);
 	}
 	//printf("sem %s opened\n", rqq_sem_mutex);

 	rqq_sem_full_t = sem_open(rqq_sem_full, O_RDWR);
 	
 	if (rqq_sem_full_t < 0) {
 		perror("can not open semaphore\n");
 		exit(1);
 	}
 	//printf("sem %s opened\n", rqq_sem_full);


 	/* access the queue state array */
 	/* and try to find a free queue */
 	sem_wait(qs_sem_mutex_t);

 	int i;
 	int index = -1;
 	for (i = 0; i < NUM_OF_CLIENTS; i++) {
 		if (!sdp->queue_state[i]) {
 			sdp->queue_state[i] = 1;
 			index = i;
 			break;
 		}
 	}

 	sem_post(qs_sem_mutex_t);
 	
 	/* if couldn't find unused queue - exit */
 	if (index == -1) {
 		printf("too many clients started\n");
 		exit(1);
 		/* I MAY WANT TO CLOSE THE SEMAPHORES HERE BEFORE EXITING */
 	} else { /* found */

 		/* create semaphores for request queue*/
 		char rsq_sem_mutex[150];
 		char rsq_sem_full[150];
 		char rsq_sem_empty[150];

 		strcpy(rsq_sem_mutex, argv[3]);
 		strcpy(rsq_sem_full, argv[3]);
 		strcpy(rsq_sem_empty, argv[3]);

 		sprintf(rsq_sem_mutex, "%s%s%d", rsq_sem_mutex, RSQ_SEM_MUTEX,index);
 		sprintf(rsq_sem_full, "%s%s%d", rsq_sem_full, RSQ_SEM_FULL, index);
 		sprintf(rsq_sem_empty, "%s%s%d", rsq_sem_empty, RSQ_SEM_EMPTY, index);


 		
 		struct request r;
 		strcpy(r.keyword, keyword);
 		r.index = index;

		sem_t *rsq_sem_mutex_t = sem_open(rsq_sem_mutex, O_RDWR | O_CREAT, 0660, 1);
 		if (rsq_sem_mutex_t < 0) {
 			perror("can not create semaphore\n");
 			exit (1);
 		}
 		//printf("sem %s created\n", rsq_sem_mutex);

 		sem_t *rsq_sem_full_t = sem_open(rsq_sem_full, O_RDWR | O_CREAT, 0660, 0);
 		if (rsq_sem_full_t < 0) {
 			perror("can not create semaphore\n");
 			exit (1);
 		}
 		//printf("sem %s created\n", rsq_sem_full);

 		sem_t *rsq_sem_empty_t = sem_open(rsq_sem_empty, O_RDWR | O_CREAT, 0660, BUFSIZE);
 		if (rsq_sem_empty_t < 0) {
 			perror("can not create semaphore\n");
 			exit (1);
 		}
 		//printf("sem %s created\n", rsq_sem_empty);

 		/* make request */
 		sem_wait(rqq_sem_mutex_t);

 		sdp->request_queue.buf[sdp->request_queue.in] = r;
 		sdp->request_queue.in = (sdp->request_queue.in + 1) % NUM_OF_CLIENTS;

 		sem_post( rqq_sem_mutex_t);
 		sem_post(rqq_sem_full_t);


 		
 		/* get output */
 		int result = 0;
 		do {
 			sem_wait(rsq_sem_full_t);
 			sem_wait(rsq_sem_mutex_t);

 			result = sdp->result_queue[index].buf[sdp->result_queue[index].out];
 			
 			if (result != -1)
 				printf("%d\n", result);
 			
 			sdp->result_queue[index].out = (sdp->result_queue[index].out + 1) % BUFSIZE;

 			sem_post(rsq_sem_mutex_t);
 			sem_post(rsq_sem_empty_t);
 		} while (result != -1);

 		/* free the state queue */
		sem_wait(qs_sem_mutex_t);

		sdp->queue_state[i] = 0;

 		sem_post(qs_sem_mutex_t);

 		/* close and unlink semaphores */
 		sem_close(rsq_sem_mutex_t);
 		sem_close(rsq_sem_full_t);
 		sem_close(rsq_sem_empty_t);

 		sem_unlink(rsq_sem_mutex);
 		sem_unlink(rsq_sem_full);
 		sem_unlink(rsq_sem_empty);
 	}
}