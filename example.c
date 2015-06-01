// Example.

#include "src/centroid.c"

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
  centroid_t* f = centroidset_floor(centroidset, 1.8);
  printf("floor: biggest lte 1.8: ");
  centroid_print(f);

	centroidset_delete(centroidset);
}
