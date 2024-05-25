#include "cachelab.h"
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

void printUsage(char* command) {
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", command);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num> Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num> Number of block bits.\n");
    printf("  -t <file> Trace file.\n");
    printf("\nExamples:\n");
    printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", command);
    printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", command);
    exit(0);
}

struct line{
    int valid;
    int tag;
    int lastUsedTime;
};

typedef struct line* set;

set* createCache(int s, int E){
    int S = 1 << s;
    set* cache = (set*)malloc(sizeof(set) * S);
    for(int i=0; i<S; ++i){
        cache[i] = (set)malloc(sizeof(struct line) * E);
        for(int j=0; j<E; ++j){
            struct line* thisLine = &cache[i][j];
            thisLine->valid = -1;
            thisLine->tag = -1;
            thisLine->lastUsedTime = -1;
        } 
    }
    return cache;
}

void simCache(size_t address, bool isModify, int s, int E, int b, set* cache, bool verbose, int* hit, int* miss, int* eviction, int time){
    int tag = address >> (b+s);
    int setIndex = (address >> b) & ((1<<s)-1);
    set thisSet = cache[setIndex];
    int lruPos = 0, lruTime = thisSet[0].lastUsedTime;
    for(int i=0; i<E; ++i){
        if (thisSet[i].tag == tag){
            ++(*hit);
            if (isModify) ++(*hit);
            thisSet[i].lastUsedTime = time;
            if (verbose) printf("hit\n");
            return;
        }

        // search for the least recent used line
        if (thisSet[i].lastUsedTime < lruTime){
            lruTime = thisSet[i].lastUsedTime;
            lruPos = i;
        }
    }
    ++(*miss);
    if (lruTime!=-1){
        ++(*eviction);
        if (isModify){
            ++(*hit);
            if (verbose) printf("miss eviction hit\n");
        }
        else{
            if(verbose) printf("miss eviction\n");
        }
    }
    else{
        if (isModify){
            ++(*hit);
            if (verbose) printf("miss hit\n");
        }
        else{
            if (verbose) printf("miss\n");
        }
    }

    thisSet[lruPos].lastUsedTime = time;
    thisSet[lruPos].tag = tag;
    return;

}




void run(int s, int E, int b, char* traceFile, bool verbose) {
    char operation;
    size_t address;
    int size;
    set* cache = createCache(s, E);

    FILE* fp = fopen(traceFile, "r");

    int hit = 0, miss = 0, eviction = 0, time = 0;
    while (fscanf(fp, "%s %lx,%d\n", &operation, &address, &size) == 3)
    {
        if(operation == 'I') {
            continue;
        }
        else if (operation == 'L' || operation == 'S') {
            if (verbose) printf("%c %lx, %d ", operation, address, size);
            simCache(address, false, s, E, b, cache, verbose, &hit, &miss, &eviction, time++);
        }
        else if (operation == 'M')
        {
            if (verbose) printf("%c %lx, %d ", operation, address, size);
            simCache(address, true, s, E, b, cache, verbose, &hit, &miss, &eviction, time++);
        }
        
    };
    free(cache);
    printSummary(hit, miss, eviction);

}

bool argValid(int s, int E, int b, char* traceFile) {
    if (s <= 0 || E <= 0 || b <= 0 || traceFile == NULL) {
        printf("Error: Invalid arguments.\n");
        return false;
    }
    return true;
}



int main(int argc, char *argv[]) {
    int opt;
    int s_value, E_value, b_value;
    bool verbose;
    char *t_value;

    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt) {
        case 's':
            s_value = atoi(optarg);
            break;
        case 'v':
            verbose = true;
            break;
        case 'E':
            E_value = atoi(optarg);
            break;
        case 'b':
            b_value = atoi(optarg);
            break;
        case 't':
            t_value = optarg;
            break;
        default: /* '?' */
            printUsage(argv[0]);
            exit(0);
        }
    }
    if(!argValid(s_value, E_value, b_value, t_value)) {
        exit(1);
    }
    run(s_value, E_value, b_value, t_value, verbose);



    return 0;
}