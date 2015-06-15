#include "tdigest.h"
#include "jsw_rbtree.h"
#include "centroid.h"
#include "centroid.c"
#include <math.h>
// #include <stdlib.h>
#include <assert.h>

tdigest_t *_tdigest_new(const double delta, const int K, const double compression_trigger)
{
	tdigest_t *tdigest = (tdigest_t *)malloc ( sizeof *tdigest );
	tdigest->count = 0;
  tdigest->K = K;
  tdigest->delta = delta;
  tdigest->centroidset = centroidset_new();

  tdigest->compression_trigger = compression_trigger;

  static bool seeded = false;
  if (!seeded) {
    seed_srand();
  }

	return tdigest;
}

tdigest_t *tdigest_new(const double delta, const int K)
{
  return _tdigest_new(delta, K, K / delta);
}

tdigest_t *tdigest_new_default()
{
  return tdigest_new(0.01, 25);
}

void tdigest__add_centroid(tdigest_t *self, centroid_t *centroid) {
  centroid_t *found;
  found = (centroid_t *)jsw_rbfind(self->centroidset, centroid);
  if (found) {
    found->weight += centroid->weight;
  } else {
    jsw_rbinsert(self->centroidset, (void *)centroid);
  }
}

static void tdigest__update_centroid(const tdigest_t * self, centroid_t *centroid, const double x, const int w)
{
  centroidset_pop(self->centroidset, centroid);
  centroid_update(centroid, x, w);
  centroidset_insert(self->centroidset, centroid);
}

static tdigest_t *tdigest_new_fromdata(tdigest_t * self, centroid_t *data_arr, size_t size) {
  tdigest_t * new_digest = _tdigest_new(self->delta, self->K, self->compression_trigger);
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

void tdigest_update(tdigest_t * tdigest, const double x, const size_t w) {
  tdigest->count += w;
  if (w == tdigest->count) { // no node yet in the tdigest.
    centroidset_weighted_insert(tdigest->centroidset, x, w);
    return;
  }
	centroid_t closest_lt = { .weight=0, .mean=0 }, closest_gt = { .weight=0, .mean=0 }, *neighbor = NULL, *closest = NULL;
  centroidset_closest(tdigest->centroidset, x, &closest_lt, &closest_gt);
  int i;
  if (closest_gt.weight != 0) {
    i = 2; // i=2 -> equi-distance 2 centroids
  } else if (closest_lt.weight != 0) {
    i = 1;
  } else {
    // we should not be here. there is at least one node to be found
    // or the tree is empty and we have done that case already.
    centroidset_weighted_insert(tdigest->centroidset, x, w);
    return;
  }
  size_t sum = centroidset_headsum(tdigest->centroidset, &closest_lt);
  double n = 1.0;
  while (i != 0) {
    if (i == 2) {
      neighbor = &closest_gt;
    } else {
      neighbor = &closest_lt;
    }
    i--;
    double q = (sum + neighbor->weight / 2.0) / tdigest->count;
    // double k = 4 * tdigest->count * q * (1 - q) / tdigest->K;
    double k = 4 * tdigest->count * q * (1 - q) * tdigest->delta;

    // when
    if (neighbor->weight + w <= k) {
      if (n == 1 || drand48() * n < 1) {
        closest = neighbor;
      }
      n++;
    }
    sum += neighbor->weight;
  }

  if (closest == NULL) {
    centroidset_weighted_insert(tdigest->centroidset, x, w);
  } else {
    tdigest__update_centroid(tdigest, closest, x, w);
  }

  if (tdigest->centroidset->size > tdigest->compression_trigger) {
    // printf("%zu Compressing %zu > %f = %d / %f \n", tdigest->count, tdigest->centroidset->size, tdigest->K / tdigest->delta, tdigest->K, tdigest->delta);
    tdigest_compress(tdigest);
    // printf("After compressing %zu > %f = %d / %f \n", tdigest->centroidset->size, tdigest->K / tdigest->delta, tdigest->K, tdigest->delta);
  }
}

/**
 * TODO: this is meant as the overloading of '+' in the original python code.
 * TODO: find out where it is used.
 */
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
double tdigest_percentile(const tdigest_t *self, double q)
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
double tdigest_quantile(tdigest_t *self, double q) {
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
  double t = 0;
  double N = self->count;
  double delta;

  centroidset_t *tree = self->centroidset;
  size_t max_ind = centroidset_size(tree) -1;

	jsw_rbtrav_t *rbtrav;
	rbtrav = jsw_rbtnew();

  centroid_t *centroid;
	centroid = jsw_rbtfirst(rbtrav, tree);
  size_t i = 0;
	do {
    if (i == max_ind) {
      delta = (centroid->mean - ((centroid_t *)jsw_rbtpeekprev(rbtrav))->mean ) / 2.0f;
    } else {
      delta = ( ( (centroid_t *)jsw_rbtpeeknext(rbtrav) )->mean - centroid->mean) / 2.0f;
    }
    double z = fmax(-1, (q - centroid->mean) / delta);
    if (z < -1) { // this looks so wrong. why would it ever be less than -1 when
      // we picked the max between a value and -1.
      // time to read the original paper again.
      return t / N + centroid->mean / N * (z + 1) / 2;
    }
    t += centroid->weight;
    i++;
	} while ((centroid = jsw_rbtnext(rbtrav)) != NULL);
  jsw_rbtdelete(rbtrav);


  return 1.0f;
}

/**
 * Computes the mean of the distribution between the two percentiles q1 and q2.
 * This is a modified algorithm than the one presented in the original t-Digest paper.
 */
// double tdigest_trimmed_mean(tdigest_t *self, double q1, double q2)
// {
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
//   return 1.0f;
// }
