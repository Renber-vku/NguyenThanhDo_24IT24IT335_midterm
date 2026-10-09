#ifndef LIST_H
#define LIST_H

#include "options.h"

/*
 * Lists directory `path`. show_header prints "path:" first; *sections counts
 * blocks already printed so a blank line separates them. With -R it recurses.
 * Returns 0 on success, -1 if some part could not be read.
 */
int list_directory(const char *path, const Options *opt, int show_header,
                   int *sections);

#endif