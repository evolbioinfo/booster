#include "sort.h"

void sort_double(double*tab, int size){
  qsort(tab, size, sizeof(double), comp_double);
}

/* void sort_indexes_double(int * indexes, int size, double * values){ */
/*   #ifdef __APPLE__ */
/*   qsort_r(indexes, size, sizeof(int), values, comp_indexes_apple); */
/*   #else */
/*   qsort_r(indexes, size, sizeof(int), comp_indexes, values); */
/*   #endif */
/* } */

int comp_double(const void * elem1, const void * elem2){
  double f = *((double*)elem1);
  double s = *((double*)elem2);
  if (f > s) return  1;
  if (f < s) return -1;
  return 0;
}

int comp_indexes(const void * elem1, const void * elem2, void * other_array){
  int i1 = *((int*)elem1);
  int i2 = *((int*)elem2);
  
  double * other = (double*)other_array;

  double val1 = other[i1];
  double val2 = other[i2];

  if (val1 > val2) return  1;
  if (val1 < val2) return -1;
  return 0;
}

int comp_indexes_apple(void * other_array, const void * elem1, const void * elem2){
  return(comp_indexes(elem1,elem2,other_array));
}
