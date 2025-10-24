# Parallel Sum Program

## Описание
Программа `parallel_sum` вычисляет сумму элементов массива с использованием многопоточности (POSIX threads).

## Компиляция

### Вариант 1: Используя основной Makefile
```bash
make parallel_sum
```

### Вариант 2: Используя отдельный Makefile
```bash
make -f Makefile.parallel_sum
```

### Компиляция всех программ лабораторной работы 4
```bash
make all
```

## Использование
```bash
./parallel_sum --threads_num <число_потоков> --seed <сид> --array_size <размер_массива>
```

### Параметры:
- `--threads_num` - количество потоков для параллельного вычисления
- `--seed` - сид для генерации случайного массива
- `--array_size` - размер массива

### Пример:
```bash
./parallel_sum --threads_num 4 --seed 42 --array_size 1000000
```

## Особенности реализации:
1. Функция суммы вынесена в отдельную библиотеку (`sum_lib.c` и `sum_lib.h`)
2. Используется функция `GenerateArray` из лабораторной работы №3
3. Время генерации массива не учитывается в замере времени
4. Программа выводит общую сумму и время выполнения в секундах
5. Линковка с библиотекой pthread (`-lpthread`)

## Структура файлов:
- `parallel_sum.c` - основная программа
- `sum_lib.c`, `sum_lib.h` - библиотека для функции суммирования
- `utils.c`, `utils.h` - утилиты (включая GenerateArray)
- `Makefile` - основной файл сборки (включает все программы лаб. работы 4)
- `Makefile.parallel_sum` - отдельный Makefile только для parallel_sum

## Makefile команды:
```bash
# Сборка parallel_sum
make parallel_sum

# Сборка всех программ
make all

# Очистка объектных файлов
make clean

# Тестирование (только для Makefile.parallel_sum)
make -f Makefile.parallel_sum test
```

## Зависимости компиляции:
- `parallel_sum` зависит от: `utils.o`, `sum_lib.o`, `parallel_sum.c`
- Линковка с библиотекой pthread (`-lpthread`)
- Используются флаги компилятора: `-Wall -Wextra -O2 -I.`
