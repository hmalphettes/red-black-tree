#include "jsw_rbtree.h"
//#include "jsw_rbtree.c" // why do we need to include the source here?
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
  if (w == tdigest->count) {
    centroidset_weighted_insert(tdigest->centroidset, x, w);
    return;
  }
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
