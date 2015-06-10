// Example.

#include "src/centroid.c"
#include <assert.h>

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
	assert (ret != ret); // is NaN
	centroidset_printset(centroidset);

  // printf("\nNow moving on to stage 2\n");
  //
  // centroid_t* c1 = centroidset_values(centroidset);
  // size_t s1 = centroid_size(centroidset);
  // centroid_arr_shuffle(c1, s1);
  // for (size_t i = 0; i < s1; i++) {
  //   centroid_print(&c1[i]);
  // }

  printf("\nNow moving on to stage 3: search\n");
  centroid_t* c = centroidset_ceiling(centroidset, 1.8);
  printf("ceiling: smallest gte 1.8: ");
  centroid_print(c);
	assert (c->mean == 2.0f);
  centroid_t* f = centroidset_floor(centroidset, 1.8);
  printf("floor: biggest lte 1.8: ");
  centroid_print(f);
	assert (f->mean == 1.5f);

	centroid_t data0, data1;
	centroidset_closest(centroidset, 1.8, &data0, &data1);
  printf("closest to 1.8: ");
  centroid_print(&data0);
	assert (data0.mean == 2.0f);
/*	p = centroidset_closest(centroidset, 1.7);
  printf("closest to 1.7: ");
  centroid_pair_print(p);
	assert (p.data0->mean == 1.5f);
  printf("closest to 1.75: ");
	p = centroidset_closest(centroidset, 1.75);
  centroid_pair_print(p);
	assert (p.data0->mean == 1.5f);
	assert (p.data1->mean == 2.0f);
	*/

	// Test find_first find_last
	c= (centroid_t*) jsw_rbfind_last(centroidset);
	printf("find_last ");
	centroid_print(c);
	assert(c->weight == 3);
	centroid_t *first;
	first = (centroid_t*) jsw_rbfind_first(centroidset);
	printf("find_first ");
	centroid_print(first);
	assert(first->mean == 1.0f);

	// Test peek and rbtrav
	jsw_rbtrav_t *rbtrav;
	rbtrav = jsw_rbtnew();
	first = jsw_rbtfirst(rbtrav, centroidset);
	printf("first ");
	centroid_print(first);
	assert(first->mean == 1.0f);
	printf("peek next ");
	c = jsw_rbtpeeknext(rbtrav);
	centroid_print(c);
	assert(c->mean == 1.5f);
	printf("jsw_rbtpeeknext 2 ");
	f = jsw_rbtpeeknext(rbtrav);
	centroid_print(f);
	assert(c == f);
	c = jsw_rbtnext(rbtrav);
	assert(c == f);
	f = jsw_rbtpeekprev(rbtrav);
	assert(f == first);
	c = jsw_rbtpeekprev(rbtrav);
	assert(c == first);
	jsw_rbtdelete(rbtrav);


	centroidset_delete(centroidset);

	return 0;
}
