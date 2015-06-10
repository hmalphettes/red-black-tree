#ifndef CENTROID_H
#define CENTROID_H

#include "jsw_rbtree.h"
#include <stdlib.h>
#include <sys/time.h>

typedef struct centroid
{
  double            mean;
  int               weight;
} centroid_t;

typedef struct jsw_rbtree centroidset_t;

// typedef struct centroid_pair
// {
//   centroid_t     *data0;
//   centroid_t     *data1;
// } centroid_pair_t;

void seed_srand() {
  static int seeded = 0;
  if (seeded) { return; }
  seeded = 1;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int usec = tv.tv_usec;
  srand48(usec);
}

#endif
