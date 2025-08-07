// pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。

#ifndef PCH_H
#define PCH_H

// 添加要在此处预编译的标头
#include "framework.h"
#include <string>
#include <ctime>
#include "HookA.h"
#include <codecvt>
#include <locale>
#include <regex>

extern HINSTANCE hModuleGlobal;
extern HWND hwndRadio1, hwndRadio2, hwndRadio3, hwndRadio4;
struct IniConfig {
    bool NoFunctionCommand = false;
    bool NoSpawnCommand = false;
    bool NoZyqCommand = false;
    bool NoSdCommand = false;
    bool NoHcqCommand = false;
    bool NoBossCommand = false;
    bool NoChatLimit = false;
    bool NoTeleportCommand = false;
    bool NoAdminCommand = false;
    bool NoBedrockCommand = false;
    bool NoBanCommand = false;
    bool NoMaxMessageKick = false;
    int zcX;
    int zcY;
    int zcZ;
    int zyqX;
    int zyqY;
    int zyqZ;
    int hcqX;
    int hcqY;
    int hcqZ;
    int bossX;
    int bossY;
    int bossZ;
    int sdX;
    int sdY;
    int sdZ;
    int Max;
};
enum class tpPermission {
    admin = 2,
    bedrock = 1,
    user = 0,
    null = -1
};

enum class UserPermission {
    admin = 2,
    bedrock = 1,
    user = 0,
    ban = -1,
    null = -2,
};

extern IniConfig g_Config;
extern tpPermission tpPermissionW;
extern UserPermission UserPermissionW;

namespace fun {
	std::wstring GetDllDirectory();
	void WriteLog(const std::wstring& message);
	std::wstring StringToWString(const std::string& str);
    std::string WStringToString(const std::wstring& wstr);
	std::wstring ReadConfigValue(const std::wstring& key);
	bool Ini_init();
    int ReadIntConfigValue(const std::wstring& key);
    std::wstring ReadIniString(const std::wstring& section, const std::wstring& key, const std::wstring& defaultValue = L"");
    std::string MD5Hash(const std::string& input);
    std::string GetUUID();
    std::string HttpGet(const std::string& url);
    bool WriteHWIDToDesktop(const std::string& hwid);
    std::string CutBeforeSpace(const std::string& input);
    std::string GetRestAfterFirstWord(const std::string& input);
    bool isNumber(std::string num);
    tpPermission GetTpUserPermission(std::string didian);
    UserPermission GetUserPermission(std::string username);
    bool WriteIniString(const std::wstring& section, const std::wstring& key, const std::wstring& value);
    std::wstring FindIniKeyByValue(const std::wstring& section, const std::wstring& targetValue, const std::wstring& defaultValue = L"");
    int CountIniPairsInSection(const std::wstring& section);
    bool DeleteIniKey(const std::wstring& section, const std::wstring& key);
    bool FileExists(const std::wstring& filePath);
    std::wstring Utf8ToWstring(const std::string& str);
    std::string ReplaceAll(const std::string& str, const std::string& from, const std::string& to);
    std::time_t WStringToTimestamp(const std::wstring& wstr);

    std::string anonymize(const std::string& name);
    std::vector<std::string> LoadBlacklist(const std::string& filename);

    bool ContainsBlacklistKeyword(const std::string& text, const std::vector<std::string>& blacklist);


    bool IsNormalCharacter(uint32_t cp);

    bool HasWeirdCharacter(const std::string& str);


}


#endif //PCH_H
