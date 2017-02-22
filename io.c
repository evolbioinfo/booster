#include "io.h"

void Generic_Exit(const char *file, int line, const char *function, int code){
  fprintf(stderr,"\n== Err. in file '%s' (line %d), function '%s'\n",file,line,function);
  exit(code);
}
