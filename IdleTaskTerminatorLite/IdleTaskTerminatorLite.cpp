/*
 * Idle Task Terminator Lite
 *
 * Copyright (C) 2019 Peter Upfold.
 *
 * This file is licensed to you under the Apache License, version 2.0 (the "License").
 * You may not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under
 * the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 *
 */
// IdleTaskTerminatorLite.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "IdleTaskTerminatorLite.h"

#define MAX_LOADSTRING 100



// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// storing time last hook invoked
DWORD lastInteraction = 0;

// last time we printed this
DWORD lastDebugOutputTickCount = 0;
DWORD lastProcessCheck = 0;

// number of times hook has been called
ULONGLONG hookCalls = 0;

HHOOK llkeyboardHandle = 0;
HHOOK llmouseHandle = 0;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
void DebugShowTickCount(LPCWSTR context, DWORD internalTicks);
void TryKillShutdownProcess(DWORD ticks);
BOOLEAN AlreadyRunning();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_IDLETASKTERMINATORLITE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }
	if (AlreadyRunning()) {
		return FALSE;
	}

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_IDLETASKTERMINATORLITE));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }


	// clean up hooks

	UnhookWindowsHookEx(llmouseHandle);
	UnhookWindowsHookEx(llkeyboardHandle);

    return (int) msg.wParam;
}

//
BOOLEAN AlreadyRunning() {
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	HANDLE process;
	PROCESSENTRY32 pe32;

	if (snapshot == INVALID_HANDLE_VALUE) {
		OutputDebugString(L"Process snapshot had INVALID_HANDLE_VALUE\n");
		return FALSE;
	}

	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(snapshot, &pe32)) {
		OutputDebugString(L"Process32First failed\n");
		CloseHandle(snapshot);
		return FALSE;
	}

	do {
		if (_wcsicmp(pe32.szExeFile, L"idletaskterminatorlite.exe") == 0 ||
			_wcsicmp(pe32.szExeFile, L"idletaskterminator.exe") == 0) {

			// ignore current instance
			if (pe32.th32ProcessID == GetCurrentProcessId()) {
				continue;
			}

			OutputDebugString(L"Found existing instance!\n");
			
			CloseHandle(snapshot);
			return TRUE;
		}
	} while (Process32Next(snapshot, &pe32));
	CloseHandle(snapshot);
	return FALSE;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_IDLETASKTERMINATORLITE));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_IDLETASKTERMINATORLITE);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	const int bufLen = 512;
	wchar_t debugStrBuffer[bufLen];

	if (swprintf_s(debugStrBuffer, bufLen, L"Began with tick count %d\n", GetTickCount()) > 0) {
		OutputDebugString(debugStrBuffer);
	}


	llkeyboardHandle = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)GetProcAddress(hInst, "UpdateLastInteractionKeyboard"), hInst, 0);
	llmouseHandle = SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC)GetProcAddress(hInst, "UpdateLastInteractionMouse"), hInst, 0);


	return TRUE;
}

extern "C" __declspec(dllexport) LRESULT UpdateLastInteractionKeyboard(int nCode, WPARAM wParam, LPARAM lParam)
{
	hookCalls++;

	// reduce frequency of calling GetTickCount
	if (hookCalls & 0xF != 0) {
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	lastInteraction = GetTickCount();

	DebugShowTickCount(L"Keyboard", hookCalls);

	TryKillShutdownProcess(lastInteraction);

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void DebugShowTickCount(LPCWSTR context, DWORD hookCalls)
{
	//const int bufLen = 512;
	//DWORD ticks = GetTickCount();


	//if (ticks - lastDebugOutputTickCount > 1500) {
	//	wchar_t debugStrBuffer[bufLen];
	//	if (swprintf_s(debugStrBuffer, bufLen, L"%s: Update tick count %d (internal 0x%x)\n", context, ticks, hookCalls) > 0) {
	//		OutputDebugString(debugStrBuffer);
	//	}
	//	lastDebugOutputTickCount = ticks;
	//}
}


extern "C" __declspec(dllexport) LRESULT UpdateLastInteractionMouse(int nCode, WPARAM wParam, LPARAM lParam)
{
	hookCalls++;

	// reduce frequency of calling GetTickCount
	if (hookCalls & 0xF != 0) {
		return CallNextHookEx(NULL, nCode, wParam, lParam);
	}

	DebugShowTickCount(L"Mouse", hookCalls);
	lastInteraction = GetTickCount();
	TryKillShutdownProcess(lastInteraction);

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

//
//	FUNCTION: TryKillShutdownProcess
void TryKillShutdownProcess(DWORD ticks) {
	if (ticks - lastProcessCheck < 2500) {
		return;
	}

	lastProcessCheck = ticks;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	HANDLE process;
	PROCESSENTRY32 pe32;
	DWORD dwPriorityClass;
	HANDLE victim = NULL;

	if (snapshot == INVALID_HANDLE_VALUE) {
		OutputDebugString(L"Process snapshot had INVALID_HANDLE_VALUE\n");
		return;
	}

	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(snapshot, &pe32)) {
		OutputDebugString(L"Process32First failed\n");
		CloseHandle(snapshot);
		return;
	}

	do {
		if (_wcsicmp(pe32.szExeFile, L"beyondlogic.shutdown.exe") == 0) {
			OutputDebugString(L"Found it!\n");

			victim = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
			if (victim != NULL) {
				OutputDebugString(L"Got a handle\n");

				TerminateProcess(victim, 0);
				CloseHandle(victim);
			}

			break;
		}
	} while (Process32Next(snapshot, &pe32));

	//if (victim == NULL) {
	//	OutputDebugString(L"Did not find victim");
	//}

	CloseHandle(snapshot);
	return;

}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:

        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

