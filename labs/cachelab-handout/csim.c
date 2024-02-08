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

int main(int argc, char *argv[])
{
    int ch;
    int set_index_bits, lines_per_set, block_offset_bits;
    bool help_flag = false;
    bool verbose_flag = false;
    bool invalid_flag = false;
    char *trace_file;
    const char *optstring = "hvs:E:b:t:";

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
            trace_file = optarg;
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
    if ((set_index_bits < 0) || (lines_per_set < 0) || (block_offset_bits < 0))
    {
        printf("Number error\n");
        exit(EXIT_FAILURE);
    }

    printSummary(0, 0, 0);
    return 0;
}
