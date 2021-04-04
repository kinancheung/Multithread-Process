/*
    The Hybrid Merge Sort to use for Operating Systems Assignment 1 2021
    written by Robert Sheehan

    Modified by: Kinan Cheung
    UPI: kche356

    By submitting a program you are claiming that you and only you have made
    adjustments and additions to this code.
 */

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h>
#include <sys/resource.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/times.h>
#include <math.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SIZE    4
#define MAX     1000
#define SPLIT   16
#define CHUNKSIZE 16384

struct block {
    int size;
    int *data;
};

void print_data(struct block *block) {
    for (int i = 0; i < block->size; ++i)
        printf("%d ", block->data[i]);
    printf("\n");
}

/* The insertion sort for smaller halves. */
void insertion_sort(struct block *block) {
    for (int i = 1; i < block->size; ++i) {
        for (int j = i; j > 0; --j) {
            if (block->data[j-1] > block->data[j]) {
                int temp;
                temp = block->data[j-1];
                block->data[j-1] = block->data[j];
                block->data[j] = temp;
            }
        }
    }
}

/* Combine the two halves back together. */
void merge(struct block *left, struct block *right) {
    int *combined = calloc(left->size + right->size, sizeof(int));
    if (combined == NULL) {
        perror("Allocating space for merge.\n");
        exit(EXIT_FAILURE);
    }
        int dest = 0, l = 0, r = 0;
        while (l < left->size && r < right->size) {
                if (left->data[l] < right->data[r])
                        combined[dest++] = left->data[l++];
                else
                        combined[dest++] = right->data[r++];
        }
        while (l < left->size)
                combined[dest++] = left->data[l++];
        while (r < right->size)
                combined[dest++] = right->data[r++];
    memmove(left->data, combined, (left->size + right->size) * sizeof(int));
    free(combined);
}

/* Merge sort the data. */
void *merge_sort(void *input){
  struct block *block = (struct block *)input;
  if (block->size > SPLIT) {
      struct block left_block;
      struct block right_block;
      left_block.size = block->size / 2;
      left_block.data = block->data;
      right_block.size = block->size - left_block.size; // left_block.size + (block->size % 2);
      right_block.data = block->data + left_block.size;

      merge_sort(&left_block);
      merge_sort(&right_block);
      merge(&left_block, &right_block);
  } else {
    insertion_sort(block);
  }
}



/* Check to see if the data is sorted. */
bool is_sorted(struct block *block) {
    bool sorted = true;
    for (int i = 0; i < block->size - 1; i++) {
        if (block->data[i] > block->data[i + 1])
            sorted = false;
    }
    return sorted;
}

/* Fill the array with random data. */
void produce_random_data(struct block *block) {
    srand(1); // the same random data seed every time
    for (int i = 0; i < block->size; i++) {
        block->data[i] = rand() % MAX;
    }
}

int main(int argc, char *argv[]) {
        long size;

        if (argc < 2) {
                size = SIZE;
        } else {
                size = atol(argv[1]);
        }
    struct block block;
    block.size = (int)pow(2, size);
    block.data = (int *)calloc(block.size, sizeof(int));
    if (block.data == NULL) {
        perror("Unable to allocate space for data.\n");
        exit(EXIT_FAILURE);
    }

    produce_random_data(&block);

    struct timeval start_wall_time, finish_wall_time, wall_time;
    struct tms start_times, finish_times;
    gettimeofday(&start_wall_time, NULL);
    times(&start_times);

    struct block left_block;
    struct block right_block;
    left_block.size = block.size / 2;
    left_block.data = block.data;
    right_block.size = block.size - left_block.size; // left_block.size + (block->size % 2);
    right_block.data = block.data + left_block.size;

    int pipe_fds[2];
    int *read_fds = &pipe_fds[0];
    int *write_fds = &pipe_fds[1];

    if(pipe(pipe_fds)== -1){
      perror("Pipe not created");
      exit(EXIT_SUCCESS);
    }

    pid_t pid = fork();
    if(pid == -1){
      perror("Fork not created");
      exit(EXIT_FAILURE); 
    }

    if(pid != 0){
      merge_sort(&left_block);
      int childData[CHUNKSIZE];
      if(right_block.size > CHUNKSIZE){
        int *right_block_place = right_block.data;
        for (int i = 0; i < right_block.size/CHUNKSIZE; i++){
        int error = read(*read_fds, &childData, sizeof(int) * CHUNKSIZE);
        memcpy(right_block_place, childData, sizeof(int) * CHUNKSIZE);
        right_block_place += CHUNKSIZE;
        }
      } else {
        int error = read(*read_fds, &childData, sizeof(int) * right_block.size);
        memcpy(right_block.data, childData, sizeof(int) * right_block.size);
      }
      close(pipe_fds[0]);
      merge(&left_block, &right_block);
    } else {
      merge_sort(&right_block);
      if(right_block.size > CHUNKSIZE){
        int *right_block_place = right_block.data;
        for (int i = 0; i < right_block.size/CHUNKSIZE; i++){
          int data[CHUNKSIZE];
          memcpy(data, right_block_place, sizeof(int) * CHUNKSIZE);
          right_block_place += CHUNKSIZE;
          int error = write(*write_fds, data, sizeof(int) * CHUNKSIZE);
        }
      } else {
        int data[right_block.size];
        memcpy(data, right_block.data, sizeof(int) * right_block.size);
        int error = write(*write_fds, data, sizeof(int) * right_block.size);  
      }
      close(pipe_fds[1]);
      exit(EXIT_SUCCESS);
    }
    
    gettimeofday(&finish_wall_time, NULL);
    times(&finish_times);
    timersub(&finish_wall_time, &start_wall_time, &wall_time);
    printf("start time in clock ticks: %ld\n", start_times.tms_utime);
    printf("finish time in clock ticks: %ld\n", finish_times.tms_utime);
    printf("wall time %ld secs and %ld microseconds\n", wall_time.tv_sec, wall_time.tv_usec);

    if (block.size < 1025)
        print_data(&block);

    printf(is_sorted(&block) ? "sorted\n" : "not sorted\n");
    free(block.data);
    exit(EXIT_SUCCESS);
}