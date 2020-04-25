PROC=gcc_rtti
O1=graph
O2=plugin
O3=utils

include ../plugin.mak

CFLAGS += -Wno-class-memaccess -Wno-deprecated-copy -Wno-parentheses -std=c++14 -D__DEFINE_INF__


# MAKEDEP dependency list ------------------
$(F)gcc_rtti$(O): $(I)allins.hpp $(I)auto.hpp $(I)bytes.hpp $(I)dbg.hpp \
	          $(I)diskio.hpp $(I)entry.hpp $(I)funcs.hpp $(I)ida.hpp    \
			  $(I)idd.hpp $(I)idp.hpp $(I)kernwin.hpp $(I)loader.hpp    \
			  $(I)pro.h $(I)segment.hpp $(I)strlist.hpp $(I)typeinf.hpp \
			  stdinc.hpp graph.hpp gcc_rtti.hpp gcc_rtti.cpp

$(F)graph$(O): $(I)allins.hpp $(I)auto.hpp $(I)bytes.hpp $(I)dbg.hpp    \
	          $(I)diskio.hpp $(I)entry.hpp $(I)funcs.hpp $(I)ida.hpp    \
			  $(I)idd.hpp $(I)idp.hpp $(I)kernwin.hpp $(I)loader.hpp    \
			  $(I)pro.h $(I)segment.hpp $(I)strlist.hpp $(I)typeinf.hpp \
	          stdinc.hpp graph.hpp graph.cpp

$(F)plugin$(O): $(I)allins.hpp $(I)auto.hpp $(I)bytes.hpp $(I)dbg.hpp   \
	          $(I)diskio.hpp $(I)entry.hpp $(I)funcs.hpp $(I)ida.hpp    \
			  $(I)idd.hpp $(I)idp.hpp $(I)kernwin.hpp $(I)loader.hpp    \
			  $(I)pro.h $(I)segment.hpp $(I)strlist.hpp $(I)typeinf.hpp \
	          stdinc.hpp gcc_rtti.hpp plugin.cpp

$(F)utils$(O): $(I)allins.hpp $(I)auto.hpp $(I)bytes.hpp $(I)dbg.hpp    \
	          $(I)diskio.hpp $(I)entry.hpp $(I)funcs.hpp $(I)ida.hpp    \
			  $(I)idd.hpp $(I)idp.hpp $(I)kernwin.hpp $(I)loader.hpp    \
			  $(I)pro.h $(I)segment.hpp $(I)strlist.hpp $(I)typeinf.hpp \
	          stdinc.hpp utils.hpp utils.cpp
