#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s seed array_size\n", argv[0]);
        printf("Эта программа запускает sequential_min_max в отдельном процессе через exec\n");
        return 1;
    }

    int seed = atoi(argv[1]);
    int array_size = atoi(argv[2]);
    
    if (seed <= 0 || array_size <= 0) {
        printf("Аргументы seed и array_size должны быть положительными числами\n");
        return 1;
    }

    printf("Запускаем sequential_min_max с параметрами: seed=%d, array_size=%d\n", seed, array_size);
    printf("Родительский процесс PID: %d\n", (int)getpid());
    
    pid_t child_pid = fork();
    
    if (child_pid == -1) {
        perror("fork failed");
        return 1;
    }
    
    if (child_pid == 0) {
        // ДОЧЕРНИЙ ПРОЦЕСС: заменяем его на sequential_min_max с помощью exec
        printf("Дочерний процесс PID: %d\n", (int)getpid());
        printf("Выполняем exec для запуска sequential_min_max...\n");
        
        // execl заменяет текущий процесс на указанную программу
        // Формат: execl(путь_к_программе, имя_программы, аргумент1, аргумент2, NULL)
        execl("./sequential_min_max", "sequential_min_max", argv[1], argv[2], NULL);
        
        // Если exec успешен, эта строка никогда не выполнится
        // Если мы здесь - значит exec завершился с ошибкой
        perror("execl failed");
        exit(1);
    } else {
        // РОДИТЕЛЬСКИЙ ПРОЦЕСС: ждем завершения дочернего процесса
        int status;
        printf("Родительский процесс ожидает завершения дочернего процесса (PID: %d)...\n", child_pid);
        
        pid_t waited_pid = wait(&status);
        
        if (waited_pid == -1) {
            perror("wait failed");
            return 1;
        }
        
        // Анализируем статус завершения дочернего процесса
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            printf("Дочерний процесс завершился нормально с кодом: %d\n", exit_code);
            
            if (exit_code == 0) {
                printf("sequential_min_max выполнился успешно!\n");
            } else {
                printf("sequential_min_max завершился с ошибкой!\n");
            }
        } else if (WIFSIGNALED(status)) {
            int signal_num = WTERMSIG(status);
            printf("Дочерний процесс был завершен сигналом: %d\n", signal_num);
        }
        
        printf("Родительский процесс завершает работу.\n");
    }
    
    return 0;
}