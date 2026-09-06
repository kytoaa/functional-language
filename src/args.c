#include <stdio.h>
#include <unistd.h>

#include "args.h"

bool parse_args(int argc, char *const argv[], struct Args *out)
{
    char arg = 0;

    struct Args args = {};

    while ((arg = getopt(argc, argv, "b:")) != -1) {
        switch (arg) {
            case 'b':
                args.bin_output = optarg;
                break;
            default:
                fprintf(stderr, "invalid argument: '-%c'\n", arg);
                return false;
        }
    }

    args.file_name = argv[optind];

    *out = args;

    return true;
}
