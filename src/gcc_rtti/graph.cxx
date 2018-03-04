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

#include "graph.hxx"

void graph_t::run()
{
	const string question = "Do you want to generate graph?\n";
	const int answer = ask_buttons("Yes", "No", "Cancel", ASKBTN_YES, question);

	if (answer != ASKBTN_YES)
	{
		return;
	}

	if (!fill_ignored_prefixes())
	{
		return;
	}

	process_ignored_prefixes();

	const string filepath = ask_file(true, "", "*.dot", "Choose file to which save the graph...");

	if (!filepath)
	{
		return;
	}

	save_to_file(filepath);
}

bool graph_t::fill_ignored_prefixes()
{
	const string default_value = "std\ntype_info";
	const string question = "List of ignored prefixes:";

	qstring ignore_namespaces_buffer;
	if (!ask_text(&ignore_namespaces_buffer, 2048, default_value, question))
	{
		return false;
	}

	sstring_t current;
	for (const char *c = ignore_namespaces_buffer.c_str(); *c; ++c)
	{
		if (*c == '\n' || *(c + 1) == '\0')
		{
			if (*c != '\n' && *(c + 1) == '\0')
			{
				current.append(*c);
			}
			if (!current.empty())
			{
				m_ignored_prefixes.push_back(current);
				current.clear();
			}
		}
		else
		{
			current.append(*c);
		}
	}

	return true;
}

void graph_t::process_ignored_prefixes()
{
	using class_t = gcc_rtti_t::class_t;

	const gcc_rtti_t::classes_t &classes = gcc_rtti_t::instance()->get_classes();

	for (const std::pair<const ea_t, unique_ptr_t<class_t>> &class_pair : classes)
	{
		if (!class_pair.second)
		{
			continue;
		}

		bool is_ignored = false;
		for (const sstring_t &ignored : m_ignored_prefixes)
		{
			if (class_pair.second->m_name.length() < ignored.length())
			{
				continue;
			}

			if (memcmp(class_pair.second->m_name.c_str(), ignored.c_str(), ignored.length()) == 0)
			{
				is_ignored = true;
			}
		}

		if (is_ignored && !class_pair.second->m_shown)
		{
			continue;
		}

		class_pair.second->m_shown = true;
		make_class_bases_visible(class_pair.second.get());
	}
}

void graph_t::make_class_bases_visible(gcc_rtti_t::class_t *const c)
{
	for (const gcc_rtti_t::class_t::base_t &base : c->m_bases)
	{
		if (!base.m_class)
		{
			continue;
		}

		base.m_class->m_shown = true;
		make_class_bases_visible(base.m_class);
	}
}

bool graph_t::save_to_file(const string filepath)
{
	FILE *const file = qfopen(filepath, "wb");
	if (!file)
	{
		warning("Unable to open file for write!");
		return false;
	}

	qfprintf(file, "digraph G {\n");
	qfprintf(file, "graph [overlap=scale]; node [fontname=Courier]; rankdir=\"LR\";\n\n");

	const auto &classes = gcc_rtti_t::instance()->get_classes();

	for (const auto &class_pair : classes)
	{
		if (!class_pair.second || !class_pair.second->m_shown)
		{
			continue;
		}

		qfprintf(file, " a%u [shape=box, label = \"%s\", color=\"blue\", tooltip=\"" ADDR_FORMAT "\"]\n",
				 class_pair.second->m_id, class_pair.second->m_name.c_str(), class_pair.first);
	}

	for (const auto &class_pair : classes)
	{
		if (!class_pair.second || !class_pair.second->m_shown)
		{
			continue;
		}

		for (const auto &base : class_pair.second->m_bases)
		{
			if (!base.m_class)
			{
				continue;
			}

			qfprintf(file, " a%u -> a%u [style = bold]\n", class_pair.second->m_id, base.m_class->m_id);
		}
	}

	qfprintf(file, "}");
	qflush(file);
	qfclose(file);
	return true;
}

/* eof */
