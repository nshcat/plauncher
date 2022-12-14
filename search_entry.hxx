#pragma once
#include <string>

enum class entry_type
{
	in_path = 0,
	custom = 1,
	builtin = 2		// For built-in commands like "!add"
};

struct search_entry final
{
	search_entry(entry_type type, const std::wstring& name)
		: m_type{type}, m_name{name}
	{

	}

	search_entry(entry_type type, const std::wstring& name, const std::wstring& usage)
		: m_type{ type }, m_name{ name }, m_usageStr{ usage }
	{

	}

	entry_type m_type{ entry_type::in_path };
	std::wstring m_name{ L"" };
	std::uint64_t m_accessed{ 0 };
	std::wstring m_path{ L"" };
	std::wstring m_usageStr{ L"" };
};