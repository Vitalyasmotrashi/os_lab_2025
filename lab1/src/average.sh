#!/bin/bash

# Вдруг обманули
if [ $# -eq 0 ]; then
    echo "ошибка: введи ченибудь"
    echo "введи: $0 число1 число2 ... и скок не впадлу"
    exit 1
fi

count=$#
sum=0

# сумма
for num in "$@"; do
    # вдруг не цифры
    if ! [[ "$num" =~ ^-?[0-9]+([.][0-9]+)?$ ]]; then
        echo "Ошибка: '$num' не является числом"
        exit 1
    fi
    sum=$(echo "$sum + $num" | bc -l)
done

average=$(echo "$sum / $count" | bc -l)

echo "Количество чисел: $count"
echo "Среднее арифметическое: $average"
