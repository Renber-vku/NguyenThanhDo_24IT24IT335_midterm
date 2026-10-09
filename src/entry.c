#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "entry.h"

void entry_list_init(EntryList *l)
{
    l->items = NULL;
    l->count = 0;
    l->cap = 0;
}

/* Builds "dir/name", avoiding a doubled slash. Caller frees the result. */
static char *join_path(const char *dir, const char *name)
{
    size_t dl, n;
    char *p;

    if (dir == NULL)
        return strdup(name);

    dl = strlen(dir);
    n = dl + strlen(name) + 2;
    p = malloc(n);
    if (p != NULL)
        snprintf(p, n, "%s%s%s", dir,
                 (dl > 0 && dir[dl - 1] != '/') ? "/" : "", name);
    return p;
}

int entry_list_add(EntryList *l, const char *dir, const char *name, int follow)
{
    struct stat st;
    char *path = join_path(dir, name);
    char *copy;
    Entry *e;
    int rc;

    if (path == NULL) {
        perror("ls");
        return -1;
    }

    rc = follow ? stat(path, &st) : lstat(path, &st);
    if (rc != 0 && follow)
        rc = lstat(path, &st);        /* dangling symlink: show the link */
    if (rc != 0) {
        fprintf(stderr, "ls: cannot access '%s': %s\n", path, strerror(errno));
        free(path);
        return -1;
    }

    if (l->count == l->cap) {
        size_t ncap = l->cap ? l->cap * 2 : 16;
        Entry *tmp = realloc(l->items, ncap * sizeof *tmp);
        if (tmp == NULL) {
            perror("ls");
            free(path);
            return -1;
        }
        l->items = tmp;
        l->cap = ncap;
    }

    copy = strdup(name);
    if (copy == NULL) {
        perror("ls");
        free(path);
        return -1;
    }

    e = &l->items[l->count++];
    e->name = copy;
    e->path = path;
    e->st = st;
    return 0;
}

void entry_list_free(EntryList *l)
{
    size_t i;

    for (i = 0; i < l->count; i++) {
        free(l->items[i].name);
        free(l->items[i].path);
    }
    free(l->items);
    entry_list_init(l);
}