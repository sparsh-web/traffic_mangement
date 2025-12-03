#include "utils.h"
#include <stdlib.h>

int clamp(int x, int lo, int hi) {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

int rand_percent(int p) {
    return (rand() % 100) < p;
}
