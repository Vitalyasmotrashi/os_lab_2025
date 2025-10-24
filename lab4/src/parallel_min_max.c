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
#include <signal.h>  // ДОПОЛНЕНО: Для работы с сигналами

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"
  
// ДОПОЛНЕНО: Глобальные переменные для обработки таймаута
static pid_t *child_pids = NULL;  // Массив PID дочерних процессов
static int child_count = 0;       // Количество дочерних процессов
static bool timeout_occurred = false;  // Флаг истечения таймаута

// ДОПОЛНЕНО: Обработчик сигнала SIGALRM (таймаут)
void alarm_handler(int sig) {
    if (sig == SIGALRM) {
        printf("\nТаймаут истек! Завершаем дочерние процессы...\n");
        timeout_occurred = true;
        
        // Отправляем SIGKILL всем дочерним процессам
        for (int i = 0; i < child_count; i++) {
            if (child_pids[i] > 0) {
                printf("Отправляем SIGKILL процессу PID: %d\n", child_pids[i]);
                kill(child_pids[i], SIGKILL);
            }
        }
    }
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  bool with_files = false;
  int timeout_seconds = -1;  // ДОПОЛНЕНО: Таймаут в секундах (-1 = не задан)

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {"timeout", required_argument, 0, 0},
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
          case 4:
            // ДОПОЛНЕНО: Обработка параметра --timeout
            timeout_seconds = atoi(optarg);
            if (timeout_seconds <= 0) {
              printf("timeout must be positive\n");
              return 1;
            }
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

  // ДОПОЛНЕНО: Инициализация для обработки таймаута
  child_pids = malloc(sizeof(pid_t) * pnum);  // Массив для хранения PID дочерних процессов
  child_count = pnum;
  
  // ДОПОЛНЕНО: Установка обработчика сигнала SIGALRM, если задан таймаут
  if (timeout_seconds > 0) {
    printf("Установлен таймаут: %d секунд\n", timeout_seconds);
    signal(SIGALRM, alarm_handler);  // Устанавливаем обработчик сигнала
    alarm(timeout_seconds);          // Запускаем таймер
  }

  // Создание структур для межпроцессного взаимодействия (IPC)
  int (*pipes)[2] = NULL;     // Массив пайпов для передачи данных между процессами
  char **filenames = NULL;    // Массив имен файлов для передачи данных через файлы    
  
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
      // successful fork
      active_child_processes += 1;
      // ДОПОЛНЕНО: Сохраняем PID дочернего процесса для возможной отправки сигнала
      child_pids[i] = child_pid;
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

  // ДОПОЛНЕНО: Ожидание завершения дочерних процессов с обработкой таймаута
  while (active_child_processes > 0) {
    int status = 0;
    // Используем неблокирующий wait с WNOHANG
    pid_t pid = waitpid(-1, &status, WNOHANG);
    
    if (pid > 0) {
      // Дочерний процесс завершился
      active_child_processes -= 1;
      printf("Дочерний процесс PID %d завершился\n", pid);
    } else if (pid == 0) {
      // Нет завершенных процессов, проверяем таймаут
      if (timeout_occurred) {
        printf("Выход из цикла ожидания из-за таймаута\n");
        break;
      }
      // Небольшая пауза, чтобы не нагружать CPU
      usleep(10000);  // 10ms
    } else {
      // Ошибка waitpid
      if (!timeout_occurred) {
        perror("waitpid");
      }
      break;
    }
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

  // ДОПОЛНЕНО: Отключаем таймер если он был установлен
  if (timeout_seconds > 0) {
    alarm(0);  // Отключить таймер
  }

  free(array);
  // ДОПОЛНЕНО: Освобождаем память для массива PID
  free(child_pids);
  
  if (with_files) {
    for (int i = 0; i < pnum; ++i) free(filenames[i]);
    free(filenames);
  } else {
    free(pipes);
  }

  // ДОПОЛНЕНО: Выводим сообщение о таймауте, если он произошел
  if (timeout_occurred) {
    printf("ВНИМАНИЕ: Выполнение прервано по таймауту!\n");
  }

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}
