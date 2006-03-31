/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Vamp

    An API for audio analysis and feature extraction plugins.

    Centre for Digital Music, Queen Mary, University of London.
    Copyright 2006 Chris Cannam.
  
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

#ifndef VAMP_HEADER_INCLUDED
#define VAMP_HEADER_INCLUDED

/*
 * C language API for Vamp plugins.
 * 
 * This is the formal plugin API for Vamp.  Plugin authors may prefer
 * to use the C++ classes defined in the sdk directory, instead of
 * using this API directly.  There is an adapter class provided that
 * makes C++ plugins available using this C API with relatively little
 * work.  See the example plugins in the examples directory.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _VampParameterDescriptor
{
    const char *name;
    const char *description;
    const char *unit;
    float minValue;
    float maxValue;
    float defaultValue;
    int isQuantized;
    float quantizeStep;

} VampParameterDescriptor;

typedef enum
{
    vampOneSamplePerStep,
    vampFixedSampleRate,
    vampVariableSampleRate

} VampSampleType;

typedef struct _VampOutputDescriptor
{
    const char *name;
    const char *description;
    const char *unit;
    int hasFixedValueCount;
    unsigned int valueCount;
    const char **valueNames;
    int hasKnownExtents;
    float minValue;
    float maxValue;
    int isQuantized;
    float quantizeStep;
    VampSampleType sampleType;
    float sampleRate;

} VampOutputDescriptor;

typedef struct _VampFeature
{
    int hasTimestamp;
    int sec;
    int nsec;
    unsigned int valueCount;
    float *values;
    char *label;

} VampFeature;

typedef struct _VampFeatureList
{
    unsigned int featureCount;
    VampFeature *features;

} VampFeatureList;

typedef enum
{
    vampTimeDomain,
    vampFrequencyDomain

} VampInputDomain;

typedef void *VampPluginHandle;

typedef struct _VampPluginDescriptor
{
    const char *name;
    const char *description;
    const char *maker;
    int pluginVersion;
    const char *copyright;
    unsigned int parameterCount;
    const VampParameterDescriptor **parameters;
    unsigned int programCount;
    const char **programs;
    VampInputDomain inputDomain;
    
    VampPluginHandle (*instantiate)(const struct _VampPluginDescriptor *,
                                   float inputSampleRate);

    void (*cleanup)(VampPluginHandle);

    int (*initialise)(VampPluginHandle,
                      unsigned int inputChannels,
                      unsigned int stepSize, 
                      unsigned int blockSize);

    void (*reset)(VampPluginHandle);

    float (*getParameter)(VampPluginHandle, int);
    void  (*setParameter)(VampPluginHandle, int, float);

    unsigned int (*getCurrentProgram)(VampPluginHandle);
    void  (*selectProgram)(VampPluginHandle, unsigned int);
    
    unsigned int (*getPreferredStepSize)(VampPluginHandle);
    unsigned int (*getPreferredBlockSize)(VampPluginHandle);
    unsigned int (*getMinChannelCount)(VampPluginHandle);
    unsigned int (*getMaxChannelCount)(VampPluginHandle);

    unsigned int (*getOutputCount)(VampPluginHandle);
    VampOutputDescriptor *(*getOutputDescriptor)(VampPluginHandle,
                                                unsigned int);
    void (*releaseOutputDescriptor)(VampOutputDescriptor *);

    VampFeatureList **(*process)(VampPluginHandle,
                                float **inputBuffers,
                                int sec,
                                int nsec);
    VampFeatureList **(*getRemainingFeatures)(VampPluginHandle);
    void (*releaseFeatureSet)(VampFeatureList **);

} VampPluginDescriptor;

const VampPluginDescriptor *vampGetPluginDescriptor(unsigned int index);

typedef const VampPluginDescriptor *(*VampGetPluginDescriptorFunction)(unsigned int);

#ifdef __cplusplus
}
#endif

#endif
