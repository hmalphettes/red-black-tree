#ifndef CENTROID_H
#define CENTROID_H

#include "jsw_rbtree.h"

typedef struct centroid
{
  double            mean;
  int               weight;
} centroid_t;

typedef struct jsw_rbtree centroidset_t;

#endif
