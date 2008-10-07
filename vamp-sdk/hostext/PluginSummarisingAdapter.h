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

#ifndef _VAMP_PLUGIN_SUMMARISING_ADAPTER_H_
#define _VAMP_PLUGIN_SUMMARISING_ADAPTER_H_

#include "PluginWrapper.h"

#include <set>

namespace Vamp {

namespace HostExt {

/**
 * \class PluginSummarisingAdapter PluginSummarisingAdapter.h <vamp-sdk/hostext/PluginSummarisingAdapter.h>
 *
 * \note This class was introduced in version 1.1 of the Vamp plugin SDK.
 */

class PluginSummarisingAdapter : public PluginWrapper
{
public:
    PluginSummarisingAdapter(Plugin *plugin); // I take ownership of plugin
    virtual ~PluginSummarisingAdapter();

    bool initialise(size_t channels, size_t stepSize, size_t blockSize);

    FeatureSet process(const float *const *inputBuffers, RealTime timestamp);
    FeatureSet getRemainingFeatures();

    typedef std::set<RealTime> SegmentBoundaries;
    void setSummarySegmentBoundaries(const SegmentBoundaries &);

    enum SummaryType {
        Minimum            = 0,
        Maximum            = 1,
        Mean               = 2,
        Median             = 3,
        Mode               = 4,
        Sum                = 5,
        Variance           = 6,
        StandardDeviation  = 7,
        Count              = 8,

        UnknownSummaryType = 999
    };

    /**
     * AveragingMethod indicates how the adapter should handle
     * average-based summaries of features whose results are not
     * equally spaced in time.
     *
     * If SampleAverage is specified, summary types based on averages
     * will be calculated by treating each result individually without
     * regard to its time: for example, the mean will be the sum of
     * all values divided by the number of values.
     *
     * If ContinuousTimeAverage is specified, each feature will be
     * considered to have a duration, either as specified in the
     * feature's duration field, or until the following feature: thus,
     * for example, the mean will be the sum of the products of values
     * and durations, divided by the total duration.
     *
     * Although SampleAverage is useful for many types of feature,
     * ContinuousTimeAverage is essential for some situations, for
     * example finding the result that spans the largest proportion of
     * the input given a feature that emits a new result only when the
     * value changes (the modal value integrated over time).
     */
    enum AveragingMethod {
        SampleAverage         = 0,
        ContinuousTimeAverage = 1,
    };

    FeatureList getSummaryForOutput(int output,
                                    SummaryType type,
                                    AveragingMethod method = SampleAverage);

    FeatureSet getSummaryForAllOutputs(SummaryType type,
                                       AveragingMethod method = SampleAverage);

protected:
    class Impl;
    Impl *m_impl;
};

}

}

#endif
