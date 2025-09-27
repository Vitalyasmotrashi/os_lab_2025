#!/bin/bash

# если не 1й раз запустил - очисщаем
> ./numbers.txt

for ((i=1; i<=150; i++)); do
    num=$(od -An -N4 -tu4 /dev/urandom | tr -d ' ')
    echo $num >> numbers.txt
done

echo "numbers.txt с 150 случайными числами ну рельно"
