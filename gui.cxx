#include "gui.hxx"
#include "helpers.hxx"
#include <CommCtrl.h>
#include <cstdio>
#include <ShellScalingApi.h>

gui::gui(search_controller* controller)
	: m_controller{controller}
{

}

LRESULT gui::mainwnd_proc_static(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_CREATE)
	{
		CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);

		gui* thisPtr = reinterpret_cast<gui*>(cs->lpCreateParams);
		::SetPropW(hwnd, gui::instance_property, reinterpret_cast<HANDLE>(thisPtr));

		thisPtr->m_hwnd = hwnd;
	}

	// Retrieve instance pointer and call window proc
	gui* thisPtr = reinterpret_cast<gui*>(::GetPropW(hwnd, gui::instance_property));
	if (thisPtr)
		return thisPtr->mainwnd_proc(hwnd, msg, wParam, lParam);
	else
		return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT gui::mainwnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_COMMAND:
		{
			if (HIWORD(wParam) == LBN_SELCHANGE)
			{
				const int idx = ::SendMessage(this->m_hwndListBox, LB_GETCURSEL, 0, 0);
				if (idx != LB_ERR)
				{
					if (this->m_controller->commit(this->m_currentResults[idx], this->get_prompt()))
					{
						::PostMessage(this->m_hwnd, WM_CLOSE, 0, 0);
					}
				}		
			}
			break;
		}
		case WM_TIMER:
		{
			::KillTimer(this->m_hwnd, 1);
			::SetForegroundWindow(this->m_hwndEdit);
			::SetFocus(this->m_hwndEdit);
			break;
		}
		case WM_CREATE:
		{
			this->on_create();
			break;
		}
		case WM_KEYDOWN:
		{
			if (wParam == VK_ESCAPE)
				::PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		}
		/*case WM_CLOSE:
		{
			//::PostQuitMessage(0);
			return 0;
		}*/
		case WM_MEASUREITEM:
		{
			MEASUREITEMSTRUCT* mi = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
			mi->itemHeight = this->scale_pixels(18);
			return 1;
		}
		case WM_DRAWITEM:
		{
			DRAWITEMSTRUCT* dis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
			if (dis->itemID == -1)
				break;

			switch (dis->itemAction)
			{
				case ODA_SELECT:
				case ODA_DRAWENTIRE:
				{
					if (dis->itemID == this->m_selectedListElement)
					{
						HPEN pen = ::CreatePen(PS_SOLID, 1, RGB(240, 216, 192));
						const auto oldPen = ::SelectObject(dis->hDC, pen);

						HBRUSH brush = ::CreateSolidBrush(RGB(107, 105, 99));
						const auto oldBrush = ::SelectObject(dis->hDC, brush);

						Rectangle(dis->hDC, dis->rcItem.left, dis->rcItem.top, dis->rcItem.right, dis->rcItem.bottom);

						::SelectObject(dis->hDC, oldPen);
						::SelectObject(dis->hDC, oldBrush);
					}

					search_entry& item = this->m_currentResults[dis->itemID];

					TEXTMETRIC tmItalic;

					HFONT hFont = CreateFont(this->scale_pixels(12), 0, 0, 0, FW_DONTCARE, TRUE, FALSE, FALSE, ANSI_CHARSET,
						OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
						DEFAULT_PITCH | FF_DONTCARE, TEXT("Tahoma"));

					const auto oldFont = ::SelectObject(dis->hDC, hFont);

					::GetTextMetrics(dis->hDC, &tmItalic);

					auto yPosItalic = (dis->rcItem.bottom + dis->rcItem.top - tmItalic.tmHeight) / 2;

					const wchar_t* typeStr;
					switch (item.m_type)
					{
					case entry_type::builtin:
						typeStr = L"command";
						break;
					case entry_type::in_path:
						typeStr = L"in path";
						break;
					case entry_type::custom:
						typeStr = L"custom";
						break;
					case entry_type::link:
						typeStr = L"link";
						break;
					case entry_type::steam:
						typeStr = L"steam";
						break;
					default:
						typeStr = L"unknown";
						break;
					}

					auto textLen = ::lstrlenW(typeStr);
					::TextOut(dis->hDC, this->scale_pixels(4), yPosItalic, typeStr, textLen);

					::SelectObject(dis->hDC, oldFont);

					// Get item text
					const wchar_t* text = item.m_name.c_str();

					TEXTMETRIC tm;
					::GetTextMetrics(dis->hDC, &tm);

					const auto yPos = (dis->rcItem.bottom + dis->rcItem.top - tm.tmHeight) / 2;

					textLen = ::lstrlenW(text);

					::TextOut(dis->hDC, this->scale_pixels(4 + 55), yPos, text, textLen);

					// Draw usage string
					if (item.m_type == entry_type::builtin && item.m_usageStr.length() > 0)
					{
						// Find longest length among names to align all usage strings the same way
						int longestWidth = 0;
						for(int idx = 0; idx < min(this->m_maxListElements, this->m_currentResults.size()); ++idx)
						{
							const std::wstring name = this->m_currentResults[idx].m_name;
							SIZE textSz;
							::GetTextExtentPoint32(dis->hDC, name.c_str(), name.length(), &textSz);
							longestWidth = max(longestWidth, textSz.cx);
						}

						HFONT hInfoFont = CreateFont(this->scale_pixels(12), 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET,
							OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
							DEFAULT_PITCH | FF_DONTCARE, TEXT("Tahoma"));

						TEXTMETRIC tmInfo;
						::GetTextMetrics(dis->hDC, &tmInfo);

						auto yPosInfo = (dis->rcItem.bottom + dis->rcItem.top - tmInfo.tmHeight) / 2;		

						const auto oldFont = ::SelectObject(dis->hDC, hInfoFont);

						const auto oldColor = ::GetTextColor(dis->hDC);

						::SetTextColor(dis->hDC, RGB(186, 167, 148));

						::TextOut(dis->hDC, this->scale_pixels(4 + 55 + 8 + longestWidth), yPosItalic, item.m_usageStr.c_str(), item.m_usageStr.length());

						::SelectObject(dis->hDC, oldFont);
						::SetTextColor(dis->hDC, oldColor);
					}

					break;
				}
				default:
					break;
			}


			break;
		}
		case WM_CTLCOLORMSGBOX:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORBTN:
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSCROLLBAR:
		case WM_CTLCOLOREDIT:
		{
			HDC hdcStatic = (HDC)wParam;
			SetTextColor(hdcStatic, RGB(240, 216, 192));
			SetBkColor(hdcStatic, RGB(79, 78, 77));
			return (INT_PTR)CreateSolidBrush(RGB(107, 105, 99));
		}
		case WM_CTLCOLORSTATIC:
		{
			HDC hdcStatic = (HDC)wParam;
			SetTextColor(hdcStatic, RGB(240, 216, 192));
			SetBkColor(hdcStatic, RGB(79, 78, 77));
			return (INT_PTR)CreateSolidBrush(RGB(107, 105, 99));
		}
		default:
		{
			break;
		}
	}
	
	return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}

void gui::on_create()
{
	// === Edit box ===
	// Create edit box
	this->m_textBoxHeight = 22;

	this->m_hwndEdit = ::CreateWindow(
		WC_EDIT,
		L"",
		WS_BORDER | WS_CHILD | WS_VISIBLE | ES_WANTRETURN | ES_AUTOHSCROLL,
		0, 0,
		this->m_width, this->scale_pixels(this->m_textBoxHeight),
		this->m_hwnd, NULL, NULL, NULL
	);

	// Setup font and subclassing
	::SetWindowSubclass(this->m_hwndEdit,
		(SUBCLASSPROC)gui::edit_proc_static, 0,
		reinterpret_cast<DWORD_PTR>(this));

	HFONT hFont = CreateFont(this->scale_pixels(18), 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET,
		OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, TEXT("Tahoma"));
	::SendMessage(this->m_hwndEdit, WM_SETFONT, (WPARAM)hFont, 0);
	::SendMessage(this->m_hwnd, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(L""));
	// ===

	// === List box ===
	this->m_hwndListBox = ::CreateWindow(
		WC_LISTBOX,
		L"",
		WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_OWNERDRAWFIXED,
		0, this->scale_pixels(this->m_textBoxHeight),
		this->m_width, this->scale_pixels(10),
		this->m_hwnd,
		NULL, NULL, NULL
	);

	HFONT hFontList = CreateFont(this->scale_pixels(16), 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET,
		OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, TEXT("Tahoma"));
	::SendMessage(this->m_hwndListBox, WM_SETFONT, (WPARAM)hFontList, 0);
	// ===

	::SetWindowPos(this->m_hwnd, NULL, 0, 0, this->m_width, this->scale_pixels(this->m_textBoxHeight), SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOOWNERZORDER);
	
	::SetFocus(this->m_hwnd);
	::SetForegroundWindow(this->m_hwnd);
	::BringWindowToTop(this->m_hwnd);
	::SetFocus(this->m_hwndEdit);

	::SetTimer(this->m_hwnd, 1, 50, NULL);
}

int gui::scale_pixels(int in)
{
	float scaleFactor = (float)this->m_dpi / 96.0;

	return (int)((float)in * scaleFactor);
}

bool gui::is_registered() const
{
	return this->m_wndClassAtom != ATOM{ };
}

HRESULT gui::register_class()
{
	WNDCLASS wndclass = { 0 };

	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.lpszMenuName = NULL;
	wndclass.hInstance = NULL;
	wndclass.lpfnWndProc = (WNDPROC)gui::mainwnd_proc_static;
	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_NOCLOSE;
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszClassName = gui::main_class_name;

	this->m_wndClassAtom = ::RegisterClass(&wndclass);
	if (!this->m_wndClassAtom)
		return E_FAIL;

	return S_OK;
}

HWND gui::get_handle()
{
	return this->m_hwnd;
}

bool gui::exists()
{
	return ::IsWindow(this->m_hwnd);
}

HRESULT gui::create()
{
	HRESULT hr = S_OK;

	// Make sure the window class is registered
	if (!this->is_registered())
	{
		hr = this->register_class();
		RETURNHRSILENT_IF_ERROR(hr);
	}

	// Retrieve primary monitor
	const POINT pt = { 0, 0 };
	const HMONITOR primaryMonitor = ::MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);

	// Retrieve monitor info
	MONITORINFOEX monitorInfo{ };
	monitorInfo.cbSize = sizeof(MONITORINFOEX);
	if (!::GetMonitorInfo(primaryMonitor, &monitorInfo))
		return E_FAIL;

	// Retrieve DPI
	UINT xDpi, yDpi;
	hr = ::GetDpiForMonitor(primaryMonitor, MDT_EFFECTIVE_DPI, &xDpi, &yDpi);
	RETURNHRSILENT_IF_ERROR(hr);
	this->m_dpi = xDpi;

	// Position window, centered in x direction and in the upper third in y direction
	const auto monWidth = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
	const auto monHeight = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

	const auto windowWidth = (monWidth / 3);
	const auto windowHeight = 100;
	const auto windowPosX = monitorInfo.rcMonitor.left + ((monWidth / 2) - (windowWidth / 2));
	const auto windowPosY = monitorInfo.rcMonitor.top + (monHeight / 3);
	this->m_width = windowWidth;
	this->m_posx = windowPosX;
	this->m_posy = windowPosY;

	// Create window and make it visible
	this->m_hwnd = ::CreateWindow(main_class_name,
		L"plauncher",
		WS_POPUP | WS_SYSMENU,
		windowPosX,
		windowPosY,
		windowWidth,
		windowHeight,
		NULL,
		NULL,
		NULL,
		reinterpret_cast<LPVOID>(this)
	);

	::ShowWindow(this->m_hwnd, SW_SHOW);
	::UpdateWindow(this->m_hwnd);
	::SetForegroundWindow(this->m_hwnd);
	::SetFocus(this->m_hwnd);

	this->m_maxListElements = (int)((monHeight / 2) * 0.75f) / this->scale_pixels(18);

	return S_OK;
}

LRESULT gui::edit_proc_static(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR data)
{
	gui* thisPtr = reinterpret_cast<gui*>(data);

	if (thisPtr)
		return thisPtr->edit_proc(hwnd, msg, wParam, lParam);
	else
		return ::DefSubclassProc(hwnd, msg, wParam, lParam);
}

void gui::update_list()
{
	::LockWindowUpdate(this->m_hwndListBox);
	::SendMessage(this->m_hwndListBox, LB_RESETCONTENT, 0, 0);

	if (this->m_currentResults.size() > 0)
	{
		const auto itemHeight = ::SendMessage(this->m_hwndListBox, LB_GETITEMHEIGHT, 0, 0);
		const auto totalHeight = (this->m_currentResults.size()) * itemHeight;

		for(auto idx = 0; idx < min(this->m_maxListElements, this->m_currentResults.size()); ++idx)
		{
			const auto& entry = this->m_currentResults[idx];
			const auto retval = SendMessage(this->m_hwndListBox, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(entry.m_name.c_str()));
		}

		::ShowWindow(this->m_hwndListBox, SW_SHOW);
		::SetWindowPos(this->m_hwndListBox, NULL, 0, 0, this->m_width, totalHeight, SWP_NOZORDER | SWP_NOMOVE);
		::SetWindowPos(this->m_hwnd, NULL, 0, 0, this->m_width, this->scale_pixels(this->m_textBoxHeight) + totalHeight, SWP_NOZORDER | SWP_NOMOVE);
	}
	else
	{
		::ShowWindow(this->m_hwndListBox, SW_HIDE);
		::SetWindowPos(this->m_hwnd, NULL, 0, 0, this->m_width, this->scale_pixels(this->m_textBoxHeight), SWP_NOZORDER | SWP_NOMOVE);
	}
	::LockWindowUpdate(NULL);
}

std::wstring gui::get_prompt()
{
	wchar_t buf[256];
	::GetWindowText(this->m_hwndEdit, buf, 256);
	return { buf };
}

LRESULT gui::edit_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
	{
		if (wParam == VK_UP)
		{
			this->m_selectedListElement = max(0, this->m_selectedListElement - 1);
			::InvalidateRect(this->m_hwndListBox, NULL, TRUE);
			return 0;
		}
		else if (wParam == VK_DOWN)
		{
			this->m_selectedListElement = min(min(this->m_maxListElements - 1, this->m_currentResults.size() - 1), this->m_selectedListElement + 1);
			::InvalidateRect(this->m_hwndListBox, NULL, TRUE);
			return 0;
		}
		break;
	}
	case WM_CHAR:
	{
		if (wParam == VK_ESCAPE)
		{
			::PostMessage(this->m_hwnd, WM_CLOSE, 0, 0);
			return 0;
		}
		else if (wParam == VK_RETURN)
		{
			bool result = false;
			if (this->m_currentResults.size() > 0)
			{
				result = this->m_controller->commit(this->m_currentResults[this->m_selectedListElement], this->get_prompt());
			}
			else
			{
				result = this->m_controller->commit_new(this->get_prompt());
			}

			if(result)
				::PostMessage(this->m_hwnd, WM_CLOSE, 0, 0);

			return 0;
		}
		else
		{
			::DefSubclassProc(hwnd, msg, wParam, lParam);
			this->m_currentResults = this->m_controller->perform_search(this->get_prompt());
			this->m_selectedListElement = 0;
			this->update_list();
			return 0;
		}
		break;
	}
	default:
		break;
	}

	return ::DefSubclassProc(hwnd, msg, wParam, lParam);
}

ATOM gui::m_wndClassAtom = { };