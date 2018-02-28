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
	, m_auxiliary_vtables_names(false)
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
	m_code.clear();
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

	// initialize code data
	m_code.resize(static_cast<size_t>(inf.maxEA - inf.minEA));
	get_many_bytes(inf.minEA, &m_code[0], m_code.size());

	const string question = "Do you want to make auxiliary vtables names (+ptr_size offset)?\n";
	const int answer = askbuttons_c("Yes", "No", "Cancel", ASKBTN_NO, question);

	if (answer == ASKBTN_CANCEL) { return; }

	m_auxiliary_vtables_names = (answer == ASKBTN_YES) ? true : false;

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

	// destroy console which was created
	utils::operating_system_t::destroy_console();

	info("Success, found %u classes.", static_cast<uint>(m_classes.size()));

	// create graph
	m_graph = std::make_unique<graph_t>();
	m_graph->run();
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
	ea_t address = get_name_ea(BADADDR, name.c_str());
	if (address == BADADDR)
	{
		// try single underscore
		name = &name[1]; // skipping first character
		address = get_name_ea(BADADDR, name.c_str());
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
		address += sizeof(ea_t) * 1; // in original python code there was (* 2), but it seems to be incorrect.. I am confused

		for (size_t current = 0; current < m_code.size(); current += sizeof(ea_t))
		{
			if (*reinterpret_cast<ea_t *>(&m_code[current]) == address && !isCode(getFlags(inf.minEA + current)))
			{
				xrefs.push_back(utils::xreference_t(inf.minEA + current, false));
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
	do_name_anyway(tis, (sstring_t("__ZTS") + proper_name).c_str(), 1024);
	do_name_anyway(address, (sstring_t("__ZTI") + proper_name).c_str(), 1024);

	char demangled_name_buffer[512] = { 0 };
	demangle_name(demangled_name_buffer, sizeof(demangled_name_buffer), (sstring_t("_Z") + proper_name).c_str(), 0);

	if (demangled_name_buffer[0] == '\0')
	{
		get_class(address)->m_name = name;
	}
	else
	{
		get_class(address)->m_name = demangled_name_buffer;
	}

	ea_t vtb = BADADDR;

	// find our vtable
	// 0 followed by ea
	for (size_t current = sizeof(ea_t); current < m_code.size(); current += sizeof(ea_t))
	{
		if (*reinterpret_cast<ea_t *>(&m_code[current - sizeof(ea_t)]) == 0 // following 0
		 && *reinterpret_cast<ea_t *>(&m_code[current]) == address)
		{
			vtb = inf.minEA + current;
		}
	}

	if (!utils::is_bad_addr(vtb))
	{
		printf("vtable for %s at " ADDR_FORMAT "\n", proper_name.c_str(), vtb);
		format_struct(vtb, "pp");
		do_name_anyway(vtb, (sstring_t("__ZTV") + proper_name).c_str(), 1024);

		if (m_auxiliary_vtables_names)
		{
			do_name_anyway(vtb + sizeof(ea_t), (sstring_t("__ZTV") + proper_name + "_0").c_str(), 1024);
		}
	}
	else
	{
		return BADADDR;
	}

	return address2;
}

ea_t gcc_rtti_t::format_si_type_info(const ea_t address)
{
	const ea_t addr = format_type_info(address);
	const ea_t pbase = utils::get_ea(addr);
	get_class(address)->add_base(class_t::base_t(get_class(pbase)));
	return format_struct(addr, "p");
}

ea_t gcc_rtti_t::format_vmi_type_info(const ea_t address)
{
	ea_t addr = format_type_info(address);
	if (addr == BADADDR)
	{
		return address;
	}

	addr = format_struct(addr, "ii");

	const uint32_t base_count = get_32bit(addr - sizeof(uint32_t));
	if (base_count > 100)
	{
		printf(ADDR_FORMAT ": over 100 base classes (%u)(" ADDR_FORMAT ")?!", address, base_count, addr - sizeof(uint32_t));
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
			doDwrd(address, sizeof(int));
			address += sizeof(int);
		}
		else if (f == 'l')
		{
		#ifdef __EA64__
			doQwrd(address, sizeof(ea_t));
		#else
			doDwrd(address, sizeof(ea_t));
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

void idaapi gcc_rtti_t::run_s(int arg)
{
	if (s_instance)
	{
		s_instance->run();
	}
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
