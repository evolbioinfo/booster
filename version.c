#include "version.h"

void version(FILE *out, char *executable){
  fprintf(out,"%s version %s\n",NAME, VERSION);
}

void short_version(FILE *out){
  fprintf(out,"%s : version %s\n",NAME,VERSION);
}
