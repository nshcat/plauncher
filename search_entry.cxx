#include "search_entry.hxx"

search_entry search_entry::create_builtin(const std::wstring& name, const std::wstring& usage, const std::wstring& description)
{
	return search_entry{entry_type::builtin, name, usage, description, L"", 0};
}

entry_type search_entry::type() const
{
	return this->m_type;
}

const std::wstring& search_entry::name() const
{
	return this->m_name;
}

std::uint64_t search_entry::access_count() const
{
	return this->m_accessed;
}

const std::wstring& search_entry::path() const
{
	return this->m_path;
}

const std::wstring& search_entry::usage_str() const
{
	return this->m_usageStr;
}

bool search_entry::has_usage_str() const
{
	return !this->m_usageStr.empty();
}

const std::wstring& search_entry::description() const
{
	return this->m_description;
}

bool search_entry::has_description() const
{
	return !this->m_description.empty();
}

bool search_entry::has_path() const
{
	return !this->m_path.empty();
}
