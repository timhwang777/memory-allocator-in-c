#include<stdio.h>

typedef struct linkBlock {
    __attribute__((packed))
    struct linkBlock* next;
    size_t value;
} linkBlock;

int main() {
    linkBlock aBlock;
    printf("*next size: %zu\n", sizeof(aBlock.next));
    printf("value size: %zu\n", sizeof(aBlock.value));
    printf("Block size: %zu\n", sizeof(aBlock));
    printf("size_t size: %zu\n", sizeof(size_t));
    return 0;
}