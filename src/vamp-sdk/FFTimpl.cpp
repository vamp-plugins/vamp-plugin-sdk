
// Override C linkage for KissFFT headers. So long as we have already
// included all of the other (system etc) headers KissFFT depends on,
// this should work out OK
#undef __cplusplus

namespace Kiss {

#undef KISS_FFT_H
#undef KISS_FTR_H
#undef KISS_FFT__GUTS_H
#undef FIXED_POINT
#undef USE_SIMD
#undef kiss_fft_scalar

#ifdef SINGLE_PRECISION_FFT
#pragma message("Using single-precision FFTs")
typedef float kiss_fft_scalar;
#define kiss_fft_scalar float
#else
typedef double kiss_fft_scalar;
#define kiss_fft_scalar double
#endif

inline void free(void *ptr) { ::free(ptr); }
#include "ext/kiss_fft.c"
#include "ext/kiss_fftr.c"

#undef kiss_fft_scalar // leaving only the namespaced typedef

}

