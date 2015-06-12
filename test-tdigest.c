#include "src/centroid.h"
#include "src/tdigest.h"
#include "src/tdigest.c"
#include <assert.h>
#include <math.h>

int main()
{

  seed_srand();

  tdigest_t * tdigest;
  tdigest = tdigest_new(0.01f, 20);//tdigest_new_default();
  centroidset_printset(tdigest->centroidset);

// crash
  // tdigest_update(tdigest, 0.26059413, 1);
  // tdigest_update(tdigest, 0.336455, 1);
  // tdigest_update(tdigest, 0.118310, 1);
  // tdigest_update(tdigest, 0.706089, 1);
  // tdigest_update(tdigest, 0.806067, 1);
  // tdigest_update(tdigest, 0.909876, 1);
// --crash

// infinite loop?
  // tdigest_update(tdigest, 0.898033, 1);
  // tdigest_update(tdigest, 0.70146493, 1);
// --infinite loop?

  size_t l = 2010;
  for (size_t i = 0; i < l-1; i++) {
    // printf("UPNG\n");
    tdigest_update(tdigest, drand48(), 1);
  // centroidset_printset(tdigest->centroidset);
  // printf("size_t i %zu\n", i);
  }
  // tdigest_compress(tdigest);

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
