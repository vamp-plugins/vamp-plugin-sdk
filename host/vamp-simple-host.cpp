/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Vamp

    An API for audio analysis and feature extraction plugins.

    Centre for Digital Music, Queen Mary, University of London.
    Copyright 2006 Chris Cannam.
    FFT code from Don Cross's public domain FFT implementation.
  
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

#include "PluginHostAdapter.h"
#include "vamp.h"

#include <iostream>
#include <sndfile.h>

#include "system.h"

#include <cmath>

using std::cout;
using std::cerr;
using std::endl;
using std::string;

void printFeatures(int, int, int, Vamp::Plugin::FeatureSet);
void transformInput(float *, size_t);
void fft(unsigned int, bool, double *, double *, double *, double *);

/*
    A very simple Vamp plugin host.  Given the name of a plugin
    library and the name of a sound file on the command line, it loads
    the first plugin in the library and runs it on the sound file,
    dumping the plugin's first output to stdout.
*/

int main(int argc, char **argv)
{
    if (argc < 2 || argc > 4) {
        cerr << "Usage: " << argv[0] << " pluginlibrary.so[:plugin] [file.wav] [outputno]" << endl;
        return 2;
    }

    cerr << endl << argv[0] << ": Running..." << endl;

    string soname = argv[1];
    string plugname = "";
    string wavname;
    if (argc >= 3) wavname = argv[2];

    int sep = soname.find(":");
    if (sep >= 0 && sep < soname.length()) {
        plugname = soname.substr(sep + 1);
        soname = soname.substr(0, sep);
    }

    void *libraryHandle = DLOPEN(soname, RTLD_LAZY);

    if (!libraryHandle) {
        cerr << argv[0] << ": Failed to open plugin library " 
                  << soname << ": " << DLERROR() << endl;
        return 1;
    }

    cerr << argv[0] << ": Opened plugin library " << soname << endl;

    VampGetPluginDescriptorFunction fn = (VampGetPluginDescriptorFunction)
        DLSYM(libraryHandle, "vampGetPluginDescriptor");
    
    if (!fn) {
        cerr << argv[0] << ": No Vamp descriptor function in library "
                  << soname << endl;
        DLCLOSE(libraryHandle);
        return 1;
    }

    cerr << argv[0] << ": Found plugin descriptor function" << endl;

    int index = 0;
    int plugnumber = -1;
    const VampPluginDescriptor *descriptor = 0;

    while ((descriptor = fn(index))) {

        Vamp::PluginHostAdapter plugin(descriptor, 48000);
        cerr << argv[0] << ": Plugin " << (index+1)
                  << " is \"" << plugin.getName() << "\"" << endl;

        if (plugin.getName() == plugname) plugnumber = index;
        
        cerr << "(testing overlap...)" << endl;
        {
            Vamp::PluginHostAdapter otherPlugin(descriptor, 48000);
            cerr << "(other plugin reports min " << otherPlugin.getMinChannelCount() << " channels)" << endl;
        }

        ++index;
    }

    cerr << argv[0] << ": Done\n" << endl;

    if (wavname == "") {
        DLCLOSE(libraryHandle);
        return 0;
    }

    if (plugnumber < 0) {
        if (plugname != "") {
            cerr << "ERROR: No such plugin as " << plugname << " in library"
                 << endl;
            DLCLOSE(libraryHandle);
            return 0;
        } else {
            plugnumber = 0;
        }
    }

    descriptor = fn(plugnumber);
    if (!descriptor) {
        DLCLOSE(libraryHandle);
        return 0;
    }
    
    SNDFILE *sndfile;
    SF_INFO sfinfo;
    memset(&sfinfo, 0, sizeof(SF_INFO));

    sndfile = sf_open(wavname.c_str(), SFM_READ, &sfinfo);
    if (!sndfile) {
	cerr << "ERROR: Failed to open input file \"" << wavname << "\": "
	     << sf_strerror(sndfile) << endl;
        DLCLOSE(libraryHandle);
	return 1;
    }

    Vamp::PluginHostAdapter *plugin =
        new Vamp::PluginHostAdapter(descriptor, sfinfo.samplerate);

    cerr << "Running " << plugin->getName() << "..." << endl;

    int blockSize = plugin->getPreferredBlockSize();
    int stepSize = plugin->getPreferredStepSize();

    cerr << "Preferred block size = " << blockSize << ", step size = "
              << stepSize << endl;

    if (blockSize == 0) blockSize = 1024;
    if (stepSize == 0) stepSize = blockSize;

    int channels = sfinfo.channels;

    float *filebuf = new float[blockSize * channels];
    float **plugbuf = new float*[channels];
    for (int c = 0; c < channels; ++c) plugbuf[c] = new float[blockSize];

    cerr << "Using block size = " << blockSize << ", step size = "
              << stepSize << endl;

    int minch = plugin->getMinChannelCount();
    int maxch = plugin->getMaxChannelCount();
    cerr << "Plugin accepts " << minch << " -> " << maxch << " channel(s)" << endl;

    Vamp::Plugin::OutputList outputs = plugin->getOutputDescriptors();
    Vamp::Plugin::OutputDescriptor od;

    int output = 0;
    if (argc == 4) output = atoi(argv[3]);

    bool mix = false;

    if (minch > channels || maxch < channels) {
        if (minch == 1) {
            cerr << "WARNING: Sound file has " << channels << " channels, mixing down to 1" << endl;
            mix = true;
            channels = 1;
        } else {
            cerr << "ERROR: Sound file has " << channels << " channels, out of range for plugin" << endl;
            goto done;
        }
    }

    if (outputs.empty()) {
	cerr << "Plugin has no outputs!" << endl;
        goto done;
    }

    if (int(outputs.size()) <= output) {
	cerr << "Output " << output << " requested, but plugin has only " << outputs.size() << " output(s)" << endl;
        goto done;
    }        

    od = outputs[output];
    cerr << "Output is " << od.name << endl;

    plugin->initialise(channels, stepSize, blockSize);

    for (size_t i = 0; i < sfinfo.frames; i += stepSize) {

        int count;

        if (sf_seek(sndfile, i, SEEK_SET) < 0) {
            cerr << "ERROR: sf_seek failed: " << sf_strerror(sndfile) << endl;
            break;
        }
        
        if ((count = sf_readf_float(sndfile, filebuf, blockSize)) < 0) {
            cerr << "ERROR: sf_readf_float failed: " << sf_strerror(sndfile) << endl;
            break;
        }

        for (int c = 0; c < channels; ++c) {
            for (int j = 0; j < blockSize; ++j) {
                plugbuf[c][j] = 0.0f;
            }
        }

        for (int c = 0; c < sfinfo.channels; ++c) {
            int tc = c;
            if (mix) tc = 0;
            for (int j = 0; j < blockSize && j < count; ++j) {
                plugbuf[tc][j] += filebuf[j * channels + c];
            }

            if (plugin->getInputDomain() == Vamp::Plugin::FrequencyDomain) {
                transformInput(plugbuf[tc], blockSize);
            }
        }

        printFeatures
            (i, sfinfo.samplerate, output, plugin->process
             (plugbuf, Vamp::RealTime::frame2RealTime(i, sfinfo.samplerate)));
    }

    printFeatures(sfinfo.frames, sfinfo.samplerate, output,
                  plugin->getRemainingFeatures());

done:
    delete plugin;

    DLCLOSE(libraryHandle);
    sf_close(sndfile);
    return 0;
}

void
printFeatures(int frame, int sr, int output, Vamp::Plugin::FeatureSet features)
{
    for (unsigned int i = 0; i < features[output].size(); ++i) {
        Vamp::RealTime rt = Vamp::RealTime::frame2RealTime(frame, sr);
        if (features[output][i].hasTimestamp) {
            rt = features[output][i].timestamp;
        }
        cout << rt.toString() << ":";
        for (unsigned int j = 0; j < features[output][i].values.size(); ++j) {
            cout << " " << features[output][i].values[j];
        }
        cout << endl;
    }
}

void
transformInput(float *buffer, size_t size)
{
    double *inbuf = new double[size * 2];
    double *outbuf = new double[size * 2];

    // Copy across with Hanning window
    for (size_t i = 0; i < size; ++i) {
        inbuf[i] = double(buffer[i]) * (0.50 - 0.50 * cos(2 * M_PI * i / size));
        inbuf[i + size] = 0.0;
    }
    
    for (size_t i = 0; i < size/2; ++i) {
        double temp = inbuf[i];
        inbuf[i] = inbuf[i + size/2];
        inbuf[i + size/2] = temp;
    }

    fft(size, false, inbuf, inbuf + size, outbuf, outbuf + size);

    for (size_t i = 0; i < size/2; ++i) {
        buffer[i * 2] = outbuf[i];
        buffer[i * 2 + 1] = outbuf[i + size];
    }
    
    delete inbuf;
    delete outbuf;
}

void
fft(unsigned int n, bool inverse, double *ri, double *ii, double *ro, double *io)
{
    if (!ri || !ro || !io) return;

    unsigned int bits;
    unsigned int i, j, k, m;
    unsigned int blockSize, blockEnd;

    double tr, ti;

    if (n < 2) return;
    if (n & (n-1)) return;

    double angle = 2.0 * M_PI;
    if (inverse) angle = -angle;

    for (i = 0; ; ++i) {
	if (n & (1 << i)) {
	    bits = i;
	    break;
	}
    }

    static unsigned int tableSize = 0;
    static int *table = 0;

    if (tableSize != n) {

	delete[] table;

	table = new int[n];

	for (i = 0; i < n; ++i) {
	
	    m = i;

	    for (j = k = 0; j < bits; ++j) {
		k = (k << 1) | (m & 1);
		m >>= 1;
	    }

	    table[i] = k;
	}

	tableSize = n;
    }

    if (ii) {
	for (i = 0; i < n; ++i) {
	    ro[table[i]] = ri[i];
	    io[table[i]] = ii[i];
	}
    } else {
	for (i = 0; i < n; ++i) {
	    ro[table[i]] = ri[i];
	    io[table[i]] = 0.0;
	}
    }

    blockEnd = 1;

    for (blockSize = 2; blockSize <= n; blockSize <<= 1) {

	double delta = angle / (double)blockSize;
	double sm2 = -sin(-2 * delta);
	double sm1 = -sin(-delta);
	double cm2 = cos(-2 * delta);
	double cm1 = cos(-delta);
	double w = 2 * cm1;
	double ar[3], ai[3];

	for (i = 0; i < n; i += blockSize) {

	    ar[2] = cm2;
	    ar[1] = cm1;

	    ai[2] = sm2;
	    ai[1] = sm1;

	    for (j = i, m = 0; m < blockEnd; j++, m++) {

		ar[0] = w * ar[1] - ar[2];
		ar[2] = ar[1];
		ar[1] = ar[0];

		ai[0] = w * ai[1] - ai[2];
		ai[2] = ai[1];
		ai[1] = ai[0];

		k = j + blockEnd;
		tr = ar[0] * ro[k] - ai[0] * io[k];
		ti = ar[0] * io[k] + ai[0] * ro[k];

		ro[k] = ro[j] - tr;
		io[k] = io[j] - ti;

		ro[j] += tr;
		io[j] += ti;
	    }
	}

	blockEnd = blockSize;
    }

    if (inverse) {

	double denom = (double)n;

	for (i = 0; i < n; i++) {
	    ro[i] /= denom;
	    io[i] /= denom;
	}
    }
}

        