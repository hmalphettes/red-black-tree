#include "src/centroid.h"
#include "src/tdigest.h"
#include "src/tdigest.c"
#include <assert.h>
#include <math.h>

int main()
{

  seed_srand();

  tdigest_t * tdigest;
  tdigest = tdigest_new_default();
  centroidset_printset(tdigest->centroidset);

// crash
  // tdigest_update(tdigest, 0.26059413, 1);
  // tdigest_update(tdigest, 0.236455, 1);
// --crash

// infinite loop?
  tdigest_update(tdigest, 0.898033, 1);
  tdigest_update(tdigest, 0.70146493, 1);
// --infinite loop?

// printf("OKI\n");
//   size_t l = 3;
//   for (size_t i = 0; i < l-1; i++) {
//     printf("UPNG\n");
//     tdigest_update(tdigest, drand48(), 1);
//   centroidset_printset(tdigest->centroidset);
//   printf("size_t i %zu\n", i);
//   }
// printf("UPED\n");
  tdigest_compress(tdigest);

  printf("tdigest_percentile 0.5 %f",fabs(tdigest_percentile(tdigest, 0.5f) - 0.5));


  return 0;
}
