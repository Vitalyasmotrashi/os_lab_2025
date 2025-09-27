#include "swap.h"

void Swap(char *left, char *right) { *left ^= *right ^= *left ^= *right; }
