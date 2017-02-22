#ifndef _VERSION_H_
#define _VERSION_H_

#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>

#define NAME "BSTools"

/** 
    Prints the version of the tools
    In the output file.
    (may be stdout or stderr)
 */
void version(FILE *out, char *executable);

/** 
    Prints the version of the tools
    In the output file.
    Without the executable name
    (may be stdout or stderr)
 */
void short_version(FILE *out);

#endif
