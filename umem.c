#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "umem.h"

#define WORD_SIZE sizeof(void*)

/*
    Memory block structure
    - next: pointer to the next memory block
    - dataSize: size of the data in the block
*/
typedef struct memoryBlock {
    __attribute__((packed))
    struct memoryBlock* next;
    size_t dataSize;
} memoryBlock;

/*
    Global variables
    - memoryBlockHead: pointer to the head of the memory block
    - lastAllocated: pointer to the last allocated memory block
    - algorithm: the algorithm used for allocation
    - sizeOfMemoryBlock: size of the memory block structure
*/
static memoryBlock* memoryBlockHead = NULL;
static memoryBlock* lastAllocated = NULL; // for next fit
static int algorithm = 0;
static size_t sizeOfMemoryBlock = sizeof(memoryBlock); //retreat the memory metadata size

/*
    Predefined memory allocation functions
*/
void *bestFit(size_t size);
void *worstFit(size_t size);
void *firstFit(size_t size);
void *nextFit(size_t size);
void *buddy(size_t size);
void umemdump();

/*
    Predefined helper functions
*/
int validBlock(memoryBlock* curr);
int validSplit(size_t freeBlockSize, size_t size);
void splitBlock(memoryBlock* currentBlock, size_t size);
void buddySplitBlock(memoryBlock* currentBlock, size_t size);
void setAllocated(memoryBlock* curr);
void setFree(memoryBlock* curr);
int isAllocated(memoryBlock* curr);
memoryBlock* getPreviousBlock(memoryBlock* currentBlock);
void merge(memoryBlock* prevBlock, memoryBlock* nextBlock);
memoryBlock* getBuddy(memoryBlock* block);
void mergeBuddies(memoryBlock* block);

int umeminit(size_t sizeOfRegion, int allocationAlgo){
    static int mmapAllocated = 0;
    int pageSize;
    int allocatedSize;
    int fd;
    void* initialBlockPtr;

    //fprintf(stderr, "umem.c debug: entring umeminit\n");

    if(mmapAllocated != 0){
        fprintf(stderr, "umem.c: mmap has already been used\n");
        return -1;
    }
    if(sizeOfRegion <= 0){
        fprintf(stderr, "umem.c: illegal sizeOfRegion %zu\n", sizeOfRegion);
        return -1;
    }
    if(allocationAlgo <= 0 || allocationAlgo >=6){
        fprintf(stderr, "umem.c: No algorithm %d\n", allocationAlgo);
        return -1;
    }
    if((sizeOfRegion & (sizeOfRegion - 1)) != 0){
        fprintf(stderr, "umem.c: sizeOfRegion must be a power of 2\n");
        return -1;
    }
    algorithm = allocationAlgo;

    pageSize = getpagesize();
    allocatedSize = (sizeOfRegion + pageSize - 1) / pageSize * pageSize;

    fd = open("/dev/zero", O_RDWR);
    if(fd == -1){
        fprintf(stderr, "umem.c: can not open the /dev/zero\n");
        return -1;
    }

    initialBlockPtr = mmap(NULL, allocatedSize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if(initialBlockPtr == MAP_FAILED){
        fprintf(stderr, "umem.c: mmap failed\n");
        mmapAllocated = 0;
        return -1;
    }
    mmapAllocated = 1;
    memoryBlockHead = (memoryBlock*) initialBlockPtr;
    
    // if it's buddy, make sure that the datasize is power of 2
    if(algorithm == BUDDY){
        memoryBlockHead->next = NULL;
        memoryBlockHead->dataSize = sizeOfRegion;
    }
    else{ // datasize will be the allocated size substrct the metadata size
        memoryBlockHead->next = NULL;
        memoryBlockHead->dataSize = allocatedSize - sizeOfMemoryBlock;
    }

    return 0;
}

void *umalloc(size_t size){
    //fprintf(stderr, "umem.c debug: entring umalloc\n");

    if(size <= 0){
        fprintf(stderr, "umem.c: illegal size %zu\n", size);
        return NULL;
    }

    // round up size to the word size
    size = (size + WORD_SIZE - 1) & (~(WORD_SIZE - 1));
    //fprintf(stderr, "umem.c debug: umalloc round size, %ld\n", size);

    // choose the algorithm
    switch(algorithm){
        case BEST_FIT:
            return bestFit(size);
            break;
        case WORST_FIT:
            return worstFit(size);
            break;
        case FIRST_FIT:
            return firstFit(size);
            break;
        case NEXT_FIT:
            return nextFit(size);
            break;
        case BUDDY:
            return buddy(size);
            break;
        default:
            fprintf(stderr, "umem.c: Unknown algorithm\n");
            return NULL;
    }
}

/*
    Different algorithms:
        - Best fit
        - Worst fit
        - First fit
        - Next fit
        - Buddy (Optional)
*/
void *bestFit(size_t size){
    size_t freeBlockSize = 0;
    memoryBlock* currentBlock = memoryBlockHead;
    memoryBlock* bestFitBlock = NULL;

    //fprintf(stderr, "umem.c debug: entring BestFit\n");

    while(currentBlock != NULL){
        if(validBlock(currentBlock)){
            if(currentBlock->dataSize >= size){
                if(freeBlockSize == 0 && currentBlock->dataSize >= size){
                    bestFitBlock = currentBlock;
                    freeBlockSize = currentBlock->dataSize + sizeOfMemoryBlock;
                }
                if((currentBlock->dataSize + sizeOfMemoryBlock) < freeBlockSize && currentBlock->dataSize >= size){
                    bestFitBlock = currentBlock;
                    freeBlockSize = currentBlock->dataSize + sizeOfMemoryBlock;
                }
            }
        }
        currentBlock = currentBlock->next;
    }

    if(bestFitBlock == NULL){
        return NULL;
    }

    // split the block
    if(validSplit(freeBlockSize, size)){
        splitBlock(bestFitBlock, size);
    }

    setAllocated(bestFitBlock);
    lastAllocated = bestFitBlock;  // update lastAllocated
    return (void*)((char*)bestFitBlock + sizeOfMemoryBlock);
}

void *worstFit(size_t size){
    size_t freeBlockSize = 0;
    memoryBlock* currentBlock = memoryBlockHead;
    memoryBlock* worstFitBlock = NULL;

    //fprintf(stderr, "umem.c debug: entring WorstFit\n");

    while(currentBlock != NULL){
        if(validBlock(currentBlock)){
            if(currentBlock->dataSize >= size){
                if((currentBlock->dataSize + sizeOfMemoryBlock) > freeBlockSize){
                    worstFitBlock = currentBlock;
                    freeBlockSize = currentBlock->dataSize + sizeOfMemoryBlock;
                }
            }
        }
        currentBlock = currentBlock->next;
    }

    if(worstFitBlock == NULL){
        return NULL;
    }

    // split the block
    if(validSplit(freeBlockSize, size)){
        splitBlock(worstFitBlock, size);
    }

    setAllocated(worstFitBlock);
    lastAllocated = worstFitBlock;  // update lastAllocated
    return (void*)((char*)worstFitBlock + sizeOfMemoryBlock);
}

void *firstFit(size_t size){
    memoryBlock* currentBlock = memoryBlockHead;

    //fprintf(stderr, "umem.c debug: entring FirstFit\n");

    while(currentBlock != NULL){
        if(validBlock(currentBlock) && currentBlock->dataSize >= size){
            // split the block
            if(validSplit(currentBlock->dataSize, size)){
                splitBlock(currentBlock, size);
            }
            setAllocated(currentBlock);
            lastAllocated = currentBlock;  // update lastAllocated
            return (void*)((char*)currentBlock + sizeOfMemoryBlock);
        }
        currentBlock = currentBlock->next;
    }

    return NULL;
}

void *nextFit(size_t size){
    // check whether lastAllocated is valid
    memoryBlock* currentBlock = lastAllocated ? lastAllocated->next : memoryBlockHead;

    //fprintf(stderr, "umem.c debug: entring NextFit\n");

    // First pass: from lastAllocated to end of memory
    while(currentBlock != NULL){
        if(validBlock(currentBlock) && currentBlock->dataSize >= size){
            // split the block
            if(validSplit(currentBlock->dataSize, size)){
               splitBlock(currentBlock, size);
            }
            setAllocated(currentBlock);
            lastAllocated = currentBlock;
            return (void*)((char*)currentBlock + sizeOfMemoryBlock);
        }
        currentBlock = currentBlock->next;
    }

    // Second pass: from start of memory to lastAllocated
    currentBlock = memoryBlockHead;
    while(currentBlock != lastAllocated){
        if(validBlock(currentBlock) && currentBlock->dataSize >= size){
            // split the block
            if(validSplit(currentBlock->dataSize, size)){
                splitBlock(currentBlock, size);
            }
            setAllocated(currentBlock);
            lastAllocated = currentBlock;
            return (void*)((char*)currentBlock + sizeOfMemoryBlock);
        }
        currentBlock = currentBlock->next;
    }

    return NULL;
}

void *buddy(size_t size){
    memoryBlock* currentBlock = memoryBlockHead;

    //fprintf(stderr, "umem.c debug: entring Buddy\n");

    // Find a block that can be split to satisfy the request
    while (currentBlock != NULL) {
        if (!isAllocated(currentBlock) && currentBlock->dataSize >= size) {
            // Split the block until it can't be split further
            while (currentBlock->dataSize / 2 >= size) {
                //fprintf(stderr, "umem.c debug: Buddy while block size: %zu\n", currentBlock->dataSize);
                buddySplitBlock(currentBlock, currentBlock->dataSize / 2);
            }
            //fprintf(stderr, "umem.c debug: Buddy datablock size: %zu\n", currentBlock->dataSize);

            setAllocated(currentBlock);
            lastAllocated = currentBlock;  // update lastAllocated
            return (void*)((char*)currentBlock + sizeOfMemoryBlock);
        }
        currentBlock = currentBlock->next;
    }

    return NULL;
}

int ufree(void *ptr){
    //fprintf(stderr, "umem.c debug: entring ufree\n");

    if(ptr == NULL){
        return -1;
    }

    memoryBlock* blockToFree = (memoryBlock*)((char*)ptr - sizeOfMemoryBlock);

    if(!isAllocated(blockToFree)){
        return -1;
    }

    setFree(blockToFree);

    // buddy free
    if(algorithm == BUDDY){
        mergeBuddies(blockToFree);
    }
    else{
        // Merge with next block if it's free
        if(blockToFree->next && !isAllocated(blockToFree->next)){
            merge(blockToFree, blockToFree->next);
        }

        // Merge with previous block if it's free
        memoryBlock* prevBlock = getPreviousBlock(blockToFree);
        if(prevBlock && !isAllocated(prevBlock)){
            merge(prevBlock, blockToFree);
        }
    }

    return 0;
}

/*
    Helper functions
*/
int validBlock(memoryBlock* curr){
    if(curr == NULL){
        return 0;
    }
    else{
        return (curr->dataSize % WORD_SIZE) == 0;
    }
}

int validSplit(size_t freeBlockSize, size_t size){
    return (freeBlockSize - size) >= sizeOfMemoryBlock;
}

void splitBlock(memoryBlock* currentBlock, size_t size){
    memoryBlock* newBlock = (memoryBlock*)((char*)currentBlock + sizeOfMemoryBlock + size);
    newBlock->next = currentBlock->next;
    newBlock->dataSize = currentBlock->dataSize - size - sizeOfMemoryBlock;
    currentBlock->next = newBlock;
    currentBlock->dataSize = size;
}

// split block for buddy
void buddySplitBlock(memoryBlock* currentBlock, size_t size){
    memoryBlock* newBlock = (memoryBlock*)((char*)currentBlock + sizeOfMemoryBlock + size);
    newBlock->next = currentBlock->next;
    newBlock->dataSize = size;
    currentBlock->next = newBlock;
    currentBlock->dataSize = size;
}

void setAllocated(memoryBlock* curr){
    curr->dataSize = curr->dataSize | 1;
}

void setFree(memoryBlock* curr){
    curr->dataSize = curr->dataSize & ~1;
}

int isAllocated(memoryBlock* curr){
    return curr->dataSize & 1;
}

memoryBlock* getPreviousBlock(memoryBlock* currentBlock){
    memoryBlock* previousBlock = memoryBlockHead;

    while(previousBlock != NULL){
        if(previousBlock->next == currentBlock){
            return previousBlock;
        }
        previousBlock = previousBlock->next;
    }

    return NULL;
}

void merge(memoryBlock* prevBlock, memoryBlock* nextBlock){
    prevBlock->dataSize += nextBlock->dataSize + sizeOfMemoryBlock;
    prevBlock->next = nextBlock->next;
}

// get the next buddy memory block
memoryBlock* getBuddy(memoryBlock* currentBlock){
    size_t buddyAddress = ((size_t)currentBlock - (size_t)memoryBlockHead) ^ currentBlock->dataSize;
    return (memoryBlock*)((char*)memoryBlockHead + buddyAddress);
}

// buddy ufree helper function
void mergeBuddies(memoryBlock* currentBlock){
    memoryBlock* buddy = getBuddy(currentBlock);
    while (buddy != NULL && !isAllocated(buddy) && buddy->dataSize == currentBlock->dataSize) {
        if (buddy < currentBlock) {
            buddy->dataSize *= 2;
            buddy->next = currentBlock->next;
            currentBlock = buddy;
        } else {
            currentBlock->dataSize *= 2;
            currentBlock->next = buddy->next;
        }
        buddy = getBuddy(currentBlock);
    }
}

/*
    Dump the memory state
*/
void umemdump(){
    memoryBlock* currentBlock = memoryBlockHead;
    int blockCount = 0;
    int allocatedBlockCount = 0;
    int freeBlockCount = 0;
    int allocatedSize = 0;
    int freeSize = 0;

    fprintf(stderr, "umem.c debug: entring umemdump\n");
    fprintf(stderr, "=====================MEMORY DUMP=====================\n");

    while(currentBlock != NULL){
        blockCount++;
        if(isAllocated(currentBlock)){
            allocatedBlockCount++;
            allocatedSize += (currentBlock->dataSize - 1);
        }
        else{
            freeBlockCount++;
            freeSize += currentBlock->dataSize;
        }
        currentBlock = currentBlock->next;
    }

    fprintf(stderr, "Size of the memory block metadata: %ld\n", sizeOfMemoryBlock);
    fprintf(stderr, "Total number of blocks: %d\n", blockCount);
    fprintf(stderr, "Total number of allocated blocks: %d\n", allocatedBlockCount);
    fprintf(stderr, "Total number of free blocks: %d\n", freeBlockCount);
    fprintf(stderr, "Total allocated size: %d\n", allocatedSize);
    fprintf(stderr, "Total free size: %d\n", freeSize);
    fprintf(stderr, "=====================MEMORY DUMP=====================\n");
}

/*
    Main function: used for VSCode debugging, uncomment to use
*/
/*
int main() {
    // Initialize memory region with size 1024 and BEST_FIT allocation algorithm
    if (umeminit(1024, BEST_FIT) != 0) {
        printf("Failed to initialize memory region\n");
        return 1;
    }

    // Allocate 256 bytes of memory
    void* ptr = umalloc(256);
    if (ptr == NULL) {
        printf("Failed to allocate memory\n");
        return 1;
    }

    // Dump the memory state
    umemdump();

    // Free the allocated memory
    if (ufree(ptr) != 0) {
        printf("Failed to free memory\n");
        return 1;
    }

    return 0;
}
*/