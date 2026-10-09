#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "display.h"
#include "entry.h"
#include "list.h"
#include "sort.h"

int list_directory(const char *path, const Options *opt, int show_header, int *sections)
{
    DIR *d = opendir(path);
    struct dirent *de;
    EntryList list;
    int status = 0;
    size_t i;

    if (d == NULL) {
        fprintf(stderr, "ls: %s: %s\n", path, strerror(errno));
        return -1;
    }

    entry_list_init(&list);
    while ((de = readdir(d)) != NULL) {
        if (!opt->all) {
            if (opt->almost_all) {
                if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
                    continue;
            } else if (de->d_name[0] == '.') {
                continue;
            }
        }

        if (entry_list_add(&list, path, de->d_name, 0) < 0)
            status = -1;
    }
    closedir(d); /* close early so deep recursion doesn't exhaust file descriptors */

    sort_entries(list.items, list.count, opt);

    if (*sections > 0)
        putchar('\n');
    if (show_header)
        printf("%s:\n", path);
    (*sections)++;

    print_entries(list.items, list.count, opt, 1);

    if (opt->recursive) {
        for (i = 0; i < list.count; i++) {
            const Entry *e = &list.items[i];
            /* lstat data: symlinks are not followed to prevent loops */
            if (S_ISDIR(e->st.st_mode) && strcmp(e->name, ".") != 0
                && strcmp(e->name, "..") != 0) {
                if (list_directory(e->path, opt, 1, sections) < 0)
                    status = -1;
            }
        }
    }

    entry_list_free(&list);
    return status;
}