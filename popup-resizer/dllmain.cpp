#include <Windows.h>
#include "MinHook.h"

//#pragma comment(lib,"libMinHook-x86-v120-md.lib")


bool GetTaskbarState()
{
    APPBARDATA data;
    return SHAppBarMessage(4, &data);
}

bool bEnlarged = false;
void UnHook();
HMODULE g_hDll;


// https://blog.csdn.net/weixin_44256803/article/details/102614168
HANDLE free(DWORD dwProcID, LPVOID lpModuleAddress)
{
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcID);

    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");

    LPTHREAD_START_ROUTINE lpStartAddress = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "FreeLibrary");

    HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, 0, lpStartAddress, lpModuleAddress, 0, NULL);

    //WaitForSingleObject(hRemoteThread, 2000);
    //CloseHandle(hRemoteThread);
    //CloseHandle(hProcess);
    return hRemoteThread;
}


typedef BOOL(WINAPI *OldSetWindowTextW)(HWND, LPCWSTR);
OldSetWindowTextW fpSetWindowTextW = NULL;
BOOL WINAPI MySetWindowTextW(HWND hWnd, LPCWSTR lpString)
{
    BOOL ret = fpSetWindowTextW(hWnd, lpString);
    //MessageBox(NULL, L"设置标题", lpString, MB_OK);
    if(wcsncmp(lpString, L"exhoo", 5)==0) {
        UnHook();
        free(GetCurrentProcessId(), g_hDll);
        //FreeLibraryAndExitThread(g_hDll, 0);  
    }
    return ret;
}

typedef BOOL(WINAPI *OldMoveWindow)(HWND, int, int, int, int, BOOL);
OldMoveWindow fpMoveWindow = NULL;
BOOL WINAPI MyMoveWindow(_In_ HWND hWnd,
    _In_ int X,
    _In_ int Y,
    _In_ int nWidth,
    _In_ int nHeight,
    _In_ BOOL bRepaint)
{
    BOOL ret = fpMoveWindow(hWnd, X, 0, nWidth, nHeight, bRepaint);
    return ret;
}


static DWORD dwNScStyle = WS_CAPTION|WS_THICKFRAME; 

typedef BOOL(WINAPI *OldSetWindowPos)(HWND, HWND, int, int, int, int, UINT);
OldSetWindowPos fpSetWindowPos = NULL;
BOOL WINAPI MySetWindowPos(_In_ HWND hWnd,
    _In_opt_ HWND hWndInsertAfter,
    _In_ int X,
    _In_ int Y,
    _In_ int cx,
    _In_ int cy,
    _In_ UINT uFlags)
{
    if(Y>10 && !bEnlarged) {
        hWndInsertAfter = HWND_TOPMOST;
        //auto style = GetWindowLong(hWnd, GWL_STYLE);
        //SetWindowLong(hWnd, GWL_STYLE , style & ~dwNScStyle | WS_POPUP );
        //auto style = GetWindowLong(hWnd, GWL_EXSTYLE);
        //SetWindowLong(hWnd, GWL_EXSTYLE , style & ~0x8 );
        uFlags=0;
    }
    if(Y>50 && Y<110 && cx>900) {
        Y-=50; // assume it's extension popup openned not in fullscreen, move up!
    } else {
        if(bEnlarged && cx<900) {
            cy-=25; // since it's not moving up (bookmark folder popup), substract back the extra bottom space.
        }
    }
    BOOL ret = fpSetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);

    return ret;
}

typedef UINT_PTR(WINAPI *OldShellMsg)(DWORD, PAPPBARDATA);
OldShellMsg fpSHAppBarMessage = NULL;
BOOL WINAPI MySHAppBarMessage(DWORD hWnd, PAPPBARDATA lpString)
{
    //MessageBox(NULL, L"MySHAppBarMessage", L"Success", MB_OK);
    return 1;
}


typedef BOOL(WINAPI *OldSystemParametersInfoW)(UINT , UINT  , PVOID , UINT);
OldSystemParametersInfoW fpSystemParametersInfoW = NULL;
BOOL WINAPI MySystemParametersInfoW(UINT hWnd, UINT lpString, PVOID p3, UINT p4)
{
    //MessageBox(NULL, L"MySystemParametersInfoW", L"Success", MB_OK);
    return false;
}


typedef BOOL(WINAPI *OldFindWindowW)(UINT , UINT  , PVOID , UINT);
OldFindWindowW fpFindWindowW = NULL;
BOOL WINAPI MyFindWindowW(UINT hWnd, UINT lpString, PVOID p3, UINT p4)
{
    //MessageBox(NULL, L"MyFindWindowW", L"Success", MB_OK);
    return false;
}


typedef BOOL(WINAPI *OldGetMonitorInfoW)(HMONITOR  , LPMONITORINFO);
OldGetMonitorInfoW fpGetMonitorInfoW = NULL;
BOOL WINAPI MyGetMonitorInfoW(_In_ HMONITOR hMonitor, _Inout_ LPMONITORINFO lpmi)
{
    //MessageBox(NULL, L"MyGetMonitorInfoW", L"Success", MB_OK);
    BOOL ret = fpGetMonitorInfoW(hMonitor, lpmi);

    auto realHeight = lpmi->rcMonitor.bottom;
    bEnlarged = lpmi->rcWork.bottom < realHeight;
    if(bEnlarged) {
        lpmi->rcWork.bottom = realHeight; // 1. popups should overlap the taskbar
        lpmi->rcWork.bottom += 25; // 2. extensions should move up, so extra bottom space could be used.
    }

    // Mutex lock communication from ahk is too slow
    //auto hMutexTemp = CreateMutex(NULL, FALSE, L"knext");
    //if (GetLastError() == ERROR_ALREADY_EXISTS) // 检查互斥体是否已存在
    //{
    //}
    //if(hMutexTemp) {
    //    ReleaseMutex(hMutexTemp);
    //    CloseHandle(hMutexTemp);
    //}

    return true;
}



bool SetHook()
{
    if (MH_Initialize() == MB_OK)
    {
        MH_CreateHook(&SetWindowTextW, &MySetWindowTextW, reinterpret_cast<void**>(&fpSetWindowTextW));
        MH_EnableHook(&SetWindowTextW);

        //MH_CreateHook(&SHAppBarMessage, &MySHAppBarMessage, reinterpret_cast<void**>(&fpSHAppBarMessage));
        //MH_EnableHook(&SHAppBarMessage);


        //MH_CreateHook(&SystemParametersInfoW, &MySystemParametersInfoW, reinterpret_cast<void**>(&fpSystemParametersInfoW));
        //MH_EnableHook(&SystemParametersInfoW);


        //MH_CreateHook(&FindWindowW, &MyFindWindowW, reinterpret_cast<void**>(&fpFindWindowW));
        //MH_EnableHook(&FindWindowW);
        
        MH_CreateHook(&GetMonitorInfoW, &MyGetMonitorInfoW, reinterpret_cast<void**>(&fpGetMonitorInfoW));
        MH_EnableHook(&GetMonitorInfoW);

        //MH_CreateHook(&MoveWindow, &MyMoveWindow, reinterpret_cast<void**>(&fpMoveWindow));
        //MH_EnableHook(&MoveWindow);

        MH_CreateHook(&SetWindowPos, &MySetWindowPos, reinterpret_cast<void**>(&fpSetWindowPos));
        MH_EnableHook(&SetWindowPos);

        return true;
    }
    return false;
}

void UnHook()
{
    //if (MH_DisableHook(&SetWindowTextW) == MB_OK)
    //{
    //    MH_Uninitialize();
    //}
    MH_DisableHook(&SetWindowTextW);
    MH_DisableHook(&GetMonitorInfoW);
    MH_DisableHook(&SetWindowPos);
    MH_Uninitialize();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        //MessageBox(NULL, L"Ez Injected", L"Success", MB_OK);
        g_hDll = hModule;
        SetHook();
        //if(SetHook())
        //    MessageBox(NULL, L"DLL Injected", L"Success", MB_OK);
        //else
        //    MessageBox(NULL, L"DLL Injected", L"Error", MB_OK);
        break;
    case DLL_PROCESS_DETACH:
        //MessageBox(NULL, L"DLL UnInjected", L"Success", MB_OK);
        //UnHook();
        break;
    }
    return TRUE;
}
