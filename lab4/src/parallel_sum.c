#define _POSIX_C_SOURCE 199309L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <getopt.h>

#include "sum_lib.h"
#include "utils.h"

void *ThreadSum(void *args) {
  struct SumArgs *sum_args = (struct SumArgs *)args;
  return (void *)(size_t)Sum(sum_args);
}

int main(int argc, char **argv) {
  uint32_t threads_num = 0;
  uint32_t array_size = 0;
  uint32_t seed = 0;
  
  // Парсинг аргументов командной строки
  struct option long_options[] = {
    {"threads_num", required_argument, 0, 't'},
    {"seed", required_argument, 0, 's'},
    {"array_size", required_argument, 0, 'a'},
    {0, 0, 0, 0}
  };
  
  int option_index = 0;
  int c;
  
  while ((c = getopt_long(argc, argv, "t:s:a:", long_options, &option_index)) != -1) {
    switch (c) {
      case 't':
        threads_num = atoi(optarg);
        break;
      case 's':
        seed = atoi(optarg);
        break;
      case 'a':
        array_size = atoi(optarg);
        break;
      case '?':
        printf("Usage: %s --threads_num <num> --seed <num> --array_size <num>\n", argv[0]);
        return 1;
    }
  }
  
  if (threads_num == 0 || array_size == 0) {
    printf("Usage: %s --threads_num <num> --seed <num> --array_size <num>\n", argv[0]);
    return 1;
  }
  
  // Генерация массива (не входит в замер времени)
  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  
  pthread_t threads[threads_num];

  // Начало замера времени
  struct timespec start_time, end_time;
  clock_gettime(CLOCK_MONOTONIC, &start_time);
  
  struct SumArgs args[threads_num];
  uint32_t part_size = array_size / threads_num;
  
  // Инициализация аргументов для каждого потока
  for (uint32_t i = 0; i < threads_num; i++) {
    args[i].array = array;
    args[i].begin = i * part_size;
    if (i == threads_num - 1) {
      // Последний поток обрабатывает оставшиеся элементы
      args[i].end = array_size;
    } else {
      args[i].end = (i + 1) * part_size;
    }
  }
  
  // Создание потоков
  for (uint32_t i = 0; i < threads_num; i++) {
    if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i])) {
      printf("Error: pthread_create failed!\n");
      return 1;
    }
  }

  int total_sum = 0;
  for (uint32_t i = 0; i < threads_num; i++) {
    int sum = 0;
    pthread_join(threads[i], (void **)&sum);
    total_sum += sum;
  }
  
  // Конец замера времени
  clock_gettime(CLOCK_MONOTONIC, &end_time);
  
  double elapsed_time = (end_time.tv_sec - start_time.tv_sec) + 
                       (end_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;

  free(array);
  printf("Total: %d\n", total_sum);
  printf("Elapsed time: %.6f seconds\n", elapsed_time);
  return 0;
}
