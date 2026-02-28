// maximum_host.c — loaded by winmm proxy, runs MaximumEntry()
// Writes a plain ASCII log (UTF-8) so it can be read by any tool.
#include <windows.h>

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

static char g_module_dir[MAX_PATH];

// Append one ASCII line to maximum.log
static void log_msg(const char *msg) {
    char path[MAX_PATH + 16];
    lstrcpyA(path, g_module_dir);
    lstrcatA(path, "\\maximum.log");
    HANDLE h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;
    SetFilePointer(h, 0, NULL, FILE_END);
    DWORD written;
    WriteFile(h, msg, (DWORD)lstrlenA(msg), &written, NULL);
    WriteFile(h, "\r\n", 2, &written, NULL);
    CloseHandle(h);
}

// Scan <game_dir>\mods\ for *.dll files, load each one,
// and call MaximumModInit() if the DLL exports it.
static void load_mods(void) {
    char pattern[MAX_PATH + 16];
    lstrcpyA(pattern, g_module_dir);
    lstrcatA(pattern, "\\mods\\*.dll");

    WIN32_FIND_DATAA fd;
    HANDLE hfind = FindFirstFileA(pattern, &fd);
    if (hfind == INVALID_HANDLE_VALUE) {
        log_msg("[mods] mods folder empty or not found");
        return;
    }

    char mod_path[MAX_PATH + 16];
    char msg[MAX_PATH + 64];
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

        lstrcpyA(mod_path, g_module_dir);
        lstrcatA(mod_path, "\\mods\\");
        lstrcatA(mod_path, fd.cFileName);

        HMODULE hmod = LoadLibraryA(mod_path);
        if (!hmod) {
            lstrcpyA(msg, "[mods] FAILED to load: ");
            lstrcatA(msg, fd.cFileName);
            log_msg(msg);
            continue;
        }

        typedef void (*ModInit_t)(void);
        ModInit_t init = (ModInit_t)(void *)GetProcAddress(hmod, "MaximumModInit");
        if (init) {
            init();
            lstrcpyA(msg, "[mods] loaded: ");
        } else {
            lstrcpyA(msg, "[mods] loaded (no MaximumModInit): ");
        }
        lstrcatA(msg, fd.cFileName);
        log_msg(msg);
    } while (FindNextFileA(hfind, &fd));

    FindClose(hfind);
}

// Called by winmm proxy after game starts
void MaximumEntry(void) {
    log_msg("[maximum] MaximumHost loaded");
    log_msg("[maximum] ready — hook is running");
    load_mods();
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

