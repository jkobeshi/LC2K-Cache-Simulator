/*
 * EECS 370, University of Michigan
 * Project 4: LC-2K Cache Simulator
 * Instructions are found in the project spec.
 */

#include <stdio.h>

#define MAX_CACHE_SIZE 256
#define MAX_BLOCK_SIZE 256

extern int mem_access(int addr, int write_flag, int write_data);
extern int get_num_mem_accesses();

enum actionType
{
    cacheToProcessor,
    processorToCache,
    memoryToCache,
    cacheToMemory,
    cacheToNowhere
};

typedef struct blockStruct
{
    int data[MAX_BLOCK_SIZE];
    int dirty;
    int lruLabel;
    int set;
    int tag;
} blockStruct;

typedef struct cacheStruct
{
    blockStruct blocks[MAX_CACHE_SIZE];
    int blockSize;
    int numSets;
    int blocksPerSet;
} cacheStruct;

/* Global Cache variable */
cacheStruct cache;

void printAction(int, int, enum actionType);
void printCache();

/*
 * Set up the cache with given command line parameters. This is 
 * called once in main(). You must implement this function.
 */
void cache_init(int blockSize, int numSets, int blocksPerSet){
    cache.blockSize = blockSize; cache.numSets = numSets; cache.blocksPerSet = blocksPerSet;
    printf("Simulating a cache with %d total lines; each line has %d words\n", numSets * blocksPerSet, blockSize);
    printf("Each set in the cache contains %d lines; there are %d sets\n", blocksPerSet, numSets);
    for (int i = 0; i < numSets * blocksPerSet; ++i) {
        cache.blocks[i].tag = -1;
    }
    return;
}

/*
 * Access the cache. This is the main part of the project,
 * and should call printAction as is appropriate.
 * It should only call mem_access when absolutely necessary.
 * addr is a 16-bit LC2K word address.
 * write_flag is 0 for reads (fetch/lw) and 1 for writes (sw).
 * write_data is a word, and is only valid if write_flag is 1.
 * The return of mem_access is undefined if write_flag is 1.
 * Thus the return of cache_access is undefined if write_flag is 1.
 */
int numHit = 0, numMiss = 0, numWriteBack = 0;
void updateLRU(int set) {
    for (int i = set * cache.blocksPerSet; i < set * cache.blocksPerSet + cache.blocksPerSet; ++i) {
        if (cache.blocks[i].tag != -1) {
            cache.blocks[i].lruLabel += 1;
        }
    }
}
int findLRU(int set) {
    int maxNum = set * cache.blocksPerSet;
    for (int i = set * cache.blocksPerSet; i < set * cache.blocksPerSet + cache.blocksPerSet; ++i) {
        if (cache.blocks[i].lruLabel > cache.blocks[maxNum].lruLabel) {
            maxNum = i;
        }
    } return maxNum;
}
int cache_access(int addr, int write_flag, int write_data) {
    int tag = (addr / cache.blockSize) / cache.numSets;
    int set = (addr / cache.blockSize) % cache.numSets;
    int blockOffset = addr % cache.blockSize;
    //Finding Hit
    for (int i = set * cache.blocksPerSet; i < set * cache.blocksPerSet + cache.blocksPerSet; ++i) {
        if (cache.blocks[i].tag == tag) {
            updateLRU(set); cache.blocks[i].lruLabel = 0; numHit++;
            if (write_flag) {
                cache.blocks[i].dirty = 1; cache.blocks[i].data[blockOffset] = write_data;
                printAction(addr, 1, processorToCache);
                return -1;
            }
            printAction(addr, 1, cacheToProcessor);
            return cache.blocks[i].data[blockOffset];
        }
    } numMiss++;
    //Finding Non valid Bit or if tag == -1
    for (int i = set * cache.blocksPerSet; i < set * cache.blocksPerSet + cache.blocksPerSet; ++i) {
        if (cache.blocks[i].tag == -1) {
            cache.blocks[i].tag = tag; updateLRU(set); cache.blocks[i].lruLabel = 0;
            for (int j = addr - blockOffset; j < (addr - blockOffset + cache.blockSize); ++j) {
                cache.blocks[i].data[j % cache.blockSize] = mem_access(j, 0, 0);
            }
            printAction(addr - blockOffset, cache.blockSize, memoryToCache);
            if (write_flag) {
                cache.blocks[i].data[blockOffset] = write_data;
                cache.blocks[i].dirty = 1; printAction(addr, 1, processorToCache);
                return -1;
            }
            cache.blocks[i].dirty = 0; printAction(addr, 1, cacheToProcessor);
            return cache.blocks[i].data[blockOffset];
        }
    }
    //LRU (Check dirty bit)
    int LRU = findLRU(set);
    int LRUadr = (cache.blocks[LRU].tag * cache.blockSize * cache.numSets) | (set * cache.blockSize);
    if (cache.blocks[LRU].dirty) {
        printAction(LRUadr, cache.blockSize, cacheToMemory); numWriteBack++;
        for (int i = LRUadr; i < (LRUadr + cache.blockSize); ++i) {
            mem_access(i, 1, cache.blocks[LRU].data[i % cache.blockSize]);
            cache.blocks[LRU].data[i % cache.blockSize] = mem_access(addr - blockOffset + i % cache.blockSize, 0, 0);
        }
    }
    else {
        printAction(LRUadr, cache.blockSize, cacheToNowhere);
        for (int i = LRUadr; i < (LRUadr + cache.blockSize); ++i) {
            cache.blocks[LRU].data[i % cache.blockSize] = mem_access(addr - blockOffset + i % cache.blockSize, 0, 0);
        }
    } cache.blocks[LRU].tag = tag;
    updateLRU(set); cache.blocks[LRU].lruLabel = 0;
    printAction(addr - blockOffset, cache.blockSize, memoryToCache);
    if (write_flag) {
        cache.blocks[LRU].data[blockOffset] = write_data;
        cache.blocks[LRU].dirty = 1; printAction(addr, 1, processorToCache);
        return -1;
    }
    cache.blocks[LRU].dirty = 0; printAction(addr, 1, cacheToProcessor);
    return cache.blocks[LRU].data[blockOffset];
}


/*
 * print end of run statistics like in the spec. This is not required,
 * but is very helpful in debugging.
 * This should be called once a halt is reached.
 * DO NOT delete this function, or else it won't compile.
 * DO NOT print $$$ in this function
 */
void printStats(){
    printf("End of run statistics:\n");
    printf("hits %d, misses %d, writebacks %d\n", numHit, numMiss, numWriteBack);
    int numDirty = 0;
    for (int i = 0; i < cache.numSets * cache.blocksPerSet; ++i) {
        if (cache.blocks[i].dirty)
            ++numDirty;
    }
    printf("%d dirty cache blocks left\n", numDirty);
    return;
}

/*
 * Log the specifics of each cache action.
 *
 * address is the starting word address of the range of data being transferred.
 * size is the size of the range of data being transferred.
 * type specifies the source and destination of the data being transferred.
 *  -    cacheToProcessor: reading data from the cache to the processor
 *  -    processorToCache: writing data from the processor to the cache
 *  -    memoryToCache: reading data from the memory to the cache
 *  -    cacheToMemory: evicting cache data and writing it to the memory
 *  -    cacheToNowhere: evicting cache data and throwing it away
 */
void printAction(int address, int size, enum actionType type)
{
    printf("$$$ transferring word [%d-%d] ", address, address + size - 1);

    if (type == cacheToProcessor) {
        printf("from the cache to the processor\n");
    }
    else if (type == processorToCache) {
        printf("from the processor to the cache\n");
    }
    else if (type == memoryToCache) {
        printf("from the memory to the cache\n");
    }
    else if (type == cacheToMemory) {
        printf("from the cache to the memory\n");
    }
    else if (type == cacheToNowhere) {
        printf("from the cache to nowhere\n");
    }
}

/*
 * Prints the cache based on the configurations of the struct
 * This is for debugging only and is not graded, so you may
 * modify it, but that is not recommended.
 */
void printCache()
{
    printf("\ncache:\n");
    for (int set = 0; set < cache.numSets; ++set) {
        printf("\tset %i:\n", set);
        for (int block = 0; block < cache.blocksPerSet; ++block) {
            printf("\t\t[ %i ]: {", block);
            for (int index = 0; index < cache.blockSize; ++index) {
                printf(" %i", cache.blocks[set * cache.blocksPerSet + block].data[index]);
            }
            printf(" }\n");
        }
    }
    printf("end cache\n");
}