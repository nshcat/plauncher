#include <Windows.h>
#include <CommCtrl.h>
#include <shellscalingapi.h>

#include "gui.hxx"
#include "search_controller.hxx"

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define HOTKEY1 1000

gui* g = nullptr;
HANDLE mutexHandle{ };
search_controller controller{ };

void on_exit()
{
	::UnregisterHotKey(NULL, HOTKEY1);

	if (mutexHandle)
	{
		::ReleaseMutex(mutexHandle);
		::CloseHandle(mutexHandle);
		mutexHandle = 0;
	}

	controller.close_db();
}

int main()
{
	mutexHandle = ::CreateMutex(NULL, TRUE, L"plauncher_mutex");
	if (!mutexHandle || GetLastError() == ERROR_ALREADY_EXISTS)
		return -1;

	atexit(on_exit);

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