#include "cachelab.h"
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

/*
############  csim usage ##############

Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>
Options:
  -h         Print this help message.
  -v         Optional verbose flag.
  -s <num>   Number of set index bits.
  -E <num>   Number of lines per set.
  -b <num>   Number of block offset bits.
  -t <file>  Trace file.

Examples:
  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace
  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace
*/

const int maxn = (1 << 16);

typedef struct
{
    int tag_bits;
    int set_index_bits;
    int block_offset_bits;
} Address;

typedef struct
{
    Address index;
    unsigned long long address;
    short size;
    char operation[3];
} Trace;

typedef struct
{
    unsigned long long tag_bits;
    bool v;
} CacheBlock;

typedef struct
{
    int hits_count;
    int misses_count;
    int evictions_count;
} Counter;

void print_block(CacheBlock block)
{
    printf("valid: %d\n", block.v);
    printf("tag: %llu\n", block.tag_bits);
}

void print_set(CacheBlock *set_header, int num_blocks)
{
    int i = 0;
    printf("-------------------\n");
    for (i = 0; i < num_blocks; ++i)
    {
        printf("%dth block:\n", i);
        print_block(set_header[i]);
        printf("-------------------\n");
    }
}

int main(int argc, char *argv[])
{
    int i = 0;
    int ch = EOF;
    int set_index_bits = 0, lines_per_set = 0, block_offset_bits = 0;
    unsigned int num_sets = 0;
    unsigned long long tag_mask, set_mask, block_mask;
    bool help_flag = false;
    bool verbose_flag = false;
    bool invalid_flag = false;
    char *trace_file_path;
    FILE *trace_file = NULL;
    Trace trace_entries[maxn];
    unsigned long long current_address = 0;
    int entries_count = 0;
    CacheBlock **set_headers;
    CacheBlock *set_header;
    const char *optstring = "hvs:E:b:t:";
    Counter result;

    while (~(ch = getopt(argc, argv, optstring)))
    {
        switch (ch)
        {
        case 'h':
            help_flag = true;
            // printf("get help!\n");
            break;
        case 'v':
            verbose_flag = true;
            // printf("verbose! \n");
            break;
        case 's':
            set_index_bits = atoi(optarg);
            // printf("s = %d\n", set_index_bits);
            break;
        case 'E':
            lines_per_set = atoi(optarg);
            // printf("E = %d\n", lines_per_set);
            break;
        case 'b':
            block_offset_bits = atoi(optarg);
            // printf("b = %d\n", block_offset_bits);
            break;
        case 't':
            trace_file_path = optarg;
            trace_file = fopen(trace_file_path, "r");
            // printf("t = %s\n", trace_file);
            break;
        default:
            invalid_flag = true;
            // printf("error\n");
            break;
        }
    }

    // Unknown options
    if (invalid_flag)
    {
        printf("invalid options\n");
        exit(EXIT_FAILURE);
    }

    // show help information
    if (help_flag)
    {
        printf("Show help information\n");
        exit(EXIT_SUCCESS);
    }

    // numbers must be greater than zero
    if ((set_index_bits <= 0) || (lines_per_set <= 0) || (block_offset_bits <= 0))
    {
        printf("Number error\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        // generate masks
        block_mask = (1ull << block_offset_bits) - 1;
        set_mask = ((1ull << set_index_bits) - 1) << block_offset_bits;
        tag_mask = (ULLONG_MAX ^ set_mask) ^ block_mask;
        num_sets = 1u << set_index_bits;

        // allocate spaces
        set_headers = (CacheBlock **)malloc(num_sets * sizeof(CacheBlock *));
        for (i = 0; i < num_sets; ++i)
        {
            set_headers[i] = (CacheBlock *)malloc(lines_per_set * sizeof(CacheBlock));
        }
        print_set(set_headers[0], lines_per_set);
    }

    // file must exist
    if (trace_file == NULL)
    {
        printf("File does not exist\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("File found: %s\n", trace_file_path);
        while (~(fscanf(trace_file, "%s %llx,%hd \n", trace_entries[entries_count].operation, &trace_entries[entries_count].address, &trace_entries[entries_count].size)))
        {
            current_address = trace_entries[entries_count].address;
            trace_entries[entries_count].index.tag_bits = (tag_mask & current_address) >> (set_index_bits + block_offset_bits);
            trace_entries[entries_count].index.set_index_bits = (set_mask & current_address) >> block_offset_bits;
            trace_entries[entries_count].index.block_offset_bits = block_mask & current_address;
            ++entries_count;
        }
    }

    printSummary(result.hits_count, result.misses_count, result.evictions_count);
    return 0;
}
