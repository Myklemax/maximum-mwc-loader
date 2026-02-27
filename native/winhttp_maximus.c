// winhttp_maximus.c
// Minimal winhttp.dll proxy bootstrap for Maximus.
// WinHTTP exports are forwarded by winhttp.def; this DLL only runs bootstrap logic.

#if defined(__has_include)
#  if __has_include(<windows.h>)
#    include <windows.h>
#    define MAXIMUS_HAVE_WINDOWS_H 1
#  endif
#endif

#ifndef MAXIMUS_HAVE_WINDOWS_H
#include <stddef.h>
#include <wchar.h>

#define WINAPI
#define __declspec(x)

typedef void *HANDLE;
typedef HANDLE HINSTANCE;
typedef void *LPVOID;
typedef unsigned long DWORD;
typedef int BOOL;
typedef wchar_t WCHAR;
typedef const WCHAR *LPCWSTR;

typedef DWORD (*MaximusThreadStart)(LPVOID);
typedef void (*EntryFn)(void);

#define TRUE 1
#define MAX_PATH 260
#define FILE_END 2
#define GENERIC_WRITE 0x40000000
#define FILE_SHARE_READ 0x00000001
#define OPEN_ALWAYS 4
#define FILE_ATTRIBUTE_NORMAL 0x00000080
#define INVALID_HANDLE_VALUE ((HANDLE)(long long)-1)
#define DLL_PROCESS_ATTACH 1

extern __declspec(dllimport) HANDLE WINAPI CreateFileW(
    LPCWSTR,
    DWORD,
    DWORD,
    LPVOID,
    DWORD,
    DWORD,
    HANDLE
);
extern __declspec(dllimport) BOOL WINAPI WriteFile(HANDLE, const void *, DWORD, DWORD *, LPVOID);
extern __declspec(dllimport) DWORD WINAPI SetFilePointer(HANDLE, long, long *, DWORD);
extern __declspec(dllimport) BOOL WINAPI CloseHandle(HANDLE);
extern __declspec(dllimport) DWORD WINAPI GetModuleFileNameW(HINSTANCE, WCHAR *, DWORD);
extern __declspec(dllimport) BOOL WINAPI DisableThreadLibraryCalls(HINSTANCE);
extern __declspec(dllimport) HANDLE WINAPI CreateThread(LPVOID, size_t, MaximusThreadStart, LPVOID, DWORD, DWORD *);
extern __declspec(dllimport) HANDLE WINAPI LoadLibraryW(LPCWSTR);
extern __declspec(dllimport) LPVOID WINAPI GetProcAddress(HANDLE, const char *);

#define lstrcpyW wcscpy
#define lstrcatW wcscat
#define lstrlenW wcslen
#else
typedef void (*EntryFn)(void);
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

static WCHAR g_module_dir[MAX_PATH] = {0};

static void log_msg(const WCHAR *msg) {
    WCHAR path[MAX_PATH];
    lstrcpyW(path, g_module_dir);
    lstrcatW(path, L"\\maximus.log");
    HANDLE h = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    SetFilePointer(h, 0, NULL, FILE_END);
    DWORD bytes = 0;
    WriteFile(h, msg, (DWORD)(lstrlenW(msg) * sizeof(WCHAR)), &bytes, NULL);
    WriteFile(h, L"\r\n", 4, &bytes, NULL);
    CloseHandle(h);
}

static DWORD WINAPI bootstrap_thread(LPVOID context) {
    (void)context;
    WCHAR host_path[MAX_PATH];
    lstrcpyW(host_path, g_module_dir);
    lstrcatW(host_path, L"\\MaximusHost.dll");

    HANDLE host = LoadLibraryW(host_path);
    if (!host) {
        log_msg(L"[maximus] MaximusHost.dll not found; skipping bootstrap");
        return 0;
    }

    EntryFn entry = (EntryFn)GetProcAddress(host, "MaximusEntry");
    if (!entry) {
        log_msg(L"[maximus] MaximusEntry not found in host");
        return 0;
    }

    entry();
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinst);

        WCHAR module_path[MAX_PATH];
        if (GetModuleFileNameW(hinst, module_path, ARRAYSIZE(module_path))) {
            int i;
            for (i = (int)lstrlenW(module_path) - 1; i >= 0; --i) {
                if (module_path[i] == L'\\' || module_path[i] == L'/') {
                    module_path[i] = 0;
                    break;
                }
            }
            lstrcpyW(g_module_dir, module_path);
        }

        HANDLE t = CreateThread(NULL, 0, bootstrap_thread, NULL, 0, NULL);
        if (t) CloseHandle(t);
    }
    return TRUE;
}
