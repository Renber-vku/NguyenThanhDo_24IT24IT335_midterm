#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "sort.h"

/* Pointer to active options during qsort */
static const Options *current_opt = NULL;

static time_t get_selected_time(const Entry *e, long *nsec)
{
    time_t sec = e->st.st_mtime;
    *nsec = 0;

    if (current_opt->time_type == TIME_CHANGE) {
        sec = e->st.st_ctime;
#if defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE >= 200809L) || defined(_GNU_SOURCE)
        *nsec = e->st.st_ctim.tv_nsec;
#endif
    } else if (current_opt->time_type == TIME_ACCESS) {
        sec = e->st.st_atime;
#if defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE >= 200809L) || defined(_GNU_SOURCE)
        *nsec = e->st.st_atim.tv_nsec;
#endif
    } else {
        sec = e->st.st_mtime;
#if defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE >= 200809L) || defined(_GNU_SOURCE)
        *nsec = e->st.st_mtim.tv_nsec;
#endif
    }

    return sec;
}

static int compare_entries(const void *p1, const void *p2)
{
    const Entry *e1 = (const Entry *)p1;
    const Entry *e2 = (const Entry *)p2;
    int diff = 0;

    if (current_opt->by_size) {
        if (e1->st.st_size < e2->st.st_size)
            diff = 1;
        else if (e1->st.st_size > e2->st.st_size)
            diff = -1;
        else
            diff = strcmp(e1->name, e2->name);
    } else if (current_opt->by_time) {
        long ns1 = 0, ns2 = 0;
        time_t t1 = get_selected_time(e1, &ns1);
        time_t t2 = get_selected_time(e2, &ns2);

        if (t1 < t2) {
            diff = 1;
        } else if (t1 > t2) {
            diff = -1;
        } else {
            if (ns1 < ns2)
                diff = 1;
            else if (ns1 > ns2)
                diff = -1;
            else
                diff = strcmp(e1->name, e2->name);
        }
    } else {
        diff = strcmp(e1->name, e2->name);
    }

    if (current_opt->reverse)
        diff = -diff;

    return diff;
}

void sort_entries(Entry *items, size_t n, const Options *opt)
{
    if (items == NULL || n < 2 || opt == NULL)
        return;

    /* -f option: output is not sorted */
    if (opt->nosort)
        return;

    current_opt = opt;
    qsort(items, n, sizeof(Entry), compare_entries);
    current_opt = NULL;
}
