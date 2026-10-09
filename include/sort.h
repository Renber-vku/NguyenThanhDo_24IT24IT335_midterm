#ifndef SORT_H
#define SORT_H

#include <stddef.h>
#include "entry.h"
#include "options.h"

/*
 * Sorts an array of Entry objects in-place according to the flags in opt.
 * - Default: lexicographical order by name (strcmp)
 * - opt->by_size (-S): file size descending (largest first)
 * - opt->by_time (-t): modification time descending (newest first)
 * - opt->reverse (-r): reverses the resulting sort order
 * Ties in size/time are broken using entry names.
 */
void sort_entries(Entry *items, size_t n, const Options *opt);

#endif /* SORT_H */
