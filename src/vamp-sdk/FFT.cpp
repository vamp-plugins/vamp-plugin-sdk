/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Vamp

    An API for audio analysis and feature extraction plugins.

    Centre for Digital Music, Queen Mary, University of London.
    Copyright 2006-2012 Chris Cannam and QMUL.
  
    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use, copy,
    modify, merge, publish, distribute, sublicense, and/or sell copies
    of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR
    ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
    CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

    Except as contained in this notice, the names of the Centre for
    Digital Music; Queen Mary, University of London; and Chris Cannam
    shall not be used in advertising or otherwise to promote the sale,
    use or other dealings in this Software without prior written
    authorization.
*/

#include <vamp-sdk/FFT.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#if ( VAMP_SDK_MAJOR_VERSION != 2 || VAMP_SDK_MINOR_VERSION != 7 )
#error Unexpected version of Vamp SDK header included
#endif

_VAMP_SDK_PLUGSPACE_BEGIN(FFT.cpp)

// Override C linkage for KissFFT headers. So long as we have already
// included all of the other (system etc) headers KissFFT depends on,
// this should work out OK
#undef __cplusplus

namespace KissSingle {
#undef KISS_FFT_H
#undef KISS_FTR_H
#undef KISS_FFT__GUTS_H
#undef FIXED_POINT
#undef USE_SIMD
#undef kiss_fft_scalar
#define kiss_fft_scalar float
inline void free(void *ptr) { ::free(ptr); }
#include "ext/kiss_fft.c"
#include "ext/kiss_fftr.c"
}

namespace KissDouble {
#undef KISS_FFT_H
#undef KISS_FTR_H
#undef KISS_FFT__GUTS_H
#undef FIXED_POINT
#undef USE_SIMD
#undef kiss_fft_scalar
#define kiss_fft_scalar double
inline void free(void *ptr) { ::free(ptr); }
#include "ext/kiss_fft.c"
#include "ext/kiss_fftr.c"
}

namespace Vamp {

void
FFT::forward(unsigned int un,
	     const double *ri, const double *ii,
	     double *ro, double *io)
{
    int n(un);
    KissDouble::kiss_fft_cfg c = KissDouble::kiss_fft_alloc(n, false, 0, 0);
    KissDouble::kiss_fft_cpx *in = new KissDouble::kiss_fft_cpx[n];
    KissDouble::kiss_fft_cpx *out = new KissDouble::kiss_fft_cpx[n];
    for (int i = 0; i < n; ++i) {
        in[i].r = ri[i];
        in[i].i = 0;
    }
    if (ii) {
        for (int i = 0; i < n; ++i) {
            in[i].i = ii[i];
        }
    }
    kiss_fft(c, in, out);
    for (int i = 0; i < n; ++i) {
        ro[i] = out[i].r;
        io[i] = out[i].i;
    }
    KissDouble::kiss_fft_free(c);
    delete[] in;
    delete[] out;
}

void
FFT::inverse(unsigned int un,
	     const double *ri, const double *ii,
	     double *ro, double *io)
{
    int n(un);
    KissDouble::kiss_fft_cfg c = KissDouble::kiss_fft_alloc(n, true, 0, 0);
    KissDouble::kiss_fft_cpx *in = new KissDouble::kiss_fft_cpx[n];
    KissDouble::kiss_fft_cpx *out = new KissDouble::kiss_fft_cpx[n];
    for (int i = 0; i < n; ++i) {
        in[i].r = ri[i];
        in[i].i = 0;
    }
    if (ii) {
        for (int i = 0; i < n; ++i) {
            in[i].i = ii[i];
        }
    }
    kiss_fft(c, in, out);
    double scale = 1.0 / double(n);
    for (int i = 0; i < n; ++i) {
        ro[i] = out[i].r * scale;
        io[i] = out[i].i * scale;
    }
    KissDouble::kiss_fft_free(c);
    delete[] in;
    delete[] out;
    KissDouble::kiss_fft_free(c);
}

class FFTReal::D
{
public:
    
    D(int n) :
        m_n(n),
        m_cf(KissSingle::kiss_fftr_alloc(n, false, 0, 0)),
        m_ci(KissSingle::kiss_fftr_alloc(n, true, 0, 0)),
        m_freq(new KissSingle::kiss_fft_cpx[n/2+1]) { }

    ~D() {
        KissSingle::kiss_fftr_free(m_cf);
        KissSingle::kiss_fftr_free(m_ci);
        delete[] m_freq;
    }

    void forward(const float *ri, float *co) {
        KissSingle::kiss_fftr(m_cf, ri, m_freq);
        int hs = m_n/2 + 1;
        for (int i = 0; i < hs; ++i) {
            co[i*2] = m_freq[i].r;
            co[i*2+1] = m_freq[i].i;
        }
    }

    void inverse(const float *ci, float *ro) {
        int hs = m_n/2 + 1;
        for (int i = 0; i < hs; ++i) {
            m_freq[i].r = ci[i*2];
            m_freq[i].i = ci[i*2+1];
        }
        KissSingle::kiss_fftri(m_ci, m_freq, ro);
        double scale = 1.0 / double(m_n);
        for (int i = 0; i < m_n; ++i) {
            ro[i] *= scale;
        }
    }
    
private:
    int m_n;
    KissSingle::kiss_fftr_cfg m_cf;
    KissSingle::kiss_fftr_cfg m_ci;
    KissSingle::kiss_fft_cpx *m_freq;
};

FFTReal::FFTReal(unsigned int n) :
    m_d(new D(n))
{
}

FFTReal::~FFTReal()
{
    delete m_d;
}

void
FFTReal::forward(const float *ri, float *co)
{
    m_d->forward(ri, co);
}

void
FFTReal::inverse(const float *ci, float *ro)
{
    m_d->inverse(ci, ro);
}

}

_VAMP_SDK_PLUGSPACE_END(FFT.cpp)

