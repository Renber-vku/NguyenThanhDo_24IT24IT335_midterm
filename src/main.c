#include <stdio.h>
#include <stdlib.h>
#include "display.h"
#include "entry.h"
#include "list.h"
#include "options.h"
#include "sort.h"

/*
 * Operands: non-directories are printed first, then each directory's contents.
 * Directory and non-directory operands are sorted separately.
 * Exit status: 0 on success, >0 if an error occurs.
 */
int main(int argc, char **argv)
{
    Options opt;
    EntryList args;
    Entry *files = NULL;
    size_t i, nfiles = 0;
    int idx, nargs, status = 0, sections = 0, header;

    idx = parse_options(argc, argv, &opt);
    if (idx < 0)
        return 1;

    entry_list_init(&args);
    nargs = argc - idx;

    if (nargs == 0) {
        if (entry_list_add(&args, NULL, ".", !opt.longfmt && !opt.dir) < 0)
            status = 1;
    } else {
        for (i = (size_t)idx; i < (size_t)argc; i++) {
            if (entry_list_add(&args, NULL, argv[i], !opt.longfmt && !opt.dir) < 0)
                status = 1;
        }
    }

    sort_entries(args.items, args.count, &opt);

    if (args.count > 0) {
        files = malloc(args.count * sizeof *files);
        if (files == NULL) {
            perror("ls");
            entry_list_free(&args);
            return 1;
        }

        for (i = 0; i < args.count; i++) {
            if (opt.dir || !S_ISDIR(args.items[i].st.st_mode))
                files[nfiles++] = args.items[i];
        }
    }

    if (nfiles > 0) {
        print_entries(files, nfiles, &opt, 0);
        sections = 1;
    }
    free(files);

    header = (nargs > 1);

    if (!opt.dir) {
        for (i = 0; i < args.count; i++) {
            if (S_ISDIR(args.items[i].st.st_mode)) {
                if (list_directory(args.items[i].path, &opt, header, &sections) < 0)
                    status = 1;
            }
        }
    }

    entry_list_free(&args);
    return status;
}