#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    printf("=== ДЕМОНСТРАЦИЯ ЗОМБИ-ПРОЦЕССА ===\n\n");
    
    printf("ЧТО ТАКОЕ ЗОМБИ-ПРОЦЕСС:\n");
    printf("- Процесс завершился, но родитель не прочитал его статус\n");
    printf("- Занимает место в таблице процессов\n");
    printf("- Показывается как <defunct> или состояние Z\n\n");
    
    pid_t child_pid = fork();
    
    if (child_pid == -1) {
        perror("fork failed");
        return 1;
    }
    
    if (child_pid == 0) {
        // ДОЧЕРНИЙ ПРОЦЕСС
        printf("Дочерний процесс (PID: %d) завершается\n", getpid());
        exit(42);  // Завершаемся с кодом 42
    } else {
        // РОДИТЕЛЬСКИЙ ПРОЦЕСС
        printf("Родитель (PID: %d) создал дочерний (PID: %d)\n", getpid(), child_pid);
        
        // НЕ ВЫЗЫВАЕМ wait() - создаем зомби!
        printf("\nРодитель НЕ вызывает wait() - создаем зомби!\n");
        sleep(1);  // Даем дочернему завершиться
        
        printf("\nПроверьте зомби в другом терминале:\n");
        printf("  ps aux | grep defunct\n");
        printf("  ps -o pid,ppid,stat,comm -p %d\n", child_pid);
        
        printf("\nЗомби будет существовать 5 секунд...\n");
        for (int i = 25; i > 0; i--) {
            printf("Осталось: %d сек\n", i);
            sleep(1);
        }
        
        printf("\nТеперь очищаем зомби с помощью wait()...\n");
        int status;
        pid_t waited_pid = wait(&status);
        
        printf("Очищен процесс PID %d, код завершения: %d\n", 
               waited_pid, WEXITSTATUS(status));
        
        printf("\nЗомби исчез! Проверьте снова:\n");
        printf("  ps -o pid,ppid,stat,comm -p %d\n", child_pid);
        
        printf("\nКАК БОРОТЬСЯ С ЗОМБИ:\n");
        printf("1. Всегда вызывайте wait()/waitpid()\n");
        printf("2. Используйте обработчик сигнала SIGCHLD\n");
        printf("3. Убить родительский процесс (зомби станут сиротами)\n");
    }
    
    return 0;
}