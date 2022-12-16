#include <cstdio>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <utility>
#include <iterator>
#include <algorithm>
#include "search_controller.hxx"

#define LOG_RETURN_IF_SQL_ERROR(rc) do { if(rc!=SQLITE_OK) { printf("SQL Failure: %s\n", sqlite3_errmsg(this->m_dbConn)); if(errmsg) sqlite3_free(errmsg);  return E_FAIL; } } while(0);
#define LOG_IF_SQL_ERROR(rc) do { if(rc!=SQLITE_OK) { printf("SQL Failure: %s\n", sqlite3_errmsg(this->m_dbConn)); if(errmsg) sqlite3_free(errmsg); } } while(0);


search_controller::search_controller()
{
}

std::vector<search_entry> search_controller::perform_search(const std::wstring& prompt)
{
	if (prompt.size() > 0 && prompt[0] == L'!')
	{
		std::wstring cmd;
		std::vector<std::wstring> args;
		this->parse_command(prompt, cmd, args);

		std::vector<search_entry> commands = {
			{ entry_type::builtin, L"!add", L"(name) (path)"},
			{ entry_type::builtin, L"!addlink", L"(name) (link)"},
			{ entry_type::builtin, L"!addsteam", L"(name) (appid)"},
			{ entry_type::builtin, L"!delete", L"(name)"}
		};

		std::vector<search_entry> matchingCommands{ };

		std::copy_if(commands.begin(), commands.end(), std::back_inserter(matchingCommands),
			[&cmd](const search_entry& entry) -> bool
			{
				return entry.m_name.rfind(cmd, 0) == 0;
			}
		);

		return matchingCommands;
	}
	else if (prompt.size() > 0)
	{
		return this->search_database(prompt, false);
	}
	else
		return { };
}

void search_controller::increment_usage(const search_entry& entry)
{
	int rc = 0;
	char* errmsg = 0;

	const auto name_narrow = this->wide_to_narrow(entry.m_name);

	const char* sql = "UPDATE Targets SET count = count + 1 WHERE name = ?;";
	sqlite3_stmt* stmt;
	rc = sqlite3_prepare_v2(this->m_dbConn, sql, -1, &stmt, 0);
	LOG_IF_SQL_ERROR(rc);
	if (rc != SQLITE_OK)
		return;

	rc = sqlite3_bind_text(stmt, 1, name_narrow.c_str(), -1, SQLITE_STATIC);
	LOG_IF_SQL_ERROR(rc);
	if (rc != SQLITE_OK)
		return;

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
}

std::vector<search_entry> search_controller::search_database(const std::wstring& prompt, bool commandMode)
{
	std::vector<search_entry> results{ };

	int rc = 0;
	char* errmsg = 0;

	const char* sql = "SELECT * FROM Targets WHERE LOWER(name) LIKE ?";
	sqlite3_stmt* stmt;
	rc = sqlite3_prepare_v2(this->m_dbConn, sql, -1, &stmt, 0);
	LOG_IF_SQL_ERROR(rc);
	if (rc != SQLITE_OK)
		return {};

	const auto prompt_narrow = this->wide_to_narrow(prompt);

	char parameter[256];
	snprintf(parameter, 256, "%s%%", prompt_narrow.c_str());

	rc = sqlite3_bind_text(stmt, 1, parameter, -1, SQLITE_STATIC);
	LOG_IF_SQL_ERROR(rc);
	if (rc != SQLITE_OK)
		return {};

	while (true)
	{
		rc = sqlite3_step(stmt);
		if (rc == SQLITE_DONE)
			break;
		if (rc == SQLITE_ROW)
		{
			const char* nameRaw = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
			if (!nameRaw)
				continue;
			const std::string name{ nameRaw };


			int typeRaw{ sqlite3_column_int(stmt, 2) };
			std::int64_t count{ sqlite3_column_int64(stmt, 4) };

			const char* pathRaw = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
			std::string path{ };
			if (pathRaw)
				path = std::string{ pathRaw };

			entry_type type{ entry_type::in_path };
			switch (typeRaw)
			{
			case 0:
				type = entry_type::in_path;
				if (commandMode)
					continue;
				break;
			case 1:
				type = entry_type::custom;
				if (commandMode)
					continue;
				break;
			case 2:
				type = entry_type::builtin;
				if (!commandMode)
					continue;
				break;
			case 3:
				type = entry_type::link;
				if (commandMode)
					continue;
				break;
			case 4:
				type = entry_type::steam;
				if (commandMode)
					continue;
				break;
			default:
				continue;
			}

			search_entry entry{ type, this->narrow_to_wide(name) };
			entry.m_accessed = count;
			entry.m_path = this->narrow_to_wide(path);
			results.push_back(entry);
		}
		else
		{
			LOG_IF_SQL_ERROR(rc);
			break;
		}
	}

	std::sort(results.begin(), results.end(), [](const search_entry& lhs, const search_entry& rhs) { return lhs.m_accessed > rhs.m_accessed; });

	return results;
}

std::wstring search_controller::narrow_to_wide(const std::string& in)
{
	std::wstring out(in.length(), L' ');
	std::copy(in.begin(), in.end(), out.begin());
	return out;
}

std::string search_controller::wide_to_narrow(const std::wstring& in)
{
	std::string out(in.length(), ' ');
	std::copy(in.begin(), in.end(), out.begin());
	return out;
}

void search_controller::parse_command(const std::wstring& prompt, std::wstring& cmd, std::vector<std::wstring>& args)
{
	args.clear();

	bool isFirst = true;
	std::wstring token;

	for (size_t i = 0; i < prompt.length(); i++) {
		wchar_t c = prompt[i];
		if (c == L' ') {
			if (isFirst)
			{
				isFirst = false;
				cmd = token;
			}
			else
			{
				args.push_back(token);
			}

			token = L"";
		}
		else if (c == L'\"') {
			i++;
			while (prompt[i] != L'\"' && i < prompt.length()) { token.push_back(prompt[i]); i++; }
		}
		else {
			token.push_back(prompt[i]);
		}
	}

	if (token != L" " && !token.empty())
	{
		if (isFirst)
			cmd = token;
		else
			args.push_back(token);
	}
}

bool search_controller::do_command(const std::wstring& cmd, const std::vector<std::wstring>& args)
{
	if (cmd.empty())
		return false;

	if (cmd == L"!add")
	{
		if (args.size() != 2)
			return false;

		this->add_custom(args[0], args[1], entry_type::custom);
		return true;
	}
	else if (cmd == L"!addlink")
	{
		if (args.size() != 2)
			return false;

		this->add_custom(args[0], args[1], entry_type::link);
		return true;
	}
	else if (cmd == L"!addsteam")
	{
		if (args.size() != 2)
			return false;

		wchar_t buf[512];
		::swprintf_s(buf, L"steam://launch/%s", args[1].c_str());

		this->add_custom(args[0], std::wstring{buf}, entry_type::steam);
		return true;
	}
	else if (cmd == L"!delete")
	{
		if (args.size() != 1)
			return false;

		this->remove_custom(args[0]);
		return true;
	}

	return false;
}

bool search_controller::commit(const search_entry& entry, const std::wstring& prompt)
{
	switch (entry.m_type)
	{
		case entry_type::in_path:
		{
			std::wstring filepath;
			if (!this->find_in_path(entry.m_name, filepath))
				return false;

			this->execute_path(filepath);

			this->increment_usage(entry);
			return true;
		}
		case entry_type::custom:
		{
			if (entry.m_path.empty())
				return false;

			this->execute_path(entry.m_path);

			this->increment_usage(entry);
			return true;
		}
		case entry_type::steam:
		case entry_type::link:
		{
			if (entry.m_path.empty())
				return false;

			this->execute_link(entry.m_path);

			this->increment_usage(entry);
			return true;
		}
		case entry_type::builtin:
		{
			std::wstring cmd;
			std::vector<std::wstring> args;
			this->parse_command(prompt, cmd, args);
			if (cmd.empty())
				return false;

			return this->do_command(cmd, args);
		}
		default:
		{
			return true;
		}
	}
}

void search_controller::add_new_in_path(const std::wstring& name)
{
	int rc = 0;
	char* errmsg = 0;

	const auto nameNarrow = this->wide_to_narrow(name);

	const char* sql = "INSERT INTO Targets(name, type, path, count) VALUES (?,0,NULL,1);";
	sqlite3_stmt* stmt;
	rc = sqlite3_prepare_v2(this->m_dbConn, sql, -1, &stmt, 0);
	LOG_IF_SQL_ERROR(rc);
	if (rc != SQLITE_OK)
		return;

	rc = sqlite3_bind_text(stmt, 1, nameNarrow.c_str(), -1, SQLITE_STATIC);
	LOG_IF_SQL_ERROR(rc);
	if (rc != SQLITE_OK)
		return;

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
}

void search_controller::execute_path(const std::wstring& path)
{
	::ShellExecute(NULL, NULL, path.c_str(), NULL, NULL, SW_SHOW);
}

void search_controller::execute_link(const std::wstring& link)
{
	::ShellExecute(NULL, L"open", link.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

void search_controller::remove_custom(const std::wstring& name)
{
	const auto nameNarrow = this->wide_to_narrow(name);

	int rc = 0;
	char* errmsg = 0;

	const char* sql = "DELETE FROM Targets WHERE name = ?";
	sqlite3_stmt* stmt;
	rc = sqlite3_prepare_v2(this->m_dbConn, sql, -1, &stmt, 0);
	LOG_IF_SQL_ERROR(rc);
	if (rc != SQLITE_OK)
		return;

	rc = sqlite3_bind_text(stmt, 1, nameNarrow.c_str(), -1, SQLITE_STATIC);
	LOG_IF_SQL_ERROR(rc);
	if (rc != SQLITE_OK)
		return;

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
}

void search_controller::add_custom(const std::wstring& name, const std::wstring& path, entry_type type)
{
	const auto nameNarrow = this->wide_to_narrow(name);
	const auto pathNarrow = this->wide_to_narrow(path);

	int rc = 0;
	char* errmsg = 0;

	const char* sql = "INSERT INTO Targets(name, type, path, count) VALUES (?,?,?,0);";
	sqlite3_stmt* stmt;
	rc = sqlite3_prepare_v2(this->m_dbConn, sql, -1, &stmt, 0);
	LOG_IF_SQL_ERROR(rc);
	if (rc != SQLITE_OK)
		return;

	rc = sqlite3_bind_text(stmt, 1, nameNarrow.c_str(), -1, SQLITE_STATIC);
	LOG_IF_SQL_ERROR(rc);
	if (rc != SQLITE_OK)
		return;

	rc = sqlite3_bind_int(stmt, 2, static_cast<int>(type));
	LOG_IF_SQL_ERROR(rc);
	if (rc != SQLITE_OK)
		return;

	rc = sqlite3_bind_text(stmt, 3, pathNarrow.c_str(), -1, SQLITE_STATIC);
	LOG_IF_SQL_ERROR(rc);
	if (rc != SQLITE_OK)
		return;

	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
}

std::wstring search_controller::get_first_token(const std::wstring& prompt)
{
	return prompt.substr(0, prompt.find(L' '));
}

bool search_controller::commit_new(const std::wstring& prompt)
{
	if (prompt.empty())
		return false;

	if (prompt[0] == L'!')
		return false;

	const auto name = this->get_first_token(prompt);

	std::wstring filepath;
	if (!this->find_in_path(name, filepath))
		return false;

	this->execute_path(filepath);

	this->add_new_in_path(name);
	return true;
}

bool search_controller::find_in_path(const std::wstring& name, std::wstring& filepath)
{
	LPWSTR filePart;
	wchar_t filename[MAX_PATH];
	if (!::SearchPath(NULL, name.c_str(), L".exe", MAX_PATH, filename, &filePart))
		return false;

	filepath = std::wstring{ filename };
	return true;
}

std::wstring search_controller::get_database_path()
{
	wchar_t appdata[MAX_PATH];
	::SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appdata);

	wchar_t prefix[MAX_PATH];
	::PathCombine(prefix, appdata, L"plauncher");

	::SHCreateDirectory(NULL, prefix);

	wchar_t path[MAX_PATH];
	::PathCombine(path, prefix, L"db.sqlite");

	return { path };
}

HRESULT search_controller::init_db()
{
	int rc = SQLITE_OK;
	char* errmsg = 0;

	const auto db_path_wide = this->get_database_path();
	const auto db_path = this->wide_to_narrow(db_path_wide);

	rc = sqlite3_open_v2(db_path.c_str(), &this->m_dbConn, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
	LOG_RETURN_IF_SQL_ERROR(rc);

	// Create table if not already exists
	const char* create_table_sql =
		"CREATE TABLE IF NOT EXISTS Targets ("
		"	id	INTEGER NOT NULL UNIQUE,"
		"	name	TEXT NOT NULL UNIQUE,"
		"	type	INTEGER NOT NULL DEFAULT 0,"
		"	path	TEXT,"
		"	count	INTEGER NOT NULL DEFAULT 0,"
		"	PRIMARY KEY(id AUTOINCREMENT)"
		"	);";

	rc = sqlite3_exec(this->m_dbConn, create_table_sql, 0, 0, &errmsg);
	LOG_RETURN_IF_SQL_ERROR(rc);

	return S_OK;
}

void search_controller::close_db()
{
	if (this->m_dbConn)
	{
		sqlite3_close(this->m_dbConn);
		this->m_dbConn = NULL;
	}
}