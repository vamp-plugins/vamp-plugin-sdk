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

    FeatureList getSummaryForOutput(int output, SummaryType type);
    FeatureSet getSummaryForAllOutputs(SummaryType type);

protected:
    Plugin *m_plugin;

    SegmentBoundaries m_boundaries;
    
    typedef std::vector<float> ValueList;
    typedef std::map<int, ValueList> BinValueMap;
    
    struct OutputAccumulator {
        int count;
        BinValueMap values;
    };

    typedef std::map<int, OutputAccumulator> OutputAccumulatorMap;
    OutputAccumulatorMap m_accumulators;

    struct OutputBinSummary {
        float minimum;
        float maximum;
        float median;
        float mode;
        float sum;
        float variance;
        int count;
    };

    typedef std::map<int, OutputBinSummary> OutputSummary;
    typedef std::map<RealTime, OutputSummary> SummarySegmentMap;
    typedef std::map<int, SummarySegmentMap> OutputSummarySegmentMap;

    OutputSummarySegmentMap m_summaries;

    RealTime m_lastTimestamp;

    void accumulate(const FeatureSet &fs, RealTime);
    void accumulate(int output, const Feature &f, RealTime);
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
PluginSummarisingAdapter::getSummaryForOutput(int output, SummaryType type)
{
    return m_impl->getSummaryForOutput(output, type);
}

Plugin::FeatureSet
PluginSummarisingAdapter::getSummaryForAllOutputs(SummaryType type)
{
    return m_impl->getSummaryForAllOutputs(type);
}

PluginSummarisingAdapter::Impl::Impl(Plugin *plugin, float inputSampleRate) :
    m_plugin(plugin)
{
}

PluginSummarisingAdapter::Impl::~Impl()
{
}

Plugin::FeatureSet
PluginSummarisingAdapter::Impl::process(const float *const *inputBuffers, RealTime timestamp)
{
    FeatureSet fs = m_plugin->process(inputBuffers, timestamp);
    accumulate(fs, timestamp);
    m_lastTimestamp = timestamp;
    return fs;
}

Plugin::FeatureSet
PluginSummarisingAdapter::Impl::getRemainingFeatures()
{
    FeatureSet fs = m_plugin->getRemainingFeatures();
    accumulate(fs, m_lastTimestamp);
    reduce();
    return fs;
}

Plugin::FeatureList
PluginSummarisingAdapter::Impl::getSummaryForOutput(int output, SummaryType type)
{
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
                if (summary.count) {
                    result = summary.sum / summary.count;
                }
                break;

            case Median:
                result = summary.median;
                break;

            case Mode:
                result = summary.mode;
                break;

            case Sum:
                result = summary.sum;
                break;

            case Variance:
                result = summary.variance;
                break;

            case StandardDeviation:
                result = sqrtf(summary.variance);
                break;

            case Count:
                result = summary.count;
                break;
            }
        }

        fl.push_back(f);
    }
    return fl;
}

Plugin::FeatureSet
PluginSummarisingAdapter::Impl::getSummaryForAllOutputs(SummaryType type)
{
    FeatureSet fs;
    for (OutputSummarySegmentMap::const_iterator i = m_summaries.begin();
         i != m_summaries.end(); ++i) {
        fs[i->first] = getSummaryForOutput(i->first, type);
    }
    return fs;
}

void
PluginSummarisingAdapter::Impl::accumulate(const FeatureSet &fs,
                                           RealTime timestamp)
{
    for (FeatureSet::const_iterator i = fs.begin(); i != fs.end(); ++i) {
        for (FeatureList::const_iterator j = i->second.begin();
             j != i->second.end(); ++j) {
            accumulate(i->first, *j, timestamp);
        }
    }
}

void
PluginSummarisingAdapter::Impl::accumulate(int output,
                                           const Feature &f,
                                           RealTime timestamp)
{
//!!! use timestamp to determine which segment we're on
    m_accumulators[output].count++;
    for (int i = 0; i < int(f.values.size()); ++i) {
        m_accumulators[output].values[i].push_back(f.values[i]);
    }
}

void
PluginSummarisingAdapter::Impl::reduce()
{
    RealTime segmentStart = RealTime::zeroTime; //!!!

    for (OutputAccumulatorMap::iterator i = m_accumulators.begin();
         i != m_accumulators.end(); ++i) {

        int output = i->first;
        OutputAccumulator &accumulator = i->second;

        for (BinValueMap::iterator j = accumulator.values.begin();
             j != accumulator.values.end(); ++j) {

            int bin = j->first;
            ValueList &values = j->second;

            OutputBinSummary summary;
            summary.minimum = 0.f;
            summary.maximum = 0.f;
            summary.median = 0.f;
            summary.mode = 0.f;
            summary.sum = 0.f;
            summary.variance = 0.f;
            summary.count = accumulator.count;
            if (summary.count == 0 || values.empty()) continue;

            std::sort(values.begin(), values.end());
            int sz = values.size();

            summary.minimum = values[0];
            summary.maximum = values[sz-1];

            if (sz % 2 == 1) {
                summary.median = values[sz/2];
            } else {
                summary.median = (values[sz/2] + values[sz/2 + 1]) / 2;
            }

            std::map<float, int> distribution;

            for (int k = 0; k < sz; ++k) {
                summary.sum += values[k];
                ++distribution[values[k]];
            }

            int md = 0;

            //!!! I don't like this.  Really the mode should be the
            //!!! value that spans the longest period of time, not the
            //!!! one that appears in the largest number of distinct
            //!!! features.  I suppose that a median by time rather
            //!!! than number of features would also be useful.
            
            for (std::map<float, int>::iterator di = distribution.begin();
                 di != distribution.end(); ++di) {
                if (di->second > md) {
                    md = di->second;
                    summary.mode = di->first;
                }
            }

            distribution.clear();

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

