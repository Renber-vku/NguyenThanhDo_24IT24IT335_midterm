#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "options.h"

int parse_options(int argc, char **argv, Options *opt)
{
    int c;
    char *bs_env;

    memset(opt, 0, sizeof *opt);

    /* Default non-printable handling: '?' if terminal, raw if not */
    opt->nonprint_q = isatty(STDOUT_FILENO) ? 1 : 0;

    /* Default block size is 512 bytes unless BLOCKSIZE env is set */
    opt->blocksize = 512;
    bs_env = getenv("BLOCKSIZE");
    if (bs_env != NULL && *bs_env != '\0') {
        int bs = atoi(bs_env);
        if (bs > 0)
            opt->blocksize = bs;
    }

    /* Super-user default: -A is always set for root unless -a is given */
    if (geteuid() == 0)
        opt->almost_all = 1;

    opterr = 0; /* handle our own error messages */

    while ((c = getopt(argc, argv, "1AacdFfhiklnqRrSstuw")) != -1) {
        switch (c) {
        case '1':
            /* -1: force single column output (already default in short mode) */
            break;
        case 'A':
            opt->almost_all = 1;
            break;
        case 'a':
            opt->all = 1;
            break;
        case 'c':
            /* -c and -u override each other */
            opt->time_type = TIME_CHANGE;
            break;
        case 'd':
            /* -R and -d override each other */
            opt->dir = 1;
            opt->recursive = 0;
            break;
        case 'F':
            opt->classify = 1;
            break;
        case 'f':
            opt->nosort = 1;
            opt->all = 1; /* -f in BSD ls forces -a and turns off sort */
            break;
        case 'h':
            /* -k and -h override each other */
            opt->size_fmt = SIZE_HUMAN;
            break;
        case 'i':
            opt->inode = 1;
            break;
        case 'k':
            /* -k and -h override each other */
            opt->size_fmt = SIZE_KILOBYTES;
            break;
        case 'l':
            /* -l and -n override each other */
            opt->longfmt = 1;
            opt->numeric = 0;
            break;
        case 'n':
            /* -l and -n override each other */
            opt->longfmt = 1;
            opt->numeric = 1;
            break;
        case 'q':
            /* -w and -q override each other */
            opt->nonprint_q = 1;
            break;
        case 'R':
            /* -R and -d override each other */
            opt->recursive = 1;
            opt->dir = 0;
            break;
        case 'r':
            opt->reverse = 1;
            break;
        case 'S':
            opt->by_size = 1;
            break;
        case 's':
            opt->blocks = 1;
            break;
        case 't':
            opt->by_time = 1;
            break;
        case 'u':
            /* -c and -u override each other */
            opt->time_type = TIME_ACCESS;
            break;
        case 'w':
            /* -w and -q override each other */
            opt->nonprint_q = 0;
            break;
        default:
            fprintf(stderr, "ls: invalid option -- '%c'\n"
                            "Usage: ls [-AacdFfhiklnqRrSstuw] [file ...]\n", optopt);
            return -1;
        }
    }

    return optind;
}