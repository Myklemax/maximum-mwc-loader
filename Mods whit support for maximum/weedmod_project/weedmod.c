// weedmod.c — Maximum MWC Loader plugin mod starter
// Gameplay loop (trigger-file driven):
// 1) Plant seeds
// 2) Water crop
// 3) Advance growth stage
// 4) Harvest buds
// 5) Sell buds for mk

#include <windows.h>
#include <stdlib.h>

typedef struct WeedState {
    int crop_active;
    int plants_planted;
    int growth_stage;
    int watered_this_stage;
    int buds_inventory;
    DWORD money_mk;
    int total_harvests;
} WeedState;

typedef struct WeedConfig {
    char plant_trigger_file[64];
    char water_trigger_file[64];
    char advance_trigger_file[64];
    char harvest_trigger_file[64];
    char sell_trigger_file[64];
    int plants_per_batch;
    int max_growth_stage;
    int min_buds_per_plant;
    int max_buds_per_plant;
    DWORD sell_price_per_bud_mk;
} WeedConfig;

static char g_dir[MAX_PATH];
static volatile LONG g_running = 1;
static WeedState g_state;
static WeedConfig g_cfg;

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

static int file_exists(const char *path) {
    DWORD attr = GetFileAttributesA(path);
    return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

static void remove_file_if_exists(const char *path) {
    DeleteFileA(path);
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

static void reset_default_state(void) {
    g_state.crop_active = 0;
    g_state.plants_planted = 0;
    g_state.growth_stage = 0;
    g_state.watered_this_stage = 0;
    g_state.buds_inventory = 0;
    g_state.money_mk = 0;
    g_state.total_harvests = 0;
}

static void reset_default_config(void) {
    lstrcpyA(g_cfg.plant_trigger_file, "weed_plant.trigger");
    lstrcpyA(g_cfg.water_trigger_file, "weed_water.trigger");
    lstrcpyA(g_cfg.advance_trigger_file, "weed_advance.trigger");
    lstrcpyA(g_cfg.harvest_trigger_file, "weed_harvest.trigger");
    lstrcpyA(g_cfg.sell_trigger_file, "weed_sell.trigger");

    g_cfg.plants_per_batch = 4;
    g_cfg.max_growth_stage = 3;
    g_cfg.min_buds_per_plant = 2;
    g_cfg.max_buds_per_plant = 4;
    g_cfg.sell_price_per_bud_mk = 120;
}

static void save_state(void) {
    char path[MAX_PATH + 64];
    char buf[512];
    build_path(path, "weed_state.ini");

    HANDLE h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;

    wsprintfA(buf,
              "crop_active=%d\r\n"
              "plants_planted=%d\r\n"
              "growth_stage=%d\r\n"
              "watered_this_stage=%d\r\n"
              "buds_inventory=%d\r\n"
              "money_mk=%lu\r\n"
              "total_harvests=%d\r\n",
              g_state.crop_active,
              g_state.plants_planted,
              g_state.growth_stage,
              g_state.watered_this_stage,
              g_state.buds_inventory,
              (unsigned long)g_state.money_mk,
              g_state.total_harvests);

    DWORD written;
    WriteFile(h, buf, (DWORD)lstrlenA(buf), &written, NULL);
    CloseHandle(h);
}

static void load_state(void) {
    char path[MAX_PATH + 64];
    build_path(path, "weed_state.ini");
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
        if (starts_with(line, "crop_active=")) {
            g_state.crop_active = lstrcmpiA(line + 12, "1") == 0;
        } else if (starts_with(line, "plants_planted=")) {
            g_state.plants_planted = atoi(line + 15);
        } else if (starts_with(line, "growth_stage=")) {
            g_state.growth_stage = atoi(line + 13);
        } else if (starts_with(line, "watered_this_stage=")) {
            g_state.watered_this_stage = lstrcmpiA(line + 18, "1") == 0;
        } else if (starts_with(line, "buds_inventory=")) {
            g_state.buds_inventory = atoi(line + 15);
        } else if (starts_with(line, "money_mk=")) {
            g_state.money_mk = (DWORD)atol(line + 9);
        } else if (starts_with(line, "total_harvests=")) {
            g_state.total_harvests = atoi(line + 15);
        }

        line = next;
    }
}

static void load_config(void) {
    char path[MAX_PATH + 64];
    build_path(path, "weedmod.ini");
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
        if (starts_with(line, "plant_trigger_file=")) {
            lstrcpynA(g_cfg.plant_trigger_file, line + 19, (int)sizeof(g_cfg.plant_trigger_file));
        } else if (starts_with(line, "water_trigger_file=")) {
            lstrcpynA(g_cfg.water_trigger_file, line + 19, (int)sizeof(g_cfg.water_trigger_file));
        } else if (starts_with(line, "advance_trigger_file=")) {
            lstrcpynA(g_cfg.advance_trigger_file, line + 21, (int)sizeof(g_cfg.advance_trigger_file));
        } else if (starts_with(line, "harvest_trigger_file=")) {
            lstrcpynA(g_cfg.harvest_trigger_file, line + 21, (int)sizeof(g_cfg.harvest_trigger_file));
        } else if (starts_with(line, "sell_trigger_file=")) {
            lstrcpynA(g_cfg.sell_trigger_file, line + 18, (int)sizeof(g_cfg.sell_trigger_file));
        } else if (starts_with(line, "plants_per_batch=")) {
            g_cfg.plants_per_batch = atoi(line + 17);
        } else if (starts_with(line, "max_growth_stage=")) {
            g_cfg.max_growth_stage = atoi(line + 17);
        } else if (starts_with(line, "min_buds_per_plant=")) {
            g_cfg.min_buds_per_plant = atoi(line + 19);
        } else if (starts_with(line, "max_buds_per_plant=")) {
            g_cfg.max_buds_per_plant = atoi(line + 19);
        } else if (starts_with(line, "sell_price_per_bud_mk=")) {
            g_cfg.sell_price_per_bud_mk = (DWORD)atol(line + 22);
        }

        line = next;
    }

    if (g_cfg.max_growth_stage < 1) g_cfg.max_growth_stage = 1;
    if (g_cfg.plants_per_batch < 1) g_cfg.plants_per_batch = 1;
    if (g_cfg.min_buds_per_plant < 1) g_cfg.min_buds_per_plant = 1;
    if (g_cfg.max_buds_per_plant < g_cfg.min_buds_per_plant) {
        g_cfg.max_buds_per_plant = g_cfg.min_buds_per_plant;
    }
    if (g_cfg.sell_price_per_bud_mk < 1) g_cfg.sell_price_per_bud_mk = 1;
}

static void write_status_file(void) {
    char path[MAX_PATH + 64];
    char content[512];
    build_path(path, "weed_status.txt");

    HANDLE h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return;

    wsprintfA(content,
              "Weed mod state\r\n"
              "crop_active=%d\r\n"
              "plants_planted=%d\r\n"
              "growth_stage=%d/%d\r\n"
              "watered_this_stage=%d\r\n"
              "buds_inventory=%d\r\n"
              "money_mk=%lu\r\n"
              "total_harvests=%d\r\n",
              g_state.crop_active,
              g_state.plants_planted,
              g_state.growth_stage,
              g_cfg.max_growth_stage,
              g_state.watered_this_stage,
              g_state.buds_inventory,
              (unsigned long)g_state.money_mk,
              g_state.total_harvests);

    DWORD written;
    WriteFile(h, content, (DWORD)lstrlenA(content), &written, NULL);
    CloseHandle(h);
}

static void on_plant(void) {
    char msg[256];
    if (g_state.crop_active) {
        mod_log("[Weed] Crop already active. Harvest before planting new batch.");
        return;
    }

    g_state.crop_active = 1;
    g_state.plants_planted = g_cfg.plants_per_batch;
    g_state.growth_stage = 0;
    g_state.watered_this_stage = 0;

    save_state();
    write_status_file();

    wsprintfA(msg,
              "[Weed] Planted %d plants. Stage=%d/%d",
              g_state.plants_planted,
              g_state.growth_stage,
              g_cfg.max_growth_stage);
    mod_log(msg);
}

static void on_water(void) {
    if (!g_state.crop_active) {
        mod_log("[Weed] No active crop to water.");
        return;
    }
    if (g_state.growth_stage >= g_cfg.max_growth_stage) {
        mod_log("[Weed] Crop already fully grown. Use harvest trigger.");
        return;
    }
    g_state.watered_this_stage = 1;
    save_state();
    write_status_file();
    mod_log("[Weed] Crop watered. Use advance trigger to progress growth.");
}

static void on_advance(void) {
    char msg[256];
    if (!g_state.crop_active) {
        mod_log("[Weed] No active crop to advance.");
        return;
    }
    if (!g_state.watered_this_stage) {
        mod_log("[Weed] Growth blocked. Water crop first.");
        return;
    }
    if (g_state.growth_stage >= g_cfg.max_growth_stage) {
        mod_log("[Weed] Growth already maxed. Ready to harvest.");
        return;
    }

    g_state.growth_stage += 1;
    g_state.watered_this_stage = 0;
    save_state();
    write_status_file();

    wsprintfA(msg,
              "[Weed] Growth advanced to stage %d/%d",
              g_state.growth_stage,
              g_cfg.max_growth_stage);
    mod_log(msg);
}

static void on_harvest(void) {
    char msg[256];
    int i;
    int total_buds = 0;

    if (!g_state.crop_active) {
        mod_log("[Weed] No crop to harvest.");
        return;
    }
    if (g_state.growth_stage < g_cfg.max_growth_stage) {
        mod_log("[Weed] Crop not ready. Keep growing first.");
        return;
    }

    for (i = 0; i < g_state.plants_planted; i++) {
        int span = g_cfg.max_buds_per_plant - g_cfg.min_buds_per_plant + 1;
        int buds = g_cfg.min_buds_per_plant;
        if (span > 1) buds += (rand() % span);
        total_buds += buds;
    }

    g_state.buds_inventory += total_buds;
    g_state.crop_active = 0;
    g_state.plants_planted = 0;
    g_state.growth_stage = 0;
    g_state.watered_this_stage = 0;
    g_state.total_harvests += 1;

    save_state();
    write_status_file();

    wsprintfA(msg,
              "[Weed] Harvest complete: +%d buds. Inventory=%d",
              total_buds,
              g_state.buds_inventory);
    mod_log(msg);
}

static void on_sell(void) {
    char msg[256];
    DWORD gain;

    if (g_state.buds_inventory <= 0) {
        mod_log("[Weed] No buds in inventory to sell.");
        return;
    }

    gain = (DWORD)g_state.buds_inventory * g_cfg.sell_price_per_bud_mk;
    g_state.money_mk += gain;

    wsprintfA(msg,
              "[Weed] Sold %d buds for %lu mk. Total money=%lu mk",
              g_state.buds_inventory,
              (unsigned long)gain,
              (unsigned long)g_state.money_mk);
    mod_log(msg);

    g_state.buds_inventory = 0;
    save_state();
    write_status_file();
}

static void show_instructions(void) {
    char msg[256];
    mod_log("[Weed] Weed grow/sell mod loaded.");

    wsprintfA(msg,
              "[Weed] Triggers: plant=%s water=%s advance=%s harvest=%s sell=%s",
              g_cfg.plant_trigger_file,
              g_cfg.water_trigger_file,
              g_cfg.advance_trigger_file,
              g_cfg.harvest_trigger_file,
              g_cfg.sell_trigger_file);
    mod_log(msg);

    wsprintfA(msg,
              "[Weed] Config: plants_per_batch=%d growth_stages=%d buds_per_plant=%d-%d sell_price=%lu",
              g_cfg.plants_per_batch,
              g_cfg.max_growth_stage,
              g_cfg.min_buds_per_plant,
              g_cfg.max_buds_per_plant,
              (unsigned long)g_cfg.sell_price_per_bud_mk);
    mod_log(msg);
}

static DWORD WINAPI weed_thread(LPVOID ctx) {
    (void)ctx;
    show_instructions();
    write_status_file();

    while (InterlockedCompareExchange(&g_running, 1, 1) == 1) {
        char plant_path[MAX_PATH + 64];
        char water_path[MAX_PATH + 64];
        char advance_path[MAX_PATH + 64];
        char harvest_path[MAX_PATH + 64];
        char sell_path[MAX_PATH + 64];

        build_path(plant_path, g_cfg.plant_trigger_file);
        build_path(water_path, g_cfg.water_trigger_file);
        build_path(advance_path, g_cfg.advance_trigger_file);
        build_path(harvest_path, g_cfg.harvest_trigger_file);
        build_path(sell_path, g_cfg.sell_trigger_file);

        if (file_exists(plant_path)) {
            remove_file_if_exists(plant_path);
            on_plant();
        }
        if (file_exists(water_path)) {
            remove_file_if_exists(water_path);
            on_water();
        }
        if (file_exists(advance_path)) {
            remove_file_if_exists(advance_path);
            on_advance();
        }
        if (file_exists(harvest_path)) {
            remove_file_if_exists(harvest_path);
            on_harvest();
        }
        if (file_exists(sell_path)) {
            remove_file_if_exists(sell_path);
            on_sell();
        }

        Sleep(300);
    }
    return 0;
}

__declspec(dllexport) void MaximumModInit(void) {
    load_config();
    load_state();

    HANDLE t = CreateThread(NULL, 0, weed_thread, NULL, 0, NULL);
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

        srand((unsigned int)GetTickCount());
    } else if (reason == DLL_PROCESS_DETACH) {
        InterlockedExchange(&g_running, 0);
    }
    return TRUE;
}
