#pragma once

#include <Windows.h>
#include <vector>
#include "search_controller.hxx"


class gui final
{
public:
	static constexpr wchar_t const* const instance_property = L"thisptr";
	static constexpr wchar_t const* const main_class_name = L"PLAUNCHER_MAIN_WND";

public:
	gui(search_controller* controller);
	HRESULT create();
	bool exists();
	HWND get_handle();

protected:
	// === Helpers ===
	bool is_registered() const;
	HRESULT register_class();
	void update_list();
	std::wstring get_prompt();
	int scale_pixels(int in);


	// === Event Handlers ===
	void on_create();

	// === Window Procs  ===
	static LRESULT mainwnd_proc_static(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT mainwnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	static LRESULT edit_proc_static(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR);
	LRESULT edit_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	search_controller* m_controller;
	std::vector<search_entry> m_currentResults;

	long m_width{ };
	long m_posx{ };
	long m_posy{ };
	long m_textBoxHeight{ };
	HWND m_hwnd{ };
	HWND m_hwndEdit{ };
	HWND m_hwndListBox{ };
	static ATOM m_wndClassAtom;
	int m_dpi{ };
};

