/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

#include "vamp-sdk/PluginHostAdapter.h"
#include "vamp-sdk/hostext/PluginChannelAdapter.h"
#include "vamp-sdk/hostext/PluginInputDomainAdapter.h"
#include "vamp-sdk/hostext/PluginLoader.h"
#include "vamp/vamp.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <sndfile.h>

#include <cmath>

using std::cout;
using std::cin;
using std::cerr;
using std::getline;
using std::endl;
using std::string;
using std::vector;
using std::ofstream;
using std::ios;

using Vamp::HostExt::PluginLoader;
using Vamp::Plugin;

/*
  
   usage:

   template-generator vamp:aubioonset:onsets

*/

string programURI = "http://www.vamp-plugins.org/doap.rdf#template-generator";

void usage()
{
    cerr << "usage: template-generator [PLUGIN_BASE_URI YOUR_URI] vamp:soname:plugin[:output]" << endl;
    exit(2);
}

template <class T>
inline string to_string (const T& t)
{
    std::stringstream ss;
    ss << t;
    return ss.str();
}

string describe_namespaces(Plugin* plugin, string pluginBundleBaseURI)
{
	string res=\
"@prefix rdfs:     <http://www.w3.org/2000/01/rdf-schema#> .\n\
@prefix xsd:      <http://www.w3.org/2001/XMLSchema#> .\n\
@prefix vamp:     <http://www.vamp-plugins.org/ontology/> .\n\
@prefix vampex:   <http://www.vamp-plugins.org/examples/> .\n\
@prefix plugbase: <"+pluginBundleBaseURI+"> .\n\
@prefix owl:      <http://www.w3.org/2002/07/owl#> .\n\
@prefix dc:       <http://purl.org/dc/elements/1.1/> .\n\
@prefix af:       <http://purl.org/ontology/af/> .\n\
@prefix foaf:     <http://xmlns.com/foaf/0.1/> .\n\
@prefix cc:       <http://web.resource.org/cc/> .\n\
@prefix thisplug: <"+pluginBundleBaseURI+plugin->getIdentifier()+"#> .\n\
@prefix :         <> .\n\n";
	
	return res;
}

string describe_doc(Plugin* plugin, string describerURI)
{
	string res=\
"<>  a   vamp:PluginDescription ;\n\
     foaf:maker          <"+describerURI+"> ;\n\
     foaf:maker          <"+programURI+"> ;\n\
     foaf:primaryTopic   plugbase:"+plugin->getIdentifier()+" .\n\n";
	return res;
}


string describe_plugin(Plugin* plugin)
{
	string res=\
"plugbase:"+plugin->getIdentifier()+" a   vamp:Plugin ;\n\
    dc:title              \""+plugin->getName()+"\" ;\n\
    dc:description        \""+plugin->getDescription()+"\" ;\n\
    foaf:maker            [ foaf:name \""+plugin->getMaker()+"\"] ; # FIXME could give plugin author's URI here\n\
    cc:license            <FIXME license for the plugin> ; \n\
    vamp:identifier       \""+plugin->getIdentifier()+"\" ;\n\
    vamp:vamp_API_version vamp:api_version_"+to_string(plugin->getVampApiVersion())+" ;\n\
    owl:versionInfo       \""+to_string(plugin->getPluginVersion())+"\" ;\n";
	if (plugin->getInputDomain() == Vamp::Plugin::FrequencyDomain)
		res+="    vamp:input_domain     vamp:FrequencyDomain ;\n\n";
	else
		res+="    vamp:input_domain     vamp:TimeDomain ;\n\n";
	

	Plugin::ParameterList params = plugin->getParameterDescriptors();
	for (Plugin::ParameterList::const_iterator i = params.begin(); i != params.end(); i++)
		res+="    vamp:parameter_descriptor   thisplug:param_"+(*i).identifier+" ;\n";
	res+="\n";

	Plugin::OutputList outputs = plugin->getOutputDescriptors();
	for (Plugin::OutputList::const_iterator i = outputs.begin(); i!= outputs.end(); i++)
		res+="    vamp:output_descriptor      thisplug:output_"+(*i).identifier+" ;\n";
	res+="    .\n";
	
	return res;
}

string describe_param(Plugin::ParameterDescriptor p)
{
	string res=\
"thisplug:param_"+p.identifier+" a  vamp:ParameterDescriptor ;\n\
    vamp:identifier     \""+p.identifier+"\" ;\n\
    dc:title            \""+p.name+"\" ;\n\
    dc:format           \""+p.unit+"\" ;\n\
    vamp:minValue       "+to_string(p.minValue)+" ;\n\
    vamp:maxValue       "+to_string(p.maxValue)+" ;\n\
    vamp:defaultValue   "+to_string(p.defaultValue)+" .\n\n";
	return res;
}

string describe_output(Plugin::OutputDescriptor o)
{
	string res=\
"thisplug:output_"+o.identifier+" a  vamp:OutputDescriptor ;\n\
    vamp:identifier     \""+o.identifier+"\" ;\n\
    dc:title            \""+o.name+"\" ;\n\
    vamp:fixed_bin_count \""+(o.hasFixedBinCount == 1 ? "true" : "false")+"\" ;\n";

	// FIXME ? Bin names may vary based on plugin setup, so including them here might be misleading...
	if (o.hasFixedBinCount)
	{
		res+="    vamp:bin_count      "+to_string(o.binCount)+" ;\n";
		res+="    vamp:bin_names      (";

		unsigned int i;
		for (i=0; i+1 < o.binNames.size(); i++)
			res+=" \""+o.binNames[i]+"\"";
		if (i < o.binNames.size())
			res+=" \""+o.binNames[i]+"\"";
		res+=");\n";
	}
	
	if (o.sampleType == Plugin::OutputDescriptor::VariableSampleRate)
	{
		res+="    vamp:sample_type    vamp:VariableSampleRate ;\n";
		if (o.sampleRate > 0.0f)
			res+="    vamp:sample_rate    "+to_string(o.sampleRate)+" ;\n";
	}
	else if (o.sampleType == Plugin::OutputDescriptor::FixedSampleRate)
	{
		res+="    vamp:sample_type    vamp:FixedSampleRate ;\n";
		res+="    vamp:sample_rate    "+to_string(o.sampleRate)+" ;\n";
	}
	else if (o.sampleType == Plugin::OutputDescriptor::OneSamplePerStep)
		res+="    vamp:sample_type    vamp:OneSamplePerStep ;\n";
	else
	{
		cerr<<"Incomprehensible sampleType for output descriptor "+o.identifier<<" !"<<endl;
		exit(1);
	}

	res+="    vamp:computes_feature_type  <FIXME feature type URI> ;\n";
	res+="    vamp:computes_event_type    <FIXME event type URI> ;\n";
	res+="    .\n";

	return res;
}
string describe(Plugin* plugin, string pluginBundleBaseURI, string describerURI)
{
	string res = describe_namespaces(plugin, pluginBundleBaseURI);
	
	res += describe_doc(plugin, describerURI);
	
	res += describe_plugin(plugin);
	
	Plugin::ParameterList params = plugin->getParameterDescriptors();
	for (Plugin::ParameterList::const_iterator i = params.begin(); i != params.end(); i++)
		res += describe_param(*i);
	
	Plugin::OutputList outputs = plugin->getOutputDescriptors();
	for (Plugin::OutputList::const_iterator i = outputs.begin(); i!= outputs.end(); i++)
		res += describe_output(*i);
	
	return res;
}

int main(int argc, char **argv)
{
    if (argc != 2 && argc != 4) usage();

    std::string pluginName = argv[argc-1];

    if (pluginName.substr(0, 5) == "vamp:") {
        pluginName = pluginName.substr(5);
    }

    Vamp::Plugin *plugin = PluginLoader::getInstance()->loadPlugin
        (pluginName, size_t(44100), PluginLoader::ADAPT_ALL_SAFE);

    if (!plugin) {
        cerr << "ERROR: Plugin \"" << pluginName << "\" could not be loaded" << endl;
        exit(1);
    }

    string pluginBundleBaseURI, describerURI;
	
    if (argc == 4)
    {
        pluginBundleBaseURI = argv[1];
        describerURI = argv[2];
    }
    else
    {
        cerr << "Please enter the base URI for the plugin bundle : ";
        getline(cin, pluginBundleBaseURI);
        cerr << "Please enter your URI : ";
        getline(cin, describerURI);
    }
	
    cout << describe(plugin, pluginBundleBaseURI, describerURI) << endl;

    return 0;
}


