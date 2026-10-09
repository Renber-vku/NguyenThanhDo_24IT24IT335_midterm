#ifndef ENTRY_H
#define ENTRY_H

#include <stddef.h>
#include <sys/stat.h>

/* One file to be listed: display name, full path, and its stat data. */
typedef struct {
    char *name;
    char *path;
    struct stat st;
} Entry;

/* Dynamic array of entries. */
typedef struct {
    Entry *items;
    size_t count;
    size_t cap;
} EntryList;

void entry_list_init(EntryList *l);

/*
 * Adds dir/name (or just name if dir is NULL) to the list.
 * follow != 0 uses stat() (falls back to lstat() for dangling links),
 * otherwise lstat(). Returns 0 on success, -1 on error (message printed).
 */
int entry_list_add(EntryList *l, const char *dir, const char *name, int follow);

void entry_list_free(EntryList *l);

#endif