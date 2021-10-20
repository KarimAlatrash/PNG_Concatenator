/*By: Karim Al-Atrash*/
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "curl_util.h"
#include "png_util.h"
#include "crc.h"
#include "lab_png.h"
#include "zutil.h"
#include "l_list.h"

#define IMG_URL1 "http://ece252-1.uwaterloo.ca:2520/image?img=1"
#define IMG_URL2 "http://ece252-2.uwaterloo.ca:2520/image?img=1"
#define IMG_URL3 "http://ece252-3.uwaterloo.ca:2520/image?img=1"

/*TODO
  1. Free all memory left in queue properly
  2. Implement reading command line input for # of processes and image #
  3. Move stuff in while loop to its own consumer function for readability
*/


struct producer_thread_args {
  char url[sizeof(IMG_URL1)];
  l_list *queue;
  pthread_mutex_t *quit_mtx;
  pthread_mutex_t *queue_mtx;
};

struct consumer_thread_args {
  l_list *queue;
  pthread_mutex_t *queue_mtx;
  struct simple_PNG **png_arr;
  struct data_IHDR *ihdr_dat;
  short *arr_count;
};

/*returns 1 if the quit mutex is unlocked*/
int quit_thread (pthread_mutex_t *mtx) {
  switch(pthread_mutex_trylock(mtx)) {
  case 0: /* if we got the lock, unlock and return 1 (true) */
    pthread_mutex_unlock(mtx);
    return 1;
  case EBUSY: /* return 0 (false) if the mutex was locked */
    return 0;
  }
  return 1;
}

void *produce_png(void *arg) {
  struct producer_thread_args *struc_arg = arg;
  char *url = struc_arg->url;
  l_list *queue = struc_arg->queue;
  pthread_mutex_t *quit_mtx = struc_arg->quit_mtx;
  pthread_mutex_t *queue_mtx = struc_arg->queue_mtx;

  /*if URL is null, default server will be 1*/
  CURL *curl_handle = curl_setup_fetch_file(url);

  while (!quit_thread(quit_mtx) && curl_handle) {
    RECV_BUF *recv_buf = malloc(sizeof(RECV_BUF));
    curl_fetch_file(recv_buf, curl_handle);

    node *new_node = create_node(recv_buf);
    new_node->byte_size = recv_buf->size;

    pthread_mutex_lock(queue_mtx);
    enqueue(queue, new_node);
    pthread_mutex_unlock(queue_mtx);
  }

  curl_easy_cleanup(curl_handle);
  return NULL;
}
void* consume_png(void *arg) {
  struct consumer_thread_args *args = arg;
  l_list *queue = args->queue;
  struct simple_PNG **png_arr =  args->png_arr;
  pthread_mutex_t *queue_mtx = args->queue_mtx;
  struct data_IHDR *ihdr_dat = args->ihdr_dat;
  short *arr_count = args->arr_count;

  RECV_BUF *rec_png;

  while(*arr_count != 50) {
    /*critical section start*/
    pthread_mutex_lock(queue_mtx);
    if(queue->length == 0) {
      pthread_mutex_unlock(queue_mtx);
      sleep(1); /*sleep 50ms*/
      continue;
    }
    rec_png = (RECV_BUF *)dequeue(queue);
    pthread_mutex_unlock(queue_mtx);
    /*critical section end*/

    /*fill the array with data if it is not already there*/
    if (png_arr[rec_png->seq] == NULL) {
      png_arr[rec_png->seq] = malloc(sizeof(struct simple_PNG));
      read_png_buf(png_arr[rec_png->seq], ihdr_dat, rec_png->buf, rec_png->size);
      (*arr_count)++;
    }

    /*free all unused data*/
    recv_buf_cleanup(rec_png);
    free(rec_png);
    rec_png = NULL;
  }

  return NULL;
}

void recv_buf_node_cleanup (void * arg) {
  RECV_BUF *recv_buf = arg;
  recv_buf_cleanup(recv_buf);
  free(recv_buf);
}

int get_cmdline (int *t, int*n, int argc, char** argv) {
  int c;
  char *str = "option requires an argument";
    
  while ((c = getopt (argc, argv, "t:n:")) != -1) {
    switch (c) {
    case 't':
	    *t = strtoul(optarg, NULL, 10);
	    printf("option -t specifies a value of %d.\n", *t);
	    if (*t <= 0) {
        fprintf(stderr, "%s: %s > 0 -- 't'\n", argv[0], str);
        return -1;
      }
      break;
    case 'n':
      *n = strtoul(optarg, NULL, 10);
	    printf("option -n specifies a value of %d.\n", *n);
      if (*n <= 0 || *n > 3) {
        fprintf(stderr, "%s: %s 1, 2, or 3 -- 'n'\n", argv[0], str);
        return -1;
      }
      break;
    default:
      return -1;
    }
  }
  return -1;
}

int produce_png_st(char* url) {
  struct simple_PNG *png_arr[50] = {NULL};
  short arr_count = 0;
  l_list *queue = create_list();
  curl_global_setup();
  
  struct data_IHDR ihdr_dat;
  RECV_BUF *rec_png;
  CURL *curl_handle = curl_setup_fetch_file(url);

  while(arr_count != 50) {
    /*produce data*/
    RECV_BUF *recv_buf = malloc(sizeof(RECV_BUF));
    /*if URL is null, default server will be 1*/
    curl_fetch_file(recv_buf, curl_handle);
    node *new_node = create_node(recv_buf);
    new_node->byte_size = recv_buf->size;
    enqueue(queue, new_node);


    /*consume data*/
    rec_png = (RECV_BUF *)dequeue(queue);
    if (png_arr[rec_png->seq] == NULL) {
      png_arr[rec_png->seq] = malloc(sizeof(struct simple_PNG));
      read_png_buf(png_arr[rec_png->seq], &ihdr_dat, rec_png->buf, rec_png->size);
      arr_count++;
    }

    /*free all unused data*/
    recv_buf_cleanup(rec_png);
    free(rec_png);
    rec_png = NULL;
  }
  curl_easy_cleanup(curl_handle);
  curl_global_teardown();
  clear_list_helper(queue, recv_buf_node_cleanup);

  for(int i = 0; i < 50; i++) {
    free_png(png_arr[i]);
    free(png_arr[i]);
    png_arr[i] = NULL;
  }

  return 0;

}

int main (int argc, char** argv) {

  /*READING COMMAND LINE ARGUMENT*/
  char url_list[3][sizeof(IMG_URL1)] = {{IMG_URL1}, {IMG_URL2}, {IMG_URL3}};
  int thread_count = 1;
  int img_num;
  get_cmdline(&thread_count, &img_num, argc, argv);
  char img_num_c = '1'+img_num-1;
  url_list[0][sizeof(url_list[0]) -2 ] = img_num_c;
  url_list[1][sizeof(url_list[0]) -2 ] = img_num_c;
  url_list[2][sizeof(url_list[0]) -2 ] = img_num_c;

  /*SINGLE THREADED IMPLEMENTATION*/
  if(thread_count == 1) {
    return produce_png_st(url_list[0]);
  }

  /*MULITHREADED IMPLEMENTATION */
  struct simple_PNG *png_arr[50] = {NULL};
  l_list *queue = create_list();
  struct data_IHDR ihdr_dat;
  short arr_count = 0;

  pthread_mutex_t quit_mtx;
  pthread_mutex_init(&quit_mtx,NULL);

  pthread_mutex_t queue_mtx;
  pthread_mutex_init(&queue_mtx,NULL);

  pthread_t *p_tids = malloc(sizeof(pthread_t) * thread_count);
  struct producer_thread_args *producer_params = (struct producer_thread_args *)malloc(sizeof(struct producer_thread_args) * thread_count);

  /*
   *PRODUCE DATA
   */

  /*Tells the threads spawned not to quit*/
  pthread_mutex_lock(&quit_mtx);
  curl_global_setup();
  for (int i = 0; i < thread_count; i++) {
    strcpy(producer_params[i].url, url_list[i%3]);
    pthread_mutex_lock(&queue_mtx);
    producer_params[i].queue = queue;
    pthread_mutex_unlock(&queue_mtx);
  	producer_params[i].queue_mtx = &queue_mtx;
    producer_params[i].quit_mtx = &quit_mtx;
    pthread_create(p_tids + i, NULL, produce_png, &producer_params[i]); 
  }

  /*
   *CONSUME DATA
   */
  struct consumer_thread_args cons_arg;

  pthread_mutex_lock(&queue_mtx);
  cons_arg.queue = queue;
  pthread_mutex_unlock(&queue_mtx);

  cons_arg.arr_count = &arr_count;
  cons_arg.png_arr = png_arr;
  cons_arg.queue_mtx = &queue_mtx;
  cons_arg.ihdr_dat = &ihdr_dat;

  consume_png(&cons_arg);

  /*tells all threads to quit next chance they get*/
  pthread_mutex_unlock(&quit_mtx);
  
  /*free all data*/
  for (int i = 0; i < thread_count; i++) {
    pthread_join(p_tids[i], NULL);
  }
  curl_global_teardown();
  clear_list_helper(queue, recv_buf_node_cleanup);
  free(producer_params);
  free(p_tids);

  cat_png(png_arr, 50);

  for(int i = 0; i < 50; i++) {
    free_png(png_arr[i]);
    free(png_arr[i]);
    png_arr[i] = NULL;
  }
  
  return 0;
}
