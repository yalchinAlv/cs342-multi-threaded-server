#ifndef COMMON_H
#define COMMON_H

#define NUM_OF_CLIENTS 10
#define BUFSIZE 100
#define KEYWORD_SIZE 128

struct result_queue_struct {
	int buf[BUFSIZE];
	int in;
	int out;
	int count;
};

struct request_queue_struct {
	char buf[NUM_OF_CLIENTS][KEYWORD_SIZE];
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