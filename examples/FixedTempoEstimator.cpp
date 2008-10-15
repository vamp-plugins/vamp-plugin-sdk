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
    m_df(0),
    m_r(0),
    m_fr(0),
    m_t(0),
    m_n(0)
{
}

FixedTempoEstimator::~FixedTempoEstimator()
{
    delete[] m_priorMagnitudes;
    delete[] m_df;
    delete[] m_r;
    delete[] m_fr;
    delete[] m_t;
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
    return 64;
}

size_t
FixedTempoEstimator::getPreferredBlockSize() const
{
    return 256;
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
    cerr << "FixedTempoEstimator: reset called" << endl;

    if (!m_priorMagnitudes) return;

    cerr << "FixedTempoEstimator: resetting" << endl;

    for (size_t i = 0; i < m_blockSize/2; ++i) {
        m_priorMagnitudes[i] = 0.f;
    }
    for (size_t i = 0; i < m_dfsize; ++i) {
        m_df[i] = 0.f;
    }

    delete[] m_r;
    m_r = 0;

    delete[] m_fr; 
    m_fr = 0;

    delete[] m_t; 
    m_t = 0;

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

static int TempoOutput = 0;
static int CandidatesOutput = 1;
static int DFOutput = 2;
static int ACFOutput = 3;
static int FilteredACFOutput = 4;

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

    d.identifier = "candidates";
    d.name = "Tempo candidates";
    d.description = "Possible tempo estimates, one per bin with the most likely in the first bin";
    d.unit = "bpm";
    d.hasFixedBinCount = false;
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
    d.unit = "r";
    list.push_back(d);

    d.identifier = "filtered_acf";
    d.name = "Filtered Autocorrelation";
    d.description = "Filtered autocorrelation of onset detection function";
    d.unit = "r";
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

//    if (m_n < m_dfsize) cerr << "m_n = " << m_n << endl;

    if (m_n == 0) m_start = ts;
    m_lasttime = ts;

    if (m_n == m_dfsize) {
        calculate();
        fs = assembleFeatures();
        ++m_n;
        return fs;
    }

    if (m_n > m_dfsize) return FeatureSet();

    float value = 0.f;

    bool print = (ts == RealTime::zeroTime);

    for (size_t i = 1; i < m_blockSize/2; ++i) {

        float real = inputBuffers[0][i*2];
        float imag = inputBuffers[0][i*2 + 1];

        float sqrmag = real * real + imag * imag;
        value += fabsf(sqrmag - m_priorMagnitudes[i]);

//        if (i == 1 && ts == RealTime::zeroTime) {
//            cerr << "First sqrmag: " << sqrmag << ", value = " << value << endl;
//        }

        m_priorMagnitudes[i] = sqrmag;
    }

    m_df[m_n] = value;

    ++m_n;
    return fs;
}

FixedTempoEstimator::FeatureSet
FixedTempoEstimator::getRemainingFeatures()
{
    FeatureSet fs;
    if (m_n > m_dfsize) return fs;
    calculate();
    fs = assembleFeatures();
    ++m_n;
    return fs;
}

float
FixedTempoEstimator::lag2tempo(int lag)
{
    return 60.f / ((lag * m_stepSize) / m_inputSampleRate);
}

int
FixedTempoEstimator::tempo2lag(float tempo)
{
    return ((60.f / tempo) * m_inputSampleRate) / m_stepSize;
}

void
FixedTempoEstimator::calculate()
{    
    cerr << "FixedTempoEstimator::calculate: m_n = " << m_n << endl;
    
    if (m_r) {
        cerr << "FixedTempoEstimator::calculate: calculation already happened?" << endl;
        return;
    }

    if (m_n < m_dfsize / 6) {
        cerr << "FixedTempoEstimator::calculate: Not enough data to go on (have " << m_n << ", want at least " << m_dfsize/4 << ")" << endl;
        return; // not enough data (perhaps we should return the duration of the input as the "estimated" beat length?)
    }

    int n = m_n;

    m_r = new float[n/2];
    m_fr = new float[n/2];
    m_t = new float[n/2];

    for (int i = 0; i < n/2; ++i) {
        m_r[i] = 0.f;
        m_fr[i] = 0.f;
        m_t[i] = lag2tempo(i);
    }

    for (int i = 0; i < n/2; ++i) {

        for (int j = i; j < n-1; ++j) {
            m_r[i] += m_df[j] * m_df[j - i];
        }

        m_r[i] /= n - i - 1;
    }

    for (int i = 1; i < n/2; ++i) {

        float weight = 1.f - fabsf(128.f - lag2tempo(i)) * 0.005;
        if (weight < 0.f) weight = 0.f;
        weight = weight * weight;

        cerr << "i = " << i << ": tempo = " << lag2tempo(i) << ", weight = " << weight << endl;

        m_fr[i] = m_r[i] * (1 + weight / 3.f);
    }

    float related[4] = { 2, 3, 4 };

    for (int i = 0; i < n/2; ++i) {

        if (i == 0 || i == n/2 - 1 ||
            !(m_fr[i] > m_fr[i-1] &&
              m_fr[i] >= m_fr[i+1])) {
            continue;
        }

        int div = 1;

        for (int j = 0; j < sizeof(related)/sizeof(related[0]); ++j) {

            int k0 = i * related[j];
            
            if (k0 > 1 && k0 < n/2 - 2) {

                int kmax = 0, kmin = 0;
                float kvmax = 0, kvmin = 0;

                for (int k = k0 - 2; k <= k0 + 2; ++k) {

                    if (k == k0 - 2 || m_r[k] > kvmax) {
                        kmax = k;
                        kvmax = m_r[k];
                    }

                    if (k == k0 - 2 || m_r[k] < kvmin) {
                        kmin = k;
                        kvmin = m_r[k];
                    }
                }

                if (m_r[kmax] > m_r[kmax-1] &&
                    m_r[kmax] > m_r[kmax+1] &&
                    kvmax > kvmin * 1.05) {

//                        cerr << "peak at " << i << " (val " << m_r[i] << ", tempo " << lag2tempo(i) << ") has sympathetic peak at " << kmax << " (val " << m_r[kmax] << " for relative tempo " << lag2tempo(kmax) * related[j] << ")" << endl;

                    m_t[i] = m_t[i] + lag2tempo(kmax) * related[j];
                    ++div;
                }
            }
        }

        m_t[i] /= div;
        
//        if (div > 1) {
//            cerr << "adjusting tempo from " << lag2tempo(i) << " to "
//                      << m_t[i] << endl;
//        }
    }

    int e = tempo2lag(60.f);
    int div = (n/2 - 1) / e;

//    cerr << "e = " << e << ", n/2 = " << n/2 << ", div = " << div << endl;
    if (div > 1) {
        for (int j = 2; j <= div && j <= 8; j *= 2) {
            for (int i = 1; i <= e; ++i) {
                m_fr[i] += m_fr[i * j] * (1.f / j);
            }
        }
    }

//        cerr << "i = " << i << ", (n/2 - 1)/i = " << (n/2 - 1)/i << ", sum = " << m_fr[i] << ", div = " << div << ", val = " << m_fr[i] / div << ", t = " << lag2tempo(i) << endl;


//    }

    cerr << "FixedTempoEstimator::calculate done" << endl;
}
    

FixedTempoEstimator::FeatureSet
FixedTempoEstimator::assembleFeatures()
{
    FeatureSet fs;
    if (!m_r) return fs; // No results

    Feature feature;
    feature.hasTimestamp = true;
    feature.hasDuration = false;
    feature.label = "";
    feature.values.clear();
    feature.values.push_back(0.f);

    char buffer[40];

    int n = m_n;

    for (int i = 0; i < n; ++i) {
        feature.timestamp = RealTime::frame2RealTime(i * m_stepSize,
                                                     m_inputSampleRate);
        feature.values[0] = m_df[i];
        feature.label = "";
        fs[DFOutput].push_back(feature);
    }

    for (int i = 1; i < n/2; ++i) {
        feature.timestamp = RealTime::frame2RealTime(i * m_stepSize,
                                                     m_inputSampleRate);
        feature.values[0] = m_r[i];
        sprintf(buffer, "%.1f bpm", lag2tempo(i));
        if (i == n/2-1) feature.label = "";
        else feature.label = buffer;
        fs[ACFOutput].push_back(feature);
    }

    float t0 = 60.f;
    float t1 = 180.f;

    int p0 = tempo2lag(t1);
    int p1 = tempo2lag(t0);

    cerr << "p0 = " << p0 << ", p1 = " << p1 << endl;

    int pc = p1 - p0 + 1;
//    cerr << "pc = " << pc << endl;

//    int maxpi = 0;
//    float maxp = 0.f;

    std::map<float, int> candidates;

    for (int i = p0; i <= p1 && i < n/2-1; ++i) {

        candidates[m_fr[i]] = i;

        feature.timestamp = RealTime::frame2RealTime(i * m_stepSize,
                                                     m_inputSampleRate);
        feature.values[0] = m_fr[i];
        sprintf(buffer, "%.1f bpm", lag2tempo(i));
        if (i == p1 || i == n/2-2) feature.label = "";
        else feature.label = buffer;
        fs[FilteredACFOutput].push_back(feature);
    }

//    cerr << "maxpi = " << maxpi << " for tempo " << lag2tempo(maxpi) << " (value = " << maxp << ")" << endl;

    if (candidates.empty()) {
        cerr << "No tempo candidates!" << endl;
        return fs;
    }

    feature.hasTimestamp = true;
    feature.timestamp = m_start;
    
    feature.hasDuration = true;
    feature.duration = m_lasttime - m_start;

    std::map<float, int>::const_iterator ci = candidates.end();
    --ci;
    int maxpi = ci->second;

    if (m_t[maxpi] > 0) {
        cerr << "*** Using adjusted tempo " << m_t[maxpi] << " instead of lag tempo " << lag2tempo(maxpi) << endl;
        feature.values[0] = m_t[maxpi];
    } else {
        // shouldn't happen -- it would imply that this high value was not a peak!
        feature.values[0] = lag2tempo(maxpi);
        cerr << "WARNING: No stored tempo for index " << maxpi << endl;
    }

    sprintf(buffer, "%.1f bpm", feature.values[0]);
    feature.label = buffer;

    fs[TempoOutput].push_back(feature);

    feature.values.clear();
    feature.label = "";

    while (feature.values.size() < 8) {
        if (m_t[ci->second] > 0) {
            feature.values.push_back(m_t[ci->second]);
        } else {
            feature.values.push_back(lag2tempo(ci->second));
        }
        if (ci == candidates.begin()) break;
        --ci;
    }

    fs[CandidatesOutput].push_back(feature);
    
    return fs;
}
