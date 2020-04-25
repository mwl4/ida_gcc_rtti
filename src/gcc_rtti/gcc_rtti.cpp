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

#include "gcc_rtti.hxx"

#include "graph.hxx"

const string gcc_rtti_t::ti_names[gcc_rtti_t::TI_COUNT] = {
	"St9type_info",
	"N10__cxxabiv117__class_type_infoE",
	"N10__cxxabiv120__si_class_type_infoE",
	"N10__cxxabiv121__vmi_class_type_infoE",
};

gcc_rtti_t::gcc_rtti_t()
	: m_current_class_id(0)
{
}

gcc_rtti_t::~gcc_rtti_t()
{
}

bool gcc_rtti_t::init()
{
	return true;
}

void gcc_rtti_t::destroy()
{
	m_segments_data.clear();
	m_strings.clear();
	m_graph.reset();
}

void gcc_rtti_t::run()
{
	// turn on GCC3 demangling
	inf.demnames |= DEMNAM_GCC3;

	// initialize strings list
	m_strings = utils::get_strings();
	if (m_strings.empty())
	{
		warning("Strings list is empty, generate strings list firstly.");
		return;
	}

	m_classes.clear();
	m_current_class_id = 0;

	initialize_segments_data();

	// there is no way to get stdout/in from IDA application,
	// so we must create system console and use cstdlib stdout/in instead
	// that means also using standard printf (not qprintf)
	utils::operating_system_t::create_console();

	printf("Looking for standard type info classes\n");
	find_type_info(TI_TINFO);
	find_type_info(TI_CTINFO);
	find_type_info(TI_SICTINFO);
	find_type_info(TI_VMICTINFO);

	printf("Looking for simple classes\n");
	handle_classes(TI_CTINFO, &gcc_rtti_t::format_type_info);

	printf("Looking for single-inheritance classes\n");
	handle_classes(TI_SICTINFO, &gcc_rtti_t::format_si_type_info);

	printf("Looking for multiple-inheritance classes\n");
	handle_classes(TI_VMICTINFO, &gcc_rtti_t::format_vmi_type_info);

	info("Success, found %u classes.", static_cast<uint>(m_classes.size()));

	// destroy console which was created
	utils::operating_system_t::destroy_console();

	// create graph
	m_graph = std::make_unique<graph_t>();
	m_graph->run();
}

void gcc_rtti_t::initialize_segments_data()
{
	m_segments_data.clear();

	for (int segment_id = 0; segment_id < get_segm_qty(); ++segment_id)
	{
		segment_t *const segment = getnseg(segment_id);
		if (!segment)
		{
			continue;
		}

		qstring segment_name;
		get_segm_name(&segment_name, segment);

		qstring segment_class;
		get_segm_class(&segment_class, segment);

		if (segment_class != "DATA" && segment_class != "CONST")
		{
			continue;
		}

		segment_data_t segcode;
		segcode.m_start_ea = segment->start_ea;
		segcode.m_end_ea = segment->end_ea;

		if (segment->start_ea == BADADDR || segment->end_ea == BADADDR)
		{
			warning("Code begins/end in inproper place, begin = " ADDR_FORMAT "; end = " ADDR_FORMAT, segment->start_ea, segment->end_ea);
			continue;
		}

		if ((segment->end_ea - segment->start_ea) > 100 * 1024 * 1024) // 100 MB limit
		{
			warning
			(
				"Segment (%s) data size exceeds limit of 100 MB (%u MB) [ " ADDR_FORMAT " - " ADDR_FORMAT " ]",
				segment_name.c_str(),
				static_cast<uint>((segment->end_ea - segment->start_ea) / 1024 / 1024),
				segcode.m_start_ea, segcode.m_end_ea
			);
			continue;
		}

		segcode.m_data.resize(static_cast<size_t>(segcode.m_end_ea - segcode.m_start_ea));
		if (!get_bytes(&segcode.m_data[0], segcode.m_data.size(), segcode.m_start_ea, GMB_READALL))
		{
			warning("get_bytes() returned failure, expect problems.. [" ADDR_FORMAT " - " ADDR_FORMAT "]", segcode.m_start_ea, segcode.m_end_ea);
		}

		m_segments_data.push_back(segcode);
	}
}

ea_t gcc_rtti_t::find_string(const string s) const
{
	for (const utils::string_data_t &str : m_strings)
	{
		if (str.m_data == s)
		{
			return str.m_address;
		}
	}
	return BADADDR;
}

void gcc_rtti_t::find_type_info(const ti_types_t idx)
{
	const ea_t address = find_string(ti_names[idx]);
	if (address == BADADDR)
	{
		return;
	}

	utils::xreferences_t xrefs = utils::xref_or_find(address);
	if (xrefs.empty())
	{
		return;
	}

	const ea_t ti_start = xrefs[0].m_address - sizeof(ea_t);
	if (utils::is_bad_addr(ti_start))
	{
		return;
	}

	printf("found %d at " ADDR_FORMAT "\n", static_cast<int>(idx), ti_start);
	const ea_t ea = format_type_info(ti_start);
	if (idx >= TI_CTINFO)
	{
		format_struct(ea, "p");
	}
}

void gcc_rtti_t::handle_classes(ti_types_t idx, ea_t(gcc_rtti_t::*const formatter)(const ea_t address))
{
	sstring_t name = vtname(ti_names[idx]);

	// try single underscore first
	ea_t address = get_name_ea(BADADDR, &name[1]);
	if (address != BADADDR)
	{
		name = &name[1];
	}
	else
	{
		address = get_name_ea(BADADDR, &name[0]);
	}

	if (address == BADADDR)
	{
		printf("Could not find vtable for %s\n", ti_names[idx]);
		return;
	}

	idx = TI_TINFO;
	utils::xreferences_t xrefs;
	xrefs.reserve(100); // just for optimization
	map_t<ea_t, bool> handled;

	while (address != BADADDR)
	{
		xrefs.clear();

		printf("Looking for refs to vtable " ADDR_FORMAT "\n", address);

		if (is_spec_ea(address))
		{
			xrefs = utils::xref_or_find(address, true);
		}

		address += sizeof(ea_t) * 2; // We are looking for +8(32)/+16(64) offset to type vtable

		for (const segment_data_t &segment_data : m_segments_data)
		{
			for (size_t current = 0; current < segment_data.m_data.size() - sizeof(ea_t); current += sizeof(ea_t))
			{
				if (*reinterpret_cast<const ea_t *>(&segment_data.m_data[current]) != address
				 || is_code(get_flags(segment_data.m_start_ea + current)))
				{
					continue;
				}

				const ea_t next_ea = *reinterpret_cast<const ea_t *>(&segment_data.m_data[current + sizeof(ea_t)]);

				if (is_code(get_flags(next_ea)))
				{
					continue;
				}

				sstring_t mangled_name = utils::get_string(next_ea);
				if (mangled_name[0] == '\0' || mangled_name[0] == -1) { continue; }
				if (mangled_name[0] == '*') { mangled_name = &mangled_name[1]; }
				if (detect_compiler_using_demangler((sstring_t("_ZTV") + mangled_name).c_str()) <= 0)
				{
					continue;
				}

				xrefs.push_back(utils::xreference_t(segment_data.m_start_ea + current, false));
			}
		}

		for (const utils::xreference_t &xref : xrefs)
		{
			if (utils::is_bad_addr(xref.m_address) || handled.find(xref.m_address) != handled.end())
			{
				continue;
			}

			printf("found %s at " ADDR_FORMAT "\n", name.c_str(), xref.m_address);
			(this->*formatter)(xref.m_address);
			handled[xref.m_address] = true;
		}

		sstring_t name2; name2.sprnt("%s_%d", name.c_str(), static_cast<int>(idx));
		address = get_name_ea(BADADDR, name2.c_str());
		idx = static_cast<ti_types_t>(static_cast<int>(idx) + 1);
	}
}

ea_t gcc_rtti_t::format_type_info(const ea_t address)
{
	// dd `vtable for'std::type_info+8
	// dd `typeinfo name for'std::type_info

	const ea_t tis = utils::get_ea(address + sizeof(ea_t));
	if (utils::is_bad_addr(tis))
	{
		return BADADDR;
	}

	const sstring_t name = utils::get_string(tis);

	if (name.empty())
	{
		return BADADDR;
	}	

	/* skip '*' character in case of type defined in function */
	const sstring_t proper_name = (name[0] == '*' ? name.c_str() + 1 : name.c_str());

	// looks good, let's do it
	const ea_t address2 = format_struct(address, "vp");
	set_name(tis, (sstring_t("__ZTS") + proper_name).c_str(), SN_NOWARN);
	set_name(address, (sstring_t("__ZTI") + proper_name).c_str(), SN_NOWARN);

	qstring demangled_name;
	if(demangle_name(&demangled_name, (sstring_t("_Z") + proper_name).c_str(), 0) >= 0)
	{
		get_class(address)->m_name = demangled_name;
	}
	else
	{
		get_class(address)->m_name = name;
	}

	ea_t vtb = BADADDR;

	// find our vtable
	// 0 followed by ea
	for (const segment_data_t &segment_data : m_segments_data)
	{
		for (size_t current = sizeof(ea_t); current < segment_data.m_data.size(); current += sizeof(ea_t))
		{
			if (*reinterpret_cast<const ea_t *>(&segment_data.m_data[current - sizeof(ea_t)]) != 0 // following 0
			 || *reinterpret_cast<const ea_t *>(&segment_data.m_data[current]) != address)
			{
				continue;
			}

			vtb = segment_data.m_start_ea + current;
		}
	}

	if (!utils::is_bad_addr(vtb))
	{
		printf("vtable for %s at " ADDR_FORMAT "\n", proper_name.c_str(), vtb);
		format_struct(vtb, "pp");
		set_name(vtb, (sstring_t("__ZTV") + proper_name).c_str(), SN_NOWARN);
	}
	else
	{
		return BADADDR;
	}

	return address2;
}

ea_t gcc_rtti_t::format_si_type_info(const ea_t address)
{
	// dd `vtable for'__cxxabiv1::__si_class_type_info+8
	// dd `typeinfo name for'MyClass
	// dd `typeinfo for'BaseClass

	const ea_t addr = format_type_info(address);
	const ea_t pbase = utils::get_ea(addr);
	get_class(address)->add_base(class_t::base_t(get_class(pbase)));
	return format_struct(addr, "p");
}

ea_t gcc_rtti_t::format_vmi_type_info(const ea_t address)
{
	// dd `vtable for'__cxxabiv1::__si_class_type_info+8
	// dd `typeinfo name for'MyClass
	// dd flags
	// dd base_count
	// (base_type, offset_flags) x base_count

	ea_t addr = format_type_info(address);
	if (addr == BADADDR)
	{
		return address;
	}

	addr = format_struct(addr, "ii");

	const uint32_t base_count = get_32bit(addr - sizeof(uint32_t));
	if (base_count > 100)
	{
		printf(ADDR_FORMAT ": over 100 base classes (%u)(" ADDR_FORMAT ")?!\n", address, base_count, static_cast<ea_t>(addr - sizeof(uint32_t)));
		return BADADDR;
	}

	for (uint32_t i = 0; i < base_count; ++i)
	{
		const ea_t base_ti = utils::get_ea(addr);
		const ea_t flags_off = utils::get_ea(addr + sizeof(ea_t));
		const ea_t off = utils::sig_next(flags_off >> 8, 24);

		get_class(address)->add_base(class_t::base_t(get_class(base_ti), static_cast<uint>(off), static_cast<uint>(flags_off & 0xff)));

		addr = format_struct(addr, "pl");
	}

	return addr;
}

/**
 * p pointer
 * v vtable pointer (delta ptrsize * 2)
 * i integer (32-bit)
 * l integer (32 or 64-bit)
 */
ea_t gcc_rtti_t::format_struct(ea_t address, const string fmt)
{
	for (const char *cp = fmt; *cp; ++cp)
	{
		const char f = *cp;
		if (f == 'p' || f == 'v')
		{
			size_t delta = 0;
			if (f == 'v')
			{
				delta = sizeof(ea_t) * 2;
			}
			utils::force_ptr(address, delta);
			address += sizeof(ea_t);
		}
		else if (f == 'i')
		{
			
			create_dword(address, sizeof(int));
			address += sizeof(int);
		}
		else if (f == 'l')
		{
		#ifdef __EA64__
			create_qword(address, sizeof(ea_t));
		#else
			create_dword(address, sizeof(ea_t));
		#endif
			address += sizeof(ea_t);
		}
	}
	return address;
}

sstring_t gcc_rtti_t::vtname(const sstring_t &name) const
{
	return sstring_t("__ZTV") + name;
}

auto gcc_rtti_t::get_class(const ea_t address) -> class_t *
{
	unique_ptr_t<class_t> &class_ptr = m_classes[address];
	if (!class_ptr)
	{
		class_ptr = std::make_unique<class_t>();
		class_ptr->m_id = m_current_class_id++;
	}
	return class_ptr.get();
}

auto gcc_rtti_t::get_classes() const -> const classes_t &
{
	return m_classes;
}

gcc_rtti_t *gcc_rtti_t::s_instance = nullptr;

gcc_rtti_t *gcc_rtti_t::instance()
{
	return s_instance;
}

int idaapi gcc_rtti_t::init_s(void)
{
	if (s_instance)
	{
		s_instance->destroy();
		delete s_instance;
	}

	s_instance = new gcc_rtti_t;
	if (s_instance->init())
	{
		return PLUGIN_KEEP;
	}
	else
	{
		return PLUGIN_SKIP;
	}
}

bool idaapi gcc_rtti_t::run_s(size_t arg)
{
	if (s_instance)
	{
		s_instance->run();
	}
	return true;
}

void idaapi gcc_rtti_t::term_s()
{
	if (s_instance)
	{
		s_instance->destroy();
		delete s_instance;
		s_instance = nullptr;
	}
}

/* eof */
