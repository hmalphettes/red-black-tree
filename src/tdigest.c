#include "jsw_rbtree.h"
#include "centroid.h"
#include "centroid.c"

typedef struct tdigest
{
  centroidset_t *centroidset;
  int               count;
  double            delta;
  int               compression;
} tdigest_t;

tdigest_t *tdigest_new(double delta, int compression)
{
	tdigest_t *tdigest = (tdigest_t *)malloc ( sizeof *tdigest );
	tdigest->count = 0;
  tdigest->compression = compression;
  tdigest->delta = delta;
  tdigest->centroidset = centroidset_new();

	return tdigest;
}

void tdigest__update_centroid(tdigest_t * tdigest, double x, int w)
{
  // TODO
  return;
}

void tdigest_update(tdigest_t * tdigest, double x, int w) {
  tdigest->count += w;
  if (w == tdigest->count) { // no node yet in the tdigest.
    centroidset_weighted_insert(tdigest->centroidset, x, w);
    return;
  }
  // centroid_t *closest_centroid;
  // closest_centroid = centroidset_closest(tdigest->centroidset, x);
  //
  // struct timeval tv;
  // gettimeofday(&tv, NULL);
  // int usec = tv.tv_usec;
  // srand48(usec);
  //
  // while (closest_centroid->count != 0 && w > 0) {
  //   size_t j = (unsigned int) (drand48()*(closest_centroid->count));
  //   c_j = closest_centroid[j]
  // }
  // TODO
}

tdigest_t * tdigest__add(tdigest_t *self, tdigest_t *other_digest)
{
  size_t s1 = centroid_size(self->centroidset);
  size_t s2 = centroid_size(other_digest->centroidset);

  centroid_t* c1 = centroidset_values(self->centroidset);
  centroid_t* c2 = centroidset_values(other_digest->centroidset);

  centroid_t *data;
  data = (centroid_t*)malloc(sizeof(centroid_t) * (s1 + s2));

  size_t i;
  for (i = 0; i < s1; i++) {
    data[i] = c1[i];
  }
  for (i = 0; i < s2; i++) {
    data[i+s1] = c2[i];
  }
  tdigest_t * new_digest = tdigest_new(self->delta, self->compression);
  centroid_arr_shuffle(data, s1 + s2);
  for (i = 0; i < s1 + s2; i++) {
    tdigest_update(new_digest, data[i].mean, data[i].weight);
  }
  return new_digest;
}
