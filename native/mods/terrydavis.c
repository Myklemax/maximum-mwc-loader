// terrydavis.c — Maximum MWC Loader mod
// A tribute to Terry A. Davis — creator of TempleOS.
// Displays Terry quotes in the log while you drive.
#include <windows.h>

static char g_dir[MAX_PATH];

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

// ── Terry quotes ─────────────────────────────────────────────────────────────
static const char *quotes[] = {
    "[Terry] It's trivial. It's a Commodore 64 in a shoebox. U KNOW THAT RIGHT?",
    "[Terry] God said 640x480 16 color. I think that sounds beautiful.",
    "[Terry] I am the smartest programmer who has ever lived.",
    "[Terry] The compiler doesn't care about your feelings.",
    "[Terry] TempleOS is the Third Temple, and I built it.",
    "[Terry] An idiot admires complexity, a genius admires simplicity.",
    "[Terry] I talk to God every day. He gives me good ideas.",
    "[Terry] HolyC is better than C. God told me so.",
    "[Terry] I wrote a compiler, an OS, and a game. What did YOU do today?",
    "[Terry] The government put a man on the moon but they cant stop ME.",
    "[Terry] 640x480 is perfect resolution. God said so. I agree.",
    "[Terry] Ring 0. I live in Ring 0. Do you?",
    "[Terry] I started programming when I was 5. FIVE.",
    "[Terry] This car is a vehicle of God. Drive with purpose.",
    "[Terry] Temple of the Lord runs at 60fps. On a Pentium.",
};

#define NUM_QUOTES (sizeof(quotes) / sizeof(quotes[0]))

// ── quote thread ─────────────────────────────────────────────────────────────
static DWORD WINAPI terry_thread(LPVOID ctx) {
    (void)ctx;
    DWORD idx = 0;

    // Opening banner
    mod_log("[Terry] ───────────────────────────────────────");
    mod_log("[Terry]  TERRY A. DAVIS  (1969 - 2018)");
    mod_log("[Terry]  Creator of TempleOS  |  Top 1% IQ");
    mod_log("[Terry] ───────────────────────────────────────");

    Sleep(4000);

    while (1) {
        mod_log(quotes[idx % NUM_QUOTES]);
        idx++;
        // New quote every 30 seconds
        Sleep(30000);
    }
    return 0;
}

// ── Maximum mod entry point ───────────────────────────────────────────────────
__declspec(dllexport) void MaximumModInit(void) {
    HANDLE t = CreateThread(NULL, 0, terry_thread, NULL, 0, NULL);
    if (t) CloseHandle(t);
}

// ── DllMain ──────────────────────────────────────────────────────────────────
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinst);
        char buf[MAX_PATH];
        if (GetModuleFileNameA(hinst, buf, MAX_PATH)) {
            char *p = buf + lstrlenA(buf) - 1;
            while (p > buf && *p != '\\' && *p != '/') p--;
            *p = '\0';
            p--;
            while (p > buf && *p != '\\' && *p != '/') p--;
            if (*p == '\\' || *p == '/') *p = '\0';
            lstrcpyA(g_dir, buf);
        }
    }
    return TRUE;
}
