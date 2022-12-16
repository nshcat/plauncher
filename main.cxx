#include <Windows.h>
#include <CommCtrl.h>
#include <shellscalingapi.h>

#include "gui.hxx"
#include "resource.h"
#include "search_controller.hxx"

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define HOTKEY1 1000

gui* g = nullptr;
HINSTANCE instance{ };
HANDLE mutexHandle{ };
search_controller controller{ };
HWND hwndInvisWindow{ };
UINT WM_TASKBAR_RESTART{ };
UINT WM_NOTIF_ICON_EVENT{ };

void update_notif_icon(bool add)
{
	NOTIFYICONDATA nid{ };
	nid.cbSize = sizeof(nid);
	nid.hWnd = hwndInvisWindow;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_NOTIF_ICON_EVENT;
	nid.hIcon = reinterpret_cast<HICON>(
		LoadImage(instance, MAKEINTRESOURCE(IDI_APPICON), IMAGE_ICON, 16, 16, 0));
	lstrcpy(nid.szTip, L"plauncher");
	::Shell_NotifyIcon(add ? NIM_ADD : NIM_DELETE, &nid);
}

void on_exit()
{
	::UnregisterHotKey(NULL, HOTKEY1);

	if (mutexHandle)
	{
		::ReleaseMutex(mutexHandle);
		::CloseHandle(mutexHandle);
		mutexHandle = 0;
	}

	update_notif_icon(false);

	controller.close_db();
}

UINT show_notif_icon_menu()
{
	HMENU allMenus = ::LoadMenu(instance, MAKEINTRESOURCE(IDR_MENU));
	HMENU subMenu = ::GetSubMenu(allMenus, 0);
	::SetMenuDefaultItem(subMenu, ID_QUIT, MF_BYCOMMAND);


	POINT cursor_pos;
	::GetCursorPos(&cursor_pos);
	::SetForegroundWindow(hwndInvisWindow);

	UINT id = ::TrackPopupMenu(
		subMenu,
		TPM_RETURNCMD | TPM_NONOTIFY,
		cursor_pos.x, cursor_pos.y,
		0,
		hwndInvisWindow,
		nullptr);
	::PostMessage(hwndInvisWindow, WM_NULL, 0, 0);
	::DestroyMenu(allMenus);

	return id;
}

LRESULT CALLBACK invis_window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR subclass_id, DWORD_PTR ref_data)
{
	switch (msg)
	{
		case WM_DESTROY:
		{
			::PostQuitMessage(0);
			break;
		}
		default:
		{
			if (msg == WM_TASKBAR_RESTART)
				update_notif_icon(true);
			else if (msg == WM_NOTIF_ICON_EVENT)
			{
				if (lParam == WM_RBUTTONUP)
				{
					const auto result = show_notif_icon_menu();
					if (result == ID_QUIT)
						::DestroyWindow(hwndInvisWindow);
				}
			}
			break;
		}
	}

	return ::DefSubclassProc(hwnd, msg, wParam, lParam);
}

int CALLBACK WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nShowCmd)
{
	mutexHandle = ::CreateMutex(NULL, TRUE, L"plauncher_mutex");
	if (!mutexHandle || GetLastError() == ERROR_ALREADY_EXISTS)
		return -1;

	atexit(on_exit);

	WM_TASKBAR_RESTART = ::RegisterWindowMessage(TEXT("TaskbarCreated"));
	WM_NOTIF_ICON_EVENT = ::RegisterWindowMessage(TEXT("NotifIconEvent"));
	instance = static_cast<HINSTANCE>(GetModuleHandle(nullptr));

	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_STANDARD_CLASSES;
	::InitCommonControlsEx(&icex);

	::SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
	::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	
	HRESULT hr = controller.init_db();
	if (FAILED(hr))
	{
		printf("Failed to initialize DB connection");
		return -1;
	}

	hwndInvisWindow = ::CreateWindow(
		L"STATIC", L"plauncher_invis",
		0, 0, 0, 0, 0, NULL, NULL, instance,
		nullptr
	);

	::SetWindowSubclass(hwndInvisWindow, invis_window_proc, 0, 0);
	update_notif_icon(true);

	::RegisterHotKey(NULL, HOTKEY1, MOD_CONTROL | MOD_SHIFT, 0x52);

	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0) > 0)
	{
		if (msg.message == WM_HOTKEY && LOWORD(msg.wParam) == HOTKEY1)
		{
			if (!g)
			{
				g = new gui(&controller);
				g->create();
			}
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (g && !g->exists())
			{
				delete g;
				g = nullptr;
			}
		}
	}

	controller.close_db();
	::UnregisterHotKey(NULL, HOTKEY1);
	::ReleaseMutex(mutexHandle);
	::CloseHandle(mutexHandle);
	mutexHandle = 0;
	return msg.wParam;
}