#pragma once
#include <string>

enum class entry_type
{
	in_path = 0,
	custom = 1,
	builtin = 2,		// For built-in commands like "!add"
	link = 3,
	steam = 4
};

class search_entry final
{
public:
	search_entry(	entry_type type,
					const std::wstring& name,
					const std::wstring& usage,
					const std::wstring& description,
					const std::wstring& path,
					std::uint64_t accesscount)
		:	m_type{ type }, m_name{ name },
			m_usageStr{ usage }, m_description{ description },
			m_path{ path }, m_accessed{ accesscount }
	{

	}

public:
	static search_entry create_builtin(	const std::wstring& name,
										const std::wstring& usage,
										const std::wstring& description);

public:
	entry_type type() const;
	const std::wstring& name() const;
	std::uint64_t access_count() const;
	const std::wstring& path() const;
	const std::wstring& usage_str() const;
	bool has_usage_str() const;
	const std::wstring& description() const;
	bool has_description() const;
	bool has_path() const;

protected:
	entry_type m_type{ entry_type::in_path };
	std::wstring m_name{ L"" };
	std::uint64_t m_accessed{ 0 };
	std::wstring m_path{ L"" };
	std::wstring m_usageStr{ L"" };
	std::wstring m_description{ L"" };
};