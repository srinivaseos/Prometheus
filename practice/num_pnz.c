// C Program to check if a number is positive, negative,
// or zero using bitwise operators
#include <stdio.h>

void checkNum(int n) {

    // Check if the number is zero
    if (n == 0) {
        printf("Zero\n");
        return;
    }

    // Extracting msb
    int msb = (n >> 31) & 1;

    if (msb)
        printf("Negative\n");
    else
        printf("Positive\n");
}

int main() {
    int N = -10;
    checkNum(N);
    return 0;
}