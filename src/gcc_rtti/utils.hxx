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

namespace utils
{
	class string_data_t
	{
	public:
		string_data_t(const sstring_t &data)
			: m_address(BADADDR), m_data(data)
		{
		}

		string_data_t(const ea_t &address, const sstring_t &data)
			: m_address(address), m_data(data)
		{
		}

		inline bool operator==(const string_data_t &rhs) const
		{
			return m_data == rhs.m_data;
		}

	public:
		ea_t      m_address;
		sstring_t m_data;
	};

	using strings_data_t = array_dyn_t<string_data_t>;
	strings_data_t get_strings();

	sstring_t get_string(const ea_t address);

	class xreference_t
	{
	public:
		xreference_t()
			: m_address(0), m_code(false)
		{
		}

		xreference_t(ea_t address, bool code)
			: m_address(address), m_code(code)
		{
		}

	public:
		ea_t m_address;
		bool m_code;
	};

	using xreferences_t = array_dyn_t<xreference_t>;
	xreferences_t get_xrefs(ea_t ea, int flags /* XREF_ALL | XREF_FAR | XREF_DATA */);

	xreferences_t xref_or_find(const ea_t address, const bool allow_many = false);

	ea_t get_ea(const ea_t address);

	void force_ptr(const ea_t address, size_t delta = 0);

	static inline bool is_bad_addr(const ea_t address)
	{
		return address == 0 || address == BADADDR || is_spec_ea(address) || !isLoaded(address);
	}

	sstring_t ea_to_bytes(const ea_t address);

	/* sign extend b low bits in x */
	/* from "Bit Twiddling Hacks" */
	ea_t sig_next(ea_t x, ea_t b);

#ifdef __EA64__
#	define ADDR_FORMAT "0x%016llX"
#else
#	define ADDR_FORMAT "0x%08X"
#endif

#ifdef __EA64__
	const ea_t ALL_BYTES_EA_MASK = 0x0101010101010101;
#else
	const ea_t ALL_BYTES_EA_MASK = 0x01010101;
#endif

	class operating_system_t
	{
	public:
		static void create_console();
		static void destroy_console();

	private:
		static FILE *s_fstdout;
		static FILE *s_fstdin;
	};
} // namespace utils

/* eof */
