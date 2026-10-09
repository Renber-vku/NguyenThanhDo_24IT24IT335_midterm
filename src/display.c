#include <ctype.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#if defined(__linux__)
#include <sys/sysmacros.h>
#endif
#include <time.h>
#include <unistd.h>
#include <util.h>
#include "display.h"

#define SIX_MONTHS 15778476L

static int digits(unsigned long long n)
{
    int d = 1;
    while (n >= 10) {
        n /= 10;
        d++;
    }
    return d;
}

static void format_human(unsigned long long bytes, char *buf, size_t len)
{
    (void)len;
    humanize_number(buf, 5, (int64_t)bytes, "", HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);
}

/* Calculates block count for an entry based on -h, -k, and BLOCKSIZE */
static unsigned long long calc_blocks(const Entry *e, const Options *opt)
{
    unsigned long long raw_bytes = (unsigned long long)e->st.st_blocks * 512ULL;

    if (opt->size_fmt == SIZE_KILOBYTES)
        return (raw_bytes + 1023ULL) / 1024ULL;
    
    if (opt->blocksize > 0)
        return (raw_bytes + (unsigned long long)opt->blocksize - 1ULL) / (unsigned long long)opt->blocksize;

    return (unsigned long long)e->st.st_blocks;
}

static void format_blocks(const Entry *e, const Options *opt, char *buf, size_t len)
{
    if (opt->size_fmt == SIZE_HUMAN) {
        format_human((unsigned long long)e->st.st_blocks * 512ULL, buf, len);
    } else {
        snprintf(buf, len, "%llu", calc_blocks(e, opt));
    }
}

/* Formats size column for -l */
static void format_size(const Entry *e, const Options *opt, char *buf, size_t len)
{
    mode_t m = e->st.st_mode;

    if (S_ISCHR(m) || S_ISBLK(m)) {
        snprintf(buf, len, "%u, %u", major(e->st.st_rdev), minor(e->st.st_rdev));
    } else if (opt->size_fmt == SIZE_HUMAN) {
        format_human((unsigned long long)e->st.st_size, buf, len);
    } else {
        snprintf(buf, len, "%llu", (unsigned long long)e->st.st_size);
    }
}

/* Builds 10-character mode string (e.g. "drwxr-xr-x") */
static void mode_string(mode_t m, char *s)
{
    char t = '-';

    if (S_ISDIR(m))       t = 'd';
    else if (S_ISLNK(m))  t = 'l';
    else if (S_ISCHR(m))  t = 'c';
    else if (S_ISBLK(m))  t = 'b';
    else if (S_ISFIFO(m)) t = 'p';
    else if (S_ISSOCK(m)) t = 's';

    s[0] = t;
    s[1] = (m & S_IRUSR) ? 'r' : '-';
    s[2] = (m & S_IWUSR) ? 'w' : '-';
    s[3] = (m & S_ISUID) ? ((m & S_IXUSR) ? 's' : 'S') : ((m & S_IXUSR) ? 'x' : '-');
    s[4] = (m & S_IRGRP) ? 'r' : '-';
    s[5] = (m & S_IWGRP) ? 'w' : '-';
    s[6] = (m & S_ISGID) ? ((m & S_IXGRP) ? 's' : 'S') : ((m & S_IXGRP) ? 'x' : '-');
    s[7] = (m & S_IROTH) ? 'r' : '-';
    s[8] = (m & S_IWOTH) ? 'w' : '-';
    s[9] = (m & S_ISVTX) ? ((m & S_IXOTH) ? 't' : 'T') : ((m & S_IXOTH) ? 'x' : '-');
    s[10] = '\0';
}

static const char *user_name(uid_t uid, const Options *opt, char *buf, size_t len)
{
    if (!opt->numeric) {
        struct passwd *pw = getpwuid(uid);
        if (pw != NULL)
            return pw->pw_name;
    }
    snprintf(buf, len, "%u", (unsigned)uid);
    return buf;
}

static const char *group_name(gid_t gid, const Options *opt, char *buf, size_t len)
{
    if (!opt->numeric) {
        struct group *gr = getgrgid(gid);
        if (gr != NULL)
            return gr->gr_name;
    }
    snprintf(buf, len, "%u", (unsigned)gid);
    return buf;
}

/* Returns selected timestamp for display */
static time_t get_time(const Entry *e, const Options *opt)
{
    if (opt->time_type == TIME_CHANGE)
        return e->st.st_ctime;
    if (opt->time_type == TIME_ACCESS)
        return e->st.st_atime;
    return e->st.st_mtime;
}

static void print_time(time_t t)
{
    char buf[32];
    struct tm tm;
    time_t now = time(NULL);
    const char *fmt = (t > now || now - t > SIX_MONTHS) ? "%b %e  %Y"
                                                        : "%b %e %H:%M";

    localtime_r(&t, &tm);
    strftime(buf, sizeof buf, fmt, &tm);
    fputs(buf, stdout);
}

/* Prints file name, filtering unprintable characters if -q */
static void print_name(const char *name, const Options *opt)
{
    if (opt->nonprint_q) {
        while (*name != '\0') {
            unsigned char ch = (unsigned char)*name;
            if (isprint(ch))
                putchar(ch);
            else
                putchar('?');
            name++;
        }
    } else {
        fputs(name, stdout);
    }
}

/* Prints trailing type indicator for -F */
static void print_classifier(mode_t m, const Options *opt)
{
    if (!opt->classify)
        return;

    if (S_ISDIR(m))
        putchar('/');
    else if (S_ISLNK(m))
        putchar('@');
    else if (S_ISSOCK(m))
        putchar('=');
    else if (S_ISFIFO(m))
        putchar('|');
    else if (m & (S_IXUSR | S_IXGRP | S_IXOTH))
        putchar('*');
}

static void print_total_line(const Entry *items, size_t n, const Options *opt)
{
    unsigned long long total_raw_blocks = 0;
    size_t i;

    for (i = 0; i < n; i++)
        total_raw_blocks += (unsigned long long)items[i].st.st_blocks;

    if (opt->size_fmt == SIZE_HUMAN) {
        char buf[32];
        format_human(total_raw_blocks * 512ULL, buf, sizeof buf);
        printf("total %s\n", buf);
    } else if (opt->size_fmt == SIZE_KILOBYTES) {
        printf("total %llu\n", (total_raw_blocks * 512ULL + 1023ULL) / 1024ULL);
    } else if (opt->blocksize > 0) {
        unsigned long long total_bytes = total_raw_blocks * 512ULL;
        printf("total %llu\n", (total_bytes + (unsigned long long)opt->blocksize - 1ULL) / (unsigned long long)opt->blocksize);
    } else {
        printf("total %llu\n", total_raw_blocks);
    }
}

static void print_long(const Entry *items, size_t n, const Options *opt, int with_total)
{
    int wi = 1, wb = 1, wl = 1, wu = 1, wg = 1, ws = 1;
    char ub[32], gb[32], sb[64], bb[32], mode[11];
    size_t i;

    if (with_total)
        print_total_line(items, n, opt);

    /* Calculate alignment widths */
    for (i = 0; i < n; i++) {
        const Entry *e = &items[i];
        int w;

        if (opt->inode) {
            w = digits(e->st.st_ino);
            if (w > wi) wi = w;
        }
        if (opt->blocks) {
            format_blocks(e, opt, bb, sizeof bb);
            w = (int)strlen(bb);
            if (w > wb) wb = w;
        }
        w = digits(e->st.st_nlink);
        if (w > wl) wl = w;

        w = (int)strlen(user_name(e->st.st_uid, opt, ub, sizeof ub));
        if (w > wu) wu = w;

        w = (int)strlen(group_name(e->st.st_gid, opt, gb, sizeof gb));
        if (w > wg) wg = w;

        format_size(e, opt, sb, sizeof sb);
        w = (int)strlen(sb);
        if (w > ws) ws = w;
    }

    for (i = 0; i < n; i++) {
        const Entry *e = &items[i];

        if (opt->inode)
            printf("%*lu ", wi, (unsigned long)e->st.st_ino);
        if (opt->blocks) {
            format_blocks(e, opt, bb, sizeof bb);
            printf("%*s ", wb, bb);
        }

        mode_string(e->st.st_mode, mode);
        format_size(e, opt, sb, sizeof sb);

        printf("%s  %*lu %-*s  %-*s  %*s ", mode,
               wl, (unsigned long)e->st.st_nlink,
               wu, user_name(e->st.st_uid, opt, ub, sizeof ub),
               wg, group_name(e->st.st_gid, opt, gb, sizeof gb),
               ws, sb);

        print_time(get_time(e, opt));
        printf(" ");
        print_name(e->name, opt);
        print_classifier(e->st.st_mode, opt);

        if (S_ISLNK(e->st.st_mode)) {
            char target[4096];
            ssize_t len = readlink(e->path, target, sizeof target - 1);
            if (len >= 0) {
                target[len] = '\0';
                printf(" -> ");
                print_name(target, opt);
            }
        }
        putchar('\n');
    }
}

static void print_short(const Entry *items, size_t n, const Options *opt, int with_total)
{
    int wi = 1, wb = 1;
    char bb[32];
    size_t i;

    /* If -s is given and output is to terminal, print total sum first */
    if (opt->blocks && isatty(STDOUT_FILENO) && with_total)
        print_total_line(items, n, opt);

    for (i = 0; i < n; i++) {
        const Entry *e = &items[i];
        int w;
        if (opt->inode) {
            w = digits(e->st.st_ino);
            if (w > wi) wi = w;
        }
        if (opt->blocks) {
            format_blocks(e, opt, bb, sizeof bb);
            w = (int)strlen(bb);
            if (w > wb) wb = w;
        }
    }

    for (i = 0; i < n; i++) {
        const Entry *e = &items[i];

        if (opt->inode)
            printf("%*lu ", wi, (unsigned long)e->st.st_ino);
        if (opt->blocks) {
            format_blocks(e, opt, bb, sizeof bb);
            printf("%*s ", wb, bb);
        }

        print_name(e->name, opt);
        print_classifier(e->st.st_mode, opt);
        putchar('\n');
    }
}

void print_entries(const Entry *items, size_t n, const Options *opt, int with_total)
{
    if (opt->longfmt)
        print_long(items, n, opt, with_total);
    else if (n > 0 || (opt->blocks && isatty(STDOUT_FILENO) && with_total))
        print_short(items, n, opt, with_total);
}