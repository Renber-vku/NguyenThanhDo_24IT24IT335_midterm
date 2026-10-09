#ifndef OPTIONS_H
#define OPTIONS_H

/* Time selection for sorting (-t) or printing (-l) */
typedef enum {
    TIME_MOD = 0,   /* default: modification time (st_mtime) */
    TIME_CHANGE,    /* -c: status change time (st_ctime)     */
    TIME_ACCESS     /* -u: access time (st_atime)            */
} TimeType;

/* Size reporting format for -s and -l */
typedef enum {
    SIZE_DEFAULT = 0, /* default: 512-byte blocks (or BLOCKSIZE env) */
    SIZE_KILOBYTES,   /* -k: 1024-byte (1KB) blocks                  */
    SIZE_HUMAN        /* -h: human-readable (B, K, M, G, T, P)       */
} SizeFormat;

/* Flags selected on the command line based on NetBSD ls(1) manual. */
typedef struct {
    int almost_all;      /* -A: list all except '.' and '..'                  */
    int all;             /* -a: include entries starting with '.'             */
    TimeType time_type;  /* -c / -u: ctime or atime (last one wins)           */
    int dir;             /* -d: list directory as plain file                  */
    int classify;        /* -F: append type indicator (/ * @ % = |)           */
    int nosort;          /* -f: output is not sorted (also sets -a)           */
    SizeFormat size_fmt; /* -h / -k: human-readable or kilobytes              */
    int inode;           /* -i: print inode number                            */
    int longfmt;         /* -l / -n: long listing format                      */
    int numeric;         /* -n: numeric UID/GID (overrides -l names)          */
    int nonprint_q;      /* -q / -w: replace non-printable with '?' (1 or 0)  */
    int recursive;       /* -R: recursively list subdirectories               */
    int reverse;         /* -r: reverse sort order                            */
    int by_size;         /* -S: sort by file size                             */
    int blocks;          /* -s: print allocated block count                   */
    int by_time;         /* -t: sort by time                                  */
    int blocksize;       /* effective block size in bytes (from BLOCKSIZE)    */
} Options;

/* Parses argv into opt according to NetBSD ls(1) manual.
 * Returns index of first operand, or -1 on error. */
int parse_options(int argc, char **argv, Options *opt);

#endif /* OPTIONS_H */