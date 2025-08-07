#include "MinHook.h"
#include "pch.h"
#include "HookA.h"
#include "mc.h"
#include "xuid.h"
#include <thread>
#include <windows.h>
#include <psapi.h>
#include <vector>
#include <sstream>
#include <string>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <locale>
#include <codecvt>
char Send_this[0x20000] = { 0x00 }, Send_this_2[0x80] = { 0x00 }, Send_this_3[0x80] = { 0x00 }, Send_this_4[0x80] = { 0x00 }, Json_Addr[0x8000] = { 0x00 }, e_this[0xDAC] = { 0x00 };
int cch = 0;
using json = nlohmann::json;
using jsonA = nlohmann::json;
static std::chrono::steady_clock::time_point lastCallTime;
std::string User_Json = "";
std::string Local_Json = "";
bool initXuid = true;
GameXuid xuid_addr;
GameXuid* pXuid;

namespace Hook {

    std::wstring utf8_to_wstring(const std::string& str) {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
        return conv.from_bytes(str);
    }

    std::string fixJson(std::string input) {
        // 替换单引号为双引号（只处理 key 和 value，不处理内容中的 '）
        for (size_t i = 0; i < input.length(); ++i) {
            if (input[i] == '\'') {
                input[i] = '"';
            }
        }

        // 替换 True -> true，False -> false
        size_t pos = 0;
        while ((pos = input.find("True", pos)) != std::string::npos) {
            input.replace(pos, 4, "true");
            pos += 4;
        }
        pos = 0;
        while ((pos = input.find("False", pos)) != std::string::npos) {
            input.replace(pos, 5, "false");
            pos += 5;
        }

        return input;
    }

    std::string UnescapeHex(const std::string& input) {
        std::string output;
        size_t i = 0;
        while (i < input.size()) {
            if (input[i] == '\\' && i + 3 < input.size() && input[i + 1] == 'x') {
                // 解析 \xXX
                std::string hex = input.substr(i + 2, 2);
                char byte = static_cast<char>(std::stoi(hex, nullptr, 16));
                output += byte;
                i += 4;
            }
            else {
                output += input[i];
                ++i;
            }
        }
        return output;
    }

    bool IsCooldownElapsed(double cooldownSeconds = 0.5) {
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - lastCallTime).count();
        if (elapsed < cooldownSeconds) {
            return false;  // 冷却未到
        }
        lastCallTime = now;  // 更新时间戳
        return true;
    }

    bool IsValidJson(const char* data, size_t maxLen = 4096)
    {
        if (!data) return false;
        std::string input;

        const char* scanStart = data;
        size_t maxBackScan = 300;

        // 向前扫描，找起始 {
        for (size_t i = 0; i < maxBackScan; ++i) {
            const char* p = data + i;

            // 如果你有 IsReadableMemory(p, 1)，可以加个判断防止崩溃
            if (*p == '{') {
                scanStart = p;

                const char* next = p + 1;

                if (*next != '\'') {
                    return false;
                }

                // 提取内容显示出来（最多 maxLen 长度）
                std::string jsonText(scanStart, strnlen(scanStart, maxLen));

                // 转换为宽字串用于 MessageBoxW
                int len = MultiByteToWideChar(CP_UTF8, 0, jsonText.c_str(), -1, NULL, 0);
                if (len > 0) {
                    std::wstring wtext(len, 0);
                    input = fixJson(jsonText);
                    input = UnescapeHex(input);
                }
                
                break;
            }
        }

        try {
            if (input.empty() || (input[0] != '{' && input[0] != '['))
                return false;

            json::parse(input);
            User_Json = input;
            Local_Json = User_Json;
            return true;
        }
        catch (json::parse_error&) {
            return false;
        }
    }

    bool IsMessageAllowed(){
    
        jsonA j = json::parse(Local_Json);

        std::string username = j.value("username", "");
        std::string message = j.value("message", "");

        if (!g_Config.NoChatLimit)
        {
            if (mc::IsPlayerSpamming(username))
            {

                if (pXuid == nullptr)
                {
                    std::string cmd = std::string(u8"tellraw @a {\"rawtext\":[{\"text\":\"玩家" + username + u8" 发言超过限制,已被kick" + "\"}]}");
                    SendNethwork ret = mc::SendCommand(cmd);
                    cmd = std::string(u8"kick \"") + username + u8"\" 由于违反刷屏规则,已被kick";
                    mc::SendCommand(cmd);
                    return false;
                }

                std::string xuid;
                std::string name;

                GameXuid* x = &xuid_addr;

                for (size_t i = 0; i < 20; i++)
                {
                    x = x->next;
                    if (x->name_size > 9)
                    {
                        if (Hook::IsReadableMemory((void*)x->name_pointer, 80)) {
                            const char* name_ptr = reinterpret_cast<const char*>(x->name_pointer);
                            name.assign(name_ptr, x->name_size);
                        }
                    }
                    else
                    {
                        name.assign(reinterpret_cast<const char*>(&x->name_pointer), x->name_size);
                    }

                    if (name == username)
                    {

                        if (x->xuid_size > 9)
                        {
                            if (Hook::IsReadableMemory((void*)x->xuid, 80)) {
                                const char* xuid_ptr = reinterpret_cast<const char*>(x->xuid);
                                xuid.assign(xuid_ptr, x->xuid_size);
                            }
                        }
                        else
                        {
                            xuid.assign(reinterpret_cast<const char*>(&x->xuid), x->xuid_size);
                        }


                        std::string cmd = std::string(u8"tellraw @a {\"rawtext\":[{\"text\":\"玩家" + fun::anonymize(username) + u8" 发言超过限制,已被kick - INRBot(1030888791)" + "\"}]}");
                        SendNethwork ret = mc::SendCommand(cmd);
                        mc::SendCommand(u8"kick " + xuid + u8R"( §l§c连接中断
                        §r§7依据此服务器的刷屏规则,你已命中此服务器设定的发言最大限制而被kick
                        §8➤ §7请遵守服务器的发言规范。
                        §8➤ §7服务器机器人：§bINRBot §8(1030888791)
                        §7如有疑问，请联系管理员。
                        §c➤ # 0001。)");
                        return false;
                    }

                }


            }
        }

        if (mc::IsMessageMax(fun::StringToWString(message),g_Config.Max))
        {

            if (pXuid == nullptr)
            {
                std::string cmd = std::string(u8"tellraw @a {\"rawtext\":[{\"text\":\"玩家" + username + u8" 发言超过限制,已被kick" + "\"}]}");
                SendNethwork ret = mc::SendCommand(cmd);
                mc::SendCommand(u8"kick " + username + u8R"( §l§c连接中断
                    §r§7依据此服务器的字数限制,你已命中此服务器设定的发言最大限制而被kick
                    §8➤ §7请遵守服务器的发言规范。
                    §8➤ §7服务器机器人：§bINRBot §8(1030888791)
                    §7如有疑问，请联系管理员。
                    §c➤ # 0002。))");
                return false;
            }

            std::string xuid;
            std::string name;

            GameXuid* x = &xuid_addr;

            for (size_t i = 0; i < 20; i++)
            {
                x = x->next;
                if (x->name_size > 9)
                {
                    if (Hook::IsReadableMemory((void*)x->name_pointer, 80)) {
                        const char* name_ptr = reinterpret_cast<const char*>(x->name_pointer);
                        name.assign(name_ptr, x->name_size);
                    }
                }
                else
                {
                    name.assign(reinterpret_cast<const char*>(&x->name_pointer), x->name_size);
                }

                if (name == username)
                {

                    if (x->xuid_size > 9)
                    {
                        if (Hook::IsReadableMemory((void*)x->xuid, 80)) {
                            const char* xuid_ptr = reinterpret_cast<const char*>(x->xuid);
                            xuid.assign(xuid_ptr, x->xuid_size);
                        }
                    }
                    else
                    {
                        xuid.assign(reinterpret_cast<const char*>(&x->xuid), x->xuid_size);
                    }


                    std::string cmd = std::string(u8"tellraw @a {\"rawtext\":[{\"text\":\"玩家" + fun::anonymize(username) + u8" 发言字数超过限制,已被kick - INRBot(1030888791)" + "\"}]}");
                    SendNethwork ret = mc::SendCommand(cmd);
                    mc::SendCommand(u8"kick " + xuid + u8R"( §l§c连接中断
                    §r§7依据此服务器的字数限制,你已命中此服务器设定的发言最大限制而被kick
                    §8➤ §7请遵守服务器的发言规范。
                    §8➤ §7服务器机器人：§bINRBot §8(1030888791)
                    §7如有疑问，请联系管理员。
                    §c➤ # 0003。))");
                    return false;
                }

            }
        }
        return true;
    
    }


    void Copy_Json(void* c1, void* json,int c2) {

        if (!IsReadableMemory(json))
        {
            if (g_json_ret)
                g_json_ret(c1, json, c2);
            return;
        }

        if (!IsValidJson(reinterpret_cast<const char*>(json)))
        {

            if (g_json_ret)
                g_json_ret(c1, json, c2);
            return;
        }

        if (!IsMessageAllowed())
        {
            return;
        }

        if (IsCooldownElapsed(0.5)) {

            constexpr size_t max_len = 0x8000;
            char* dst = static_cast<char*>(Json_Addr);
            const char* src = User_Json.data();

            size_t i = 0;
            for (; i < max_len; ++i) {
                dst[i] = src[i];
                if (src[i] == '\0') {
                    // 遇到0x00终止字节，停止复制
                    break;
                }
            }

            

            // 如果超过最大长度还没遇到 '\0'，手动补一个终止符
            if (i == max_len)
                dst[max_len - 1] = '\0';

            {
                std::lock_guard<std::mutex> lock(mtx);
                hasNewJson = true;
            }
            cv.notify_one(); // 通知后台线程

        }


        if (g_json_ret)
            g_json_ret(c1, json, c2);
        return;

    }

    uintptr_t ResolveCallTarget(uintptr_t baseAddr, std::initializer_list<size_t> offsets) {
        uintptr_t addr = baseAddr;

        // 按照偏移逐步加
        for (size_t offset : offsets) {
            addr += offset;
        }

        // 读取 call 指令是否为 E8（相对跳转）
        BYTE opcode = *(BYTE*)addr;
        if (opcode != 0xE8) {
            return 0;
        }

        // 读取相对偏移 (4字节带符号)
        int32_t relOffset = *(int32_t*)(addr + 1);

        // 返回真实跳转目标地址 = 当前call地址 + 5（指令长度）+ 偏移
        uintptr_t target = addr + 5 + relOffset;

        return target;
    }

    //检测内存地址是否可读
    bool IsReadableMemory(void* ptr, SIZE_T size) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0)
            return false;

        bool isCommited = (mbi.State == MEM_COMMIT);
        bool isReadable = (mbi.Protect & PAGE_READONLY) ||
            (mbi.Protect & PAGE_READWRITE) ||
            (mbi.Protect & PAGE_WRITECOPY) ||
            (mbi.Protect & PAGE_EXECUTE_READ) ||
            (mbi.Protect & PAGE_EXECUTE_READWRITE) ||
            (mbi.Protect & PAGE_EXECUTE_WRITECOPY);

        bool notGuard = !(mbi.Protect & PAGE_GUARD);
        bool notNoAccess = !(mbi.Protect & PAGE_NOACCESS);

        return isCommited && isReadable && notGuard && notNoAccess && mbi.RegionSize >= size;
    }

    void HookThisMethod(void* g_this) {

        if (cch == 0)
        {


            if (!g_this) {
                MessageBoxW(nullptr, L"this无效！！", L"this", MB_OK);
                return;
            }

            char* offsetPtr = static_cast<char*>(g_this) + 0xC60;

            if (!IsReadableMemory(offsetPtr))
            {
                MessageBoxW(nullptr, L"this不合法(不可读)！！", L"this", MB_OK);
                return;
            }

            void* level1Ptr = *(void**)offsetPtr;
            if (!IsReadableMemory(level1Ptr, 20000)) {
                MessageBoxW(nullptr, L"第一层指针无效!!", L"this", MB_OK);
                return;
            }
            cch = 1;
            memcpy(Send_this, level1Ptr, 20000);

            SendMessage(hwndRadio1, BM_SETCHECK, BST_CHECKED, 0);

        }

        if (g_originalMessageBoxW)
            g_originalMessageBoxW(g_this);
    };

    bool SetupMinHook(void* target, void* hook, void** originalOut)
    {
        if (MH_Initialize() != MH_OK && MH_Initialize() != MH_ERROR_ALREADY_INITIALIZED)
            return false;

        if (MH_CreateHook(target, hook, originalOut) != MH_OK)
            return false;

        if (MH_EnableHook(target) != MH_OK)
            return false;

        return true;
    }

    void ParsePattern(const std::string& patternStr, std::vector<BYTE>& bytes, std::string& mask) {
        std::istringstream iss(patternStr);
        std::string byteStr;
        while (iss >> byteStr) {
            if (byteStr == "??") {
                bytes.push_back(0x00);
                mask += '?';
            }
            else {
                bytes.push_back((BYTE)strtoul(byteStr.c_str(), nullptr, 16));
                mask += 'x';
            }
        }
    }

    bool MemoryCompare(const BYTE* data, const BYTE* pattern, const char* mask, size_t length) {
        for (size_t i = 0; i < length; ++i) {
            if (mask[i] == 'x' && data[i] != pattern[i])
                return false;
        }
        return true;
    }

    uintptr_t FindPatternInMainModule(const std::string& patternStr) {
        std::vector<BYTE> patternBytes;
        std::string mask;
        ParsePattern(patternStr, patternBytes, mask);
        size_t patternLen = patternBytes.size();
        if (patternLen == 0) return 0;

        HMODULE hModule = GetModuleHandleW(NULL);  // 主模块
        if (!hModule) return 0;

        MODULEINFO modInfo = {};
        if (!GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo)))
            return 0;

        BYTE* base = (BYTE*)modInfo.lpBaseOfDll;
        size_t size = modInfo.SizeOfImage;

        for (size_t i = 0; i <= size - patternLen; ++i) {
            if (MemoryCompare(base + i, patternBytes.data(), mask.c_str(), patternLen)) {
                return (uintptr_t)(base + i);
            }
        }
        return 0;
    }

    void Copy_Esc_this(void* e_ehis, void* can)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      {
        void* ptrAt8 = nullptr;
        memcpy(&ptrAt8, e_this + 0x8, sizeof(void*));
        if (ptrAt8 == nullptr)
        {

            //[2856C95D7C0 + 8] + C60


            if (!IsReadableMemory(e_ehis, 0x08 + sizeof(void*))) // 确保能读取 e_ehis+0x08 这个指针
                return;

            // 读取第一级指针 level1 = *(void**)(e_ehis + 0x08)
            void* level1 = *(void**)((BYTE*)e_ehis + 0x08);

            if (!IsReadableMemory(level1, 0xC60 + sizeof(void*))) // 确保能读取 level1 + 0xB98 的指针
                return;

            // 读取第二级指针 level2 = *(void**)(level1 + 0xB98)
            void* level2 = *(void**)((BYTE*)level1 + 0xC60);

            if (!IsReadableMemory(level2, 120)) // 确保能读取 level2 指向的结构，长度120字节
                return;

            // 复制 level2 指向的数据到 e_this+0x50 处，大小120字节
            memcpy((BYTE*)e_this + 0x50, level2, 120);

            // 计算 e_this + 0x10 地址，作为 level3 指针
            BYTE* level3 = (BYTE*)e_this + 0x10;

            // 把 level3 赋值给 e_this + 0x8 （一级指针）
            *(void**)((BYTE*)e_this + 0x8) = level3;

            // level3 + 0xB98 位置，存放指针，指向 e_this + 0x50（二级指针）
            void** ptr_to_level50 = (void**)(level3 + 0xC60);

            // 写入指针前也检测一下安全性
            if (!IsReadableMemory(ptr_to_level50, sizeof(void*)))
                return;

            *ptr_to_level50 = (BYTE*)e_this + 0x50;


            MH_DisableHook((void*)Esc_addr);
            SendMessage(hwndRadio2, BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(hwndRadio3, FALSE);

        }

        // 调用原函数
        if (Esc_ret)
            Esc_ret(e_ehis, can);

    }


    bool IsUtf8CharStart(unsigned char c) {
        // UTF-8 字符的起始字节：0xxxxxxx 或 11xxxxxx
        return (c & 0xC0) != 0x80;
    }


    void* BanIf(void* c1, void** messageaddr) {

        if (!throttler::CanRunNow())
        {
                return g_player_message ? g_player_message(c1, messageaddr) : nullptr;
        }


        char* raw = *(char**)messageaddr;


        if (!IsReadableMemory(raw))  // 再次确保原始指针可读
            return g_player_message ? g_player_message(c1, messageaddr) : nullptr;


        size_t raw_len = strnlen(raw, 150);
        if (raw_len < 16)  // 太短不处理，节省开销
            return g_player_message ? g_player_message(c1, messageaddr) : nullptr;

        if (raw_len < 3)
            return g_player_message ? g_player_message(c1, messageaddr) : nullptr;

        if (!(static_cast<unsigned char>(raw[0]) == 0xC2 &&
            static_cast<unsigned char>(raw[1]) == 0xA7 &&
            static_cast<unsigned char>(raw[2]) == 0x65)) {
            return g_player_message ? g_player_message(c1, messageaddr) : nullptr;
        }


        char* msg = raw + 3;
        size_t msg_len = raw_len - 3;


        std::vector<unsigned char*> char_positions;
        for (size_t i = 0; i < msg_len; ++i) {
            unsigned char c = (unsigned char)msg[i];
            if (IsUtf8CharStart(c)) {
                char_positions.push_back((unsigned char*)(msg + i));
            }
        }


        if (char_positions.size() < 5)
            return g_player_message ? g_player_message(c1, messageaddr) : nullptr;


        unsigned char* target = char_positions[char_positions.size() - 5];

        const unsigned char pattern[] = {
            0xE5, 0x8A, 0xA0, // 加
            0xE5, 0x85, 0xA5, // 入
            0xE4, 0xBA, 0x86, // 了
            0xE6, 0xB8, 0xB8, // 游
            0xE6, 0x88, 0x8F  // 戏
        };

        size_t offset_in_msg = target - (unsigned char*)msg;

        if (offset_in_msg + sizeof(pattern) > msg_len)
            return g_player_message ? g_player_message(c1, messageaddr) : nullptr;

        if (memcmp(target, pattern, sizeof(pattern)) != 0)
            return g_player_message ? g_player_message(c1, messageaddr) : nullptr;
         

        size_t name_len = (size_t)(target - (unsigned char*)msg);
        std::string name_utf8(msg, name_len);

        int wlen = MultiByteToWideChar(CP_UTF8, 0, name_utf8.c_str(), -1, NULL, 0);
        if (wlen <= 0)
            return g_player_message ? g_player_message(c1, messageaddr) : nullptr;

        std::wstring wname(wlen, 0);
        MultiByteToWideChar(CP_UTF8, 0, name_utf8.c_str(), -1, &wname[0], wlen);

        if (fun::GetUserPermission(name_utf8) != UserPermission::ban)
            return g_player_message ? g_player_message(c1, messageaddr) : nullptr;

        std::time_t timestamp_start = fun::WStringToTimestamp(fun::ReadIniString(L"bantime_start", wname, L"0"));
        std::time_t timestamp_end = fun::WStringToTimestamp(fun::ReadIniString(L"bantime_end", wname, L"0"));

        if (timestamp_start == 0 || timestamp_end == 0)
            return g_player_message ? g_player_message(c1, messageaddr) : nullptr;

        std::tm localTime_start = {}, localTime_end = {};
        localtime_s(&localTime_start, &timestamp_start);
        localtime_s(&localTime_end, &timestamp_end);

        std::wstringstream ss_start, ss_end;
        ss_start << (1900 + localTime_start.tm_year) << L"-"
            << (1 + localTime_start.tm_mon) << L"-" << localTime_start.tm_mday << L" "
            << (localTime_start.tm_hour < 10 ? L"0" : L"") << localTime_start.tm_hour << L":"
            << (localTime_start.tm_min < 10 ? L"0" : L"") << localTime_start.tm_min << L":"
            << (localTime_start.tm_sec < 10 ? L"0" : L"") << localTime_start.tm_sec;

        ss_end << (1900 + localTime_end.tm_year) << L"-"
            << (1 + localTime_end.tm_mon) << L"-" << localTime_end.tm_mday << L" "
            << (localTime_end.tm_hour < 10 ? L"0" : L"") << localTime_end.tm_hour << L":"
            << (localTime_end.tm_min < 10 ? L"0" : L"") << localTime_end.tm_min << L":"
            << (localTime_end.tm_sec < 10 ? L"0" : L"") << localTime_end.tm_sec;

        if (timestamp_end >= timestamp_start) {
            mc::SendCommand(u8"/tellraw @a {\"rawtext\":[{\"text\":\"依据ini文件,上述进入游戏的玩家 " + name_utf8 +
                u8" 已被封禁,开始时间:" + fun::WStringToString(ss_start.str()) +
                u8",结束时间:" + fun::WStringToString(ss_end.str()) + "\"}]}");

            mc::SendCommand(u8"/kick " + name_utf8 +
                u8"\" 依据ini文件,你已被此服务器封禁 结束时间:" +
                fun::WStringToString(ss_end.str()) + " - [INRBot]\"");
        }

        // 原始回调
        if (g_player_message)
            return g_player_message(c1, messageaddr);

    }

    



} // namespace Hook