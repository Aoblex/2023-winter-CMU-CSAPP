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

typedef struct
{
    unsigned long long tag_index;
    unsigned long long set_index;
    unsigned long long block_index;
} Address;

typedef struct
{
    Address index;
    unsigned long long address;
    short size;
    char operation_results[4];
    char operation[2];
} Trace;

typedef struct
{
    unsigned long long tag_bits;
    int use_count;
    bool v;
} CacheBlock;

// max number of trace entries
#define ENTRIES 65536

// Count results
int hit_count = 0, miss_count = 0, eviction_count = 0;

// Cache architecture
int set_index_bits = 0, lines_per_set = 0, block_offset_bits = 0;
unsigned long long tag_mask, set_mask, block_mask;
unsigned int num_sets = 0;
CacheBlock **set_headers;

// optional flags
bool help_flag = false;
bool verbose_flag = false;
bool invalid_flag = false;
const char *optstring = "hvs:E:b:t:";

// file parameters
char *trace_file_path;
FILE *trace_file = NULL;
Trace trace_entries[ENTRIES];
int entries_count = 0;

void execute_data_load(const Trace *trace_entry)
{
    unsigned long long set_index = trace_entry->index.set_index;
    CacheBlock *set_header = set_headers[set_index];
    printf("Current data load operation set index: %llx\n", set_index);
}

void execute_data_store(const Trace *trace_entry)
{
}

void execute_command(const Trace *trace_entry)
{
    char operation = trace_entry->operation[0];
    switch (operation)
    {
    case 'L':
        execute_data_load(trace_entry);
        break;
    case 'S':
        execute_data_store(trace_entry);
        break;
    case 'M':
        execute_data_load(trace_entry);
        execute_data_store(trace_entry);
        break;
    case 'I':
        break; // do nothing.
    default:
        printf("Invalid operation: '%c'.\n", operation);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    int ch = EOF;
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
        for (int i = 0; i < num_sets; ++i)
        {
            set_headers[i] = (CacheBlock *)malloc(lines_per_set * sizeof(CacheBlock));
            for (int j = 0; j < lines_per_set; ++j)
            {
                // initialize valid flag
                set_headers[i][j].v = false;
                set_headers[i][j].use_count = 0;
            }
        }
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
            unsigned long long current_address = trace_entries[entries_count].address;
            trace_entries[entries_count].index.tag_index = (tag_mask & current_address) >> (set_index_bits + block_offset_bits);
            trace_entries[entries_count].index.set_index = (set_mask & current_address) >> block_offset_bits;
            trace_entries[entries_count].index.block_index = block_mask & current_address;
            for (int i = 0; i < 4; ++i)
            {
                trace_entries[entries_count].operation_results[i] = '\0';
            }
            ++entries_count;
        }
        fclose(trace_file);
    }

    for (int i = 0; i < entries_count; ++i)
    {
        execute_command(trace_entries + i);
    }

    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
