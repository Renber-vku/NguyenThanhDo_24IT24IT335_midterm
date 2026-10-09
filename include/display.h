#ifndef DISPLAY_H
#define DISPLAY_H

#include <stddef.h>
#include "entry.h"
#include "options.h"

/* Prints entries in short or long format. with_total adds the "total" line. */
void print_entries(const Entry *items, size_t n, const Options *opt,
                   int with_total);

#endif