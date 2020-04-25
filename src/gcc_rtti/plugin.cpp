/***************************************************************************************************************
 *
 * Class informer, plugin for Interactive Disassembler (IDA)
 *
 * Rewritten to C++14, modified and optimized GCC RTTI parsing code originally written by:
 * ^ Igor Skochinsky, see http://www.hexblog.com/?p=704 for the original version of this code
 * ^ NCC Group, see https://github.com/nccgroup/PythonClassInformer for the modified version of the code above
 *
 * This code has been written by Michał Wójtowicz a.k.a mwl4, 02/2018
 *
 ***************************************************************************************************************/

#include <stdinc.hxx>

#include <gcc_rtti.hxx>

plugin_t PLUGIN =
{
	IDP_INTERFACE_VERSION,			// version
	NULL,							// flags
	&gcc_rtti_t::init_s,			// initialize
	&gcc_rtti_t::term_s,			// destroy
	&gcc_rtti_t::run_s,				// invoke plugin
	"Class informer - GCC RTTI",	// comment
	"Class informer - GCC RTTI",	// help
	"Class informer - GCC RTTI",	// wanted name
	NULL,							// wanted hotkey
};

/* eof */
