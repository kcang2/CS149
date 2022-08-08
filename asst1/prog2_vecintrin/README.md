Summary:

#define VECTOR_WIDTH 16

template <typename T>
struct __cs149_vec {
  T value[VECTOR_WIDTH];
};
  
Basically, a vector is a struct with an array of values. Value type can be float or int or bool (mask).
  
See `void absVector(float* values, float* output, int N)` for example implementation
