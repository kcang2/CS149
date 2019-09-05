#include <immintrin.h>
#include <stdio.h>
#include <stdlib.h>

void sqrtAVX2(int N,
	      float initialGuess,
              float values[],
              float output[])
{
   // set thresholds
   static const float kThreshold = 0.00001f;
   __m256 threshs;
   threshs = _mm256_set1_ps(kThreshold);

   // set initial guesses
   __m256 inits;
   inits = _mm256_set1_ps(initialGuess);

   // set halves
   __m256 halves;
   halves = _mm256_set1_ps(0.5f);
	
   // Number of elements, ceil to multiple of 8. Eg. N=10, n=16
   // We use single precision (SP) floats, each 32 bits
   // We have 256 bits per vector, so 8 floats
   int n = (N+7) & ~7UL;
     
   for(int i=0; i<n; i+=8) {
      __m256 s = _mm256_load_ps(values+i);  // X^2 
      __m256 x = inits;  // X0

      // |X0^2 - X^2|. AVX only has abs for ints
      __m256 error = _mm256_mul_ps(x, x) - s;  // X0^2 - X^2
      __m256 error2 = _mm256_sub_ps(_mm256_setzero_ps(), error);  // 0 - (X0^2 - X^2)
      error = _mm256_max_ps(error, error2);  // max(X0^2 - X^2, X^2 - X0^2)

      // cmp returns 0xFFFFFFFF(8x4 bits) for a[i]>b[i], 0 otherwise. i=0:7
      // movemask gets the MSB of each float. (Can get any other bit actually)
      // 8 MSBs, set to dst[7:0], dst[31:8] = 0.
      // For all errors to be < threshold, movemask should return 0
      __m256 masknotdone = _mm256_cmp_ps(error, threshs, _CMP_GT_OQ);
      unsigned int notdone = _mm256_movemask_ps(masknotdone);

      while (notdone > 0) {
         // xn+1 = 0.5(xn + S/xn)
	 __m256 tmp;
         tmp = _mm256_mul_ps(halves, _mm256_add_ps(x, _mm256_div_ps(s, x)));
 
         // But only update guess based on mask
         // 0 = done already
         __m256 maskdone = _mm256_cmp_ps(masknotdone, _mm256_setzero_ps(), _CMP_EQ_OQ);
         x = _mm256_add_ps(_mm256_and_ps(maskdone, x), _mm256_and_ps(masknotdone, tmp));

         // Get error of updated guess
         error = _mm256_mul_ps(x, x) - s;
         error2 = _mm256_sub_ps(_mm256_setzero_ps(), error);
         error = _mm256_max_ps(error, error2);
         
         // Update mask
         masknotdone = _mm256_cmp_ps(error, threshs, _CMP_GT_OQ);
         notdone = _mm256_movemask_ps(masknotdone);
      }
      
      _mm256_store_ps(output+i, x); // use address, not values
   }
}


//int main(int argc, char *argv[]){
//   float values[10] = {0};
//   float output[10] = {0};
//   sqrtAVX2(10, 0.1f, values, output);
//}
