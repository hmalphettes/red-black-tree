#include "src/centroid.h"
#include "src/tdigest.h"
#include "src/tdigest.c"
#include <assert.h>
#include <math.h>

int main()
{

  seed_srand();

  tdigest_t * tdigest;
  tdigest = tdigest_new_default();//tdigest_new(0.01, 25);

  // tdigest_update(tdigest, 0.26059413, 1);
  // tdigest_update(tdigest, 0.336455, 1);
  // tdigest_update(tdigest, 0.118310, 1);
  // tdigest_update(tdigest, 0.706089, 1);
  // tdigest_update(tdigest, 0.806067, 1);
  // tdigest_update(tdigest, 0.909876, 1);

  // tdigest_update(tdigest, 0.898033, 1);
  // tdigest_update(tdigest, 0.70146493, 1);

  size_t l = 12001;
  for (size_t i = 0; i < l-1; i++) {
    tdigest_update(tdigest, drand48(), 1);
  }
  printf("Before compressing %zu\n", tdigest->centroidset->size);
  tdigest_compress(tdigest);
  printf("After compressing %zu\n", tdigest->centroidset->size);

  // centroidset_printset(tdigest->centroidset);

  double tp = tdigest_percentile(tdigest, 0.5f);
  printf("tdigest_percentile 0.5 %f err: %f\n", tp, fabs(tp - 0.5));

  tp = tdigest_percentile(tdigest, 0.6f);
  printf("tdigest_percentile 0.6 %f err: %f\n", tp, fabs(tp - 0.6));

  tp = tdigest_percentile(tdigest, 0.8f);
  printf("tdigest_percentile 0.8 %f err: %f\n", tp, fabs(tp - 0.8));

  tp = tdigest_percentile(tdigest, 0.9f);
  printf("tdigest_percentile 0.9 %f err: %f\n", tp, fabs(tp - 0.9));

  tp = tdigest_percentile(tdigest, 0.99f);
  printf("tdigest_percentile 0.99 %f err: %f\n", tp, fabs(tp - 0.99));

  return 0;
}
