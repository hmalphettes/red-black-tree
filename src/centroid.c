#include "centroid.h"
#include "jsw_rbtree.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

static double DBL_EPSILON = 2.220446e-15;//2.220446e-16;

void centroid_update(centroid_t *centroid, double x, int weight)
{
  centroid->weight += weight;
  centroid->mean += weight * (x - centroid->mean) / centroid->weight;
}

void centroid_print(centroid_t *centroid)
{
  if (!centroid) {
    printf("<Centroid NULL>\n");
    return;
  }
	printf("<Centroid: mean=%.8f, weight=%d>\n", centroid->mean, centroid->weight);
}

static int centroid_cmp(const void *p1, const void *p2)
{
	centroid_t *centroid1, *centroid2;

	centroid1 = (centroid_t*)p1;
	centroid2 = (centroid_t*)p2;

  // return centroid1->mean - centroid2->mean;
	if (centroid1->mean > centroid2->mean) {
    if (centroid2->mean + DBL_EPSILON >= centroid1->mean) {
      return 0;
    }
		return 1;
  } else if (centroid1->mean < centroid2->mean) {
    if (centroid1->mean + DBL_EPSILON >= centroid2->mean) {
      return 0;
    }
		return -1;
  }

	return 0;
}

/**
 * Returns the closest to p0 amongst plt or pgt
 * Assumes none of the values are null and that plt is strictly lesser than p0
 * and pgt is strictly greater than p0
 *
 * When plt and gpt are at equal distance we assign to the return pointers plt and pgt and return NULL
 */
static void centroid_closest(const void *p0, const void *plt, const void *pgt, void *data0, void *data1)
{
	centroid_t *cntrd_0, *cntrd_lt, *cntrd_gt;

	cntrd_0 = (centroid_t*)p0;
  if (!plt) {
    centroid_t *d0;
	  d0 = (centroid_t *)data0;
    *d0 = *(centroid_t*)pgt;
    return;
  }
  if (!pgt) {
    centroid_t *d0;
	  d0 = (centroid_t *)data0;
    *d0 = *(centroid_t*)plt;
    return;
  }
	cntrd_lt = (centroid_t*)plt;
	cntrd_gt = (centroid_t*)pgt;

  double gt_diff = cntrd_gt->mean - cntrd_0->mean;
  double lt_diff = cntrd_0->mean - cntrd_lt->mean;

// TODO: do we need to use epsilon for approximately equal comparisons?
// refer to the original tdigest java source where this type of issue was fixed.
  if (lt_diff > gt_diff) {
    centroid_t *d0;
	  d0 = (centroid_t *)data0;
    *d0 = *cntrd_gt;
    // *d1 = NULL;
  } else if (lt_diff < gt_diff) {
    centroid_t *d0;
	  d0 = (centroid_t *)data0;
    *d0 = *cntrd_lt;
    // *data1 = NULL;
  } else {
    //equi-distance
    centroid_t *d0, *d1;
	  d0 = (centroid_t *)data0;
	  d1 = (centroid_t *)data1;
    *d0 = *cntrd_lt;
    *d1 = *cntrd_gt;
  }
}

static void *centroid_dup(void *p)
{
	void *dup_p;

	dup_p = calloc(1, sizeof(struct centroid));
	memmove(dup_p, p, sizeof(struct centroid));

	return dup_p;
}

static void centroid_rel(void *p)
{
	free(p);
}

centroidset_t *centroidset_new()
{
	jsw_rbtree_t *rbtree;
	rbtree = jsw_rbnew(centroid_cmp, centroid_closest, centroid_dup, centroid_rel);

	return rbtree;
}

void centroidset_delete(centroidset_t *centroidset)
{
	jsw_rbdelete(centroidset);
}

int centroidset_weighted_insert(centroidset_t *centroidset, double mean, int weight)
{
	int ret;

	centroid_t *centroid;
	centroid = calloc(1, sizeof(centroid_t));

	centroid->mean = mean;
	centroid->weight = weight;

	ret = jsw_rbinsert(centroidset, (void *)centroid);
	if (ret == 0) {
		printf("failed to insert the centroid with mean %f and weight %d\n", mean, weight);
		free(centroid);
		return -1;
	}

	return 0;
}

int centroidset_insert(centroidset_t *centroidset, double mean)
{
	return centroidset_weighted_insert(centroidset, mean, 1);
}

int centroidset_erase(centroidset_t *centroidset, double mean)
{
	int ret;
	centroid_t *centroid;

	centroid = calloc(1, sizeof(centroid_t));
  centroid->mean = mean;

	ret = jsw_rberase(centroidset, (void*)centroid);
	if (ret == 0) {
		printf("failed to erase the centroid with mean %f\n", mean);
		free(centroid);
		return -1;
	}

	return 0;
}

double centroidset_find(centroidset_t *centroidset, double mean)
{
	centroid_t *centroid, centroid_find;

	centroid_find.mean = mean;
	centroid = jsw_rbfind(centroidset, &centroid_find);
  if (!centroid) {
    return NAN;
  }
	return centroid->mean;
}

void centroidset_printset(centroidset_t *centroidset)
{
  if (centroidset == NULL) {
    printf("NULL");
    return;
  }

	centroid_t *centroid;

	jsw_rbtrav_t *rbtrav;
	rbtrav = jsw_rbtnew();

	centroid = jsw_rbtfirst(rbtrav, centroidset);
	do {
		centroid_print(centroid);
    if (!centroid) { break; }
	} while ((centroid = jsw_rbtnext(rbtrav)) != NULL);
  jsw_rbtdelete(rbtrav);
}

void centroidset_pop(centroidset_t *centroidset, centroid_t *centroid)
{
  // jsw_rberase(centroidset, centroid);
	int ret = jsw_rberase(centroidset, (void*)centroid);
	if (ret == 0) {
		printf("failed to pop the centroid with mean %f\n", centroid->mean);
	}
}

centroid_t * centroidset_values(centroidset_t *centroidset)
{
  size_t size = jsw_rbsize(centroidset);
  centroid_t *centroid_arr;
  centroid_arr = (centroid_t*)malloc (sizeof (centroid_t)*size );

	jsw_rbtrav_t *rbtrav;
	rbtrav = jsw_rbtnew();

	centroid_t *centroid;
	centroid = jsw_rbtfirst(rbtrav, centroidset);
  int i = 0;
	do {
		centroid_arr[i] = *centroid;
    i++;
	} while ((centroid = jsw_rbtnext(rbtrav)) != NULL);
  jsw_rbtdelete(rbtrav);

  return centroid_arr;
}

size_t centroidset_size(centroidset_t *centroidset)
{
  return jsw_rbsize(centroidset);
}

/* Arrange the N elements of ARRAY in random order.
   Only effective if N is much smaller than RAND_MAX;
   if this may not be the case, use a better random
   number generator. */
void centroid_arr_shuffle(centroid_t *array, size_t n)
{
  static bool seeded = false;
  if (!seeded) {
    seed_srand();
  }
  if (n <= 1) {
    return;
  }
  size_t i;
  for (i = n - 1; i > 0; i--) {
    size_t j = (unsigned int) (drand48()*(i+1));
    centroid_t t = array[j];
    array[j] = array[i];
    array[i] = t;
  }
}

centroid_t* centroidset_floor(centroidset_t *centroidset, double x)
{
	centroid_t *centroid, centroid_find;

	centroid_find.mean = x;
	centroid = jsw_rbfind_floor(centroidset, &centroid_find);
	return centroid;
}

centroid_t* centroidset_ceiling(centroidset_t *centroidset, double x)
{
	centroid_t *centroid, centroid_find;

	centroid_find.mean = x;
	centroid = jsw_rbfind_ceiling(centroidset, &centroid_find);
	return centroid;
}

/**
 *
 */
void centroidset_closest(centroidset_t *centroidset, double x, centroid_t *data0, centroid_t *data1)
{
	centroid_t centroid_find = { .mean=x, .weight=1 };
  // centroid_t *centroid_find = (centroid_t *)malloc ( sizeof *rt );

	// centroid_find.mean = x;
	jsw_rbfind_closest(centroidset, &centroid_find, data0, data1);
}

/*

int main()
{
	centroidset_t *centroidset;
	centroidset = centroidset_new();
	centroidset_insert(centroidset, 1.0);
	centroidset_insert(centroidset, 2.0);
	centroidset_insert(centroidset, 1.5);
	centroidset_weighted_insert(centroidset, 9.9, 3);
	centroidset_insert(centroidset, 1.8);
	centroidset_insert(centroidset, 3.3);
	double ret;
	ret = centroidset_find(centroidset, 1.0f);
	printf("find 1.0: %f\n", ret);
	ret = centroidset_find(centroidset, 9.900000);
	printf("find 9.9: %f\n", ret);
	ret = centroidset_find(centroidset, 1.800000);
	printf("find 1.8: %f\n", ret);
	ret = centroidset_find(centroidset, 0);
	printf("find 0 (not there) %f\n", ret);
	ret = centroidset_erase(centroidset, 1.800000);
	printf("erase 1.800000: %f\n", ret);
	ret = centroidset_find(centroidset, 1.800000);
	printf("find 1.8: %f\n", ret);
	centroidset_printset(centroidset);
	centroidset_delete(centroidset);
}


*/
