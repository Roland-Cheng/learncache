#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

typedef struct
{
    int valid;
    int lru;
    unsigned long tag;
} CacheLine;

typedef CacheLine *CacheSet;
typedef CacheSet *Cache;

int hits = 0;
int misses = 0;
int evictions = 0;
char buf[20];
const char *usage = "Usage: %s [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n";

int verbose = 0;
int s;
int E;
int b;
FILE *fp = NULL;
Cache cache;

int power(int a, int b);
void handleArgv(int argc, char *argv[]);
void cacheSimulator();
int cacheChange(unsigned long addr);

int main(int argc, char *argv[])
{
    handleArgv(argc, argv);
    cacheSimulator();
    printSummary(hits, misses, evictions);
    return 0;
}

int power(int a, int b)
{
    int i = 0;
    int result = 1;
    for (; i < b; i++)
        result *= a;
    return result;
}

void handleArgv(int argc, char *argv[])
{
    int opt;
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            fprintf(stdout, usage, argv[0]);
            printf("Options:\n");
            printf("  -h         Print this help message.\n");
            printf("  -v         Optional verbose flag.\n");
            printf("  -s <num>   Number of set index bits.\n");
            printf("  -E <num>   Number of lines per set.\n");
            printf("  -b <num>   Number of block offset bits.\n");
            printf("  -t <file>  Trace file.\n\n");
            exit(1);
        case 'v':
            verbose = 1;
            break;
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            fp = fopen(optarg, "r");
            break;
        default:
            fprintf(stdout, usage, argv[0]);
            exit(1);
        }
    }
}

void cacheSimulator()
{
    int i;
    int S = power(2, s);
    cache = (Cache)malloc(sizeof(CacheSet) * S);
    if (cache == NULL)
        exit(1);
    for (i = 0; i < S; i++)
    {
        cache[i] = (CacheSet)calloc(E, sizeof(CacheLine));
        if (cache[i] == NULL)
            exit(1);
    }

    char operation;
    unsigned long addr;
    int size;
    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
        if (buf[0] == 'I')
            continue;
        else
        {
            sscanf(buf, " %c %lx,%d", &operation, &addr, &size);
            int result = cacheChange(addr);
            if (verbose)
            {
                switch (result)
                {
                case 0:
                    printf(" %c %lx,%d hit", operation, addr, size);
                    break;
                case 1:
                    printf(" %c %lx,%d miss", operation, addr, size);
                    break;
                case 2:
                    printf(" %c %lx,%d miss eviction", operation, addr, size);
                    break;
                }
            }
            if (operation == 'M')
            {
                hits++;
                if (verbose)
                    printf(" hit");
            }
            printf("\n");
        }
    }
    for (i = 0; i < S; i++)
        free(cache[i]);
    free(cache);
    fclose(fp);
    return;
}

int cacheChange(unsigned long addr)
{
    int i;
    unsigned long tag = addr >> (s + b);
    unsigned int setIndex = addr << (sizeof(addr) * 8 - s - b) >> (sizeof(addr) * 8 - s);
    int evict = 0;
    int empty = -1;
    CacheSet usingcache = cache[setIndex];
    for (i = 0; i < E; i++)
    {
        if (usingcache[i].valid)
        {
            if (usingcache[i].tag == tag)
            {
                hits++;
                usingcache[i].lru = 1;
                return 0;
            }
            usingcache[i].lru++;
            if (usingcache[evict].lru <= usingcache[i].lru)
                evict = i;
        }
        else
            empty = i;
    }
    misses++;
    if (empty != -1)
    {
        usingcache[empty].valid = 1;
        usingcache[empty].tag = tag;
        usingcache[empty].lru = 1;
        return 1;
    }
    else
    {
        usingcache[evict].tag = tag;
        usingcache[evict].lru = 1;
        evictions++;
        return 2;
    }
}