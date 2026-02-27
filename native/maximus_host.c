// maximus_host.c — loaded by winmm proxy, runs MaximusEntry()
// Writes a plain ASCII log (UTF-8) so it can be read by any tool.
#include <windows.h>

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

static char g_module_dir[MAX_PATH];

// Append one ASCII line to maximus.log
static void log_msg(const char *msg) {
    char path[MAX_PATH + 16];
    lstrcpyA(path, g_module_dir);
    lstrcatA(path, "\\maximus.log");
    HANDLE h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    SetFilePointer(h, 0, NULL, FILE_END);
    DWORD written;
    WriteFile(h, msg, (DWORD)lstrlenA(msg), &written, NULL);
    WriteFile(h, "\r\n", 2, &written, NULL);
    CloseHandle(h);
}

// Called by winmm proxy after game starts — put mod logic here
void MaximusEntry(void) {
    log_msg("[maximus] MaximusHost loaded");
    log_msg("[maximus] ready — hook is running");
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinst);
        // Capture game folder path so log_msg knows where to write
        char buf[MAX_PATH];
        if (GetModuleFileNameA(hinst, buf, ARRAYSIZE(buf))) {
            char *p = buf + lstrlenA(buf) - 1;
            while (p > buf && *p != '\\' && *p != '/') p--;
            if (*p == '\\' || *p == '/') *p = '\0';
            lstrcpyA(g_module_dir, buf);
        }
    }
    return TRUE;
}

