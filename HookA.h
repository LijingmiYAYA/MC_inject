#pragma once
#include "MinHook.h"
#include "json/json.hpp"
#include <string>
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include "xuid.h"
typedef void(*OriginalFuncType)(void*);
extern OriginalFuncType g_originalMessageBoxW;
typedef void(*Json_MinHook)(void*,void*,int);
typedef void*(*player_message)(void*, void**);
typedef void* (*xuid_init_addr)(void*, void*,void*, void*);
typedef void* (*xuid_join_addr)(void*, void*, void*,void*);

extern Json_MinHook g_json_ret;
extern player_message g_player_message;
extern xuid_init_addr xuid_init_ret;
extern xuid_join_addr xuid_join_ret;
typedef void(*Copy_Esc_MinHook)(void*, void*);
extern Copy_Esc_MinHook Esc_ret;
extern std::string User_Json;
extern char Json_Addr[0x8000];
typedef void(*g_SengCommand)(void* param1, void* param2);
extern g_SengCommand g_SendCommandW;
typedef void(*g_Esc)(void* eq_this,void* air);
extern g_Esc g_ESCW;

extern std::atomic<bool> hasNewJson;
extern std::mutex mtx;
extern std::condition_variable cv;

extern char Send_this[0x20000], Send_this_2[0x80], Send_this_3[0x80], Send_this_4[0x80], e_this[0xDAC];

extern uintptr_t Send_addr;
extern uintptr_t Esc_addr;
extern uintptr_t addr;
extern uintptr_t init_Xuid;

namespace Hook {
    uintptr_t FindPatternInMainModule(const std::string& patternStr);
    bool SetupMinHook(void* target, void* hook, void** originalOut);
    void HookThisMethod(void* g_this);
    bool IsReadableMemory(void* ptr, SIZE_T size = 0x80);
    uintptr_t ResolveCallTarget(uintptr_t baseAddr, std::initializer_list<size_t> offsets);
    void Copy_Json(void* c1, void* json, int c2);
    std::wstring utf8_to_wstring(const std::string& str);
    void Copy_Esc_this(void* e_ehis, void* can);
    void* BanIf(void* c1, void** messageaddr);
}//45 33 C0 48 8D 54 24 ?? 48 8B CB E8 ?? ?? ?? ?? 48 8B CF E8 ?? ?? ?? ?? 48 8D 97 

namespace throttler {
    static std::chrono::steady_clock::time_point lastRun = std::chrono::steady_clock::now() - std::chrono::seconds(10);
    static int threshold_ms = 500;  // 5 秒

    // 返回 true 表示“可以执行”
    inline bool CanRunNow() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRun).count();
        if (elapsed >= threshold_ms) {
            lastRun = now;  // 更新时间
            return true;
        }
        return false;
    }
}