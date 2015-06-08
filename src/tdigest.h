#ifndef TDIGEST_H
#define TDIGEST_H

#include "centroid.h"

typedef struct tdigest
{
  centroidset_t *centroidset;
  size_t            count;
  double            delta;
  int               compression;
} tdigest_t;

tdigest_t   *tdigest_new(double delta, int compression);
void        tdigest_update(tdigest_t * tdigest, double x, const size_t w);

#endif
