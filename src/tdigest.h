#ifndef TDIGEST_H
#define TDIGEST_H

#include "centroid.h"

typedef struct tdigest
{
  centroidset_t *centroidset;
  size_t      count;
  double      delta;
  int         K;
  size_t      compression_trigger;
} tdigest_t;

tdigest_t   *tdigest_new_default();
tdigest_t   *tdigest_new(const double delta, const int K);
void        tdigest_update(tdigest_t * tdigest, double x, const size_t w);
void        tdigest_delete(tdigest_t * tdigest);

double tdigest_percentile(const tdigest_t *self, const double q);

#endif
