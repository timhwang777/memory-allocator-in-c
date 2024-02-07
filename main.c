#include <stdio.h>
#include <stdlib.h>
#include "umem.h"

/*
    Predefined test functions:
*/
void testMalloc();
void testCoalescing();
/*
    Predefined helper functions:
*/
void insertElement(int* array, int arraySize);
int compareArrays(int* array1, int* array2, int arraySize);
void printArray(int* array, int arraySize);

void testMalloc() {
    int arraySize = 5;
    int* array1 = (int*)malloc(sizeof(int) * arraySize);
    if (array1 == NULL) {
        printf("Memory allocation failed\n");
        return;
    }
    insertElement(array1, arraySize);
    printArray(array1, arraySize);

    // allocate the same memory region using umalloc
    int* array2 = (int*)umalloc(sizeof(int) * arraySize);
    if (array2 == NULL) {
        printf("Memory allocation failed\n");
        return;
    }
    insertElement(array2, arraySize);
    printArray(array2, arraySize);

    // compare the two arrays
    if (compareArrays(array1, array2, arraySize) == 1) {
        printf("Arrays are equal\n");
    } else {
        printf("Arrays are not equal\n");
    }

    // free the memory
    free(array1);
    ufree(array2);
}

void testCoalescing() {
    // Allocate three blocks of memory
    void* block1 = umalloc(100);
    void* block2 = umalloc(300);
    void* block3 = umalloc(500);

    // Deallocate the first and third blocks
    ufree(block1);
    ufree(block3);

    // Allocate a new block that is larger than any single free block, but smaller than the total amount of free memory
    void* block4 = umalloc(610);

    if (block4 == NULL) {
        printf("Coalescing failed\n");
    } else {
        printf("Coalescing succeeded\n");
    }

    // Deallocate the remaining blocks
    ufree(block2);
    ufree(block4);
}

void insertElement(int* array, int arraySize) {
    for (int i = 0; i < arraySize; i++) {
        array[i] = i;
    }
}

int compareArrays(int* array1, int* array2, int arraySize) {
    for (int i = 0; i < arraySize; i++) {
        if (array1[i] != array2[i]) {
            return -1;
        }
    }
    return 1;
}

void printArray(int* array, int arraySize) {
    for (int i = 0; i < arraySize; i++) {
        printf("%d ", array[i]);
    }
    printf("\n");
}

int main() {
    /* CAN NOT WORK: NEED umeminit RETURN *void */
    /*
    for(int i=1 ; i<=5 ; i++){
        printf("Testing algorithm %d\n", i);
        void* mem = umeminit(1024, i);
        if (mem != 0) {
            printf("Failed to initialize memory region\n");
            return 1;
        }
        testMalloc();
        testCoalescing();

        // Deinitialize the memory
        if (munmap(mem, 1024) != 0) {
            printf("Failed to deinitialize memory region\n");
            return 1;
        }
    }*/

    if (umeminit(1024, BEST_FIT) != 0) {
            printf("Failed to initialize memory region\n");
            return 1;
    }
    testMalloc();
    testCoalescing();

    return 0;
}