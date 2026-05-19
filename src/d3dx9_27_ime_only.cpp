#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <imm.h>
#include <string.h>
#include <wchar.h>

static HMODULE g_real = NULL;
static HINSTANCE g_module = NULL;

static HMODULE real_dll()
{
    if (g_real)
        return g_real;

    wchar_t path[MAX_PATH];
    DWORD n = GetModuleFileNameW(g_module, path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) {
        lstrcpynW(path, L"d3dx9_27_real.dll", MAX_PATH);
    } else {
        wchar_t* slash = wcsrchr(path, L'\\');
        if (slash)
            lstrcpynW(slash + 1, L"d3dx9_27_real.dll", (int)(MAX_PATH - (slash + 1 - path)));
        else
            lstrcpynW(path, L"d3dx9_27_real.dll", MAX_PATH);
    }
    g_real = LoadLibraryW(path);
    return g_real;
}

static FARPROC real_proc(const char* name)
{
    HMODULE h = real_dll();
    return h ? GetProcAddress(h, name) : NULL;
}

#include "d3dx_forwarders.inc"

extern "C" __declspec(dllexport) __declspec(naked) void D3DXCreateFontA() { __asm { jmp dword ptr [p_D3DXCreateFontA] } }
extern "C" __declspec(dllexport) __declspec(naked) void D3DXCreateFontW() { __asm { jmp dword ptr [p_D3DXCreateFontW] } }
extern "C" __declspec(dllexport) __declspec(naked) void D3DXCreateFontIndirectA() { __asm { jmp dword ptr [p_D3DXCreateFontIndirectA] } }
extern "C" __declspec(dllexport) __declspec(naked) void D3DXCreateFontIndirectW() { __asm { jmp dword ptr [p_D3DXCreateFontIndirectW] } }

typedef BOOL (WINAPI* GetMessageA_Fn)(LPMSG, HWND, UINT, UINT);
typedef BOOL (WINAPI* GetMessageW_Fn)(LPMSG, HWND, UINT, UINT);
typedef BOOL (WINAPI* PeekMessageA_Fn)(LPMSG, HWND, UINT, UINT, UINT);
typedef BOOL (WINAPI* PeekMessageW_Fn)(LPMSG, HWND, UINT, UINT, UINT);
typedef BOOL (WINAPI* TranslateMessage_Fn)(const MSG*);
typedef LRESULT (WINAPI* DispatchMessageA_Fn)(const MSG*);
typedef LRESULT (WINAPI* DispatchMessageW_Fn)(const MSG*);
typedef HIMC (WINAPI* ImmGetContext_Fn)(HWND);
typedef BOOL (WINAPI* ImmReleaseContext_Fn)(HWND, HIMC);
typedef LONG (WINAPI* ImmGetCompositionStringW_Fn)(HIMC, DWORD, LPVOID, DWORD);

static GetMessageA_Fn o_GetMessageA = NULL;
static GetMessageW_Fn o_GetMessageW = NULL;
static PeekMessageA_Fn o_PeekMessageA = NULL;
static PeekMessageW_Fn o_PeekMessageW = NULL;
static TranslateMessage_Fn o_TranslateMessage = NULL;
static DispatchMessageA_Fn o_DispatchMessageA = NULL;
static DispatchMessageW_Fn o_DispatchMessageW = NULL;
static ImmGetContext_Fn o_ImmGetContext = NULL;
static ImmReleaseContext_Fn o_ImmReleaseContext = NULL;
static ImmGetCompositionStringW_Fn o_ImmGetCompositionStringW = NULL;

static void* proc_from_module(const char* module, const char* name)
{
    HMODULE h = GetModuleHandleA(module);
    if (!h)
        h = LoadLibraryA(module);
    return h ? (void*)GetProcAddress(h, name) : NULL;
}

static void init_original_procs()
{
    if (!o_GetMessageA) o_GetMessageA = (GetMessageA_Fn)proc_from_module("user32.dll", "GetMessageA");
    if (!o_GetMessageW) o_GetMessageW = (GetMessageW_Fn)proc_from_module("user32.dll", "GetMessageW");
    if (!o_PeekMessageA) o_PeekMessageA = (PeekMessageA_Fn)proc_from_module("user32.dll", "PeekMessageA");
    if (!o_PeekMessageW) o_PeekMessageW = (PeekMessageW_Fn)proc_from_module("user32.dll", "PeekMessageW");
    if (!o_TranslateMessage) o_TranslateMessage = (TranslateMessage_Fn)proc_from_module("user32.dll", "TranslateMessage");
    if (!o_DispatchMessageA) o_DispatchMessageA = (DispatchMessageA_Fn)proc_from_module("user32.dll", "DispatchMessageA");
    if (!o_DispatchMessageW) o_DispatchMessageW = (DispatchMessageW_Fn)proc_from_module("user32.dll", "DispatchMessageW");
    if (!o_ImmGetContext) o_ImmGetContext = (ImmGetContext_Fn)proc_from_module("imm32.dll", "ImmGetContext");
    if (!o_ImmReleaseContext) o_ImmReleaseContext = (ImmReleaseContext_Fn)proc_from_module("imm32.dll", "ImmReleaseContext");
    if (!o_ImmGetCompositionStringW) o_ImmGetCompositionStringW = (ImmGetCompositionStringW_Fn)proc_from_module("imm32.dll", "ImmGetCompositionStringW");
}

static bool has_hangul_w(const WCHAR* text, int count)
{
    if (!text || count <= 0)
        return false;
    __try {
        for (int i = 0; i < count; ++i) {
            WCHAR c = text[i];
            if (c >= 0xAC00 && c <= 0xD7AF)
                return true;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {}
    return false;
}

static bool same_recent_ime(HWND hwnd, const WCHAR* text, int chars)
{
    static HWND lastHwnd = NULL;
    static WCHAR lastText[128];
    static DWORD lastTick = 0;

    DWORD now = GetTickCount();
    bool same = false;
    if (hwnd == lastHwnd && now - lastTick < 800) {
        same = true;
        int n = chars < 127 ? chars : 127;
        for (int i = 0; i < n; ++i) {
            if (lastText[i] != text[i]) {
                same = false;
                break;
            }
        }
        if (same && lastText[n] != 0)
            same = false;
    }

    if (!same) {
        lastHwnd = hwnd;
        int n = chars < 127 ? chars : 127;
        for (int i = 0; i < n; ++i)
            lastText[i] = text[i];
        lastText[n] = 0;
        lastTick = now;
    }
    return same;
}

static void handle_ime_result(HWND hwnd, LPARAM lp)
{
    if (!hwnd || !(lp & GCS_RESULTSTR))
        return;

    init_original_procs();
    if (!o_ImmGetContext || !o_ImmGetCompositionStringW || !o_ImmReleaseContext)
        return;

    HIMC imc = o_ImmGetContext(hwnd);
    if (!imc)
        return;

    WCHAR text[128];
    ZeroMemory(text, sizeof(text));
    LONG bytes = o_ImmGetCompositionStringW(imc, GCS_RESULTSTR, text, sizeof(text) - sizeof(WCHAR));
    o_ImmReleaseContext(hwnd, imc);
    if (bytes <= 0)
        return;

    int chars = (int)(bytes / sizeof(WCHAR));
    if (chars > 127)
        chars = 127;
    text[chars] = 0;

    if (!has_hangul_w(text, chars) || same_recent_ime(hwnd, text, chars))
        return;

    for (int i = 0; i < chars; ++i) {
        WCHAR c = text[i];
        if (!c || c == L'\r' || c == L'\n')
            continue;
        PostMessageW(hwnd, WM_CHAR, (WPARAM)c, 1);
    }
}

static void probe_message(const MSG* msg, bool inject)
{
    if (inject && msg && msg->message == WM_IME_COMPOSITION)
        handle_ime_result(msg->hwnd, msg->lParam);
}

static BOOL WINAPI HookGetMessageA(LPMSG msg, HWND hwnd, UINT minMsg, UINT maxMsg)
{
    init_original_procs();
    BOOL ret = o_GetMessageA ? o_GetMessageA(msg, hwnd, minMsg, maxMsg) : FALSE;
    if (ret > 0)
        probe_message(msg, true);
    return ret;
}

static BOOL WINAPI HookGetMessageW(LPMSG msg, HWND hwnd, UINT minMsg, UINT maxMsg)
{
    init_original_procs();
    BOOL ret = o_GetMessageW ? o_GetMessageW(msg, hwnd, minMsg, maxMsg) : FALSE;
    if (ret > 0)
        probe_message(msg, true);
    return ret;
}

static BOOL WINAPI HookPeekMessageA(LPMSG msg, HWND hwnd, UINT minMsg, UINT maxMsg, UINT removeMsg)
{
    init_original_procs();
    BOOL ret = o_PeekMessageA ? o_PeekMessageA(msg, hwnd, minMsg, maxMsg, removeMsg) : FALSE;
    if (ret && msg)
        probe_message(msg, (removeMsg & PM_REMOVE) != 0);
    return ret;
}

static BOOL WINAPI HookPeekMessageW(LPMSG msg, HWND hwnd, UINT minMsg, UINT maxMsg, UINT removeMsg)
{
    init_original_procs();
    BOOL ret = o_PeekMessageW ? o_PeekMessageW(msg, hwnd, minMsg, maxMsg, removeMsg) : FALSE;
    if (ret && msg)
        probe_message(msg, (removeMsg & PM_REMOVE) != 0);
    return ret;
}

static BOOL WINAPI HookTranslateMessage(const MSG* msg)
{
    init_original_procs();
    return o_TranslateMessage ? o_TranslateMessage(msg) : FALSE;
}

static LRESULT WINAPI HookDispatchMessageA(const MSG* msg)
{
    init_original_procs();
    if (msg)
        probe_message(msg, true);
    return o_DispatchMessageA ? o_DispatchMessageA(msg) : 0;
}

static LRESULT WINAPI HookDispatchMessageW(const MSG* msg)
{
    init_original_procs();
    if (msg)
        probe_message(msg, true);
    return o_DispatchMessageW ? o_DispatchMessageW(msg) : 0;
}

struct InlineHook {
    const char* module;
    const char* name;
    void* hook;
    void** originalSlot;
    BYTE original[16];
    BYTE patchLen;
    bool installed;
};

static bool install_inline_hook(InlineHook* h)
{
    if (!h || h->installed)
        return h && h->installed;

    BYTE* target = (BYTE*)proc_from_module(h->module, h->name);
    if (!target)
        return false;

    BYTE len = 5;
    if (target[0] == 0xFF && target[1] == 0x25)
        len = 6;

    memcpy(h->original, target, len);
    BYTE* tramp = (BYTE*)VirtualAlloc(NULL, 32, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!tramp)
        return false;

    memcpy(tramp, target, len);
    tramp[len] = 0xE9;
    *(DWORD*)(tramp + len + 1) = (DWORD)((target + len) - (tramp + len + 5));
    if (h->originalSlot)
        *h->originalSlot = tramp;

    DWORD oldProtect = 0;
    if (!VirtualProtect(target, len, PAGE_EXECUTE_READWRITE, &oldProtect))
        return false;

    target[0] = 0xE9;
    *(DWORD*)(target + 1) = (DWORD)((BYTE*)h->hook - target - 5);
    for (BYTE i = 5; i < len; ++i)
        target[i] = 0x90;

    DWORD ignored = 0;
    VirtualProtect(target, len, oldProtect, &ignored);
    FlushInstructionCache(GetCurrentProcess(), target, len);
    h->patchLen = len;
    h->installed = true;
    return true;
}

static void install_inline_hooks()
{
    static InlineHook hooks[] = {
        { "user32.dll", "GetMessageA", (void*)HookGetMessageA, (void**)&o_GetMessageA },
        { "user32.dll", "GetMessageW", (void*)HookGetMessageW, (void**)&o_GetMessageW },
        { "user32.dll", "PeekMessageA", (void*)HookPeekMessageA, (void**)&o_PeekMessageA },
        { "user32.dll", "PeekMessageW", (void*)HookPeekMessageW, (void**)&o_PeekMessageW },
        { "user32.dll", "TranslateMessage", (void*)HookTranslateMessage, (void**)&o_TranslateMessage },
        { "user32.dll", "DispatchMessageA", (void*)HookDispatchMessageA, (void**)&o_DispatchMessageA },
        { "user32.dll", "DispatchMessageW", (void*)HookDispatchMessageW, (void**)&o_DispatchMessageW },
    };

    for (int i = 0; i < (int)(sizeof(hooks) / sizeof(hooks[0])); ++i)
        install_inline_hook(&hooks[i]);
}

static DWORD WINAPI worker_thread(void*)
{
    Sleep(200);
    init_original_procs();
    install_inline_hooks();
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = hinst;
        DisableThreadLibraryCalls(hinst);
        real_dll();
        resolve_forwarders();

        wchar_t exe[MAX_PATH];
        if (GetModuleFileNameW(NULL, exe, MAX_PATH)) {
            wchar_t* name = wcsrchr(exe, L'\\');
            name = name ? name + 1 : exe;
            if (lstrcmpiW(name, L"KartRider.exe") == 0) {
                HANDLE h = CreateThread(NULL, 0, worker_thread, NULL, 0, NULL);
                if (h)
                    CloseHandle(h);
            }
        }
    }
    return TRUE;
}
