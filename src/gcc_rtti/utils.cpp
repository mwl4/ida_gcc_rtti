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

#include "stdinc.hpp"

#include "utils.hpp"

#ifdef _WIN32
#include <Windows.h> // for AllocConsole, FreeConsole
#endif

namespace utils
{
	sstring_t get_segment_name(ea_t address)
	{
		if (segment_t *const seg = getseg(address))
		{
			qstring segname;
			get_visible_segm_name(&segname, seg);
			return segname;
		}
		return sstring_t();
	}

	xreferences_t get_xrefs(ea_t ea, int flags)
	{
		xreferences_t result;
		xrefblk_t xb;
		for (bool xb_ok = xb.first_to(ea, flags); xb_ok; xb_ok = xb.next_to())
		{
			if (get_segment_name(xb.from) != "_pdata")
			{
				result.push_back(xreference_t(xb.from, xb.iscode ? true : false));
			}
		}
		return result;
	}

	xreferences_t xref_or_find(const ea_t address, const bool allow_many/*= false*/)
	{
		xreferences_t found;

		const ea_t min_ea = get_first_seg()->start_ea;
		const ea_t max_ea = get_last_seg()->end_ea;

		for (ea_t start_address = min_ea, current = 0;; start_address = current + sizeof(ea_t))
		{
			const ea_t mask = ALL_BYTES_EA_MASK;
			current = bin_search2
			(
				start_address, max_ea,
				reinterpret_cast<const uchar *>(&address),
				reinterpret_cast<const uchar *>(&mask),
				sizeof(ea_t),
				BIN_SEARCH_FORWARD | BIN_SEARCH_CASE
			);
			if (current != BADADDR)
			{
				if (get_segment_name(current) != "LOAD")
				{
					found.push_back(xreference_t(current, is_code(get_flags(current))));
				}
			}
			else
			{
				break;
			}
		}

		if (found.empty())
		{
			found = get_xrefs(address, XREF_DATA);
		}

		if (found.size() > 1 && !allow_many)
		{
			msg("Too many xrefs to " ADDR_FORMAT, address);
			return xreferences_t();
		}

		found.erase(std::remove_if(
			found.begin(), found.end(),
			[](const xreference_t &ref)
			{
				return ref.m_code;
			}
		), found.end());

		return found;
	}

	strings_data_t get_strings()
	{
		strings_data_t result;
		string_info_t info;
		size_t n = get_strlist_qty();
		for (size_t i = 0; i < n; ++i)
		{
			get_strlist_item(&info, i);
			size_t buffer_size = info.length + 1;
			char *buffer = new char[buffer_size];
			get_bytes(buffer, info.length, info.ea, GMB_READALL);
			buffer[info.length] = '\0';
			result.push_back(string_data_t(info.ea, buffer));
			delete[] buffer;
		}
		return result;
	}

	sstring_t get_string(const ea_t address)
	{
		if (is_bad_addr(address))
		{
			return sstring_t();
		}

		sstring_t result;
		for (uchar c = 0; (c = get_byte(address + result.length()));)
		{
			if (result.length() < 1000) // limit reached
			{
				result.append(static_cast<char>(c));
			}
			else break;
		}
		return result;
	}

	sstring_t ea_to_bytes(const ea_t address)
	{
		sstring_t result;
		for (int i = 0; i < sizeof(address); ++i)
		{
			result.cat_sprnt("%02X", (unsigned char)((address >> (i * 8)) & 0xff));
			if (i + 1 != sizeof(address)) { result += ' '; }
		}
		return result;
	}

	ea_t get_ea(const ea_t address)
	{
	#ifdef __EA64__
		return get_64bit(address);
	#else
		return get_32bit(address);
	#endif
	}

	void force_ptr(const ea_t address, size_t delta/* = 0 */)
	{
	#ifdef __EA64__
		create_qword(address, 8);
	#else
		create_dword(address, 4);
	#endif

		if (is_off0(get_flags(address)))
		{
			return; // don't touch fixups
		}

		const ea_t pv = get_ea(address);
		if (pv != 0 && pv != BADADDR)
		{
			// apply offset again
			if (is_spec_ea(pv))
			{
				delta = 0;
			}

			insn_t instruction;
			create_insn(address, &instruction);
			op_stroff(instruction, 0, nullptr, 0, delta);
		}
	}

	ea_t sig_next(ea_t x, ea_t b)
	{
	#ifdef __EA64__
		const ea_t m = 1ULL << (b - 1);
		x = x & ((1ULL << b) - 1);
		return (x ^ m) - m;
	#else
		const ea_t m = 1 << (b - 1);
		x = x & ((1 << b) - 1);
		return (x ^ m) - m;
	#endif
	}

	void operating_system_t::create_console()
	{
	#ifdef _WIN32
		AllocConsole();
		s_fstdout = freopen("CONOUT$", "w", stdout);
		s_fstdin = freopen("CONIN$", "r", stdin);
	#else
		// TODO: Write implementation which opens console in linux application
	#endif
	}

	void operating_system_t::destroy_console()
	{
	#ifdef _WIN32
		fclose(s_fstdin);
		fclose(s_fstdout);
		FreeConsole();
	#else
		// TODO: Write implementation which frees console in linux application
	#endif
	}

	FILE *operating_system_t::s_fstdout = nullptr;
	FILE *operating_system_t::s_fstdin = nullptr;
} // namespace utils

/* eof */
