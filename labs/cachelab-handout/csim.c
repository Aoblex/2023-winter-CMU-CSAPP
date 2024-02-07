#include "cachelab.h"
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

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

int main(int argc, char* argv[]){
    int ch;
    int set_index_bits, lines_per_set, block_offset_bits;
    char* trace_file;
    const char* optstring = "hvs:E:b:t:";

    while(~(ch = getopt(argc, argv, optstring))){
        switch(ch){
            case 'h':
                break;
            case 'v':
                break;
            case 's':
                break;
            case 'E':
                break;
            case 'b':
                break;
            case 't':
                break;
            default:
                break;
        }
    }

    printSummary(0, 0, 0);
    return 0;
}
