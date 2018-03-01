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

#include <utils.hxx>

/* forward declarations */
class graph_t;

class gcc_rtti_t
{
public:
	gcc_rtti_t();
	gcc_rtti_t(gcc_rtti_t const&) = delete;
	gcc_rtti_t(gcc_rtti_t &&) = delete;
	~gcc_rtti_t();

	gcc_rtti_t &operator=(gcc_rtti_t const&) = delete;
	gcc_rtti_t &operator=(gcc_rtti_t &&) = delete;

	bool init();
	void destroy();
	void run();

	static gcc_rtti_t *instance();

public:
	static  int idaapi init_s(void);
	static bool idaapi run_s(size_t arg);
	static void idaapi term_s();

private:
	static gcc_rtti_t *s_instance;

public:
	class class_t;
	using classes_t = map_t<ea_t, unique_ptr_t<class_t>>;

private:
	enum ti_types_t
	{
		TI_TINFO = 0,
		TI_CTINFO,
		TI_SICTINFO,
		TI_VMICTINFO,
		TI_COUNT /* always at end */
	};
	static const string ti_names[gcc_rtti_t::TI_COUNT];

	ea_t find_string(const string s) const;
	void find_type_info(const ti_types_t idx);
	void handle_classes(ti_types_t idx, ea_t(gcc_rtti_t::*const formatter)(const ea_t address));

	ea_t format_type_info(const ea_t address);
	ea_t format_si_type_info(const ea_t address);
	ea_t format_vmi_type_info(const ea_t address);

	ea_t format_struct(ea_t address, const string fmt);

	sstring_t vtname(const sstring_t &name) const;

	class_t *get_class(const ea_t address);

public:
	const classes_t &get_classes() const;

private:
	utils::strings_data_t	m_strings;
	array_dyn_t<uchar>		m_code;
	ea_t					m_code_begin = BADADDR;
	ea_t					m_code_end = BADADDR;
	classes_t				m_classes;
	unique_ptr_t<graph_t>	m_graph;
	unsigned int			m_current_class_id;
	bool					m_auxiliary_vtables_names;
};

class gcc_rtti_t::class_t
{
public:
	class base_t
	{
	public:
		base_t(class_t *const class_ptr)
			: m_class(class_ptr)
			, m_offset(0)
			, m_flags(0)
		{
		}

		base_t(class_t *const class_ptr, uint offset, uint flags)
			: m_class(class_ptr)
			, m_offset(offset)
			, m_flags(flags)
		{
		}

	public:
		class_t *m_class;
		uint	 m_offset;
		uint	 m_flags;
	};

public:
	void add_base(const base_t &base)
	{
		m_bases.push_back(base);
	}

public:
	sstring_t			m_name;
	array_dyn_t<base_t> m_bases;
	unsigned int		m_id;
	bool				m_shown = false;
};

/* eof */