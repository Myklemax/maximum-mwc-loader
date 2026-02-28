// winmm_maximum.c
// winmm.dll proxy for Maximum.
// Each exported function loads the REAL system winmm via LOAD_LIBRARY_SEARCH_SYSTEM32
// (bypasses the game folder entirely so there is no circular reference).
// DllMain bootstraps MaximumHost.dll.

#include <windows.h>

// Suppress harmless "dllimport redeclared" warnings when we define our own
// bodies for functions that windows.h declares as __declspec(dllimport).
#pragma GCC diagnostic ignored "-Wattributes"

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

// ------------ real winmm handle -----------------------------------------

static HMODULE g_real_winmm = NULL;

// Pre-loaded in DllMain BEFORE any stub can fire.
// Uses GetSystemDirectoryW so it always hits Wine's system32, never our proxy.
static void load_real_winmm(void) {
    if (g_real_winmm) return;
    WCHAR path[MAX_PATH];
    GetSystemDirectoryW(path, MAX_PATH);
    lstrcatW(path, L"\\winmm.dll");
    g_real_winmm = LoadLibraryW(path);
}

static HMODULE get_real(void) {
    return g_real_winmm;
}

// Helper: get a function pointer from the real winmm
#define REAL_PROC(name) ((void*)GetProcAddress(get_real(), #name))

// ------------ bootstrap ------------------------------------------------

static WCHAR g_dir[MAX_PATH];

typedef void (*EntryFn)(void);

static DWORD WINAPI bootstrap_thread(LPVOID ctx) {
    (void)ctx;
    WCHAR host_path[MAX_PATH];
    lstrcpyW(host_path, g_dir);
    lstrcatW(host_path, L"\\MaximumHost.dll");

    HMODULE host = LoadLibraryW(host_path);
    if (!host) return 0;

    EntryFn entry = (EntryFn)GetProcAddress(host, "MaximumEntry");
    if (entry) entry();
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        // Load the real system winmm FIRST, before any stub can be called.
        load_real_winmm();

        DisableThreadLibraryCalls(hinst);

        // Capture our own directory (game folder)
        WCHAR buf[MAX_PATH];
        if (GetModuleFileNameW(hinst, buf, ARRAYSIZE(buf))) {
            int i;
            for (i = (int)lstrlenW(buf) - 1; i >= 0; --i) {
                if (buf[i] == L'\\' || buf[i] == L'/') { buf[i] = 0; break; }
            }
            lstrcpyW(g_dir, buf);
        }

        HANDLE t = CreateThread(NULL, 0, bootstrap_thread, NULL, 0, NULL);
        if (t) CloseHandle(t);
    }
    return TRUE;
}

// ------------ pass-through stubs ---------------------------------------
// Each stub fetches the address from the real system winmm on first call.

#define STUB0(name, ret_t) \
    ret_t WINAPI name(void) { \
        typedef ret_t (WINAPI *fn_t)(void); \
        static fn_t _f; if (!_f) _f = (fn_t)REAL_PROC(name); \
        return _f(); \
    }

#define STUB1(name, ret_t, t1) \
    ret_t WINAPI name(t1 a1) { \
        typedef ret_t (WINAPI *fn_t)(t1); \
        static fn_t _f; if (!_f) _f = (fn_t)REAL_PROC(name); \
        return _f(a1); \
    }

#define STUB2(name, ret_t, t1, t2) \
    ret_t WINAPI name(t1 a1, t2 a2) { \
        typedef ret_t (WINAPI *fn_t)(t1, t2); \
        static fn_t _f; if (!_f) _f = (fn_t)REAL_PROC(name); \
        return _f(a1, a2); \
    }

#define STUB3(name, ret_t, t1, t2, t3) \
    ret_t WINAPI name(t1 a1, t2 a2, t3 a3) { \
        typedef ret_t (WINAPI *fn_t)(t1, t2, t3); \
        static fn_t _f; if (!_f) _f = (fn_t)REAL_PROC(name); \
        return _f(a1, a2, a3); \
    }

#define STUB4(name, ret_t, t1, t2, t3, t4) \
    ret_t WINAPI name(t1 a1, t2 a2, t3 a3, t4 a4) { \
        typedef ret_t (WINAPI *fn_t)(t1, t2, t3, t4); \
        static fn_t _f; if (!_f) _f = (fn_t)REAL_PROC(name); \
        return _f(a1, a2, a3, a4); \
    }

#define STUB5(name, ret_t, t1, t2, t3, t4, t5) \
    ret_t WINAPI name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5) { \
        typedef ret_t (WINAPI *fn_t)(t1, t2, t3, t4, t5); \
        static fn_t _f; if (!_f) _f = (fn_t)REAL_PROC(name); \
        return _f(a1, a2, a3, a4, a5); \
    }

// --- timeXxx ---
STUB1(timeBeginPeriod, MMRESULT, UINT)
STUB1(timeEndPeriod,   MMRESULT, UINT)
STUB0(timeGetTime,     DWORD)

// --- waveOut ---
// waveOutOpen(LPHWAVEOUT, UINT_PTR, LPWAVEFORMATEX, DWORD_PTR, DWORD_PTR, DWORD) -> 6 args
#define STUB6(name, ret_t, t1, t2, t3, t4, t5, t6) \
    ret_t WINAPI name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6) { \
        typedef ret_t (WINAPI *fn_t)(t1, t2, t3, t4, t5, t6); \
        static fn_t _f; if (!_f) _f = (fn_t)REAL_PROC(name); \
        return _f(a1, a2, a3, a4, a5, a6); \
    }

STUB6(waveOutOpen,           MMRESULT, LPHWAVEOUT, UINT, LPCWAVEFORMATEX, DWORD_PTR, DWORD_PTR, DWORD)
STUB1(waveOutClose,          MMRESULT, HWAVEOUT)
STUB1(waveOutReset,          MMRESULT, HWAVEOUT)
STUB3(waveOutPrepareHeader,  MMRESULT, HWAVEOUT,   LPWAVEHDR,  UINT)
STUB3(waveOutUnprepareHeader,MMRESULT, HWAVEOUT,   LPWAVEHDR,  UINT)
STUB3(waveOutWrite,          MMRESULT, HWAVEOUT,   LPWAVEHDR,  UINT)
STUB3(waveOutGetPosition,    MMRESULT, HWAVEOUT,   LPMMTIME,   UINT)
STUB0(waveOutGetNumDevs,     UINT)
STUB3(waveOutGetDevCapsA,    MMRESULT, UINT_PTR,   LPWAVEOUTCAPSA, UINT)
STUB3(waveOutGetDevCapsW,    MMRESULT, UINT_PTR,   LPWAVEOUTCAPSW, UINT)

// --- waveIn ---
// waveInOpen(LPHWAVEIN, UINT, LPCWAVEFORMATEX, DWORD_PTR, DWORD_PTR, DWORD) -> 6 args
STUB6(waveInOpen,            MMRESULT, LPHWAVEIN, UINT, LPCWAVEFORMATEX, DWORD_PTR, DWORD_PTR, DWORD)
STUB1(waveInClose,           MMRESULT, HWAVEIN)
STUB1(waveInStart,           MMRESULT, HWAVEIN)
STUB1(waveInReset,           MMRESULT, HWAVEIN)
STUB3(waveInPrepareHeader,   MMRESULT, HWAVEIN,   LPWAVEHDR,  UINT)
STUB3(waveInUnprepareHeader, MMRESULT, HWAVEIN,   LPWAVEHDR,  UINT)
STUB3(waveInAddBuffer,       MMRESULT, HWAVEIN,   LPWAVEHDR,  UINT)
STUB0(waveInGetNumDevs,      UINT)
STUB3(waveInGetDevCapsA,     MMRESULT, UINT_PTR,  LPWAVEINCAPSA, UINT)
STUB3(waveInGetDevCapsW,     MMRESULT, UINT_PTR,  LPWAVEINCAPSW, UINT)
