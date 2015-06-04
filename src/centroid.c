#include "centroid.h"
#include "jsw_rbtree.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>

void centroid_update(centroid_t *centroid, double x, int delta_weight)
{
  centroid->weight += delta_weight;
  centroid->mean += delta_weight * (x - centroid->mean) / centroid->weight;
}

void centroid_print(centroid_t *centroid)
{
	printf("<Centroid: mean=%.8f, weight=%d>\n", centroid->mean, centroid->weight);
}

static int centroid_cmp(const void *p1, const void *p2)
{
	centroid_t *centroid1, *centroid2;

	centroid1 = (centroid_t*)p1;
	centroid2 = (centroid_t*)p2;

  // return centroid1->mean - centroid2->mean;
	if (centroid1->mean > centroid2->mean)
		return 1;

	else if (centroid1->mean < centroid2->mean)
		return -1;

	return 0;
}

/**
 * Returns the closest to p0 amongst plt or pgt
 * Assumes none of the values are null and that plt is strictly lesser than p0
 * and pgt is strictly greater than p0
 */
static const void* centroid_closest(const void *p0, const void *plt, const void *pgt)
{
	centroid_t *cntrd_0, *cntrd_lt, *cntrd_gt;

	cntrd_0 = (centroid_t*)p0;
	cntrd_lt = (centroid_t*)plt;
	cntrd_gt = (centroid_t*)pgt;

  if (cntrd_0->mean - cntrd_lt->mean > cntrd_gt->mean - cntrd_0->mean)
    return pgt;
  else
    return plt;
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
  centroid_print(centroid);

	while ((centroid = jsw_rbtnext(rbtrav)) != NULL) {
		centroid_print(centroid);
	}
  jsw_rbtdelete(rbtrav);
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
  centroid_arr[0] = *centroid;

  int i = 1;
	while ((centroid = jsw_rbtnext(rbtrav)) != NULL) {
		centroid_arr[i] = *centroid;
    i++;
	}
  jsw_rbtdelete(rbtrav);
  return centroid_arr;
}

size_t centroid_size(centroidset_t *centroidset)
{
  return jsw_rbsize(centroidset);
}

/* Arrange the N elements of ARRAY in random order.
   Only effective if N is much smaller than RAND_MAX;
   if this may not be the case, use a better random
   number generator. */
void centroid_arr_shuffle(centroid_t *array, size_t n)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int usec = tv.tv_usec;
    srand48(usec);

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

centroid_t* centroidset_closest(centroidset_t *centroidset, double x)
{
	centroid_t *centroid, centroid_find;

	centroid_find.mean = x;
	centroid = (centroidset_t *)jsw_rbfind_closest(centroidset, &centroid_find);
	return centroid;
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
