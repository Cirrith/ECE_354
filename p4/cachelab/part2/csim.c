/* Name: Ryan Bambrough
 * Partner's name: Cheng-Hsiang Hsu
 * CS logins: cheng-hsiang, bambrough
 * Section(s): 1
 *
 *
 *
 * csim.c - A cache simulator that can replay traces from Valgrind
 *     and output statistics such as number of hits, misses, and
 *     evictions.  The replacement policy is LRU.
 *
 * Implementation and assumptions:
 *  1. Each load/store can cause at most one cache miss. (I examined the trace,
 *  the largest request I saw was for 8 bytes).
 *  2. Instruction loads (I) are ignored, since we are interested in evaluating
 *  trans.c in terms of its data cache performance.
 *  3. data modify (M) is treated as a load followed by a store to the same
 *  address. Hence, an M operation can result in two cache hits, or a miss and a
 *  hit plus an possible eviction.
 *
 * The function printSummary() is given to print output.
 * Please use this function to print the number of hits, misses and evictions.
 * This is crucial for the driver to evaluate your work. 
 */
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include<stdbool.h>

#include "cachelab.h"

// #define DEBUG_ON 
#define ADDRESS_LENGTH 64

/****************************************************************************/
/***** DO NOT MODIFY THESE VARIABLE NAMES ***********************************/

/* Globals set by command line args */
int verbosity = 0; /* print trace if set */
int s = 0; /* set index bits */
int b = 0; /* block offset bits */
int E = 0; /* associativity */
char* trace_file = NULL;

/* Derived from command line args */
int S; /* number of sets S = 2^s In C, you can use "pow" function*/
int B; /* block size (bytes) B = 2^b In C, you can use "pow" function*/

/* Counters used to record cache statistics */
int miss_count = 0;
int hit_count = 0;
int eviction_count = 0;
/*****************************************************************************/


/* Type: Memory address 
 * Use this type whenever dealing with addresses or address masks
  */
typedef unsigned long long int mem_addr_t;

/* Type: Cache line
 * TODO 
 * 
 * NOTE: 
 * You might (not necessarily though) want to add an extra field to this struct
 * depending on your implementation
 * 
 * For example, to use a linked list based LRU,
 * you might want to have a field "struct cache_line * next" in the struct 
 */
typedef struct cache_line {
    char valid;
    mem_addr_t tag;
    struct cache_line *next;
} cache_line_t;

typedef cache_line_t* cache_set_t;
typedef cache_set_t* cache_t;


/* The cache we are simulating */
cache_t cache;  

/* TODO - COMPLETE THIS FUNCTION
 * initCache - 
 * Allocate data structures to hold info regrading the sets and cache lines
 * use struct "cache_line_t" here
 * Initialize valid and tag field with 0s.
 * calculate S = 2^s
 * use S and E while allocating the data structures here
 */
void initCache()
{
	S = pow(2,s);
	cache = (cache_set_t*) malloc(S*sizeof(cache_set_t*));

	for (int i = 0; i < S; i++)
	{
		cache_line_t *head = NULL;
		
		for (int j = 0; j < E; j++)
		{
			//allocate a single new line
			cache_line_t *line_new = (cache_line_t *) malloc(sizeof(cache_line_t));
			line_new->valid = (char)0;
			line_new->tag = 0;
			line_new->next = NULL;
			
			if(head == NULL)
			{
				head = line_new;
			}
			else
			{
				cache_line_t *curr = head;
				while(curr->next != NULL)
				{
					curr = curr->next;
				}
				curr->next = line_new;
			}
		}
		// Assign the head to the cache set
		cache[i] = head;
	}
}


/* TODO - COMPLETE THIS FUNCTION 
 * freeCache - free each piece of memory you allocated using malloc 
 * inside initCache() function
 */
void freeCache()
{
	for (int i = 0; i < S; i++)
	{
		cache_line_t *head = cache[i];
		
		for (int j = 0; j < E; j++)
		{
			cache_line_t *curr = head;
			cache_line_t *prev = NULL;
			
			while(curr->next != NULL)
			{
				prev = curr;
				curr = curr->next;
			}
			if(prev != NULL)
			{
				prev->next = NULL;
			}
			free(curr);
		}		
	}
}


/* TODO - COMPLETE THIS FUNCTION 
 * accessData - Access data at memory address addr.
 *   If it is already in cache, increase hit_count
 *   If it is not in cache, bring it in cache, increase miss count.
 *   Also increase eviction_count if a line is evicted.
 *   
 *   You will manipulate data structures allocated in initCache() here
 *   Implement Least-Recently-Used (LRU) cache replacement policy
 */
void accessData(mem_addr_t addr)
{
	mem_addr_t tag = (mem_addr_t)addr >> (s+b);
	long long int mask_s = (long long int)((S-1) << b);
	int set = (int) ((long long int)mask_s & ((long long int)addr)) >> b;	
	cache_line_t *head = cache[set];
	cache_line_t *curr = head;
	cache_line_t *prev = NULL; 
	
	if(E == 1) //Catch the Direct Mapped Case
	{
		if((curr->tag == tag) && (curr->valid == (char) 1))
		{
			hit_count++;
		}
		else
		{
			if(curr->valid == (char)1)
			{
				eviction_count++;
			}
			miss_count++;
			curr->valid = (char)1;
			curr->tag = tag;
			cache[set] = curr;
		}
		return;
	}
	
 	while(curr->next != NULL)
	{
		if((curr->tag == tag) && (curr->valid == (char) 1)) //Cache hit, increment and stuff
		{
			printf("Hit\n");
			hit_count++;
			if(curr == head)
			{
				//DO nothing. you all good
			}
			else
			{
				prev->next = curr->next;
				curr->next = head;
				cache[set] = curr;
			}
			return;
		}
		else if(curr->valid == (char)0)//Cold Case
		{
			if(curr == head)
			{
				curr->tag = tag;
				curr->valid = (char) 1;
			}
			else
			{
				prev->next = curr->next;
				curr->tag = tag;
				curr->valid = (char) 1;
				curr->next = head;
				cache[set] = curr;
			}
			printf("Miss\n");
			miss_count++;
			return;
		}
		else
		{
			prev = curr;
			curr = curr->next;	//Move to next line
		}
	}
	if((curr->tag == tag) & (curr->valid == (char)1))
	{
		prev->next = curr->next;
		curr->next = head;
		cache[set] = curr;
		hit_count++;
		return;
	}
	if(curr->valid == (char) 1)
	{
		eviction_count++;
		printf("Eviction");
	}
	printf("Miss\n");
	prev->next = NULL;			//Cache miss
	curr->tag = tag;
	curr->valid = (char)1;
	curr->next = head;
	cache[set] = curr;
	miss_count++;
	return;
}

/* TODO - FILL IN THE MISSING CODE
 * replayTrace - replays the given trace file against the cache 
 * reads the input trace file line by line
 * extracts the type of each memory access : L/S/M
 * YOU MUST TRANSLATE one "L" as a load i.e. 1 memory access
 * YOU MUST TRANSLATE one "S" as a store i.e. 1 memory access
 * YOU MUST TRANSLATE one "M" as a load followed by a store i.e. 2 memory accesses 
 * Ignore instruction fetch "I"
 */
void replayTrace(char* trace_fn)
{
    char buf[1000];
    mem_addr_t addr=0;
    unsigned int len=0;
    FILE* trace_fp = fopen(trace_fn, "r");

    if(!trace_fp){
        fprintf(stderr, "%s: %s\n", trace_fn, strerror(errno));
        exit(1);
    }

    while( fgets(buf, 1000, trace_fp) != NULL) {
        if(buf[1]=='S' || buf[1]=='L' || buf[1]=='M') {
            sscanf(buf+3, "%llx,%u", &addr, &len);
      
            if(verbosity)
                printf("%c %llx,%u ", buf[1], addr, len);

			if(buf[1] =='M')
			{
				accessData(addr);
			}
			accessData(addr);
           // TODO - MISSING CODE
           // now you have: 
           // 1. address accessed in variable - addr 
           // 2. type of acccess(S/L/M)  in variable - buf[1] 
           // call accessData function here depending on type of access




            if (verbosity)
                printf("\n");
        }
    }

    fclose(trace_fp);
}

/*
 * printUsage - Print usage info
 */
void printUsage(char* argv[])
{
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\nExamples:\n");
    printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv[0]);
    printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv[0]);
    exit(0);
}

/*
 * main - Main routine 
 */
int main(int argc, char* argv[])
{
    char c;
    
    // Parse the command line arguments: -h, -v, -s, -E, -b, -t 
    while( (c=getopt(argc,argv,"s:E:b:t:vh")) != -1){
        switch(c){
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
            trace_file = optarg;
            break;
        case 'v':
            verbosity = 1;
            break;
        case 'h':
            printUsage(argv);
            exit(0);
        default:
            printUsage(argv);
            exit(1);
        }
    }

    /* Make sure that all required command line args were specified */
    if (s == 0 || E == 0 || b == 0 || trace_file == NULL) {
        printf("%s: Missing required command line argument\n", argv[0]);
        printUsage(argv);
        exit(1);
    }


    /* Initialize cache */
    initCache();

#ifdef DEBUG_ON
    printf("DEBUG: S:%u E:%u B:%u trace:%s\n", S, E, B, trace_file);
#endif
 
    replayTrace(trace_file);

    /* Free allocated memory */
    freeCache();

    /* Output the hit and miss statistics for the autograder */
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
