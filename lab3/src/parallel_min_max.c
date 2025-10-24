#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            // 
            if (seed <= 0) {
              printf("seed must be positive\n");
              return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            // 
            if (array_size <= 0) {
              printf("array_size must be positive\n");
              return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            // 
            if (pnum <= 0) {
              printf("pnum must be positive\n");
              return 1;
            }
            break;
          case 3:
            with_files = true;
            break;

          default:
            // 
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" \n",
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  //
  int (*pipes)[2] = NULL;     
  char **filenames = NULL;    
  
  if (with_files) {
    // синхронизация через файлы
    filenames = malloc(sizeof(char *) * pnum);
    for (int i = 0; i < pnum; ++i) {
      filenames[i] = malloc(64);  
      // генерируею имя 
      snprintf(filenames[i], 64, "parallel_%d_%d.bin", (int)getpid(), i);
    }
  } else {
    
    pipes = malloc(sizeof(int[2]) * pnum);
    for (int i = 0; i < pnum; ++i) {
      if (pipe(pipes[i]) == -1) {
        perror("pipe");
        return 1;
      }
    }
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // 
      active_child_processes += 1;
      if (child_pid == 0) {
        // 
        int idx = i; 
        
        int chunk_size = array_size / pnum;           
        int begin = idx * chunk_size;                 
        int end = (idx == pnum - 1) ? array_size : begin + chunk_size; 
        //если array_size не делится на pnum

        struct MinMax mm = GetMinMax(array, (unsigned)begin, (unsigned)end);

        if (with_files) {
          // 
          FILE *f = fopen(filenames[idx], "wb");      
          if (!f) { perror("fopen child"); _exit(2); }
          if (fwrite(&mm, sizeof(mm), 1, f) != 1) { 
            perror("fwrite child"); 
            fclose(f); 
            _exit(3); 
          }
          fclose(f);
        } else {
          // 
          close(pipes[idx][0]);  
          ssize_t w = write(pipes[idx][1], &mm, sizeof(mm));
          if (w != (ssize_t)sizeof(mm)) { 
            perror("write child"); 
            close(pipes[idx][1]); 
            _exit(4); 
          }
          close(pipes[idx][1]);  
        }
        return 0;  // дочерний процесс завершается
      }

    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  // ожидает завершения всех дочерних 
  while (active_child_processes > 0) {
    int status = 0;
    pid_t pid = wait(&status);
    if (pid == -1) {
      perror("wait");
      break;
    }
    active_child_processes -= 1;  
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      struct MinMax mm;
      FILE *f = fopen(filenames[i], "rb");  
      if (!f) { perror("fopen parent"); continue; }
      if (fread(&mm, sizeof(mm), 1, f) != 1) { 
        perror("fread parent"); 
        fclose(f); 
        continue; 
      }
      fclose(f);
      min = mm.min;
      max = mm.max;
      unlink(filenames[i]);
    } else {
      struct MinMax mm;
      close(pipes[i][1]);  
      ssize_t r = read(pipes[i][0], &mm, sizeof(mm));
      if (r != (ssize_t)sizeof(mm)) { 
        perror("read parent"); 
        close(pipes[i][0]); 
        continue; 
      }
      close(pipes[i][0]);  
      min = mm.min;
      max = mm.max;
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  if (with_files) {
    for (int i = 0; i < pnum; ++i) free(filenames[i]);
    free(filenames);
  } else {
    free(pipes);
  }

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}
