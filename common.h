#ifndef COMMON_H
#define COMMON_H

#define NUM_OF_CLIENTS 10
#define BUFSIZE 100
#define KEYWORD_SIZE 128

// semaphore sufixes for queue state
#define QS_SEM_MUTEX "qs_sem_mutex"

// semaphore sufixes for request queue
#define RQQ_SEM_MUTEX "rqq_sem_mutex"
#define RQQ_SEM_FULL "rqq_sem_full"

// semaphore sufixes for result queue
#define RSQ_SEM_MUTEX "rsq_sem_mutex_"
#define RSQ_SEM_FULL "rsq_sem_full_"
#define RSQ_SEM_EMPTY "rsq_sem_empty_"

struct result_queue_struct {
	int buf[BUFSIZE];
	int in;
	int out;
	int count;
};

struct request_queue_struct {
	char buf[NUM_OF_CLIENTS][KEYWORD_SIZE + 1];
	int in;
	int out;
	int count;
};

struct shared_data {
	int queue_state[NUM_OF_CLIENTS];
	struct result_queue_struct result_queue[NUM_OF_CLIENTS];
	struct request_queue_struct request_queue;
};

#endif