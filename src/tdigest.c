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

/////////////////////// PUBLIC API
/*
 * Computes the percentile of a specific value in [0,1], ie. computes F^{-1}(q) where F^{-1} denotes
 * the inverse CDF of the distribution.
 */
double percentile(tdigest_t *self, double q)
{
  if (q < 0 || q >1) {
    printf("Invalid argument to request a percentile: %f must be between 0 and 1, inclusive.", q);
    return -1.0f;
  }
  double t = 0, k;
  q = q * self->count;

  // iterate over all the values in the tree that are between the minimum and
  // strictly less than the centroid mean. and sum up their weight.
  centroidset_t *tree = self->centroidset;
  size_t max_ind = centroidset_size(tree) -1;

	jsw_rbtrav_t *rbtrav;
	rbtrav = jsw_rbtnew();

  centroid_t *centroid;
	centroid = jsw_rbtfirst(rbtrav, tree);
  size_t i = 0;
	do {
    k = centroid->weight;
    if (q < t + k) {
      if (i == 0 || i == max_ind) {
        return centroid->mean;
      }
      double delta = (((centroid_t *)jsw_rbtpeeknext(rbtrav))->mean - ((centroid_t *)jsw_rbtpeekprev(rbtrav))->mean) / 2.0f;
      return centroid->mean + ((q - t) / k - 0.5) * delta;
    }
    t += k;
    i++;
	} while ((centroid = jsw_rbtnext(rbtrav)) != NULL);
  jsw_rbtdelete(rbtrav);

  return ((centroid_t *)jsw_rbfind_last(tree))->mean;
/*def percentile(self, q):
    if not (0 <= q <= 1):
        raise ValueError("q must be between 0 and 1, inclusive.")

    t = 0
    q *= self.n

    for i, key in enumerate(self.C.keys()):
        c_i = self.C[key]
        k = c_i.count
        if q < t + k:
            if i == 0:
                return c_i.mean
            elif i == len(self) - 1:
                return c_i.mean
            else:
                delta = (self.C.succ_item(key)[1].mean - self.C.prev_item(key)[1].mean) / 2.
            return c_i.mean + ((q - t) / k - 0.5) * delta

        t += k
    return self.C.max_item()[1].mean
*/
}

/*
 * Computes the quantile of a specific value, ie. computes F(q) where F denotes
 * the CDF of the distribution.
 */
double quantile(tdigest_t *self, double q) {
/*def quantile(self, q):
    t = 0
    N = float(self.n)

    for i, key in enumerate(self.C.keys()):
        c_i = self.C[key]
        if i == len(self) - 1:
            delta = (c_i.mean - self.C.prev_item(key)[1].mean) / 2.
        else:
            delta = (self.C.succ_item(key)[1].mean - c_i.mean) / 2.
        z = max(-1, (q - c_i.mean) / delta)

        if z < 1:
            return t / N + c_i.count / N * (z + 1) / 2

        t += c_i.count
    return 1
*/
  // size_t t = 0;

  return 1.0f;
}

/**
 * Computes the mean of the distribution between the two percentiles q1 and q2.
 * This is a modified algorithm than the one presented in the original t-Digest paper.
 */
double trimmed_mean(tdigest_t *self, double q1, double q2)
{
/*
def trimmed_mean(self, q1, q2):
    if not (q1 < q2):
        raise ValueError("q must be between 0 and 1, inclusive.")

    s = k = t = 0
    q1 *= self.n
    q2 *= self.n
    for i, key in enumerate(self.C.keys()):
        c_i = self.C[key]
        k_i = c_i.count
        if q1 < t + k_i:
            if i == 0:
                delta = self.C.succ_item(key)[1].mean - c_i.mean
            elif i == len(self) - 1:
                delta = c_i.mean - self.C.prev_item(key)[1].mean
            else:
                delta = (self.C.succ_item(key)[1].mean - self.C.prev_item(key)[1].mean) / 2.
            nu = ((q1 - t) / k_i - 0.5) * delta
            s += nu * k_i * c_i.mean
            k += nu * k_i

        if q2 < t + k_i:
            return s/k
        t += k_i

    return s/k
*/
  return 1.0f;
}
