#include "tdigest.h"
#include "jsw_rbtree.h"
#include "centroid.h"
#include "centroid.c"
// #include <stdlib.h>

tdigest_t *tdigest_new(double delta, int compression)
{
	tdigest_t *tdigest = (tdigest_t *)malloc ( sizeof *tdigest );
	tdigest->count = 0;
  tdigest->compression = compression;
  tdigest->delta = delta;
  tdigest->centroidset = centroidset_new();

	return tdigest;
}

static void tdigest__update_centroid(tdigest_t * self, centroid_t *centroid, double x, int w)
{
  centroidset_pop(self->centroidset, centroid);
  centroid_update(centroid, x, w);
  centroidset_weighted_insert(self->centroidset, x, w);
}

static double _threshold(tdigest_t *self, double q)
{
  return 4 * self->count * self->delta * q * (1 - q);
}

/*static*/ double _compute_centroid_quantile(tdigest_t *self, centroid_t *centroid)
{
/*
  def _compute_centroid_quantile(self, centroid):
      denom = self.n
      cumulative_sum = sum(
          c_i.count for c_i in self.C.value_slice(-float('Inf'), centroid.mean))
      return (centroid.count / 2. + cumulative_sum) / denom
 */
  size_t denom = self->count;
  int cumulative_sum = 0;
  // iterate over all the values in the tree that are between the minimum and
  // strictly less than the centroid mean. and sum up their weight.
  centroidset_t *tree = self->centroidset;
  jsw_rbnode_t *it = tree->root;

  while ( it != NULL ) {
    int cmp = tree->cmp ( it->data, (void *)centroid );

    if (cmp == 0)
      cmp = -1; // dont visit the bigger part of the tree
    else if (cmp < 0)
      cumulative_sum += ((centroid_t *)it->data)->weight;

    /*
      If the tree supports duplicates, they should be
      chained to the right subtree for this to work
    */
    it = it->link[cmp < 0];
  }

 return (centroid->weight /2.0f + cumulative_sum) / denom;
}

static tdigest_t *tdigest_new_fromdata(tdigest_t * self, centroid_t *data_arr, size_t size) {
  tdigest_t * new_digest = tdigest_new(self->delta, self->compression);
  centroid_arr_shuffle(data_arr, size);
  for (size_t i = 0; i < size; i++) {
    tdigest_update(new_digest, data_arr[i].mean, data_arr[i].weight);
  }
  return new_digest;
}

static void tdigest_compress(tdigest_t * self) {
  centroid_t *data;
  data = centroidset_values(self->centroidset);
  tdigest_t * new_digest;

  new_digest = tdigest_new_fromdata(self, data, centroidset_size(self->centroidset));
  self->centroidset = new_digest->centroidset;
}

void tdigest_update(tdigest_t * tdigest, double x, const size_t w) {
  tdigest->count += w;
  if (w == tdigest->count) { // no node yet in the tdigest.
    centroidset_weighted_insert(tdigest->centroidset, x, w);
    return;
  }
  centroid_pair_t closest_centroids = centroidset_closest(tdigest->centroidset, x);

  static bool seeded = false;
  if (!seeded) {
    seed_srand();
  }

  double w_d = w;

  centroid_t *centroid;
  while (w_d > 0) {
    // choose one of the 2 centroids - randomly if there are 2 of them.
    if (closest_centroids.data1 == NULL) {
      if (closest_centroids.data0 == NULL) {
        centroidset_weighted_insert(tdigest->centroidset, x, w_d);
        break; // no more things to do.
      }
      centroid = closest_centroids.data0;
    } else if (closest_centroids.data0 != NULL) {
      const int random_bit = rand() & 1;
      if (random_bit == 0) {
        centroid = closest_centroids.data0;
        closest_centroids.data0 = NULL;
      } else {
        centroid = closest_centroids.data1;
        closest_centroids.data1 = NULL;
      }
    } else {
      centroid = closest_centroids.data1;
      closest_centroids.data1 = NULL;
    }

    double q = _compute_centroid_quantile(tdigest, centroid);

    // This filters the out centroids that do not satisfy the second part
    // of the definition of S. See original paper by Dunning.
    double threshold = _threshold(tdigest, q);
    if (centroid->weight + w_d > threshold) {
      continue;
    }

    // delta_w = min(self._theshold(q) - c_j.count, w_d)
    double delta_w = threshold - centroid->weight;
    if (delta_w > w_d) {
      delta_w = w_d;
    }

    tdigest__update_centroid(tdigest, centroid, x, delta_w);
    w_d -= delta_w;
  }

  if (tdigest->count > tdigest->compression / tdigest->delta) {
    tdigest_compress(tdigest);
  }
}

tdigest_t * tdigest__add(tdigest_t *self, tdigest_t *other_digest)
{
  size_t s1 = centroidset_size(self->centroidset);
  size_t s2 = centroidset_size(other_digest->centroidset);

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
  i++;
  return tdigest_new_fromdata(self, data, i);
}
