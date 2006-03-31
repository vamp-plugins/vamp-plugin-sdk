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

#include <iostream>

#include "vamp.h"

#include "PluginHostAdapter.h"

#include "system.h"

/*
    A very simple Vamp plugin host.  So far all this does is load the
    plugin library given on the command line and dump out the names of
    the plugins found in it.
*/

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " pluginlibrary.so" << std::endl;
        return 2;
    }

    std::cerr << std::endl << argv[0] << ": Running..." << std::endl;

    std::string soname = argv[1];

    void *libraryHandle = DLOPEN(soname, RTLD_LAZY);

    if (!libraryHandle) {
        std::cerr << argv[0] << ": Failed to open plugin library " 
                  << soname << std::endl;
        return 1;
    }

    std::cout << argv[0] << ": Opened plugin library " << soname << std::endl;

    VampGetPluginDescriptorFunction fn = (VampGetPluginDescriptorFunction)
        DLSYM(libraryHandle, "vampGetPluginDescriptor");
    
    if (!fn) {
        std::cerr << argv[0] << ": No Vamp descriptor function in library "
                  << soname << std::endl;
        DLCLOSE(libraryHandle);
        return 1;
    }

    std::cout << argv[0] << ": Found plugin descriptor function" << std::endl;

    int index = 0;
    const VampPluginDescriptor *descriptor = 0;

    while ((descriptor = fn(index))) {

        Vamp::PluginHostAdapter adapter(descriptor, 48000);
        std::cout << argv[0] << ": Plugin " << (index+1)
                  << " is \"" << adapter.getName() << "\"" << std::endl;
        
        ++index;
    }

    std::cout << argv[0] << ": Done\n" << std::endl;

    DLCLOSE(libraryHandle);
    return 0;
}

