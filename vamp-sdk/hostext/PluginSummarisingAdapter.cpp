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

#include "PluginSummarisingAdapter.h"

#include <map>
#include <cmath>

namespace Vamp {

namespace HostExt {

class PluginSummarisingAdapter::Impl
{
public:
    Impl(Plugin *plugin, float inputSampleRate);
    ~Impl();

    FeatureSet process(const float *const *inputBuffers, RealTime timestamp);
    FeatureSet getRemainingFeatures();

    void setSummarySegmentBoundaries(const SegmentBoundaries &);

    FeatureList getSummaryForOutput(int output,
                                    SummaryType type,
                                    AveragingMethod avg);

    FeatureSet getSummaryForAllOutputs(SummaryType type,
                                       AveragingMethod avg);

protected:
    Plugin *m_plugin;
    float m_inputSampleRate;

    SegmentBoundaries m_boundaries;
    
    typedef std::vector<float> ValueList;
    typedef std::map<int, ValueList> BinValueMap;
    typedef std::vector<RealTime> DurationList;
    
    struct OutputAccumulator {
        int count;
        BinValueMap values; // bin number -> values ordered by time
        DurationList durations;
        OutputAccumulator() : count(0), values(), durations() { }
    };

    typedef std::map<int, OutputAccumulator> OutputAccumulatorMap;
    OutputAccumulatorMap m_accumulators; // output number -> accumulator

    typedef std::map<int, RealTime> OutputTimestampMap;
    OutputTimestampMap m_prevTimestamps; // output number -> timestamp

    struct OutputBinSummary {

        int count;

        // extents
        float minimum;
        float maximum;
        float sum;

        // sample-average results
        float median;
        float mode;
        float variance;

        // continuous-time average results
        float median_c;
        float mode_c;
        float mean_c;
        float variance_c;
    };

    typedef std::map<int, OutputBinSummary> OutputSummary;
    typedef std::map<RealTime, OutputSummary> SummarySegmentMap;
    typedef std::map<int, SummarySegmentMap> OutputSummarySegmentMap;

    OutputSummarySegmentMap m_summaries;

    RealTime m_lastTimestamp;
    RealTime m_prevDuration;

    void accumulate(const FeatureSet &fs, RealTime, bool final);
    void accumulate(int output, const Feature &f, RealTime, bool final);
    void reduce();
};
    
PluginSummarisingAdapter::PluginSummarisingAdapter(Plugin *plugin) :
    PluginWrapper(plugin)
{
    m_impl = new Impl(plugin, m_inputSampleRate);
}

PluginSummarisingAdapter::~PluginSummarisingAdapter()
{
    delete m_impl;
}

Plugin::FeatureSet
PluginSummarisingAdapter::process(const float *const *inputBuffers, RealTime timestamp)
{
    return m_impl->process(inputBuffers, timestamp);
}

Plugin::FeatureSet
PluginSummarisingAdapter::getRemainingFeatures()
{
    return m_impl->getRemainingFeatures();
}

Plugin::FeatureList
PluginSummarisingAdapter::getSummaryForOutput(int output,
                                              SummaryType type,
                                              AveragingMethod avg)
{
    return m_impl->getSummaryForOutput(output, type, avg);
}

Plugin::FeatureSet
PluginSummarisingAdapter::getSummaryForAllOutputs(SummaryType type,
                                                  AveragingMethod avg)
{
    return m_impl->getSummaryForAllOutputs(type, avg);
}

PluginSummarisingAdapter::Impl::Impl(Plugin *plugin, float inputSampleRate) :
    m_plugin(plugin),
    m_inputSampleRate(inputSampleRate)
{
}

PluginSummarisingAdapter::Impl::~Impl()
{
}

Plugin::FeatureSet
PluginSummarisingAdapter::Impl::process(const float *const *inputBuffers, RealTime timestamp)
{
    FeatureSet fs = m_plugin->process(inputBuffers, timestamp);
    accumulate(fs, timestamp, false);
    m_lastTimestamp = timestamp;
    return fs;
}

Plugin::FeatureSet
PluginSummarisingAdapter::Impl::getRemainingFeatures()
{
    FeatureSet fs = m_plugin->getRemainingFeatures();
    accumulate(fs, m_lastTimestamp, true);
    reduce();
    return fs;
}

Plugin::FeatureList
PluginSummarisingAdapter::Impl::getSummaryForOutput(int output,
                                                    SummaryType type,
                                                    AveragingMethod avg)
{
    bool continuous = (avg == ContinuousTimeAverage);

    //!!! need to ensure that this is only called after processing is
    //!!! complete (at the moment processing is "completed" in the
    //!!! call to getRemainingFeatures, but we don't want to require
    //!!! the host to call getRemainingFeatures at all unless it
    //!!! actually wants the raw features too -- calling getSummary
    //!!! should be enough -- we do need to ensure that all data has
    //!!! been processed though!)
    FeatureList fl;
    for (SummarySegmentMap::const_iterator i = m_summaries[output].begin();
         i != m_summaries[output].end(); ++i) {

        Feature f;
        f.hasTimestamp = true;
        f.timestamp = i->first;
        f.hasDuration = false;

        for (OutputSummary::const_iterator j = i->second.begin();
             j != i->second.end(); ++j) {

            // these will be ordered by bin number, and no bin numbers
            // will be missing except at the end (because of the way
            // the accumulators were initially filled in accumulate())

            const OutputBinSummary &summary = j->second;
            float result = 0.f;

            switch (type) {

            case Minimum:
                result = summary.minimum;
                break;

            case Maximum:
                result = summary.maximum;
                break;

            case Mean:
                if (continuous) {
                    result = summary.mean_c;
                } else if (summary.count) {
                    result = summary.sum / summary.count;
                }
                break;

            case Median:
                if (continuous) result = summary.median_c;
                else result = summary.median;
                break;

            case Mode:
                if (continuous) result = summary.mode_c;
                else result = summary.mode;
                break;

            case Sum:
                result = summary.sum;
                break;

            case Variance:
                if (continuous) result = summary.variance_c;
                else result = summary.variance;
                break;

            case StandardDeviation:
                if (continuous) result = sqrtf(summary.variance_c);
                else result = sqrtf(summary.variance);
                break;

            case Count:
                result = summary.count;
                break;

            default:
                break;
            }
            
            f.values.push_back(result);
        }

        fl.push_back(f);
    }
    return fl;
}

Plugin::FeatureSet
PluginSummarisingAdapter::Impl::getSummaryForAllOutputs(SummaryType type,
                                                        AveragingMethod avg)
{
    FeatureSet fs;
    for (OutputSummarySegmentMap::const_iterator i = m_summaries.begin();
         i != m_summaries.end(); ++i) {
        fs[i->first] = getSummaryForOutput(i->first, type, avg);
    }
    return fs;
}

void
PluginSummarisingAdapter::Impl::accumulate(const FeatureSet &fs,
                                           RealTime timestamp, 
                                           bool final)
{
    for (FeatureSet::const_iterator i = fs.begin(); i != fs.end(); ++i) {
        for (FeatureList::const_iterator j = i->second.begin();
             j != i->second.end(); ++j) {
            accumulate(i->first, *j, timestamp, final);
        }
    }
}

void
PluginSummarisingAdapter::Impl::accumulate(int output,
                                           const Feature &f,
                                           RealTime timestamp,
                                           bool final)
{
//!!! to do: use timestamp to determine which segment we're on

    m_accumulators[output].count++;

    if (m_prevDuration == RealTime::zeroTime) {
        if (m_prevTimestamps.find(output) != m_prevTimestamps.end()) {
            m_prevDuration = timestamp - m_prevTimestamps[output];
        }
    }
    if (m_prevDuration != RealTime::zeroTime ||
        !m_accumulators[output].durations.empty()) {
        // ... i.e. if not first result.  We don't push a duration
        // when we process the first result; then the duration we push
        // each time is that for the result before the one we're
        // processing, and we push an extra one at the end.  This
        // permits handling the case where the feature itself doesn't
        // have a duration field, and we have to calculate it from the
        // time to the following feature.  The net effect is simply
        // that values[n] and durations[n] refer to the same result.
        m_accumulators[output].durations.push_back(m_prevDuration);
    }

    m_prevTimestamps[output] = timestamp;

    for (int i = 0; i < int(f.values.size()); ++i) {
        m_accumulators[output].values[i].push_back(f.values[i]);
    }

    if (final) {
        RealTime finalDuration;
        if (f.hasDuration) finalDuration = f.duration;
        m_accumulators[output].durations.push_back(finalDuration);
    }

    if (f.hasDuration) m_prevDuration = f.duration;
    else m_prevDuration = RealTime::zeroTime;
}

struct ValueDurationFloatPair
{
    float value;
    float duration;

    ValueDurationFloatPair() : value(0), duration(0) { }
    ValueDurationFloatPair(float v, float d) : value(v), duration(d) { }
    ValueDurationFloatPair &operator=(const ValueDurationFloatPair &p) {
        value = p.value;
        duration = p.duration;
        return *this;
    }
    bool operator<(const ValueDurationFloatPair &p) const {
        return value < p.value;
    }
};

static double toSec(const RealTime &r)
{
    return r.sec + double(r.nsec) / 1000000000.0;
}

void
PluginSummarisingAdapter::Impl::reduce()
{
    RealTime segmentStart = RealTime::zeroTime; //!!!

    for (OutputAccumulatorMap::iterator i = m_accumulators.begin();
         i != m_accumulators.end(); ++i) {

        int output = i->first;
        OutputAccumulator &accumulator = i->second;

        double totalDuration;
        for (int k = 0; k < accumulator.durations.size(); ++k) {
            totalDuration += toSec(accumulator.durations[k]);
        }

        for (BinValueMap::iterator j = accumulator.values.begin();
             j != accumulator.values.end(); ++j) {

            // work on all values over time for a single bin

            int bin = j->first;
            const ValueList &values = j->second;
            const DurationList &durations = accumulator.durations;

            OutputBinSummary summary;

            summary.count = accumulator.count;

            summary.minimum = 0.f;
            summary.maximum = 0.f;

            summary.median = 0.f;
            summary.mode = 0.f;
            summary.sum = 0.f;
            summary.variance = 0.f;

            summary.median_c = 0.f;
            summary.mode_c = 0.f;
            summary.mean_c = 0.f;
            summary.variance_c = 0.f;

            if (summary.count == 0 || values.empty()) continue;

            int sz = values.size();

            if (sz != durations.size()) {
                std::cerr << "WARNING: sz " << sz << " != durations.size() "
                          << durations.size() << std::endl;
//                while (durations.size() < sz) {
//                    durations.push_back(RealTime::zeroTime);
//                }
//!!! then what?
            }

            std::vector<ValueDurationFloatPair> valvec;

            for (int k = 0; k < sz; ++k) {
                valvec.push_back(ValueDurationFloatPair(values[k],
                                                        toSec(durations[k])));
            }

            std::sort(valvec.begin(), valvec.end());

            summary.minimum = valvec[0].value;
            summary.maximum = valvec[sz-1].value;

            if (sz % 2 == 1) {
                summary.median = valvec[sz/2].value;
            } else {
                summary.median = (valvec[sz/2].value + valvec[sz/2 + 1].value) / 2;
            }
            
            double duracc = 0.0;
            summary.median_c = valvec[sz-1].value;

            for (int k = 0; k < sz; ++k) {
                duracc += valvec[k].duration;
                if (duracc > totalDuration/2) {
                    summary.median_c = valvec[k].value;
                    break;
                }
            }
                
            std::map<float, int> distribution;

            for (int k = 0; k < sz; ++k) {
                summary.sum += values[k];
                distribution[values[k]] += 1;
            }

            int md = 0;

            for (std::map<float, int>::iterator di = distribution.begin();
                 di != distribution.end(); ++di) {
                if (di->second > md) {
                    md = di->second;
                    summary.mode = di->first;
                }
            }

            distribution.clear();

            //!!! we want to omit this bit if the features all have
            //!!! equal duration (and set mode_c equal to mode instead)
            
            std::map<float, double> distribution_c;

            for (int k = 0; k < sz; ++k) {
                distribution_c[values[k]] += toSec(durations[k]);
            }

            double mrd = 0.0;

            for (std::map<float, double>::iterator di = distribution_c.begin();
                 di != distribution_c.end(); ++di) {
                if (di->second > mrd) {
                    mrd = di->second;
                    summary.mode_c = di->first;
                }
            }

            distribution_c.clear();

            if (totalDuration > 0.0) {

                double sum_c = 0.0;

                for (int k = 0; k < sz; ++k) {
                    double value = values[k] * toSec(durations[k]);
                    sum_c += value;
                }
                
                summary.mean_c = sum_c / totalDuration;

                for (int k = 0; k < sz; ++k) {
                    double value = values[k] * toSec(durations[k]);
                    summary.variance_c +=
                        (value - summary.mean_c) * (value - summary.mean_c);
                }

                summary.variance_c /= summary.count;
            }

            //!!! still to handle: median_c

            float mean = summary.sum / summary.count;

            for (int k = 0; k < sz; ++k) {
                summary.variance += (values[k] - mean) * (values[k] - mean);
            }
            summary.variance /= summary.count;

            m_summaries[output][segmentStart][bin] = summary;
        }
    }

    m_accumulators.clear();
}


}

}

