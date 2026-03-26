#include "nvapi_controller.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace {

// NVAPI structs
struct NV_DISPLAY_DVC_INFO_EX {
    unsigned int version;
    int currentLevel;
    int minLevel;
    int maxLevel;
    int defaultLevel;
};

struct NV_DISPLAY_HUE_INFO {
    unsigned int version;
    int currentHueAngle;
    int defaultHueAngle;
};

// Function pointer types
typedef void* (*NvAPI_QueryInterface_t)(unsigned int);
typedef int (*NvAPI_Initialize_t)();
typedef int (*NvAPI_Unload_t)();
typedef int (*NvAPI_EnumNvidiaDisplayHandle_t)(int index, int* displayHandle);
typedef int (*NvAPI_GetDVCInfoEx_t)(int displayHandle, int outputId, NV_DISPLAY_DVC_INFO_EX* info);
typedef int (*NvAPI_SetDVCLevelEx_t)(int displayHandle, int outputId, NV_DISPLAY_DVC_INFO_EX* info);
typedef int (*NvAPI_GetHUEInfo_t)(int displayHandle, int outputId, NV_DISPLAY_HUE_INFO* info);
typedef int (*NvAPI_SetHUEAngle_t)(int displayHandle, int outputId, int hueAngle);

// Function IDs
constexpr unsigned int ID_Initialize              = 0x0150E828;
constexpr unsigned int ID_Unload                  = 0xD22BDD7E;
constexpr unsigned int ID_EnumNvidiaDisplayHandle  = 0x9ABDD40D;
constexpr unsigned int ID_GetDVCInfoEx            = 0x0E45002D;
constexpr unsigned int ID_SetDVCLevelEx           = 0x4A82C2B1;
constexpr unsigned int ID_GetHUEInfo              = 0x95B64341;
constexpr unsigned int ID_SetHUEAngle             = 0xF5A0F22C;

// State
static HMODULE g_hNvapi = NULL;
static bool g_available = false;
static int g_displayHandle = 0;

// Resolved function pointers
static NvAPI_Initialize_t              fnInitialize = nullptr;
static NvAPI_Unload_t                  fnUnload = nullptr;
static NvAPI_EnumNvidiaDisplayHandle_t fnEnumDisplay = nullptr;
static NvAPI_GetDVCInfoEx_t            fnGetDVCInfoEx = nullptr;
static NvAPI_SetDVCLevelEx_t           fnSetDVCLevelEx = nullptr;
static NvAPI_GetHUEInfo_t              fnGetHUEInfo = nullptr;
static NvAPI_SetHUEAngle_t             fnSetHUEAngle = nullptr;

} // anonymous namespace

namespace NvapiController {

bool Init() {
    g_available = false;

    // Try 64-bit first, then 32-bit
    g_hNvapi = LoadLibraryA("nvapi64.dll");
    if (!g_hNvapi) {
        g_hNvapi = LoadLibraryA("nvapi.dll");
    }
    if (!g_hNvapi) return false;

    auto queryInterface = reinterpret_cast<NvAPI_QueryInterface_t>(
        GetProcAddress(g_hNvapi, "nvapi_QueryInterface"));
    if (!queryInterface) {
        FreeLibrary(g_hNvapi);
        g_hNvapi = NULL;
        return false;
    }

    // Resolve all functions
    fnInitialize    = reinterpret_cast<NvAPI_Initialize_t>(queryInterface(ID_Initialize));
    fnUnload        = reinterpret_cast<NvAPI_Unload_t>(queryInterface(ID_Unload));
    fnEnumDisplay   = reinterpret_cast<NvAPI_EnumNvidiaDisplayHandle_t>(queryInterface(ID_EnumNvidiaDisplayHandle));
    fnGetDVCInfoEx  = reinterpret_cast<NvAPI_GetDVCInfoEx_t>(queryInterface(ID_GetDVCInfoEx));
    fnSetDVCLevelEx = reinterpret_cast<NvAPI_SetDVCLevelEx_t>(queryInterface(ID_SetDVCLevelEx));
    fnGetHUEInfo    = reinterpret_cast<NvAPI_GetHUEInfo_t>(queryInterface(ID_GetHUEInfo));
    fnSetHUEAngle   = reinterpret_cast<NvAPI_SetHUEAngle_t>(queryInterface(ID_SetHUEAngle));

    if (!fnInitialize || !fnEnumDisplay || !fnSetDVCLevelEx) {
        FreeLibrary(g_hNvapi);
        g_hNvapi = NULL;
        return false;
    }

    // Initialize NVAPI
    if (fnInitialize() != 0) {
        FreeLibrary(g_hNvapi);
        g_hNvapi = NULL;
        return false;
    }

    // Get primary display handle (index 0)
    if (fnEnumDisplay(0, &g_displayHandle) != 0) {
        if (fnUnload) fnUnload();
        FreeLibrary(g_hNvapi);
        g_hNvapi = NULL;
        return false;
    }

    g_available = true;
    return true;
}

bool IsAvailable() {
    return g_available;
}

void SetVibrance(int level) {
    if (!g_available || !fnGetDVCInfoEx || !fnSetDVCLevelEx) return;

    NV_DISPLAY_DVC_INFO_EX info = {};
    info.version = sizeof(NV_DISPLAY_DVC_INFO_EX) | 0x10000;

    if (fnGetDVCInfoEx(g_displayHandle, 0, &info) != 0) return;

    info.currentLevel = level;
    fnSetDVCLevelEx(g_displayHandle, 0, &info);
}

void SetHue(int angle) {
    if (!g_available || !fnSetHUEAngle) return;
    fnSetHUEAngle(g_displayHandle, 0, angle);
}

void ResetDefaults() {
    SetVibrance(50);
    SetHue(0);
}

void Shutdown() {
    if (g_available && fnUnload) {
        fnUnload();
    }
    if (g_hNvapi) {
        FreeLibrary(g_hNvapi);
        g_hNvapi = NULL;
    }
    g_available = false;
}

} // namespace NvapiController
