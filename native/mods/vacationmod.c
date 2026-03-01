// vacationmod.c — Maximum MWC Loader plugin mod (external DLL in /mods)
// Ticket flow for Teimo gas-station clipboard:
// - Buy ticket (trigger or runtime bridge)
// - Clipboard paper disappears
// - Phone number is revealed behind paper

#include <windows.h>
#include <stdlib.h>

typedef struct VacationState {
    int ticket_bought;
    int call_completed;
    int clipboard_visual_applied;
    char ticket_phone[32];
    DWORD ticket_price_mk;
} VacationState;

typedef struct VacationConfig {
    char paper_object[64];
    char number_object[64];
    char buy_trigger_file[64];
    char call_trigger_file[64];
    char autodetect_trigger_file[64];
    int auto_detect_objects;
    int enable_unity_object_swap;
} VacationConfig;

typedef void MonoDomain;
typedef void MonoThread;
typedef void MonoAssembly;
typedef void MonoImage;
typedef void MonoClass;
typedef void MonoMethod;
typedef void MonoString;
typedef void MonoObject;

typedef MonoDomain *(*mono_get_root_domain_t)(void);
typedef MonoDomain *(*mono_domain_get_t)(void);
typedef MonoThread *(*mono_thread_attach_t)(MonoDomain *);
typedef MonoString *(*mono_string_new_t)(MonoDomain *, const char *);
typedef MonoAssembly *(*mono_domain_assembly_open_t)(MonoDomain *, const char *);
typedef MonoImage *(*mono_assembly_get_image_t)(MonoAssembly *);
typedef MonoClass *(*mono_class_from_name_t)(MonoImage *, const char *, const char *);
typedef MonoMethod *(*mono_class_get_method_from_name_t)(MonoClass *, const char *, int);
typedef MonoObject *(*mono_runtime_invoke_t)(MonoMethod *, void *, void **, MonoObject **);

typedef struct MonoBridge {
    HMODULE mono_module;
    int initialized;
    int unity_ready;
    DWORD last_init_attempt_tick;

    mono_get_root_domain_t mono_get_root_domain;
    mono_domain_get_t mono_domain_get;
    mono_thread_attach_t mono_thread_attach;
    mono_string_new_t mono_string_new;
    mono_domain_assembly_open_t mono_domain_assembly_open;
    mono_assembly_get_image_t mono_assembly_get_image;
    mono_class_from_name_t mono_class_from_name;
    mono_class_get_method_from_name_t mono_class_get_method_from_name;
    mono_runtime_invoke_t mono_runtime_invoke;

    MonoDomain *domain;
    MonoClass *game_object_class;
    MonoMethod *game_object_find;
    MonoMethod *game_object_set_active;
} MonoBridge;

static char g_dir[MAX_PATH];
static volatile LONG g_running = 1;
static VacationState g_state;
static VacationConfig g_cfg;
static MonoBridge g_mono;
static int g_autodetect_finished = 0;

typedef struct ClipboardPair {
    const char *paper;
    const char *number;
} ClipboardPair;

static const ClipboardPair k_clipboard_pairs[] = {
    {"AllClipboard", "NoClipboard"},
    {"OwnClipboard", "NoClipboard"},
    {"Clipboard", "NoClipboard"},
    {"clipboard", "NoClipboard"},
    {"_clipboard", "NoClipboard"},
    {"AllClipboard", "clipboardFlag"},
    {"OwnClipboard", "clipboardFlag"},
    {"Teimo/AllClipboard", "Teimo/NoClipboard"},
    {"Teimo/OwnClipboard", "Teimo/NoClipboard"},
    {"Teimo/Clipboard", "Teimo/NoClipboard"},
    {"Shop/AllClipboard", "Shop/NoClipboard"},
    {"Shop/OwnClipboard", "Shop/NoClipboard"},
    {"Shop/Clipboard", "Shop/NoClipboard"},
    {"Store/AllClipboard", "Store/NoClipboard"},
    {"Store/OwnClipboard", "Store/NoClipboard"},
    {"Store/Clipboard", "Store/NoClipboard"},
    {"Kauppa/AllClipboard", "Kauppa/NoClipboard"},
    {"Kauppa/OwnClipboard", "Kauppa/NoClipboard"},
    {"Kauppa/Clipboard", "Kauppa/NoClipboard"},
    {"TeimonKauppa/AllClipboard", "TeimonKauppa/NoClipboard"},
    {"TeimonKauppa/OwnClipboard", "TeimonKauppa/NoClipboard"},
    {"TeimonKauppa/Clipboard", "TeimonKauppa/NoClipboard"},
    {"TeimoStore/AllClipboard", "TeimoStore/NoClipboard"},
    {"TeimoStore/OwnClipboard", "TeimoStore/NoClipboard"},
    {"TeimoStore/Clipboard", "TeimoStore/NoClipboard"},
    {"TeimoShop/AllClipboard", "TeimoShop/NoClipboard"},
    {"TeimoShop/OwnClipboard", "TeimoShop/NoClipboard"},
    {"TeimoShop/Clipboard", "TeimoShop/NoClipboard"},
    {"Office/AllClipboard", "Office/NoClipboard"},
    {"Office/OwnClipboard", "Office/NoClipboard"},
    {"Office/Clipboard", "Office/NoClipboard"},
    {"PartsCatalogPivot/AllClipboard", "PartsCatalogPivot/NoClipboard"},
    {"PartsCatalogPivot/OwnClipboard", "PartsCatalogPivot/NoClipboard"},
    {"PartsCatalogPivot/Clipboard", "PartsCatalogPivot/NoClipboard"},
};

static void build_path(char *out, const char *name) {
    lstrcpyA(out, g_dir);
    lstrcatA(out, "\\");
    lstrcatA(out, name);
}

static int starts_with(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s != *prefix) return 0;
        s++;
        prefix++;
    }
    return 1;
}

static void trim_line(char *s) {
    while (*s) {
        if (*s == '\r' || *s == '\n') {
            *s = '\0';
            return;
        }
        s++;
    }
}

static void mod_log(const char *msg) {
    char path[MAX_PATH + 32];
    build_path(path, "maximum.log");

    HANDLE h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;

    SetFilePointer(h, 0, NULL, FILE_END);
    DWORD written;
    WriteFile(h, msg, (DWORD)lstrlenA(msg), &written, NULL);
    WriteFile(h, "\r\n", 2, &written, NULL);
    CloseHandle(h);
}

static int file_exists(const char *path) {
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

static void remove_file_if_exists(const char *path) {
    DeleteFileA(path);
}

static void write_clipboard_hint_file(void) {
    char path[MAX_PATH + 64];
    char content[512];
    build_path(path, "vacation_clipboard_hint.txt");

    HANDLE h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;

    wsprintfA(content,
              "Vacation mod clipboard state\r\n"
              "ticket_bought=%d\r\n"
              "call_completed=%d\r\n"
              "paper_object=%s\r\n"
              "number_object=%s\r\n"
              "phone_number=%s\r\n",
              g_state.ticket_bought,
              g_state.call_completed,
              g_cfg.paper_object,
              g_cfg.number_object,
              g_state.ticket_phone);

    DWORD written;
    WriteFile(h, content, (DWORD)lstrlenA(content), &written, NULL);
    CloseHandle(h);
}

static void save_state(void) {
    char path[MAX_PATH + 64];
    char buf[512];
    build_path(path, "vacation_state.ini");

    HANDLE h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;

    wsprintfA(buf,
              "ticket_bought=%d\r\n"
              "call_completed=%d\r\n"
              "clipboard_visual_applied=%d\r\n"
              "ticket_phone=%s\r\n"
              "ticket_price_mk=%lu\r\n",
              g_state.ticket_bought,
              g_state.call_completed,
              g_state.clipboard_visual_applied,
              g_state.ticket_phone,
              (unsigned long)g_state.ticket_price_mk);

    DWORD written;
    WriteFile(h, buf, (DWORD)lstrlenA(buf), &written, NULL);
    CloseHandle(h);
}

static void reset_default_state(void) {
    g_state.ticket_bought = 0;
    g_state.call_completed = 0;
    g_state.clipboard_visual_applied = 0;
    g_state.ticket_price_mk = 1450;
    lstrcpyA(g_state.ticket_phone, "");
}

static void reset_default_config(void) {
    lstrcpyA(g_cfg.paper_object, "AllClipboard");
    lstrcpyA(g_cfg.number_object, "NoClipboard");
    lstrcpyA(g_cfg.buy_trigger_file, "vacation_buy_ticket.trigger");
    lstrcpyA(g_cfg.call_trigger_file, "vacation_call_number.trigger");
    lstrcpyA(g_cfg.autodetect_trigger_file, "vacation_autodetect.trigger");
    g_cfg.auto_detect_objects = 1;
    g_cfg.enable_unity_object_swap = 0;
}

static void load_state(void) {
    char path[MAX_PATH + 64];
    build_path(path, "vacation_state.ini");
    reset_default_state();

    HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;

    char data[1024];
    DWORD readn = 0;
    if (!ReadFile(h, data, sizeof(data) - 1, &readn, NULL)) {
        CloseHandle(h);
        return;
    }
    data[readn] = '\0';
    CloseHandle(h);

    char *line = data;
    while (*line) {
        char *next = line;
        while (*next && *next != '\n') next++;
        if (*next == '\n') {
            *next = '\0';
            next++;
        }

        trim_line(line);
        if (starts_with(line, "ticket_bought=")) {
            g_state.ticket_bought = lstrcmpiA(line + 14, "1") == 0;
        } else if (starts_with(line, "call_completed=")) {
            g_state.call_completed = lstrcmpiA(line + 15, "1") == 0;
        } else if (starts_with(line, "clipboard_visual_applied=")) {
            g_state.clipboard_visual_applied = lstrcmpiA(line + 25, "1") == 0;
        } else if (starts_with(line, "ticket_phone=")) {
            lstrcpynA(g_state.ticket_phone, line + 13, (int)sizeof(g_state.ticket_phone));
        } else if (starts_with(line, "ticket_price_mk=")) {
            g_state.ticket_price_mk = (DWORD)atol(line + 16);
        }

        line = next;
    }
}

static void load_config(void) {
    char path[MAX_PATH + 64];
    build_path(path, "vacation_clipboard.ini");
    reset_default_config();

    HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;

    char data[1024];
    DWORD readn = 0;
    if (!ReadFile(h, data, sizeof(data) - 1, &readn, NULL)) {
        CloseHandle(h);
        return;
    }
    data[readn] = '\0';
    CloseHandle(h);

    char *line = data;
    while (*line) {
        char *next = line;
        while (*next && *next != '\n') next++;
        if (*next == '\n') {
            *next = '\0';
            next++;
        }

        trim_line(line);
        if (starts_with(line, "paper_object=")) {
            lstrcpynA(g_cfg.paper_object, line + 13, (int)sizeof(g_cfg.paper_object));
        } else if (starts_with(line, "number_object=")) {
            lstrcpynA(g_cfg.number_object, line + 14, (int)sizeof(g_cfg.number_object));
        } else if (starts_with(line, "buy_trigger_file=")) {
            lstrcpynA(g_cfg.buy_trigger_file, line + 17, (int)sizeof(g_cfg.buy_trigger_file));
        } else if (starts_with(line, "call_trigger_file=")) {
            lstrcpynA(g_cfg.call_trigger_file, line + 18, (int)sizeof(g_cfg.call_trigger_file));
        } else if (starts_with(line, "autodetect_trigger_file=")) {
            lstrcpynA(g_cfg.autodetect_trigger_file, line + 24, (int)sizeof(g_cfg.autodetect_trigger_file));
        } else if (starts_with(line, "auto_detect_objects=")) {
            g_cfg.auto_detect_objects = lstrcmpiA(line + 20, "0") != 0;
        } else if (starts_with(line, "enable_unity_object_swap=")) {
            g_cfg.enable_unity_object_swap = lstrcmpiA(line + 25, "0") != 0;
        }

        line = next;
    }
}

static int init_mono_bridge(void) {
    DWORD now = GetTickCount();

    if (!g_cfg.enable_unity_object_swap) return 0;

    if (g_mono.unity_ready) return 1;
    if (g_mono.initialized && (now - g_mono.last_init_attempt_tick) < 2000) {
        return 0;
    }

    g_mono.initialized = 1;
    g_mono.last_init_attempt_tick = now;

    g_mono.mono_module = GetModuleHandleA("mono-2.0-bdwgc.dll");
    if (!g_mono.mono_module) g_mono.mono_module = GetModuleHandleA("mono-2.0-sgen.dll");
    if (!g_mono.mono_module) g_mono.mono_module = GetModuleHandleA("mono.dll");
    if (!g_mono.mono_module) g_mono.mono_module = GetModuleHandleA("mono-2.0.dll");
    if (!g_mono.mono_module) {
        mod_log("[Vacation] Mono module not loaded yet (will retry).");
        return 0;
    }

    g_mono.mono_get_root_domain = (mono_get_root_domain_t)(void *)GetProcAddress(g_mono.mono_module, "mono_get_root_domain");
    g_mono.mono_domain_get = (mono_domain_get_t)(void *)GetProcAddress(g_mono.mono_module, "mono_domain_get");
    g_mono.mono_thread_attach = (mono_thread_attach_t)(void *)GetProcAddress(g_mono.mono_module, "mono_thread_attach");
    g_mono.mono_string_new = (mono_string_new_t)(void *)GetProcAddress(g_mono.mono_module, "mono_string_new");
    g_mono.mono_domain_assembly_open = (mono_domain_assembly_open_t)(void *)GetProcAddress(g_mono.mono_module, "mono_domain_assembly_open");
    g_mono.mono_assembly_get_image = (mono_assembly_get_image_t)(void *)GetProcAddress(g_mono.mono_module, "mono_assembly_get_image");
    g_mono.mono_class_from_name = (mono_class_from_name_t)(void *)GetProcAddress(g_mono.mono_module, "mono_class_from_name");
    g_mono.mono_class_get_method_from_name = (mono_class_get_method_from_name_t)(void *)GetProcAddress(g_mono.mono_module, "mono_class_get_method_from_name");
    g_mono.mono_runtime_invoke = (mono_runtime_invoke_t)(void *)GetProcAddress(g_mono.mono_module, "mono_runtime_invoke");

    if ((!g_mono.mono_get_root_domain && !g_mono.mono_domain_get) || !g_mono.mono_thread_attach ||
        !g_mono.mono_string_new || !g_mono.mono_domain_assembly_open ||
        !g_mono.mono_assembly_get_image || !g_mono.mono_class_from_name ||
        !g_mono.mono_class_get_method_from_name || !g_mono.mono_runtime_invoke) {
        mod_log("[Vacation] Mono exports missing; Unity clipboard swap pending retry.");
        return 0;
    }

    g_mono.domain = g_mono.mono_get_root_domain ? g_mono.mono_get_root_domain() : NULL;
    if (!g_mono.domain && g_mono.mono_domain_get) {
        g_mono.domain = g_mono.mono_domain_get();
    }
    if (!g_mono.domain) {
        mod_log("[Vacation] Mono root domain unavailable (will retry).");
        return 0;
    }
    g_mono.mono_thread_attach(g_mono.domain);

    char unity_path[MAX_PATH + 128];
    lstrcpyA(unity_path, g_dir);
    lstrcatA(unity_path, "\\mywintercar_Data\\Managed\\UnityEngine.dll");

    MonoAssembly *unity_asm = g_mono.mono_domain_assembly_open(g_mono.domain, unity_path);
    if (!unity_asm) {
        mod_log("[Vacation] Failed to open UnityEngine.dll in Mono domain (will retry).");
        return 0;
    }

    MonoImage *unity_img = g_mono.mono_assembly_get_image(unity_asm);
    if (!unity_img) {
        mod_log("[Vacation] Failed to get UnityEngine image (will retry).");
        return 0;
    }

    g_mono.game_object_class = g_mono.mono_class_from_name(unity_img, "UnityEngine", "GameObject");
    if (!g_mono.game_object_class) {
        mod_log("[Vacation] UnityEngine.GameObject not found (will retry).");
        return 0;
    }

    g_mono.game_object_find = g_mono.mono_class_get_method_from_name(g_mono.game_object_class, "Find", 1);
    g_mono.game_object_set_active = g_mono.mono_class_get_method_from_name(g_mono.game_object_class, "SetActive", 1);
    if (!g_mono.game_object_find || !g_mono.game_object_set_active) {
        mod_log("[Vacation] GameObject.Find/SetActive unavailable (will retry).");
        return 0;
    }

    g_mono.unity_ready = 1;
    mod_log("[Vacation] Mono bridge ready (Unity GameObject.Find/SetActive).");
    return 1;
}

static int unity_set_object_active(const char *object_name, int active) {
    if (!init_mono_bridge()) return 0;

    MonoObject *exc = NULL;
    MonoString *mono_name = g_mono.mono_string_new(g_mono.domain, object_name);
    if (!mono_name) return 0;

    void *find_args[1];
    find_args[0] = mono_name;
    MonoObject *obj = g_mono.mono_runtime_invoke(g_mono.game_object_find, NULL, find_args, &exc);
    if (exc || !obj) return 0;

    char flag = active ? 1 : 0;
    void *set_args[1];
    set_args[0] = &flag;
    exc = NULL;
    g_mono.mono_runtime_invoke(g_mono.game_object_set_active, obj, set_args, &exc);
    if (exc) return 0;

    return 1;
}

static int unity_object_exists(const char *object_name) {
    if (!init_mono_bridge()) return 0;

    MonoObject *exc = NULL;
    MonoString *mono_name = g_mono.mono_string_new(g_mono.domain, object_name);
    if (!mono_name) return 0;

    void *find_args[1];
    find_args[0] = mono_name;
    MonoObject *obj = g_mono.mono_runtime_invoke(g_mono.game_object_find, NULL, find_args, &exc);
    if (exc || !obj) return 0;
    return 1;
}

static void write_autodetect_file(const char *paper, const char *number, int success) {
    char path[MAX_PATH + 64];
    char content[1024];
    build_path(path, "vacation_clipboard_autodetect.ini");

    HANDLE h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;

    wsprintfA(content,
              "# Generated by vacation mod runtime detector\r\n"
              "# Copy wanted values into vacation_clipboard.ini\r\n"
              "autodetect_success=%d\r\n"
              "paper_object=%s\r\n"
              "number_object=%s\r\n"
              "buy_trigger_file=%s\r\n"
              "call_trigger_file=%s\r\n"
              "autodetect_trigger_file=%s\r\n"
              "auto_detect_objects=%d\r\n",
              success,
              paper,
              number,
              g_cfg.buy_trigger_file,
              g_cfg.call_trigger_file,
              g_cfg.autodetect_trigger_file,
              g_cfg.auto_detect_objects);

    DWORD written;
    WriteFile(h, content, (DWORD)lstrlenA(content), &written, NULL);
    CloseHandle(h);
}

static int auto_detect_clipboard_objects(void) {
    char msg[256];
    int i;

    if (!g_cfg.enable_unity_object_swap) {
        mod_log("[Vacation] Auto-detect disabled: enable_unity_object_swap=0");
        write_autodetect_file(g_cfg.paper_object, g_cfg.number_object, 0);
        return 0;
    }

    if (!init_mono_bridge()) {
        mod_log("[Vacation] Auto-detect waiting for Mono bridge.");
        write_autodetect_file(g_cfg.paper_object, g_cfg.number_object, 0);
        return 0;
    }

    if (unity_object_exists(g_cfg.paper_object) && unity_object_exists(g_cfg.number_object)) {
        wsprintfA(msg,
                  "[Vacation] Config objects confirmed: paper=%s number=%s",
                  g_cfg.paper_object,
                  g_cfg.number_object);
        mod_log(msg);
        write_autodetect_file(g_cfg.paper_object, g_cfg.number_object, 1);
        g_autodetect_finished = 1;
        return 1;
    }

    for (i = 0; i < (int)(sizeof(k_clipboard_pairs) / sizeof(k_clipboard_pairs[0])); i++) {
        const char *paper = k_clipboard_pairs[i].paper;
        const char *number = k_clipboard_pairs[i].number;
        int has_paper = unity_object_exists(paper);
        int has_number = unity_object_exists(number);

        wsprintfA(msg,
                  "[Vacation] Detect pair %s/%s -> %d/%d",
                  paper, number, has_paper, has_number);
        mod_log(msg);

        if (has_paper && has_number) {
            lstrcpynA(g_cfg.paper_object, paper, (int)sizeof(g_cfg.paper_object));
            lstrcpynA(g_cfg.number_object, number, (int)sizeof(g_cfg.number_object));

            wsprintfA(msg,
                      "[Vacation] Auto-detected clipboard objects: paper=%s number=%s",
                      g_cfg.paper_object,
                      g_cfg.number_object);
            mod_log(msg);

            write_autodetect_file(g_cfg.paper_object, g_cfg.number_object, 1);
            g_autodetect_finished = 1;
            return 1;
        }
    }

    mod_log("[Vacation] Auto-detect did not find a complete pair yet.");
    write_autodetect_file(g_cfg.paper_object, g_cfg.number_object, 0);
    return 0;
}

static int apply_clipboard_visual_state(int ticket_bought) {
    char msg[256];
    int ok_paper = 0;
    int ok_number = 0;

    if (!g_cfg.enable_unity_object_swap) {
        wsprintfA(msg,
                  "[Vacation] Clipboard swap skipped (enable_unity_object_swap=0)");
        mod_log(msg);
        return 0;
    }

    if (ticket_bought) {
        ok_paper = unity_set_object_active(g_cfg.paper_object, 0);
        ok_number = unity_set_object_active(g_cfg.number_object, 1);
    } else {
        ok_paper = unity_set_object_active(g_cfg.paper_object, 1);
        ok_number = unity_set_object_active(g_cfg.number_object, 0);
    }

    wsprintfA(msg,
              "[Vacation] Clipboard swap %s (paper:%s=%d, number:%s=%d)",
              ticket_bought ? "ticket-bought" : "ticket-not-bought",
              g_cfg.paper_object,
              ok_paper,
              g_cfg.number_object,
              ok_number);
    mod_log(msg);

    return ok_paper && ok_number;
}

static void generate_ticket_phone(char *out, int out_size) {
    SYSTEMTIME st;
    GetLocalTime(&st);

    DWORD seed = GetTickCount() ^
                 ((DWORD)st.wMilliseconds << 16) ^
                 ((DWORD)st.wSecond << 8) ^
                 (DWORD)st.wMinute;
    int suffix = (int)(1000 + (seed % 9000));

    wsprintfA(out, "040-77%d", suffix);
    out[out_size - 1] = '\0';
}

static void on_buy_ticket(void) {
    char msg[256];
    if (g_state.ticket_bought) {
        mod_log("[Vacation] Ticket already bought.");
        return;
    }

    generate_ticket_phone(g_state.ticket_phone, (int)sizeof(g_state.ticket_phone));
    g_state.ticket_bought = 1;
    g_state.call_completed = 0;
    g_state.clipboard_visual_applied = apply_clipboard_visual_state(1);
    save_state();
    write_clipboard_hint_file();

    wsprintfA(msg,
              "[Vacation] Ticket purchased at Teimo clipboard for %lu mk. Number revealed: %s",
              (unsigned long)g_state.ticket_price_mk,
              g_state.ticket_phone);
    mod_log(msg);
}

static void on_call_ticket_number(void) {
    char msg[256];
    if (!g_state.ticket_bought) {
        mod_log("[Vacation] Buy ticket first at Teimo clipboard.");
        return;
    }
    if (g_state.call_completed) {
        mod_log("[Vacation] Ticket number already called.");
        return;
    }

    g_state.call_completed = 1;
    save_state();
    write_clipboard_hint_file();

    wsprintfA(msg,
              "[Vacation] Call completed on %s. Booking confirmed.",
              g_state.ticket_phone);
    mod_log(msg);
}

static void show_instructions(void) {
    char msg[256];
    mod_log("[Vacation] Vacation clipboard mod loaded (external mod DLL).\n");
    mod_log("[Vacation] Detected game symbols from assembly/assets: AllClipboard, NoClipboard, OwnClipboard, clipboardFlag, bag_teimonkauppa");

    wsprintfA(msg,
              "[Vacation] Using objects paper=%s number=%s",
              g_cfg.paper_object,
              g_cfg.number_object);
    mod_log(msg);

    wsprintfA(msg,
              "[Vacation] Buy trigger file=%s | Call trigger file=%s",
              g_cfg.buy_trigger_file,
              g_cfg.call_trigger_file);
    mod_log(msg);

    wsprintfA(msg,
              "[Vacation] Auto-detect=%d trigger=%s",
              g_cfg.auto_detect_objects,
              g_cfg.autodetect_trigger_file);
    mod_log(msg);

    wsprintfA(msg,
              "[Vacation] Unity object swap enabled=%d",
              g_cfg.enable_unity_object_swap);
    mod_log(msg);

    mod_log("[Vacation] If object names differ in your build, edit vacation_clipboard.ini");
    mod_log("[Vacation] Detector output file: vacation_clipboard_autodetect.ini");
}

static DWORD WINAPI vacation_thread(LPVOID ctx) {
    (void)ctx;
    DWORD visual_retry_tick = 0;
    DWORD detect_retry_tick = 0;

    show_instructions();
    if (g_cfg.auto_detect_objects) auto_detect_clipboard_objects();
    apply_clipboard_visual_state(g_state.ticket_bought ? 1 : 0);

    while (InterlockedCompareExchange(&g_running, 1, 1) == 1) {
        char buy_path[MAX_PATH + 64];
        char call_path[MAX_PATH + 64];
        char detect_path[MAX_PATH + 64];
        DWORD now = GetTickCount();

        build_path(buy_path, g_cfg.buy_trigger_file);
        build_path(call_path, g_cfg.call_trigger_file);
        build_path(detect_path, g_cfg.autodetect_trigger_file);

        if (file_exists(buy_path)) {
            remove_file_if_exists(buy_path);
            on_buy_ticket();
        }
        if (file_exists(call_path)) {
            remove_file_if_exists(call_path);
            on_call_ticket_number();
        }
        if (file_exists(detect_path)) {
            remove_file_if_exists(detect_path);
            g_autodetect_finished = 0;
            auto_detect_clipboard_objects();
        }

        if (g_cfg.auto_detect_objects && !g_autodetect_finished) {
            if (now - detect_retry_tick > 10000) {
                auto_detect_clipboard_objects();
                detect_retry_tick = now;
            }
        }

        if (g_state.ticket_bought && !g_state.clipboard_visual_applied) {
            if (now - visual_retry_tick > 5000) {
                g_state.clipboard_visual_applied = apply_clipboard_visual_state(1);
                if (g_state.clipboard_visual_applied) save_state();
                visual_retry_tick = now;
            }
        }

        Sleep(500);
    }
    return 0;
}

__declspec(dllexport) void MaximumModInit(void) {
    load_config();
    load_state();

    HANDLE t = CreateThread(NULL, 0, vacation_thread, NULL, 0, NULL);
    if (t) CloseHandle(t);
}

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
    } else if (reason == DLL_PROCESS_DETACH) {
        InterlockedExchange(&g_running, 0);
    }
    return TRUE;
}
