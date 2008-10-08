/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Vamp

    An API for audio analysis and feature extraction plugins.

    Centre for Digital Music, Queen Mary, University of London.
    Copyright 2006-2008 Chris Cannam and QMUL.
  
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

#include "FixedTempoEstimator.h"

using std::string;
using std::vector;
using std::cerr;
using std::endl;

using Vamp::RealTime;

#include <cmath>


FixedTempoEstimator::FixedTempoEstimator(float inputSampleRate) :
    Plugin(inputSampleRate),
    m_stepSize(0),
    m_blockSize(0),
    m_priorMagnitudes(0),
    m_df(0)
{
}

FixedTempoEstimator::~FixedTempoEstimator()
{
    delete[] m_priorMagnitudes;
    delete[] m_df;
}

string
FixedTempoEstimator::getIdentifier() const
{
    return "fixedtempo";
}

string
FixedTempoEstimator::getName() const
{
    return "Simple Fixed Tempo Estimator";
}

string
FixedTempoEstimator::getDescription() const
{
    return "Study a short section of audio and estimate its tempo, assuming the tempo is constant";
}

string
FixedTempoEstimator::getMaker() const
{
    return "Vamp SDK Example Plugins";
}

int
FixedTempoEstimator::getPluginVersion() const
{
    return 1;
}

string
FixedTempoEstimator::getCopyright() const
{
    return "Code copyright 2008 Queen Mary, University of London.  Freely redistributable (BSD license)";
}

size_t
FixedTempoEstimator::getPreferredStepSize() const
{
    return 0;
}

size_t
FixedTempoEstimator::getPreferredBlockSize() const
{
    return 128;
}

bool
FixedTempoEstimator::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
    if (channels < getMinChannelCount() ||
	channels > getMaxChannelCount()) return false;

    m_stepSize = stepSize;
    m_blockSize = blockSize;

    float dfLengthSecs = 8.f;
    m_dfsize = (dfLengthSecs * m_inputSampleRate) / m_stepSize;

    m_priorMagnitudes = new float[m_blockSize/2];
    m_df = new float[m_dfsize];

    for (size_t i = 0; i < m_blockSize/2; ++i) {
        m_priorMagnitudes[i] = 0.f;
    }
    for (size_t i = 0; i < m_dfsize; ++i) {
        m_df[i] = 0.f;
    }

    m_n = 0;

    return true;
}

void
FixedTempoEstimator::reset()
{
    std::cerr << "FixedTempoEstimator: reset called" << std::endl;

    if (!m_priorMagnitudes) return;

    std::cerr << "FixedTempoEstimator: resetting" << std::endl;

    for (size_t i = 0; i < m_blockSize/2; ++i) {
        m_priorMagnitudes[i] = 0.f;
    }
    for (size_t i = 0; i < m_dfsize; ++i) {
        m_df[i] = 0.f;
    }

    m_n = 0;

    m_start = RealTime::zeroTime;
    m_lasttime = RealTime::zeroTime;
}

FixedTempoEstimator::ParameterList
FixedTempoEstimator::getParameterDescriptors() const
{
    ParameterList list;
    return list;
}

float
FixedTempoEstimator::getParameter(std::string id) const
{
    return 0.f;
}

void
FixedTempoEstimator::setParameter(std::string id, float value)
{
}

FixedTempoEstimator::OutputList
FixedTempoEstimator::getOutputDescriptors() const
{
    OutputList list;

    OutputDescriptor d;
    d.identifier = "tempo";
    d.name = "Tempo";
    d.description = "Estimated tempo";
    d.unit = "bpm";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.sampleRate = m_inputSampleRate;
    d.hasDuration = true; // our returned tempo spans a certain range
    list.push_back(d);

    d.identifier = "detectionfunction";
    d.name = "Detection Function";
    d.description = "Onset detection function";
    d.unit = "";
    d.hasFixedBinCount = 1;
    d.binCount = 1;
    d.hasKnownExtents = true;
    d.minValue = 0.0;
    d.maxValue = 1.0;
    d.isQuantized = false;
    d.quantizeStep = 0.0;
    d.sampleType = OutputDescriptor::FixedSampleRate;
    if (m_stepSize) {
        d.sampleRate = m_inputSampleRate / m_stepSize;
    } else {
        d.sampleRate = m_inputSampleRate / (getPreferredBlockSize()/2);
    }
    d.hasDuration = false;
    list.push_back(d);

    d.identifier = "acf";
    d.name = "Autocorrelation Function";
    d.description = "Autocorrelation of onset detection function";
    d.hasKnownExtents = false;
    list.push_back(d);

    d.identifier = "filtered_acf";
    d.name = "Filtered Autocorrelation";
    d.description = "Filtered autocorrelation of onset detection function";
    list.push_back(d);

    return list;
}

FixedTempoEstimator::FeatureSet
FixedTempoEstimator::process(const float *const *inputBuffers, RealTime ts)
{
    FeatureSet fs;

    if (m_stepSize == 0) {
	cerr << "ERROR: FixedTempoEstimator::process: "
	     << "FixedTempoEstimator has not been initialised"
	     << endl;
	return fs;
    }

    if (m_n < m_dfsize) std::cerr << "m_n = " << m_n << std::endl;

    if (m_n == 0) m_start = ts;
    m_lasttime = ts;

    if (m_n == m_dfsize) {
        fs = calculateFeatures();
        ++m_n;
        return fs;
    }

    if (m_n > m_dfsize) return FeatureSet();

    int count = 0;

    for (size_t i = 1; i < m_blockSize/2; ++i) {

        float real = inputBuffers[0][i*2];
        float imag = inputBuffers[0][i*2 + 1];

        float sqrmag = real * real + imag * imag;

        if (m_priorMagnitudes[i] > 0.f) {
            float diff = 10.f * log10f(sqrmag / m_priorMagnitudes[i]);
            if (diff >= 3.f) ++count;
        }

        m_priorMagnitudes[i] = sqrmag;
    }

    m_df[m_n] = float(count) / float(m_blockSize/2);
    ++m_n;
    return fs;
}

FixedTempoEstimator::FeatureSet
FixedTempoEstimator::getRemainingFeatures()
{
    FeatureSet fs;
    if (m_n > m_dfsize) return fs;
    fs = calculateFeatures();
    ++m_n;
    return fs;
}

float
FixedTempoEstimator::lag2tempo(int lag) {
    return 60.f / ((lag * m_stepSize) / m_inputSampleRate);
}

FixedTempoEstimator::FeatureSet
FixedTempoEstimator::calculateFeatures()
{
    FeatureSet fs;
    Feature feature;
    feature.hasTimestamp = true;
    feature.hasDuration = false;
    feature.label = "";
    feature.values.clear();
    feature.values.push_back(0.f);

    char buffer[20];
    
    if (m_n < m_dfsize / 4) return fs; // not enough data (perhaps we should return the duration of the input as the "estimated" beat length?)

    std::cerr << "FixedTempoEstimator::calculateTempo: m_n = " << m_n << std::endl;
    
    int n = m_n;
    float *f = m_df;

    for (int i = 0; i < n; ++i) {
        feature.timestamp = RealTime::frame2RealTime(i * m_stepSize,
                                                     m_inputSampleRate);
        std::cerr << "step = " << m_stepSize << ", timestamp = " << feature.timestamp << std::endl;
        feature.values[0] = f[i];
        feature.label = "";
        fs[1].push_back(feature);
    }

    float *r = new float[n/2];
    for (int i = 0; i < n/2; ++i) r[i] = 0.f;

    int minlag = 10;

    for (int i = 0; i < n/2; ++i) {
        for (int j = i; j < n-1; ++j) {
            r[i] += f[j] * f[j - i];
        }
        r[i] /= n - i - 1;
    }

    for (int i = 0; i < n/2; ++i) {
        feature.timestamp = RealTime::frame2RealTime(i * m_stepSize,
                                                     m_inputSampleRate);
        feature.values[0] = r[i];
        sprintf(buffer, "%f bpm", lag2tempo(i));
        feature.label = buffer;
        fs[2].push_back(feature);
    }

    float max = 0.f;
    int maxindex = 0;

    std::cerr << "n/2 = " << n/2 << std::endl;

    for (int i = minlag; i < n/2; ++i) {
        
        if (i == minlag || r[i] > max) {
            max = r[i];
            maxindex = i;
        }

        if (i == 0 || i == n/2-1) continue;

        if (r[i] > r[i-1] && r[i] > r[i+1]) {
            std::cerr << "peak at " << i << " (value=" << r[i] << ", tempo would be " << lag2tempo(i) << ")" << std::endl;
        }
    }

    std::cerr << "overall max at " << maxindex << " (value=" << max << ")" << std::endl;
    
    float tempo = lag2tempo(maxindex);

    std::cerr << "provisional tempo = " << tempo << std::endl;

    float t0 = 60.f;
    float t1 = 180.f;

    int p0 = ((60.f / t1) * m_inputSampleRate) / m_stepSize;
    int p1 = ((60.f / t0) * m_inputSampleRate) / m_stepSize;

    std::cerr << "p0 = " << p0 << ", p1 = " << p1 << std::endl;

    int pc = p1 - p0 + 1;
    std::cerr << "pc = " << pc << std::endl;
//    float *filtered = new float[pc];
//    for (int i = 0; i < pc; ++i) filtered[i] = 0.f;

    int maxpi = 0;
    float maxp = 0.f;

    for (int i = p0; i <= p1; ++i) {

//        int fi = i - p0;

        float filtered = 0.f;
        
        for (int j = 1; j <= (n/2)/p1; ++j) {
            std::cerr << "j = " << j << ", i = " << i << std::endl;
            filtered += r[i * j];
        }

        if (i == p0 || filtered > maxp) {
            maxp = filtered;
            maxpi = i;
        }

        feature.timestamp = RealTime::frame2RealTime(i * m_stepSize,
                                                     m_inputSampleRate);
        feature.values[0] = filtered;
        sprintf(buffer, "%f bpm", lag2tempo(i));
        feature.label = buffer;
        fs[3].push_back(feature);
    }

    std::cerr << "maxpi = " << maxpi << " for tempo " << lag2tempo(maxpi) << " (value = " << maxp << ")" << std::endl;
    
    tempo = lag2tempo(maxpi);

    delete[] r;

    feature.hasTimestamp = true;
    feature.timestamp = m_start;
    
    feature.hasDuration = true;
    feature.duration = m_lasttime - m_start;

    feature.values[0] = tempo;

    fs[0].push_back(feature);

    return fs;
}
