#include <stdio.h>
#include <algorithm>
#include <pthread.h>
#include <math.h>

#include "CycleTimer.h"
#include "sqrt_ispc.h"

using namespace ispc;

extern void sqrtSerial(int N, float startGuess, float* values, float* output);
extern void sqrtAVX2(int N, float startGuess, float* values, float* output);

static void verifyResult(int N, float* result, float* gold) {
    for (int i=0; i<N; i++) {
        if (fabs(result[i] - gold[i]) > 1e-4) {
            printf("Error: [%d] Got %f expected %f\n", i, result[i], gold[i]);
        }
    }
}

int main() {

    const unsigned int N = 20 * 1000 * 1000;
    const float initialGuess = 1.0f;

    float* values = NULL;  // If you allocated here, posix_memalign would reallocate. Memory Leak
    float* output = NULL;  // DITTO
    float* gold = new float [N];  // Allocate since didn't call posix_memalign later

    // Allocate N floats (Nx4 bytes) for ptr, with a starting address that is multiple of 32 bytes
    // The start address prevents misaligned memory. We are using 256-bit vectors,
    // so that would be 32 bytes. The CPU fetches 32 bytes of data from memory per vector.
    // Assuming a 32-bit CPU, it has a memory granularity of 4 bytes (4 bytes loaded per cycle)
    char ret = 0;
    ret = posix_memalign((void **)&values, 32, sizeof(float) * N);
    if (ret != 0)
    {
        printf("%s\n", strerror(ret));
        return 1;
    }

    ret = posix_memalign((void **)&output, 32, sizeof(float) * N);
    if (ret != 0)
    {
        printf("%s\n", strerror(ret));
        return 1;
    }

    for (unsigned int i=0; i<N; i++)
    {
        // TODO: CS149 students.  Attempt to change the values in the
        // array here to meet the instructions in the handout: we want
        // to you generate best and worse-case speedups
        
        // starter code populates array with random input values
        values[i] = .001f + 2.998f * static_cast<float>(rand()) / RAND_MAX;
        // LOWER BOUND + RANGE
//        values[i] = 2.999f;  // BEST
//        values[i] = 1.0f;   // WORST
    }

    // generate a gold version to check results
    for (unsigned int i=0; i<N; i++)
        gold[i] = sqrt(values[i]);

    //
    // And run the serial implementation 3 times, again reporting the
    // minimum time.
    //
    double minSerial = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        sqrtSerial(N, initialGuess, values, output);
        double endTime = CycleTimer::currentSeconds();
        minSerial = std::min(minSerial, endTime - startTime);
    }

    printf("[sqrt serial]:\t\t[%.3f] ms\n", minSerial * 1000);

    verifyResult(N, output, gold);

    //
    // Compute the square-root using the ispc implementation; report the minimum
    // time of three runs.
    //
    double minISPC = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        sqrt_ispc(N, initialGuess, values, output);
        double endTime = CycleTimer::currentSeconds();
        minISPC = std::min(minISPC, endTime - startTime);
    }

    printf("[sqrt ispc]:\t\t[%.3f] ms\n", minISPC * 1000);

    verifyResult(N, output, gold);

    // Clear out the buffer
    for (unsigned int i = 0; i < N; ++i)
        output[i] = 0;

    //
    // ISPC with tasks
    //
    double minTaskISPC = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        sqrt_ispc_withtasks(N, initialGuess, values, output);
        double endTime = CycleTimer::currentSeconds();
        minTaskISPC = std::min(minTaskISPC, endTime - startTime);
    }

    printf("[sqrt task ispc]:\t[%.3f] ms\n", minTaskISPC * 1000);

    verifyResult(N, output, gold);

    printf("\t\t\t\t(%.2fx speedup from ISPC)\n", minSerial/minISPC);
    printf("\t\t\t\t(%.2fx speedup from task ISPC)\n", minSerial/minTaskISPC);

    // Clear out the buffer
    for (unsigned int i = 0; i < N; ++i)
        output[i] = 0;

    //
    // Tasking version of the AVX code
    //
    double minAVX2 = 1e30;
    for (int i = 0; i < 3; ++i) {
        double startTime = CycleTimer::currentSeconds();
        sqrtAVX2(N, initialGuess, values, output);
        double endTime = CycleTimer::currentSeconds();
        minAVX2 = std::min(minAVX2, endTime - startTime);
    }

    printf("[sqrt avx2]:\t\t[%.3f] ms\n", minAVX2 * 1000);

    verifyResult(N, output, gold);

    printf("\t\t\t\t(%.2fx speedup from ISPC)\n", minSerial/minISPC);
    printf("\t\t\t\t(%.2fx speedup from task ISPC)\n", minSerial/minTaskISPC);
    printf("\t\t\t\t(%.2fx speedup from AVX2)\n", minSerial/minAVX2);

    delete [] values;
    delete [] output;
    delete [] gold;

    return 0;
}
