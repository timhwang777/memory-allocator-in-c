#include <stdio.h>

#define WORD_SIZE sizeof(void*)

int main() {
    printf("Word size: %zu bytes\n", WORD_SIZE);
    return 0;
}