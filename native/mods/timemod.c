// timemod.c — Maximum MWC Loader mod
// Logs current system time (and uptime) every 5 seconds to maximum.log.
// Compile: see native/mods/build_mods.sh

#include <windows.h>

static char g_dir[MAX_PATH];   // game folder (where maximum.log lives)

// ── logging ──────────────────────────────────────────────────────────────────
static void mod_log(const char *msg) {
    char path[MAX_PATH + 16];
    lstrcpyA(path, g_dir);
    lstrcatA(path, "\\maximum.log");
    HANDLE h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    SetFilePointer(h, 0, NULL, FILE_END);
    DWORD w;
    WriteFile(h, msg, (DWORD)lstrlenA(msg), &w, NULL);
    WriteFile(h, "\r\n", 2, &w, NULL);
    CloseHandle(h);
}

// ── clock thread ─────────────────────────────────────────────────────────────
static DWORD WINAPI clock_thread(LPVOID ctx) {
    (void)ctx;
    char buf[128];

    mod_log("[TimeTemp] clock started — ticking every 5 sec");

    while (1) {
        SYSTEMTIME t;
        GetLocalTime(&t);

        // Format:  [TimeTemp] 14:03:27  (Sat Feb 28 2026)
        wsprintfA(buf,
            "[TimeTemp] %02d:%02d:%02d  (%d-%02d-%02d)",
            (int)t.wHour, (int)t.wMinute, (int)t.wSecond,
            (int)t.wYear, (int)t.wMonth, (int)t.wDay);

        mod_log(buf);
        Sleep(5000);
    }
    return 0;
}

// ── Maximum mod entry point ───────────────────────────────────────────────────
__declspec(dllexport) void MaximumModInit(void) {
    mod_log("[TimeTemp] TimeTemp mod loaded");
    HANDLE t = CreateThread(NULL, 0, clock_thread, NULL, 0, NULL);
    if (t) CloseHandle(t);
}

// ── DllMain — capture game folder path ──────────────────────────────────────
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinst);
        char buf[MAX_PATH];
        if (GetModuleFileNameA(hinst, buf, MAX_PATH)) {
            // Strip  \mods\TimeTemp.dll  →  game dir
            char *p = buf + lstrlenA(buf) - 1;
            while (p > buf && *p != '\\' && *p != '/') p--;
            *p = '\0'; // strip filename → ....\mods
            p--;
            while (p > buf && *p != '\\' && *p != '/') p--;
            if (*p == '\\' || *p == '/') *p = '\0'; // strip \mods → game dir
            lstrcpyA(g_dir, buf);
        }
    }
    return TRUE;
}
