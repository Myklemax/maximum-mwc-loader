// maximum_host.c — loaded by winmm proxy, runs MaximumEntry()
// Writes a plain ASCII log (UTF-8) so it can be read by any tool.
#include <windows.h>

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

static char g_module_dir[MAX_PATH];

// Forward declaration for built-in tribute thread
static void log_msg(const char *msg);

// Minimal Mono embedding types
typedef struct _MonoDomain MonoDomain;
typedef struct _MonoAssembly MonoAssembly;
typedef struct _MonoImage MonoImage;
typedef struct _MonoClass MonoClass;
typedef struct _MonoMethod MonoMethod;
typedef struct _MonoObject MonoObject;
typedef struct _MonoString MonoString;
typedef struct _MonoThread MonoThread;

// Function pointer table for the Unity Mono runtime
typedef struct mono_api {
    HMODULE dll;
    MonoDomain *(*mono_get_root_domain)(void);
    MonoThread *(*mono_thread_attach)(MonoDomain *domain);
    MonoAssembly *(*mono_domain_assembly_open)(MonoDomain *domain, const char *name);
    MonoImage *(*mono_assembly_get_image)(MonoAssembly *assembly);
    MonoClass *(*mono_class_from_name)(MonoImage *image, const char *name_space, const char *name);
    MonoMethod *(*mono_class_get_method_from_name)(MonoClass *klass, const char *name, int param_count);
    MonoObject *(*mono_runtime_invoke)(MonoMethod *method, void *obj, void **params, MonoObject **exc);
    MonoString *(*mono_string_new)(MonoDomain *domain, const char *text);
} mono_api;

// Resolve the Mono embedding API exported by the game's mono.dll
static int resolve_mono(mono_api *api) {
    HMODULE dll = GetModuleHandleA("mono.dll");
    if (!dll) {
        char path[MAX_PATH];
        lstrcpyA(path, g_module_dir);
        lstrcatA(path, "\\mywintercar_Data\\Mono\\mono.dll");
        dll = LoadLibraryA(path);
    }
    if (!dll) {
        log_msg("[managed] mono.dll not loaded; skipping managed mods");
        return 0;
    }

    api->dll = dll;
    api->mono_get_root_domain = (MonoDomain *(*)(void))GetProcAddress(dll, "mono_get_root_domain");
    api->mono_thread_attach = (MonoThread *(*)(MonoDomain *))GetProcAddress(dll, "mono_thread_attach");
    api->mono_domain_assembly_open = (MonoAssembly *(*)(MonoDomain *, const char *))GetProcAddress(dll, "mono_domain_assembly_open");
    api->mono_assembly_get_image = (MonoImage *(*)(MonoAssembly *))GetProcAddress(dll, "mono_assembly_get_image");
    api->mono_class_from_name = (MonoClass *(*)(MonoImage *, const char *, const char *))GetProcAddress(dll, "mono_class_from_name");
    api->mono_class_get_method_from_name = (MonoMethod *(*)(MonoClass *, const char *, int))GetProcAddress(dll, "mono_class_get_method_from_name");
    api->mono_runtime_invoke = (MonoObject *(*)(MonoMethod *, void *, void **, MonoObject **))GetProcAddress(dll, "mono_runtime_invoke");
    api->mono_string_new = (MonoString *(*)(MonoDomain *, const char *))GetProcAddress(dll, "mono_string_new");

    if (!api->mono_get_root_domain || !api->mono_thread_attach || !api->mono_domain_assembly_open ||
        !api->mono_assembly_get_image || !api->mono_class_from_name || !api->mono_class_get_method_from_name ||
        !api->mono_runtime_invoke || !api->mono_string_new) {
        log_msg("[managed] mono API missing, cannot load managed mods");
        return 0;
    }

    return 1;
}

// ── Terry Davis tribute (built-in) ─────────────────────────────────────────-
static const char *k_terry_quotes[] = {
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
};

static DWORD WINAPI terry_thread(LPVOID ctx) {
    (void)ctx;
    DWORD idx = 0;

    // Portrait banner
    log_msg("[Terry]              =  #+*=#%%%%%%%%%%%%                          ");
    log_msg("[Terry]              =#*%%%%%*::=-=*%%%%%                          ");
    log_msg("[Terry]              ##%%%.......:::--*-%%                         ");
    log_msg("[Terry]              %*#.. .  ....:::--%*#-                        ");
    log_msg("[Terry]              %#..  ...  .+* :::-++--                       ");
    log_msg("[Terry]               %:......:=@@@@%..:-==.:                      ");
    log_msg("[Terry]                #:..:.:*%@@@%....:-=-:%                     ");
    log_msg("[Terry]                 #+#%@%::::. .....::==%#%##                 ");
    log_msg("[Terry]                  %%@@@+.::-. ...:.::==*%#==*               ");
    log_msg("[Terry]                   %@::.. ..:....::..-==+-+***+=#*==#**:++**");
    log_msg("[Terry]                      -..+:.......:::=-====+#-++==***:*=++**-++");
    log_msg("[Terry]                        :.::.::...::-=---:#*#=*+*+**=+***-***++**");
    log_msg("[Terry]                        %-.:.::::..-*----*++*#***+==-:--:.-=*++-++");
    log_msg("[Terry]                       **+#=-:... :#==--:+++:*-:++++=+*++=++++-:***+");
    log_msg("[Terry]                      +#*-**%%:-=+@#--::*=++-*+-*+****+*==**=:-+*+*+*");
    log_msg("[Terry]                    #+++**++*#*@==*=-:.+%%=+#**-+-=+-=-=:--+=-+*=**=+");
    log_msg("[Terry]                   =+*##+******=+**=+-+**==+=+**==+=++**=**-***#*+-*+");
    log_msg("[Terry]                 *#*++#*+=++*##+*=**=***++*#*+*+*=*===+***=++***-==**");
    log_msg("[Terry]                *+*#+##++*###*+*#*-*+=-==*+++-=----::::++*++*:*==*#**");
    log_msg("[Terry]               --=+%%##+--*# +#**+=*#*=+=+**=++++#**++==+****.++=***-");
    log_msg("[Terry]              ---=++#%#*##*  +**##.***=+++#*++***#+*+*+#*#+*:=+****-=");
    log_msg("[Terry]              -:--+*# %##%    *%##++==:=*===+--:+***-+*#*=#+++*=*+*=*");
    log_msg("[Terry]       -::::---::-=*#         +#*++##*+++#++#+*#*##*=++-=##+#+**+:=-=");
    log_msg("[Terry]      :::.::::::::=+#++++     *#**%-==:=##+*+*+*##****###%*++**++%#*#");
    log_msg("[Terry]     :::::.:::--:::-=#==+++++++*#*%###+*##-*:-+*+--==+*###%      ****");
    log_msg("[Terry]     ---:-:::-:---::-=*==+=++++##+%###+#+***+*#%##**++=####        #%");
    log_msg("[Terry]     -----::+=-=+-::::-++===+=+##%##%#++**+*#*#+=+***#%*#%%          ");
    log_msg("[Terry]    -==-==-=*=**+=-:.::-:-*===*%%%*#%#*#%*%##*##**+==*=*%#%          ");
    log_msg("[Terry] ");
    log_msg("[Terry]  TERRY A. DAVIS  (1969 - 2018)");
    log_msg("[Terry]  Creator of TempleOS  |  Genius  |  Top 1% IQ");
    log_msg("[Terry]  'God is in the terminal.'");
    log_msg("[Terry] ");

    Sleep(4000);

    while (1) {
        log_msg(k_terry_quotes[idx % ARRAYSIZE(k_terry_quotes)]);
        idx++;
        Sleep(30000); // new quote every 30s
    }
    return 0;
}

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

// Load MSCLoader.dll using Unity's embedded Mono runtime and run MSCShim.Loader.Run(mods_dir)
static DWORD WINAPI load_managed_mods(LPVOID ctx) {
    (void)ctx;
    Sleep(3000); // give Unity time to start Mono
    mono_api m = {0};
    log_msg("[managed] init managed loader");
    if (!resolve_mono(&m)) return 0;

    MonoDomain *domain = NULL;
    for (int i = 0; i < 30; i++) { // wait up to ~15 seconds for Mono to spin up
        domain = m.mono_get_root_domain();
        if (domain) break;
        Sleep(500);
    }
    if (!domain) {
        log_msg("[managed] mono_get_root_domain failed (Mono not ready)");
        return 0;
    }

    m.mono_thread_attach(domain);
    log_msg("[managed] attached to Mono domain");

    char loader_path[MAX_PATH + 32];
    lstrcpyA(loader_path, g_module_dir);
    lstrcatA(loader_path, "\\mods\\MSCLoader.dll");

    DWORD attrs = GetFileAttributesA(loader_path);
    if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        log_msg("[managed] MSCLoader.dll not found in mods; skipping managed mods");
        return 0;
    }

    MonoAssembly *assembly = m.mono_domain_assembly_open(domain, loader_path);
    if (!assembly) {
        log_msg("[managed] mono_domain_assembly_open failed for MSCLoader.dll");
        return 0;
    }

    log_msg("[managed] MSCLoader.dll loaded via mono");

    MonoImage *image = m.mono_assembly_get_image(assembly);
    if (!image) {
        log_msg("[managed] mono_assembly_get_image failed");
        return 0;
    }

    MonoClass *klass = m.mono_class_from_name(image, "MSCShim", "Loader");
    if (!klass) {
        log_msg("[managed] MSCShim.Loader not found");
        return 0;
    }

    MonoMethod *run = m.mono_class_get_method_from_name(klass, "Run", 1);
    if (!run) {
        log_msg("[managed] MSCShim.Loader.Run(string) missing");
        return 0;
    }

    char mods_dir[MAX_PATH + 8];
    lstrcpyA(mods_dir, g_module_dir);
    lstrcatA(mods_dir, "\\mods");

    void *args[1];
    args[0] = m.mono_string_new(domain, mods_dir);

    MonoObject *exc = NULL;
    m.mono_runtime_invoke(run, NULL, args, &exc);
    if (exc) {
        log_msg("[managed] MSCShim.Loader.Run threw an exception");
    } else {
        log_msg("[managed] MSCLoader invoked successfully");
    }
    return 0;
}

// Called by winmm proxy after game starts
void MaximumEntry(void) {
    log_msg("[maximum] MaximumHost loaded");
    log_msg("[maximum] ready — hook is running");
    // Start built-in Terry Davis tribute thread
    HANDLE terry = CreateThread(NULL, 0, terry_thread, NULL, 0, NULL);
    if (terry) CloseHandle(terry);
    load_mods();
    // Defer managed loader to a new thread so we don't block the main hook and can wait for Mono
    HANDLE managed = CreateThread(NULL, 0, load_managed_mods, NULL, 0, NULL);
    if (managed) CloseHandle(managed);
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

