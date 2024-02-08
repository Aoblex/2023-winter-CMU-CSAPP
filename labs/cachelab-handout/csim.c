#include "cachelab.h"
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

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
    size_t address;
    short size;
    char instruction_bit;
    char data_bit;
} Trace;

int main(int argc, char *argv[])
{
    int i = 0;
    int ch = EOF;
    int entries_count = 0;
    int set_index_bits = 0, lines_per_set = 0, block_offset_bits = 0;
    bool help_flag = false;
    bool verbose_flag = false;
    bool invalid_flag = false;
    FILE *trace_file = NULL;
    char line[16];
    const char *optstring = "hvs:E:b:t:";
    Trace trace_entries[maxn];

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
            trace_file = fopen(optarg, "r");
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

    // file must exist
    if (trace_file == NULL)
    {
        printf("File does not exist\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("File found\n");
        while (~(fscanf(trace_file, "%c%c %zu,%hd \n", &trace_entries[entries_count].instruction_bit, &trace_entries[entries_count].data_bit, &trace_entries[entries_count].address, &trace_entries[entries_count].size)))
        {
            printf("ins=%c, data=%c, address=%zu, size=%hd\n", trace_entries[entries_count].instruction_bit, trace_entries[entries_count].data_bit, trace_entries[entries_count].address, trace_entries[entries_count].size);
            ++entries_count;
        }
        for (i = 0; i < entries_count; ++i)
        {
            // printf("ins=%c, data=%c, address=%zu, size=%hd\n", trace_entries[i].instruction_bit, trace_entries[i].data_bit, trace_entries[i].address, trace_entries[i].size);
        }
    }

    printSummary(0, 0, 0);
    return 0;
}
