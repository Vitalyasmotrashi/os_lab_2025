#define _POSIX_C_SOURCE 200809L

#include <getopt.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct ThreadArgs {
  uint64_t start;
  uint64_t end; // inclusive
  uint64_t mod;
};

pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;
uint64_t global_result = 1;

uint64_t mul_mod(uint64_t a, uint64_t b, uint64_t mod) {
  return (a * b) % mod;
}

void *thread_func(void *arg) {
  struct ThreadArgs *args = (struct ThreadArgs *)arg;
  uint64_t res = 1;
  for (uint64_t i = args->start; i <= args->end; i++) {
    if (args->mod == 1) {
      res = 0;
      break;
    }
    res = (res * (i % args->mod)) % args->mod;
    if (res == 0) break; // early exit
  }

  pthread_mutex_lock(&result_mutex);
  global_result = (global_result * res) % args->mod;
  pthread_mutex_unlock(&result_mutex);
  return NULL;
}

int main(int argc, char **argv) {
  uint64_t k = 0;
  uint32_t pnum = 0;
  uint64_t mod = 0;

  struct option long_options[] = {{"pnum", required_argument, 0, 'p'},
                                 {"mod", required_argument, 0, 'm'},
                                 {0, 0, 0, 0}};

  int opt;
  while ((opt = getopt_long(argc, argv, "k:p:m:", long_options, NULL)) != -1) {
    switch (opt) {
    case 'k':
      k = strtoull(optarg, NULL, 10);
      break;
    case 'p':
      pnum = (uint32_t)atoi(optarg);
      break;
    case 'm':
      mod = strtoull(optarg, NULL, 10);
      break;
    default:
      printf("Usage: %s -k <num> --pnum=<num> --mod=<num>\n", argv[0]);
      return 1;
    }
  }

  if (k == 0 || pnum == 0 || mod == 0) {
    printf("Usage: %s -k <num> --pnum=<num> --mod=<num>\n", argv[0]);
    return 1;
  }

  pthread_t threads[pnum];
  struct ThreadArgs args[pnum];

  uint64_t chunk = k / pnum;
  uint64_t rem = k % pnum;
  uint64_t cur = 1;

  for (uint32_t i = 0; i < pnum; i++) {
    args[i].start = cur;
    uint64_t take = chunk + (i < rem ? 1 : 0);
    args[i].end = (take == 0) ? (cur - 1) : (cur + take - 1);
    args[i].mod = mod;
    cur = args[i].end + 1;
    if (args[i].start > args[i].end) {
      // empty range
      args[i].start = 1;
      args[i].end = 0;
    }
  }

  // create threads
  for (uint32_t i = 0; i < pnum; i++) {
    if (args[i].start <= args[i].end) {
      if (pthread_create(&threads[i], NULL, thread_func, &args[i]) != 0) {
        perror("pthread_create");
        return 1;
      }
    } else {
      // create dummy thread that does nothing
      threads[i] = 0;
    }
  }

  // join
  for (uint32_t i = 0; i < pnum; i++) {
    if (threads[i] != 0) pthread_join(threads[i], NULL);
  }

  printf("Result: %llu\n", (unsigned long long)global_result % mod);
  return 0;
}
