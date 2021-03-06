
/*
 * Copyright (C) 2011 Palmer Dabbelt
 *   <palmer@dabbelt.com>
 *
 * This file is part of tek.
 * 
 * tek is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * tek is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with tek.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "imagemagick.h"

#include <string.h>
#include <stdbool.h>

#ifdef HAVE_TALLOC
#include <talloc.h>
#else
#include "extern/talloc.h"
#endif

static bool string_ends_with(const char *string, const char *end);
static int basename_len(const char *string);
static int string_index(const char *a, const char *b);

static void process(struct processor *p_uncast, const char *filename,
                    struct stack *s, struct makefile *m);

static char *p_name;

void processor_imagemagick_boot(void *context)
{
    p_name = talloc_strdup(context, "CONVERT");
}

struct processor *processor_imagemagick_search(void *context,
                                               const char *filename)
{
    struct processor_imagemagick *p;

    p = NULL;

    if (string_ends_with(filename, ".png.pdf"))
        p = talloc(context, struct processor_imagemagick);
    if (string_ends_with(filename, ".jpeg.pdf"))
        p = talloc(context, struct processor_imagemagick);
    if (string_ends_with(filename, ".svg.pdf"))
        p = talloc(context, struct processor_imagemagick);

    if (p != NULL) {
        p->p.name = talloc_reference(p, p_name);
        p->p.process = &process;

        p->crop = false;
        p->inkscape = false;

        if (string_ends_with(filename, ".uncrop.png.pdf"))
            p->crop = true;
        if (string_ends_with(filename, ".uncrop.jpeg.pdf"))
            p->crop = true;
        if (string_ends_with(filename, ".uncrop.svg.pdf"))
            p->crop = true;

        if (string_ends_with(filename, ".inkscape.svg.pdf"))
            p->inkscape = true;
    }

    return (struct processor *)p;
}

bool string_ends_with(const char *string, const char *end)
{
    return strcmp(string + strlen(string) - strlen(end), end) == 0;
}

int basename_len(const char *string)
{
    int i;

    for (i = strlen(string) - 1; i >= 0; i--) {
        if (string[i] == '/')
            return i;
    }

    return 0;
}

int string_index(const char *a, const char *b)
{
    size_t i;

    for (i = 0; i < strlen(a); i++) {
        if (strncmp(a + i, b, strlen(b)) == 0)
            return i;
    }

    return -1;
}

void process(struct processor *p_uncast, const char *filename,
             struct stack *s, struct makefile *m)
{
    struct processor_imagemagick *p;
    void *c;
    int cachedir_index;
    char *cachedir;
    char *infile;
    char *cachename;
    char *outname;

    /* We need access to the real structure, get it safely */
    p = talloc_get_type(p_uncast, struct processor_imagemagick);

    /* Makes a new context */
    c = talloc_new(p);

    /* Finds the original filename */
    cachedir_index = string_index(filename, ".tek_cache/");
    if (cachedir_index == -1) {
        fprintf(stderr, "Bad cachedir for image\n");
        return;
    }

    cachedir = talloc_strdup(c, filename);
    cachedir[cachedir_index + strlen(".tex_cache/")] = '\0';

    infile = talloc_strdup(c, filename);
    infile[cachedir_index] = '\0';
    strcat(infile, filename + strlen(cachedir));
    infile[strlen(infile) - 4] = '\0';

    TALLOC_FREE(cachedir);
    cachedir = talloc_strndup(c, filename, basename_len(filename));

    cachename = talloc_strndup(c, filename, strlen(filename) - 4);

    /* Checks if we should crop the file, if so crop the file by
     * passing it along to pdfcrop. */
    outname = talloc_strdup(c, filename);
    if (p->crop) {
        outname = talloc_asprintf(c, "%s-tocrop.pdf", filename);
    }

    /* Creates the target to build the image */
    makefile_create_target(m, outname);
    makefile_start_deps(m);
    makefile_add_dep(m, cachename);
    makefile_end_deps(m);

    makefile_start_cmds(m);
    if (p->inkscape == false) {
        makefile_nam_cmd(m, "echo -e \"CONVERT\\t%s\"", infile);
        makefile_add_cmd(m, "mkdir -p \"%s\" >& /dev/null || true", cachedir);
        makefile_add_cmd(m, "convert \"%s\" \"%s\"", infile, outname);
    }
    else {
        makefile_nam_cmd(m, "echo -e \"INKCONV\\t%s\"", infile);
        makefile_add_cmd(m, "mkdir -p \"%s\" >& /dev/null || true", cachedir);
        makefile_add_cmd(m, "inkscape \"%s\" --export-pdf=\"%s\" -D",
                         infile, outname);
    }
    makefile_end_cmds(m);

    /* If we crop the file, then just copy it back in place. */
    if (p->crop) {
        makefile_create_target(m, filename);
        makefile_start_deps(m);
        makefile_add_dep(m, outname);
        makefile_end_deps(m);

        makefile_start_cmds(m);
        makefile_nam_cmd(m, "echo -e \"IMCROP\\t%s\"", infile);
        makefile_add_cmd(m, "mkdir -p \"%s\" >& /dev/null || true", cachedir);
        makefile_add_cmd(m, "pdfcrop \"%s\" \"%s\" >& /dev/null",
                         outname, filename);
        makefile_end_cmds(m);
    }

    /* This one is necessary for pandoc. */
    makefile_create_target(m, cachename);
    makefile_start_deps(m);
    makefile_add_dep(m, infile);
    makefile_end_deps(m);

    makefile_start_cmds(m);
    makefile_nam_cmd(m, "echo -e \"IMGCP\\t%s\"", infile);
    makefile_add_cmd(m, "mkdir -p \"%s\" >& /dev/null || true", cachedir);
    makefile_add_cmd(m, "cp \"%s\" \"%s\"", infile, cachename);
    makefile_end_cmds(m);

    /* Cleans up all the memory allocated by this code. */
    TALLOC_FREE(c);
}
