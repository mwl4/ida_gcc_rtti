# IDA GCC RTTI

##### This is Class informer plugin for Interactive Disassembler (IDA) which parses GCC RTTI

### Features
* Completely written in C++14 (Native)
* Supported at least by IDA (Windows only) versions: `6.6`, `6.8`, `7.0`
* Optimized and fast parsing methods (handling 5500 classes in about 30 seconds - including names making, etc.)
* Exporting classes to `.dot` format (graph)
* Supported platforms & binaries: x86, x64
* Extra settings to make auxiliary vtable names & exclude prefixed names from graph
* Handling anonymous names

### Installation
Download compiled plugin in proper version, i.e. (`bin/ida_ver_x_x.xxxxxx/gcc_rtti.plw` and `*.p64` or `.dll`), then put `.plw` and `.p64` or `.dll` files in `/plugins` directory in IDA.

### Usage
Load your binary to IDA, wait for the end of analysis, and if plugin was loaded successfully you should have `Class Informer - GCC RTTI` in `Edit` -> `Plugins` toolbar. 

### Graphs
It is a little problem to deal with for example 5000 classes in one graph. I have not found any software, which could render it properly, so I think the best approach, which I was using is to use Graphviz (https://www.graphviz.org) tools to convert `.dot` format to `.svg`. Then you can load .svg file into Google Chrome or any web browser, which certainly will handle it well (do not forget to disable all plugins in web browser which try to help with manipulating svg file, however they seem to be working very slowly with that amount of data).

You can easily convert `.dot` format to `.svg` using following command:

``bin\dot.exe -Tsvg classes.dot -o classes.svg``

Also do not forget to use ignored prefixes feature, since you rather do not need libraries classes in graph (it makes only a mess).

### Compilation
##### Requirements:
* Visual Studio 2015/2017
* IDA SDK (`idasdk`) - supported versions: 6.6 (`ida_older_than_70` branch), 6.8  (`ida_older_than_70` branch), 7.0 (`master` branch), and probably also older/newer versions

##### Building:
1. Put `idasdk` into `/src/libs/`, so there will be `/src/libs/idasdk/include/` and `/src/libs/idasdk/lib/`
2. Open `src/ida_gcc_rtti.sln` in Visual Studio
3. Set proper Solution Configuration (`Release`), and proper Solution Platform (`IDA32` or `IDA64` - depends on what you need)
4. Build solution.
5. If plugin was successfully build, then binaries should be available in `/bin/win32/` and `/bin/win64/`

##### Building on different platforms (Linux, MacOS), using another compilers (clang, gcc)
Feel free to adjust code and linking to make it possible. I do not need it, so I am certainly not going to do it. 

### Original GCC RTTI parsing scripts
I wrote this plugin basing on already existing python scripts, which also handle parsing RTTI. However they perform parsing tasks very very slow, they seem to be not optimized well, that is why handling few thousand classes in some binary might take even few days. If you do not have time like me to wait few days, then use this plugin to make it a lot faster. Also I added some extra stuff to it and it has few fixes comparing to original scripts.

Original scripts I was basing on:
* http://www.hexblog.com/?p=704 - By Igor Skochinsky
* https://github.com/nccgroup/PythonClassInformer - By NCC Group

### License
This software is released under three-clause BSD License.

Copyright © 2018, Michał Wójtowicz a.k.a. mwl4
