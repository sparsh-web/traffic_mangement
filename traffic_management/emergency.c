#include "emergency.h"

int find_global_emergency(int emergency[], int n) {
    for (int i = 0; i < n; i++)
        if (emergency[i] > 0)
            return i;
    return -1;
}
