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

struct CacheLine
{
    unsigned long long tag_index;
    struct CacheLine *next;
    struct CacheLine *prev;
};
typedef struct CacheLine CacheLine;

typedef struct
{
    CacheLine *head;
    int length;
} CacheSet;

// Max number of trace entries
#define ENTRIES 65536

// Count results
int hit_count = 0, miss_count = 0, eviction_count = 0;

// Cache architecture
int set_index_bits = 0, lines_per_set = 0, block_offset_bits = 0;
unsigned long long tag_mask, set_mask, block_mask;
unsigned int num_sets = 0;
CacheSet *cache_sets;

// Optional flags
bool help_flag = false;
bool verbose_flag = false;
bool invalid_flag = false;
const char *optstring = "hvs:E:b:t:";

// File parameters
char *trace_file_path;
FILE *trace_file = NULL;
Trace trace_entries[ENTRIES];
int entries_count = 0;

void show_set(const CacheSet *cache_set)
{
    CacheLine *line_head = cache_set->head;
    printf("------------------------------------------------------\n");
    printf("Set length = %d\n", cache_set->length);
    while (line_head != NULL)
    {
        printf("--> %llu ", line_head->tag_index);
        line_head = line_head->next;
    }
    printf("\n");
}

void show_command(const Trace *trace_entry)
{
    printf("type:%c, set:%llu, tag:%llu\n",
           trace_entry->operation[0], trace_entry->index.set_index, trace_entry->index.tag_index);

    printf("results: %s\n", trace_entry->operation_results);
}

CacheLine *create_new_line(unsigned long long tag_index)
{
    CacheLine *new_line = (CacheLine *)malloc(sizeof(CacheLine));
    if (new_line == NULL)
    {
        printf("Out of memory");
        exit(EXIT_FAILURE);
    }
    new_line->prev = NULL;
    new_line->next = NULL;
    new_line->tag_index = tag_index;
    return new_line;
}

CacheLine *move_to_head(CacheSet *cache_set, CacheLine *line_head)
{
    if (cache_set->length == 1)
    {
        return line_head;
    }

    // Move line_head to head, at least 2 elements
    CacheLine *cache_set_head = cache_set->head;
    CacheLine *line_prev = line_head->prev;
    CacheLine *line_next = line_head->next;

    cache_set->head = line_head;
    cache_set_head->prev = line_head;
    line_head->prev = NULL;
    line_head->next = cache_set_head;

    if (line_prev != NULL)
        line_prev->next = line_next;
    if (line_next != NULL)
        line_next->prev = line_prev;

    return line_head;
}

CacheLine *pop_line(CacheSet *cache_set)
{
    CacheLine *line_head = cache_set->head;
    if (line_head == NULL)
    {
        printf("Empty cache!\n");
        exit(EXIT_FAILURE);
    }

    // Find the last element
    while (line_head->next != NULL)
    {
        line_head = line_head->next;
    }

    if (line_head->prev != NULL)
    {
        line_head->prev->next = NULL;
    }
    else
    {
        cache_set->head = NULL;
    }
    cache_set->length -= 1;
    return line_head;
}

CacheLine *prepend_line(CacheSet *cache_set, unsigned long long tag_index)
{
    CacheLine *new_line = create_new_line(tag_index);
    cache_set->head = new_line;
    cache_set->length += 1;
    return new_line;
}

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
    return;
}

void execute_data_load(CacheSet *cache_sets, Trace *trace_entry)
{
    unsigned long long set_index = trace_entry->index.set_index;
    unsigned long long tag_index = trace_entry->index.tag_index;
    CacheSet *cache_set = cache_sets + set_index;
    CacheLine *line_head = cache_set->head;

    // Try to find by tag_index
    while ((line_head != NULL) && (line_head->tag_index != tag_index))
    {
        line_head = line_head->next;
    }

    // Found the cached line
    if (line_head != NULL)
    {
        // Record a hit
        record_result(trace_entry, 'h');
        move_to_head(cache_set, line_head);
    }
    else
    {
        // Record a miss
        record_result(trace_entry, 'm');

        // Write to cache
        if (cache_set->length == lines_per_set)
        {
            pop_line(cache_set);
            prepend_line(cache_set, tag_index);
            record_result(trace_entry, 'e');
        }
        else
        {
            prepend_line(cache_set, tag_index);
        }
    }
    return;
}

void execute_data_store(CacheSet *cache_sets, Trace *trace_entry)
{
    execute_data_load(cache_sets, trace_entry);
}

void execute_command(CacheSet *cache_set_headers, Trace *trace_entry)
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
        break; // Do nothing
    default:
        printf("Invalid operation: '%c'.\n", operation);
        exit(EXIT_FAILURE);
    }
}

void read_traces()
{
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

void initialize_cache()
{

    // Generate masks
    block_mask = (1ull << block_offset_bits) - 1;
    set_mask = ((1ull << set_index_bits) - 1) << block_offset_bits;
    tag_mask = (ULLONG_MAX ^ set_mask) ^ block_mask;
    num_sets = 1u << set_index_bits;

    // Allocate spaces for cache
    cache_sets = (CacheSet *)malloc(num_sets * sizeof(CacheSet));
    if (cache_sets == NULL)
    {
        printf("Out of memory\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < num_sets; ++i)
    {
        // Set to default value
        cache_sets[i].head = NULL;
        cache_sets[i].length = 0;
    }
}

void parse_options(int argc, char *argv[])
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
}

void load_options()
{
    // Unknown options
    if (invalid_flag)
    {
        printf("invalid options\n");
        exit(EXIT_FAILURE);
    }

    // Show help information
    if (help_flag)
    {
        printf("Show help information\n");
        exit(EXIT_SUCCESS);
    }

    // Numbers must be greater than zero
    if ((set_index_bits <= 0) || (lines_per_set <= 0) || (block_offset_bits <= 0))
    {
        printf("Number error\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        initialize_cache();
    }

    // File must exist
    if (trace_file == NULL)
    {
        printf("File does not exist\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("File found: %s\n", trace_file_path);
        read_traces();
    }
}

int main(int argc, char *argv[])
{
    parse_options(argc, argv);
    load_options();

    for (int i = 0; i < entries_count; ++i)
    {
        execute_command(cache_sets, trace_entries + i);
        // show_set(cache_sets + i);
        // show_command(trace_entries + i);
    }

    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
