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
    short result_count;
    char operation_results[4];
    char operation[2];
} Trace;

typedef struct
{
    unsigned long long tag_index;
    CacheLine *next;
} CacheLine;

typedef struct
{
    CacheLine *head;
    int length;
} CacheSetHeader;

// max number of trace entries
#define ENTRIES 65536

// Count results
int hit_count = 0, miss_count = 0, eviction_count = 0;

// Cache architecture
int set_index_bits = 0, lines_per_set = 0, block_offset_bits = 0;
unsigned long long tag_mask, set_mask, block_mask;
unsigned int num_sets = 0;
CacheSetHeader *cache_set_headers;

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

void record_result(Trace *trace_entry, char result_type)
{
    trace_entry->operation_results[trace_entry->result_count] = result_type;
    trace_entry->result_count += 1;
    switch (result_type)
    {
    case 'h':
        hit_count += 1;
        break;
    case 'e':
        eviction_count += 1;
        break;
    case 'm':
        miss_count += 1;
        break;
    default:
        printf("Invalid result type: %c\n", result_type);
        exit(EXIT_FAILURE);
        break;
    }
}

void execute_data_load(CacheSetHeader *cache_set_headers, Trace *trace_entry)
{
    unsigned long long set_index = trace_entry->index.set_index;
    unsigned long long tag_index = trace_entry->index.tag_index;
    CacheSetHeader cache_set_header = cache_set_headers[set_index];
    CacheLine *current_line_ptr = cache_set_header.head;
    while (current_line_ptr != NULL)
    {
        if (current_line_ptr->tag_index == tag_index)
        {
            record_result(trace_entry, 'h');
            return;
        }
        current_line_ptr = current_line_ptr->next;
    }

    // no hit, then record a miss
    record_result(trace_entry, 'm');
}

void execute_data_store(CacheSetHeader *cache_set_headers, Trace *trace_entry)
{
    return;
}

void execute_command(CacheSetHeader *cache_set_headers, Trace *trace_entry)
{
    char operation = trace_entry->operation[0];
    switch (operation)
    {
    case 'L':
        execute_data_load(cache_set_headers, trace_entry);
        break;
    case 'S':
        execute_data_store(cache_set_headers, trace_entry);
        break;
    case 'M':
        execute_data_load(cache_set_headers, trace_entry);
        execute_data_store(cache_set_headers, trace_entry);
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
            break;
        case 'v':
            verbose_flag = true;
            break;
        case 's':
            set_index_bits = atoi(optarg);
            break;
        case 'E':
            lines_per_set = atoi(optarg);
            break;
        case 'b':
            block_offset_bits = atoi(optarg);
            break;
        case 't':
            trace_file_path = optarg;
            trace_file = fopen(trace_file_path, "r");
            break;
        default:
            invalid_flag = true;
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

        // allocate spaces for cache
        cache_set_headers = (CacheSetHeader *)malloc(num_sets * sizeof(CacheSetHeader));
        if (cache_set_headers == NULL)
        {
            printf("Out of memory\n");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < num_sets; ++i)
        {
            cache_set_headers[i].head = NULL;
            cache_set_headers[i].length = 0;
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
            trace_entries[entries_count].result_count = 0;
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
        execute_command(cache_set_headers, trace_entries + i);
    }

    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
