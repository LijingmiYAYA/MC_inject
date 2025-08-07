// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "HookA.h"
#include "mc.h"
#include "xuid.h"
#include "json/json.hpp"
#include <thread>
#include <windows.h>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
uintptr_t addr = 0, Send_addr = 0, Hook_json_addr = 0, Esc_addr = 0, xuid = 0,init_Xuid = 0;

std::string Ver = "V7";

HINSTANCE hModuleGlobal = NULL;
HWND hwndRadio1, hwndRadio2, hwndRadio3, hwndRadio4 = NULL;
HWND LogCommandInupt;
OriginalFuncType g_originalMessageBoxW =  nullptr;
Json_MinHook g_json_ret = nullptr;
player_message g_player_message = nullptr;
xuid_init_addr xuid_init_ret = nullptr;
xuid_join_addr xuid_join_ret = nullptr;
Copy_Esc_MinHook Esc_ret = nullptr;
g_SengCommand g_SendCommandW = (g_SengCommand)nullptr;
g_Esc g_ESCW = (g_Esc)nullptr;
std::atomic<bool> g_stopDisableEsc{ false };

std::atomic<bool> hasNewJson(false);
std::mutex mtx;
std::condition_variable cv;

using json = nlohmann::json;

bool debug;

authStatus Dll_authStatus = authStatus::null;

void JsonWorkerThread() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [] { return hasNewJson.load(); });

        // 有新 JSON 数据
        hasNewJson = false;
        lock.unlock();  // 解锁防止阻塞

        // 处理 Json_Addr 中的内容

        try{
        json j = json::parse(Json_Addr);

        std::string username = j.value("username", "");
        std::string playerId = j.value("playerId", "");
        std::string message = j.value("message", "");

        /*if (!fun::HasWeirdCharacter(message))
        {
            Gxuid xxuid;
            xxuid.init2(&xuid_addr);
            mc::SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"依据此服务器的ini配置:上述发送消息的玩家字符里有非法字符,已被kick - INRBot(1030888791)""\"}]}");
            mc::SendCommand(u8"kick " + xxuid.NameFindXuid(username) + u8R"( §l§c连接中断
                §r§7依据此服务器的ini配置文件,你已命中此服务器设定的非法字符限制而被kick
                §8➤ §7请遵守服务器的发言规则。
                §8➤ §7服务器机器人：§bINRBot §8(1030888791)
                §7如有疑问，请联系管理员。
                §c➤ # 0071。))");
            continue;
        }*/

        /*if (mc::IsPlayerSpamming(username))
        {
            std::string cmd = std::string(u8"tellraw @a {\"rawtext\":[{\"text\":\"玩家"  + username + u8" 发言超过限制,已被kick"+ "\"}]}");
            SendNethwork ret = mc::SendCommand(cmd);
            cmd = std::string(u8"kick \"") + username + u8"\" 由于违反刷屏规则,已被kick";
            mc::SendCommand(cmd);
            fun::WriteLog(L"收到玩家 " + fun::StringToWString(username) + L" 的信息 " + fun::StringToWString(message) + L" 超过发信息此时,已被ban");
            continue;
        }*/

        fun::WriteLog(L"收到玩家 " + fun::StringToWString(username) + L" 的信息 " + fun::StringToWString(message));

        if (message.size() > 50)
        {
            continue;
        }

        /*std::wstring info =
            L"用户名: " + Hook::utf8_to_wstring(username) + L"\n" +
            L"玩家ID: " + Hook::utf8_to_wstring(playerId) + L"\n" +
            L"消息内容: " + Hook::utf8_to_wstring(message);

        MessageBoxW(nullptr, info.c_str(), L"JSON信息", MB_OK);*/

        std::unordered_map<std::string, std::function<void()>> commandMap = {
    {".help", []() {
        SendNethwork ret = mc::SendCommand(
            u8"tellraw @a {\"rawtext\":[{\"text\":\"---{INRBot - " + Ver + "} : [Help]\\n"
            u8".macro : 快捷指令\\n"
            u8".teleport : 传送\\n"
            u8".ban : 封禁\\n"
            u8".admin : 管理员功能\\n"
            u8".function : 执行自定义函数\\n"
            u8".about : 关于机器人\\n"
            u8"---[Help - End]\\n"
            "\"}]}"
        );

        switch (ret) {
        case SendNethwork::Success: fun::WriteLog(L"发送信息成功"); break;
            case SendNethwork::Addr_Error: fun::WriteLog(L"发指令地址错误"); break;
            case SendNethwork::Nothis: fun::WriteLog(L"没有初始化"); break;
            case SendNethwork::Timeout: fun::WriteLog(L"指令超时"); break;
            default: fun::WriteLog(L"未知错误"); break;
        }
    }},

    {".macro", []() {
        SendNethwork ret = mc::SendCommand(
            u8"tellraw @a {\"rawtext\":[{\"text\":\"---{INRBot - " + Ver + "} : [macro]\\n"
            u8".zyq : 传送资源区\\n"
            u8".spawn : 传送主城\\n"
            u8".hcq : 传送合成区\\n"
            u8".sd : 传送商店\\n"
            u8".boss : 传送boss\\n"
            u8"---[Help - End]\\n"
            "\"}]}"
        );
    }},
            {".spawn",[username,message]() {

                if (g_Config.NoSpawnCommand == false)
                {
                    mc::SendCommand("/gamerule sendcommandfeedback false");
                    std::string cmd = std::string(u8"tag @a[name=\"") + username + "\"] add bot.tp";
                    SendNethwork ret = mc::SendCommand(cmd);
                    mc::SendCommand("tp @a[tag=bot.tp] " + std::to_string(g_Config.zcX) + " " + std::to_string(g_Config.zcY) + " " + std::to_string(g_Config.zcZ));
                    mc::SendCommand("tag @a remove bot.tp");
                    mc::SendCommand("/gamerule sendcommandfeedback true");
                }
                else
                {
                    mc::SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"管理员已禁用此功能\"}]}");
                }

            }},
            {".zyq",[username,message]() {

                if (g_Config.NoZyqCommand == false)
                {
                    mc::SendCommand("/gamerule sendcommandfeedback false");
                    std::string cmd = std::string(u8"tag @a[name=\"") + username + "\"] add bot.tp";
                    SendNethwork ret = mc::SendCommand(cmd);
                    mc::SendCommand("tp @a[tag=bot.tp] " + std::to_string(g_Config.zyqX) + " " + std::to_string(g_Config.zyqY) + " " + std::to_string(g_Config.zyqZ));
                    mc::SendCommand("tag @a remove bot.tp");
                    mc::SendCommand("/gamerule sendcommandfeedback true");
                }
                else
                {
                    mc::SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"管理员已禁用此功能\"}]}");
                }

            }},
            {".hcq",[username,message]() {

                if (g_Config.NoZyqCommand == false)
                {
                    mc::SendCommand("/gamerule sendcommandfeedback false");
                    std::string cmd = std::string(u8"tag @a[name=\"") + username + "\"] add bot.tp";
                    SendNethwork ret = mc::SendCommand(cmd);
                    mc::SendCommand("tp @a[tag=bot.tp] " + std::to_string(g_Config.hcqX) + " " + std::to_string(g_Config.hcqY) + " " + std::to_string(g_Config.hcqZ));
                    mc::SendCommand("tag @a remove bot.tp");
                    mc::SendCommand("/gamerule sendcommandfeedback true");
                }
                else
                {
                    mc::SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"管理员已禁用此功能\"}]}");
                }

            }},
            {".boss",[username,message]() {

                if (g_Config.NoBossCommand == false)
                {
                    mc::SendCommand("/gamerule sendcommandfeedback false");
                    std::string cmd = std::string(u8"tag @a[name=\"") + username + "\"] add bot.tp";
                    SendNethwork ret = mc::SendCommand(cmd);
                    mc::SendCommand("tp @a[tag=bot.tp] " + std::to_string(g_Config.bossX) + " " + std::to_string(g_Config.bossY) + " " + std::to_string(g_Config.bossZ));
                    mc::SendCommand("tag @a remove bot.tp");
                    mc::SendCommand("/gamerule sendcommandfeedback true");
                }
                else
                {
                    mc::SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"管理员已禁用此功能\"}]}");
                }

            }},
            { ".sd",[username,message]() {

                if (g_Config.NoSdCommand == false)
                {
                    mc::SendCommand("/gamerule sendcommandfeedback false");
                    std::string cmd = std::string(u8"tag @a[name=\"") + username + "\"] add bot.tp";
                    SendNethwork ret = mc::SendCommand(cmd);
                    mc::SendCommand("tp @a[tag=bot.tp] " + std::to_string(g_Config.sdX) + " " + std::to_string(g_Config.sdY) + " " + std::to_string(g_Config.sdZ));
                    mc::SendCommand("tag @a remove bot.tp");
                    mc::SendCommand("/gamerule sendcommandfeedback true");
                }
                else
                {
                    mc::SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"管理员已禁用此功能\"}]}");
                }

            }},
            



                std::string firstWord = fun::CutBeforeSpace(fun::GetRestAfterFirstWord(message));

                if (!firstWord.empty() && (firstWord.front() == '.' || adminMap.find(firstWord) != adminMap.end())) {
                    auto it = adminMap.find(firstWord);

                    if (it != adminMap.end()) {
                        it->second();
                    }
                    else {
                        mc::SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"未知指令-admin分指令 可使用.admin help查看帮助\"}]}");
                    };
                }
                else
                {
                    mc::SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"未知指令-admin分指令 可使用.admin help查看帮助\"}]}");
                }


            } },

            { ".ban",[username,message]() {


                std::unordered_map<std::string, std::function<void()>> banMap = {
                    {"help", [username,message]() {

                        SendNethwork ret = mc::SendCommand(
                            u8"tellraw @a {\"rawtext\":[{\"text\":\"---{INRBot - " + Ver + "} : [Ban - Help]\\n"
                            u8"help : 查看帮助\\n"
                            u8"add : 添加ban\\n"
                            u8"delete : 删除ban\\n"
                            u8"use : 使用示例\\n"
                            u8"---[Help - End]\\n"
                            "\"}]}"
                        );

                    }},
                    {"use", [username,message]() {

                        SendNethwork ret = mc::SendCommand(
                            u8"tellraw @a {\"rawtext\":[{\"text\":\"---{INRBot - " + Ver + "} : [Ban - Help]\\n"
                            u8"查看帮助 : .ban help\\n"
                            u8"添加ban : .ban add name day(int)\\n"
                            u8"删除ban : .ban delete name\\n"
                            u8"使用示例 : .ban use\\n"
                            u8"---[Help - End]\\n"
                            "\"}]}"
                        );

                    }},

                    {"add", [username,message]() {

                        int banday;

                        if (!fun::isNumber(fun::GetRestAfterFirstWord(fun::GetRestAfterFirstWord(fun::GetRestAfterFirstWord(message)))))
                        {
                            mc::SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"参数4(day)非int类型。\"}]}");
                            return;
                        }

                        try
                        {
                            banday = std::stoi(fun::GetRestAfterFirstWord(fun::GetRestAfterFirstWord(fun::GetRestAfterFirstWord(message))));
                        }
                        catch (const std::exception&)
                        {
                            mc::SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"§l§c[严重错误] 转换数据抛出异常!!。\"}]}");
                            return;
                        }

                        UserPermission UserPermissionW;

                        UserPermissionW = fun::GetUserPermission(username);

                        if (UserPermissionW == UserPermission::bedrock || UserPermissionW == UserPermission::user)
                        {
                            mc::SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"权限不足,此操作需要Admin权限。\"}]}");
                            return;
                        }

                        std::string AddName = fun::CutBeforeSpace(fun::GetRestAfterFirstWord(fun::GetRestAfterFirstWord(message)));

                        std::time_t Ban_start_time = std::time(nullptr);
                        std::time_t Ban_end_time = std::time(nullptr) + (banday* 86400);

                        fun::DeleteIniKey(L"bantime_end", fun::StringToWString(AddName));
                        fun::DeleteIniKey(L"bantime_start", fun::StringToWString(AddName));
                        fun::DeleteIniKey(L"ban", fun::StringToWString(AddName));

                        fun::WriteIniString(L"bantime_start", fun::StringToWString(AddName), std::to_wstring(Ban_start_time));
                        fun::WriteIniString(L"bantime_end", fun::StringToWString(AddName), std::to_wstring(Ban_end_time));
                        fun::WriteIniString(L"ban", fun::StringToWString(AddName), L"1");

                        std::tm localTime_end = {};
                        std::tm localTime_start = {};

                        localtime_s(&localTime_end, &Ban_start_time);
                        localtime_s(&localTime_start, &Ban_end_time);


                        std::wstringstream ss_start;
                        std::wstringstream ss_end;
                        ss_end << (1900 + localTime_end.tm_year) << L"-"
                            << (1 + localTime_end.tm_mon) << L"-"
                            << localTime_end.tm_mday << L" "
                            << (localTime_end.tm_hour < 10 ? L"0" : L"") << localTime_end.tm_hour << L":"
                            << (localTime_end.tm_min < 10 ? L"0" : L"") << localTime_end.tm_min << L":"
                            << (localTime_end.tm_sec < 10 ? L"0" : L"") << localTime_end.tm_sec;

                        ss_start << (1900 + localTime_start.tm_year) << L"-"
                            << (1 + localTime_start.tm_mon) << L"-"
                            << localTime_start.tm_mday << L" "
                            << (localTime_start.tm_hour < 10 ? L"0" : L"") << localTime_start.tm_hour << L":"
                            << (localTime_start.tm_min < 10 ? L"0" : L"") << localTime_start.tm_min << L":"
                            << (localTime_start.tm_sec < 10 ? L"0" : L"") << localTime_start.tm_sec;

                        mc::SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"添加ban成功,name:"+ AddName +u8"开始时间:"+ fun::WStringToString(ss_end.str()) + u8",结束时间:" + fun::WStringToString(ss_start.str()) + u8"。\"}]}");
                        mc::SendCommand(u8"/kick "+ AddName + u8" 你已被 " + username + u8" 添加进此服务器的ini文件黑名单,解封时间:"+ fun::WStringToString(ss_start.str()));


                    }},
                    {"delete", [username,message]() {

                        mc::SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"此功能暂时禁用,请在ini文件里手动删除\"}]}");

                    }},

                };




                std::string firstWord = fun::CutBeforeSpace(fun::GetRestAfterFirstWord(message));

                if (!firstWord.empty() && (firstWord.front() == '.' || banMap.find(firstWord) != banMap.end())) {
                    auto it = banMap.find(firstWord);

                    if (it != banMap.end()) {
                        it->second();
                    }
                    else {
                        mc::SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"未知指令-ban分指令 可使用.ban help查看帮助\"}]}");
                    };
                }
                else
                {
                    mc::SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"未知指令-ban分指令 可使用.ban help查看帮助\"}]}");
                }


            } },

            { ".bedrock",[username,message]() {

                if (Dll_authStatus == authStatus::free)
                {
                    mc::SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"很抱歉,free版不支持此功能 :(\"}]}");
                    return;
                }

                if (g_Config.NoBedrockCommand)
                {
                    mc::SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"此功能已在ini文件中禁用\"}]}");
                    return;
                }

            } },
            { ".function",[username,message]() {

                if (g_Config.NoFunctionCommand)
                {
                    mc::SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"此功能已在ini文件中禁用\"}]}");
                    return;
                }

                std::wstring path = fun::GetDllDirectory() + L"\\function.txt";
                if (!fun::FileExists(path))
                {
                    mc::SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"function.txt文件不存在,无法执行自定义指令\"}]}");
                    return;
                }


                std::ifstream file(path);
                std::string line;
                int count = 0;

                if (Dll_authStatus == authStatus::free)
                {
                    while (std::getline(file, line) && count < 10) {
                        if (!line.empty()) {
                            // 替换 username
                            std::string replaced = fun::ReplaceAll(line, "username", username);

                            // 处理中文（转成 utf8 JSON 安全格式）
                            std::wstring wline = fun::Utf8ToWstring(replaced);
                            std::string utf8Line = mc::WStringToUTF8(wline);

                            // 发出命令
                            mc::SendCommand((utf8Line).c_str());

                            ++count;
                        }
                    }

                    mc::SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"free版只支持读取前十条内容,如果你的指令没有被完全执行可能是这个问题\"}]}");
                    return;
                
                }

                while (std::getline(file, line) && count < 999) {
                    if (!line.empty()) {
                        // 替换 username
                        std::string replaced = fun::ReplaceAll(line, "username", username);

                        // 处理中文（转成 utf8 JSON 安全格式）
                        std::wstring wline = fun::Utf8ToWstring(replaced);
                        std::string utf8Line = mc::WStringToUTF8(wline);

                        // 发出命令
                        mc::SendCommand((utf8Line).c_str());

                        ++count;
                    }
                }

                mc::SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"paid及extreme版只支持读取前45条内容,如果你的指令没有被完全执行可能是这个问题\"}]}");
                return;




                if (count == 0) {
                    mc::SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"function.txt 是空的\"}]}");
                }


            } },
            // 继续添加更多命令
        };
        

        std::string firstWord = fun::CutBeforeSpace(message);
        if (!firstWord.empty() && (firstWord.front() == '.' || commandMap.find(firstWord) != commandMap.end())) {
            auto it = commandMap.find(firstWord);
            if (it == commandMap.end() && firstWord.front() != '.') {
                it = commandMap.find("." + firstWord);
            }

            if (it != commandMap.end()) {
                it->second();
            }
            else {
                mc::SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"未知指令\"}]}");
            }
        }




        }
            catch (const std::exception& ex) {
        }
    }
}

void DisableEscInput() {

    while (g_stopDisableEsc.load())
    {
        QxTCW tcret = mc::QxTc();

        switch (tcret) {
        case QxTCW::Success:
            break;
        case QxTCW::Addr_Error:
            MessageBoxW(nullptr, L"地址错误", L"返回", MB_OK);
            break;
        case QxTCW::Nothis:
            MessageBoxW(nullptr, L"没有初始化", L"返回", MB_OK);
            break;
        case QxTCW::MouseOnTarget:
            break;
        default:
            MessageBoxW(nullptr, L"未知错误", L"返回", MB_OK);
            break;
        }
        Sleep(3000);
    }

}



LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case 201: // 我们创建按钮时指定的 HMENU 值

            // 示例调用
            {
                /*if (g_SendCommandW == nullptr)
                {
                    MessageBoxW(hwnd, L"地址错误!!", L"Send", MB_OK);
                    return 0;
                }

                LRESULT state = SendMessage(hwndRadio1, BM_GETCHECK, 0, 0);
                if (state != BST_CHECKED)
                {
                    MessageBoxW(hwnd, L"this指针未初始化!!", L"Send", MB_OK);
                    return 0;
                }

                char Send_can_2[80] = { 0 };

                g_SendCommandW(Send_this, Send_can_2);*/

            SendNethwork mcret;
            mcret = mc::SendCommand("test");

            switch (mcret) {
            case SendNethwork::Success:
                //MessageBoxW(nullptr, L"发送成功", L"返回", MB_OK);
                break;
            case SendNethwork::Addr_Error:
                MessageBoxW(nullptr, L"地址错误", L"返回", MB_OK);
                break;
            case SendNethwork::Nothis:
                MessageBoxW(nullptr, L"没有初始化", L"返回", MB_OK);
                break;
            case SendNethwork::Timeout:
                MessageBoxW(nullptr, L"操作超时", L"返回", MB_OK);
                break;
            default:
                MessageBoxW(nullptr, L"未知错误", L"返回", MB_OK);
                break;
            }


            }
            return 0;


        case 103: // 第三个选择框
            if (HIWORD(wParam) == BN_CLICKED)
            {
                // lParam 就是当前这个控件的窗口句柄，绝对正确
                LRESULT state = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0);
                if (state == BST_CHECKED)
                {
                    Esc_addr = Hook::FindPatternInMainModule("CC CC 48 89 5C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 0F 29 B4 24 ?? ?? ?? ?? 48 8B F1 48 8B 1A")+2;
                    if (Esc_addr == 0)
                    {
                        SendMessage(hwndRadio3, BM_SETCHECK, BST_UNCHECKED, 0);
                        MessageBoxW(nullptr, L"ESC字节码没有搜到!", L"ESC", MB_OK);
                        return 0;
                    }

                    g_ESCW = (g_Esc)Esc_addr;
                    Hook::SetupMinHook((void*)Esc_addr, (void*)Hook::Copy_Esc_this, (void**)&Esc_ret);

                }
                else
                {
                    MH_RemoveHook((void*)Esc_addr);
                }
            }
            return 0;

        case 102: // 第二个选择框
            if (HIWORD(wParam) == BN_CLICKED)
            {

                MH_RemoveHook((void*)Esc_addr);
                EnableWindow(hwndRadio3, FALSE);
            }
            return 0;

        case 104: // 第二个选择框
            if (HIWORD(wParam) == BN_CLICKED)
            {
                LRESULT state = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0);
                if (state == BST_CHECKED) {
                    //MessageBoxW(nullptr, L"开始创建线程", L"tips", MB_OK);
                    g_stopDisableEsc = true;
                    std::thread DisableEscInputT(DisableEscInput);
                    DisableEscInputT.detach();

                    //EnableWindow(hwndRadio4, FALSE);
                }
                else
                {
                    g_stopDisableEsc = false;
                }
            }
            return 0;


        }


    

        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

DWORD WINAPI DllThreadProc(LPVOID lpParam)
{
    HINSTANCE hInstance = hModuleGlobal;

    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MyDllWindowClass";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(
        0,
        L"MyDllWindowClass",
        L"DLL 控制窗口",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 500,
        NULL, NULL, hInstance, NULL
    );

    // 单选框1（不可控）
    hwndRadio1 = CreateWindowW(L"BUTTON", L"获取this状态",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_DISABLED | WS_GROUP,
        50, 30, 200, 25, hwnd, (HMENU)101, hInstance, NULL);

    // 单选框2（不可控）
    hwndRadio2 = CreateWindowW(L"BUTTON", L"获取this指针(2)状态",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_DISABLED,
        50, 60, 200, 25, hwnd, (HMENU)102, hInstance, NULL);

    // 单选框3（可控）
    hwndRadio3 = CreateWindowW(L"BUTTON", L"抓取this(2)指针",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        50, 90, 200, 25, hwnd, (HMENU)103, hInstance, NULL);

    // 单选框4（可控）
    hwndRadio4 = CreateWindowW(L"BUTTON", L"开始取消ESC界面",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        50, 120, 200, 25, hwnd, (HMENU)104, hInstance, NULL);

    // 设置统一字体
    HFONT hFont = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");

    HWND hwndSendButton = CreateWindowW(L"BUTTON", L"发送数据",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        50, 160, 100, 30, hwnd, (HMENU)201, hInstance, NULL);

    HWND allControls[] = {
        hwndRadio1, hwndRadio2, hwndRadio3, hwndRadio4, hwndSendButton, LogCommandInupt
    };
    for (HWND ctrl : allControls)
        SendMessage(ctrl, WM_SETFONT, (WPARAM)hFont, TRUE);

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

void ZFun(){

    addr = Hook::FindPatternInMainModule("40 53 48 83 EC ?? 48 8B D9 48 8B 89 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 8B ?? ?? ?? ?? 48 85 C9 74 ?? 48 8B 01 48 8B 40 ??");
    Send_addr = Hook::FindPatternInMainModule("45 33 C0 48 8D 54 24 ?? 48 8B CB E8 ?? ?? ?? ?? 48 8B CF E8 ?? ?? ?? ?? 48 8D 97 ");
    Send_addr = Hook::ResolveCallTarget(Send_addr, { 0x8, 3, 5, 3, 5, 3, 4, 3, 4 });
    Hook_json_addr = Hook::FindPatternInMainModule("4C 8B DC 55 41 57 49 8D AB");
    

    //Minecraft.Windows.exe+1C891A1+3+5+3+6

    if (addr == 0 || Send_addr == 0 || Hook_json_addr == 0 || xuid == 0 || init_Xuid == 0)
    {
        std::wstringstream ss;
        ss << L"init失败, 没有找到字节码，字节码状态：" << std::endl;
        ss << L"  addr           = 0x" << std::hex << std::setw(16) << std::setfill(L'0') << addr << std::endl;
        ss << L"  Send_addr      = 0x" << std::hex << std::setw(16) << std::setfill(L'0') << Send_addr << std::endl;
        ss << L"  Hook_json_addr = 0x" << std::hex << std::setw(16) << std::setfill(L'0') << Hook_json_addr << std::endl;
        //ss << L"  esc_message_addr = 0x" << std::hex << std::setw(16) << std::setfill(L'0') << esc_message_addr << std::endl;

        // 写入日志
        fun::WriteLog(ss.str());
        MessageBoxW(nullptr, L"没有找到字节码", L"Pattern Found", MB_OK | MB_ICONINFORMATION);
        return;
    }
    g_SendCommandW = (g_SengCommand)Send_addr;

    /*char msg[128];
    sprintf_s(msg, "game_message_addr address: 0x%p", (void*)game_message_addr);
    MessageBoxA(NULL, msg, "Pattern Found", MB_OK | MB_ICONINFORMATION);
    
    sprintf_s(msg, "esc_message_addr address: 0x%p", (void*)esc_message_addr);
    MessageBoxA(NULL, msg, "Pattern Found", MB_OK | MB_ICONINFORMATION);*/

    std::wstringstream ss;
    ss << L"init成功，字节码状态：" << std::endl;
    ss << L"  addr           = 0x" << std::hex << std::setw(16) << std::setfill(L'0') << addr << std::endl;
    ss << L"  Send_addr      = 0x" << std::hex << std::setw(16) << std::setfill(L'0') << Send_addr << std::endl;
    ss << L"  Hook_json_addr = 0x" << std::hex << std::setw(16) << std::setfill(L'0') << Hook_json_addr << std::endl;

    // 写入日志
    fun::WriteLog(ss.str());
    Hook::SetupMinHook((void*)addr, (void*)Hook::HookThisMethod, (void**)&g_originalMessageBoxW);
    Hook::SetupMinHook((void*)Hook_json_addr, (void*)Hook::Copy_Json, (void**)&g_json_ret);


    fun::Ini_init();

}

void startFun() {

    //Dll_authStatus = authStatus::extreme;

    //// 创建线程，显示窗口
    //CreateThread(nullptr, 0, DllThreadProc, nullptr, 0, nullptr);

    //std::thread th(ZFun);
    //th.detach();

    //std::thread JsonT(JsonWorkerThread);
    //JsonT.detach();

    std::string uuidHash = "INRBotV7" + fun::MD5Hash(fun::GetUUID());
    std::string url = "http://bot.inr.icu/hwid/check_hwid.php?hwid=" + uuidHash;
    std::string result = fun::HttpGet(url);

        if (result == "-1")
        {
            Dll_authStatus = authStatus::not_found;
            fun::WriteHWIDToDesktop(uuidHash);
            MessageBoxW(nullptr, L"此HWID未授权!请加入1030888791群进行授权,HWID已写到桌面", L"HWID", MB_OK);
            return ;
        }

        if (result == "2")
        {
            Dll_authStatus = authStatus::extreme;
        }
        else
        {
            Dll_authStatus = authStatus::not_found;
            fun::WriteHWIDToDesktop(uuidHash);
            MessageBoxW(nullptr, L"此HWID未授权!请加入1030888791群进行授权,HWID已写到桌面", L"HWID", MB_OK);
            return;
        }

        

        // 创建线程，显示窗口
        CreateThread(nullptr, 0, DllThreadProc, nullptr, 0, nullptr);

        std::thread th(ZFun);
        th.detach();

        std::thread JsonT(JsonWorkerThread);
        JsonT.detach();


}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        hModuleGlobal = hModule;
        std::thread startFunW(startFun);
        startFunW.detach();

    }
    return TRUE;
}


