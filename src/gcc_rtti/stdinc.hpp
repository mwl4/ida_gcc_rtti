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

#pragma once

/* C headers */
#include <cstdio>		// for printf

/* C++ headers */
#include <algorithm>	// for std::remove_if
#include <map>			// for std::map<>
#include <memory>		// for std::unique_ptr<>

#define USE_STANDARD_FILE_FUNCTIONS // allow using stdin, stdout, etc.

/* idaapi headers */
#include <ida.hpp>
#include <idp.hpp>
#include <auto.hpp>
#include <entry.hpp>
#include <funcs.hpp>
#include <bytes.hpp>
#include <loader.hpp>
#include <kernwin.hpp>
#include <typeinf.hpp>
#include <allins.hpp>
#include <strlist.hpp>
#include <segment.hpp>
#include <diskio.hpp>
#include <pro.h>
#include <dbg.hpp>
#include <idd.hpp>

/* aliases of types */
template < typename T >
using array_dyn_t	= qvector<T>;	// may use also std::vector instead which allows for move operations for instance

template < typename Tkey, typename Tvalue >
using map_t			= std::map<Tkey, Tvalue>; // there is no idaapi equivalent, so use std::map<> instead

using string		= const char *;		// simple c-string
using sstring_t		= qstring;			// sstring stands for smart string

template < typename T >
using unique_ptr_t	= std::unique_ptr<T>;

/* eof */
