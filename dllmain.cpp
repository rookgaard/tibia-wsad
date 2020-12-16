#include <windows.h>

HMODULE origLibrary;
WNDPROC wndProc;

typedef void (*_PushLetter) (int Letter);
_PushLetter PushLetter;

bool isOnline() {
	return (*(DWORD*) 0x79CF28) == 8;
}

LRESULT CALLBACK HookedMessageDispatcher(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (!isOnline()) {
		return CallWindowProc(wndProc, hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg) {
		case WM_KEYDOWN:
		{
			switch (wParam) {
				case 0x57:
				{
					wParam = VK_UP;
					break;
				}
				case 0x53:
				{
					wParam = VK_DOWN;
					break;
				}
				case 0x41:
				{
					wParam = VK_LEFT;
					break;
				}
				case 0x44:
				{
					wParam = VK_RIGHT;
					break;
				}
			}

			return CallWindowProc(wndProc, hWnd, uMsg, wParam, lParam);
		}
		default:
		{
			return CallWindowProc(wndProc, hWnd, uMsg, wParam, lParam);
		}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

HWND WINAPI HookedCreateWindowEx(DWORD dwExStyle, LPCTSTR lpClassName, LPCTSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
	HWND m_hWnd = CreateWindowEx(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	wndProc = (WNDPROC) GetWindowLongPtr(m_hWnd, GWL_WNDPROC);
	SetWindowLongPtr(m_hWnd, GWL_WNDPROC, (LONG_PTR) HookedMessageDispatcher);

	return m_hWnd;
}

/*_cdecl */void _stdcall HookedPushLetter(int Letter) {
	if (!isOnline()) {
		PushLetter(Letter);
		return;
	}

	if (Letter == 'A' || Letter == 'a') {
		return;
	}

	if (Letter == 'W' || Letter == 'w') {
		return;
	}

	if (Letter == 'S' || Letter == 's') {
		return;
	}

	if (Letter == 'D' || Letter == 'd') {
		return;
	}

	PushLetter(Letter);
}

void HookCall(DWORD dwCallAddress, DWORD dwNewAddress) {
	DWORD dwOldProtect, dwNewProtect, dwNewCall;
	BYTE call[4];

	dwNewCall = dwNewAddress - dwCallAddress - 5;
	*(DWORD*) call = dwNewCall;

	VirtualProtect((LPVOID) (dwCallAddress + 1), 4, PAGE_EXECUTE_WRITECOPY, &dwOldProtect);
	dwCallAddress += 1;
	*(DWORD*) dwCallAddress = *(DWORD*) & call;
	VirtualProtect((LPVOID) (dwCallAddress), 5, dwOldProtect, &dwNewProtect);
}

static int InitMain() {
	char systemDirectory[MAX_PATH];
	GetSystemDirectory(systemDirectory, MAX_PATH);
	lstrcat(systemDirectory, "\\ddraw.dll");
	origLibrary = LoadLibrary(systemDirectory);

	if (!origLibrary) {
		exit(-1);
	}

	PushLetter = (_PushLetter) 0x458200;
	HookCall(0x4CFB8A, (DWORD) & HookedPushLetter);

	DWORD dwOldProtect, dwNewProtect, funcAddress, origAddress;
	funcAddress = (DWORD) & HookedCreateWindowEx;
	origAddress = (DWORD) ((int*) 0x5B8574);
	VirtualProtect((LPVOID) origAddress, 4, PAGE_READWRITE, &dwOldProtect);
	memcpy((LPVOID) origAddress, &funcAddress, 4);
	VirtualProtect((LPVOID) origAddress, 4, dwOldProtect, &dwNewProtect);

	return 1;
}

extern "C" {

	__declspec (dllexport) HRESULT WINAPI DirectDrawCreate(void* lpGUID, void* lplp, void* pUnkOuter) {
		FARPROC proc = GetProcAddress(origLibrary, "DirectDrawCreate");
		if (!proc)
			return E_INVALIDARG;

		return ((HRESULT(WINAPI *)(void*, void*, void*))(DWORD) (proc))(lpGUID, lplp, pUnkOuter);
	}

	BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
		switch (dwReason) {
			case DLL_PROCESS_ATTACH:
				return InitMain();
			case DLL_THREAD_ATTACH:
			case DLL_THREAD_DETACH:
			case DLL_PROCESS_DETACH:
				break;
		}

		return 1;
	}
}
