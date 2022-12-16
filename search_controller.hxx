#pragma once
#include <vector>
#include <string>
#include <sqlite3.h>
#include "search_entry.hxx"

class search_controller final
{
public:
	search_controller();

public:
	std::vector<search_entry> perform_search(const std::wstring& prompt);
	bool commit(const search_entry& entry, const std::wstring& prompt);
	bool commit_new(const std::wstring& prompt);

	HRESULT init_db();
	void close_db();

protected:
	void execute_path(const std::wstring& path);
	void execute_link(const std::wstring& link);
	std::wstring get_database_path();
	std::vector<search_entry> search_database(const std::wstring& prompt, bool commandMode);
	std::string wide_to_narrow(const std::wstring& in);
	std::wstring narrow_to_wide(const std::string& in);
	void increment_usage(const search_entry& entry);
	void add_new_in_path(const std::wstring& name);
	bool find_in_path(const std::wstring& name, std::wstring& filepath);
	std::wstring get_first_token(const std::wstring& prompt);
	void parse_command(const std::wstring& prompt, std::wstring& cmd, std::vector<std::wstring>& args);
	bool do_command(const std::wstring& cmd, const std::vector<std::wstring>& args);
	void add_custom(const std::wstring& name, const std::wstring& path, entry_type type);
	void remove_custom(const std::wstring& name);

protected:
	sqlite3* m_dbConn{ };
};