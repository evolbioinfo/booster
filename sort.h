#define _GNU_SOURCE
#include <stdlib.h>

#ifndef _SORT_H
#define _SORT_H

int comp_double(const void * elem1, const void * elem2);
int comp_indexes(const void * elem1, const void * elem2, void * other_array);
int comp_indexes_apple(void * other_array, const void * elem1, const void * elem2);

void sort_double(double * tab, int size);
void sort_indexes_double(int * indexes, int size, double * values);

#endif
