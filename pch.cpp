// pch.cpp: 与预编译标头对应的源文件

#include "pch.h"
#include "mc.h"
#include "HookA.h"
#include "xuid.h"
#include <string>
#include <windows.h>
#include <fstream>
IniConfig g_Config;
tpPermission tpPermissionW;
#include <iostream>
#include <sstream>
#include <iomanip>
#include <comdef.h>
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")
// --- 简单的 MD5 实现：用 Windows Crypto API ---
#include <wincrypt.h>
#pragma comment (lib, "Advapi32")
#pragma comment(lib, "wbemuuid.lib")
#include <wininet.h>
#include <shlobj.h>     // SHGetFolderPath
#include <regex>
extern std::string Ver;
#include <ctime>




// 当使用预编译的头时，需要使用此源文件，编译才能成功。
namespace fun {

    //关键词替换
    std::string ReplaceAll(const std::string& str, const std::string& from, const std::string& to) {
        std::string result = str;
        size_t start_pos = 0;
        while ((start_pos = result.find(from, start_pos)) != std::string::npos) {
            result.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
        return result;
    }

    std::wstring Utf8ToWstring(const std::string& str) {
        if (str.empty()) return L"";
        int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        std::wstring result(sizeNeeded - 1, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], sizeNeeded);
        return result;
    }

    bool isNumber(std::string num){
    
        std::regex pattern(R"(([-+]?\d+(\.\d+)?))");
        return regex_match(num, pattern);

    }

    tpPermission GetTpUserPermission(std::string didian) {

        std::wstring UserPermissionIni = ReadIniString(L"TeleportPermission",StringToWString(didian),L"null");

        if (UserPermissionIni == L"2")
        {
            return tpPermission::admin;
        }
        if (UserPermissionIni == L"1")
        {
            return tpPermission::bedrock;
        }
        if (UserPermissionIni == L"null")
        {
            return tpPermission::null;
        }
        return tpPermission::user;

    }

    UserPermission GetUserPermission(std::string username) {

        std::wstring UserPermissionIni = ReadIniString(L"UserPermission", StringToWString(username), L"null");

        if (UserPermissionIni == L"2")
        {
            return UserPermission::admin;
        }
        if (UserPermissionIni == L"1")
        {
            return UserPermission::bedrock;
        }
        UserPermissionIni = ReadIniString(L"ban", StringToWString(username), L"null");
        if (UserPermissionIni == L"1")
        {
            return UserPermission::ban;
        }
        return UserPermission::user;

    }

    std::wstring GetDllDirectory()
    {
        wchar_t path[MAX_PATH] = { 0 };
        // 获取 DLL 模块文件路径
        GetModuleFileNameW(hModuleGlobal, path, MAX_PATH);

        // 去掉文件名，只留目录
        std::wstring dir(path);
        size_t pos = dir.find_last_of(L"\\/");
        if (pos != std::wstring::npos) {
            dir = dir.substr(0, pos);
        }
        return dir;
    }

    std::wstring GetDllDirectoryA()
    {
        wchar_t path[MAX_PATH] = { 0 };
        // 获取 DLL 模块文件路径
        GetModuleFileNameW(hModuleGlobal, path, MAX_PATH);

        // 去掉文件名，只留目录
        std::wstring dir(path);
        size_t pos = dir.find_last_of(L"\\/");
        if (pos != std::wstring::npos) {
            dir = dir.substr(0, pos);
        }
        return dir;
    }

    std::wstring ReadConfigValue(const std::wstring& key)
    {
        std::wstring iniPath = GetDllDirectory() + L"\\config.ini";

        wchar_t buffer[256] = { 0 };
        DWORD result = GetPrivateProfileStringW(
            L"InitConfig",       // section
            key.c_str(),         // key
            L"",                 // default value
            buffer,
            sizeof(buffer) / sizeof(wchar_t),
            iniPath.c_str()      // full path to ini file
        );

        if (result == 0) {
            return L"";  // 键不存在或读取失败
        }

        return std::wstring(buffer);
    }

    int ReadIntConfigValue(const std::wstring& key)
    {
        std::wstring iniPath = GetDllDirectory() + L"\\config.ini";

        wchar_t buffer[256] = { 0 };
        DWORD result = GetPrivateProfileStringW(
            L"InitConfig",       // section
            key.c_str(),         // key
            L"",                 // default value
            buffer,
            sizeof(buffer) / sizeof(wchar_t),
            iniPath.c_str()      // full path to ini file
        );

        if (result == 0) {
            return 0;  // 读取失败或键不存在，默认返回 0
        }

        // 尝试转换为 int
        try {
            return std::stoi(buffer);
        }
        catch (...) {
            return 0;  // 转换失败，默认返回 0
        }
    }

    void LoadWhitelistFromIni() {
        std::wstring iniPath = GetDllDirectory() + L"\\config.ini";
        wchar_t keysBuffer[4096] = { 0 };

        // 读取 [Whitelist] 段所有键，多个键名以 '\0' 分隔，末尾有双 '\0'
        DWORD size = GetPrivateProfileSectionW(
            L"Whitelist",
            keysBuffer,
            sizeof(keysBuffer) / sizeof(wchar_t),
            iniPath.c_str());

        if (size == 0) {
            return;
        }

        // 解析多个键名
        wchar_t* p = keysBuffer;
        while (*p) {
            std::wstring key = p;

            // 读对应值（玩家名）
            std::wstring playerName = ReadIniString(L"Whitelist", key, L"");

            if (!playerName.empty()) {
                spamWhitelist.insert(WStringToString(playerName));
            }

            p += key.size() + 1; // 移动到下一个键名
        }
    }

    void WriteLog(const std::wstring& message)
    {
        static const wchar_t BOM = 0xFEFF;

        std::wofstream logFile(L"log.txt", std::ios::app | std::ios::binary);
        if (logFile.tellp() == 0)
            logFile.write(&BOM, 1); // 写入 UTF-16 BOM

        logFile << message << L"\n";
    }

    std::wstring StringToWString(const std::string& str) {
        int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        std::wstring wstr(len, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], len);
        wstr.pop_back(); // 去除尾部空字符
        return wstr;
    }

    std::string WStringToString(const std::wstring& wstr) {
        if (wstr.empty()) return std::string();

        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &strTo[0], size_needed, nullptr, nullptr);
        return strTo;
    }

    // 读取 ini 某个 key 的字符串值
    std::wstring ReadIniString(const std::wstring& section, const std::wstring& key, const std::wstring& defaultValue) {
        wchar_t buffer[256] = { 0 };
        std::wstring iniPath = GetDllDirectory() + L"\\config.ini";

        DWORD ret = GetPrivateProfileStringW(
            section.c_str(),
            key.c_str(),
            defaultValue.c_str(),
            buffer,
            sizeof(buffer) / sizeof(wchar_t),
            iniPath.c_str());

        return std::wstring(buffer);
    }

    //写ini文件
    bool WriteIniString(const std::wstring& section, const std::wstring& key, const std::wstring& value) {
        std::wstring iniPath = GetDllDirectory() + L"\\config.ini";
        BOOL result = WritePrivateProfileStringW(section.c_str(), key.c_str(), value.c_str(), iniPath.c_str());
        return result != 0;
    }

    bool DeleteIniKey(const std::wstring& section, const std::wstring& key) {
        std::wstring iniPath = GetDllDirectory() + L"\\config.ini";
        // 要删除一个键，只需要将 value 参数设为 NULL
        BOOL result = WritePrivateProfileStringW(section.c_str(), key.c_str(), nullptr, iniPath.c_str());
        return result != 0;
    }


    //通过值找键
    std::wstring FindIniKeyByValue(const std::wstring& section, const std::wstring& targetValue, const std::wstring& defaultValue) {
        std::wstring iniPath = GetDllDirectory() + L"\\config.ini";
        wchar_t buffer[8192] = { 0 }; // 大 buffer，足够存所有 key=value

        DWORD charsRead = GetPrivateProfileSectionW(
            section.c_str(),
            buffer,
            sizeof(buffer) / sizeof(wchar_t),
            iniPath.c_str());

        if (charsRead == 0) {
            return L""; // 没找到或读取失败
        }

        wchar_t* ptr = buffer;
        while (*ptr) {
            std::wstring entry = ptr;
            size_t eqPos = entry.find(L'=');
            if (eqPos != std::wstring::npos) {
                std::wstring key = entry.substr(0, eqPos);
                std::wstring value = entry.substr(eqPos + 1);
                if (value == targetValue) {
                    return key;
                }
            }
            ptr += wcslen(ptr) + 1;
        }

        return defaultValue; // 没有匹配的值
    }

    //返回一个表里面有多少对
    int CountIniPairsInSection(const std::wstring& section) {
        std::wstring iniPath = GetDllDirectory() + L"\\config.ini";
        wchar_t buffer[8192] = { 0 };

        DWORD charsRead = GetPrivateProfileSectionW(
            section.c_str(),
            buffer,
            sizeof(buffer) / sizeof(wchar_t),
            iniPath.c_str());

        if (charsRead == 0) {
            return 0;
        }

        int count = 0;
        wchar_t* ptr = buffer;
        while (*ptr) {
            std::wstring entry = ptr;
            if (entry.find(L'=') != std::wstring::npos) {
                ++count;
            }
            ptr += wcslen(ptr) + 1;
        }

        return count;
    }


    std::time_t WStringToTimestamp(const std::wstring& wstr) {
        try {
            return static_cast<std::time_t>(std::stoll(wstr)); // 用 stoll 支持更大值
        }
        catch (...) {
            return 0; // 解析失败返回 0
        }
    }

    //查看文件是否存在
    bool FileExists(const std::wstring& filePath) {
        DWORD fileAttr = GetFileAttributesW(filePath.c_str());
        return (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY));
    }

    bool Ini_init() {

        if (ReadConfigValue(L"NoSpawnCommand") == L"true")     g_Config.NoSpawnCommand = true;
        if (ReadConfigValue(L"NoZyqCommand") == L"true")       g_Config.NoZyqCommand = true;
        if (ReadConfigValue(L"NoSdCommand") == L"true")        g_Config.NoSdCommand = true;
        if (ReadConfigValue(L"NoHcqCommand") == L"true")       g_Config.NoHcqCommand = true;
        if (ReadConfigValue(L"NoBossCommand") == L"true")      g_Config.NoBossCommand = true;
        if (ReadConfigValue(L"NoChatLimit") == L"true")        g_Config.NoChatLimit = true;
        if (ReadConfigValue(L"NoTeleportCommand") == L"true")  g_Config.NoTeleportCommand = true;
        if (ReadConfigValue(L"NoAdminCommand") == L"true")     g_Config.NoAdminCommand = true;
        if (ReadConfigValue(L"NoBedrockCommand") == L"true")   g_Config.NoBedrockCommand = true;
        if (ReadConfigValue(L"NoBanCommand") == L"true")       g_Config.NoBanCommand = true;
        if (ReadConfigValue(L"NoMaxMessageKick") == L"true")   g_Config.NoMaxMessageKick = true;
        if (ReadConfigValue(L"NoFunctionCommand") == L"true")  g_Config.NoFunctionCommand = true;


        // 读取整数坐标
        g_Config.zcX = ReadIntConfigValue(L"zcX");
        g_Config.zcY = ReadIntConfigValue(L"zcY");
        g_Config.zcZ = ReadIntConfigValue(L"zcZ");

        g_Config.zyqX = ReadIntConfigValue(L"zyqX");
        g_Config.zyqY = ReadIntConfigValue(L"zyqY");
        g_Config.zyqZ = ReadIntConfigValue(L"zyqZ");

        g_Config.hcqX = ReadIntConfigValue(L"hcqX");
        g_Config.hcqY = ReadIntConfigValue(L"hcqY");
        g_Config.hcqZ = ReadIntConfigValue(L"hcqZ");

        g_Config.sdX = ReadIntConfigValue(L"sdX");
        g_Config.sdY = ReadIntConfigValue(L"sdY");
        g_Config.sdZ = ReadIntConfigValue(L"sdZ");

        g_Config.bossX = ReadIntConfigValue(L"bossX");
        g_Config.bossY = ReadIntConfigValue(L"bossY");
        g_Config.bossZ = ReadIntConfigValue(L"bossZ");

        g_Config.Max = ReadIntConfigValue(L"maxAllowedMessageLength");

        LoadWhitelistFromIni();

        return true;
    }

    //MD5加密
    std::string MD5Hash(const std::string& input) {
        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;
        BYTE hash[16];
        DWORD hashLen = 16;
        char hexStr[33] = { 0 };

        if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
            return "MD5 Error: AcquireContext";
        }

        if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)) {
            CryptReleaseContext(hProv, 0);
            return "MD5 Error: CreateHash";
        }

        DWORD dataLen = static_cast<DWORD>(input.length());
        if (dataLen != input.length()) {
            return "MD5 Error: Input too long";
        }
        CryptHashData(hHash, (BYTE*)input.c_str(), dataLen, 0);


        if (!CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0)) {
            CryptReleaseContext(hProv, 0);
            CryptDestroyHash(hHash);
            return "MD5 Error: GetHashParam";
        }

        std::ostringstream oss;
        for (DWORD i = 0; i < hashLen; i++) {
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }

        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return oss.str();
    }

    // --- 获取 UUID ---
    std::string GetUUID() {
        HRESULT hres;

        hres = CoInitializeEx(0, COINIT_MULTITHREADED);
        if (FAILED(hres)) return "CoInitializeEx failed";

        hres = CoInitializeSecurity(NULL, -1, NULL, NULL,
            RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL, EOAC_NONE, NULL);
        if (FAILED(hres)) {
            CoUninitialize();
            return "CoInitializeSecurity failed";
        }

        IWbemLocator* pLoc = NULL;
        hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
            IID_IWbemLocator, (LPVOID*)&pLoc);
        if (FAILED(hres)) {
            CoUninitialize();
            return "CoCreateInstance failed";
        }

        IWbemServices* pSvc = NULL;
        hres = pLoc->ConnectServer(
            _bstr_t(L"ROOT\\CIMV2"),
            NULL, NULL, 0, NULL, 0, 0, &pSvc);
        if (FAILED(hres)) {
            pLoc->Release();
            CoUninitialize();
            return "ConnectServer failed";
        }

        hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
            NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL, EOAC_NONE);
        if (FAILED(hres)) {
            pSvc->Release();
            pLoc->Release();
            CoUninitialize();
            return "CoSetProxyBlanket failed";
        }

        IEnumWbemClassObject* pEnumerator = NULL;
        hres = pSvc->ExecQuery(
            bstr_t("WQL"),
            bstr_t("SELECT UUID FROM Win32_ComputerSystemProduct"),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL, &pEnumerator);
        if (FAILED(hres)) {
            pSvc->Release();
            pLoc->Release();
            CoUninitialize();
            return "ExecQuery failed";
        }

        IWbemClassObject* pclsObj = NULL;
        ULONG uReturn = 0;
        std::string uuid = "Unknown";

        if (pEnumerator) {
            HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
            if (uReturn) {
                VARIANT vtProp;
                VariantInit(&vtProp);
                hr = pclsObj->Get(L"UUID", 0, &vtProp, 0, 0);
                if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR) {
                    uuid = _bstr_t(vtProp.bstrVal);
                }
                VariantClear(&vtProp);
                pclsObj->Release();
            }
            pEnumerator->Release();
        }

        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return uuid;
    }

    //Get请求
    std::string HttpGet(const std::string& url) {
        HINTERNET hInternet = InternetOpenA("HWIDCheck", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) {
            std::cerr << "InternetOpenA failed\n";
            return "";
        }

        HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (!hConnect) {
            std::cerr << "InternetOpenUrlA failed\n";
            InternetCloseHandle(hInternet);
            return "";
        }

        char buffer[4096];
        DWORD bytesRead;
        std::string response;

        while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            response.append(buffer, bytesRead);
        }

        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return response;
    }

    //写文件到桌面
    bool WriteHWIDToDesktop(const std::string& hwid) {
        char desktopPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_DESKTOP, NULL, 0, desktopPath))) {
            std::string filePath = std::string(desktopPath) + "\\hwid.txt";

            std::ofstream outFile(filePath);
            if (outFile.is_open()) {
                outFile << "HWID: " << hwid << std::endl;
                outFile.close();
                return true;
            }
            else {
                std::cerr << "无法打开文件写入。" << std::endl;
                return false;
            }
        }
        else {
            std::cerr << "无法获取桌面路径。" << std::endl;
            return false;
        }
    }

    //截取空格前面的string
    std::string CutBeforeSpace(const std::string& input) {
        size_t pos = input.find(' ');
        if (pos != std::string::npos) {
            return input.substr(0, pos);
        }
        return input; // 如果没有空格，返回整个字符串
    }

    std::string GetRestAfterFirstWord(const std::string& input) {
        size_t pos = input.find(' ');
        if (pos == std::string::npos) return "";  // 没有空格，返回空串

        // 跳过一个或多个空格后的内容
        size_t start = input.find_first_not_of(' ', pos);
        return (start != std::string::npos) ? input.substr(start) : "";
    }

    std::string anonymize(const std::string& name) {
        if (name.length() <= 2) {
            return name;  // 太短不处理
        }

        return name.front() + std::string(name.length() - 2, '*') + name.back();
    }

    std::vector<std::string> LoadBlacklist(const std::string& filename) {
        std::vector<std::string> blacklist;
        std::ifstream file(GetDllDirectoryA() + L"\\" + fun::StringToWString(filename));
        if (!file) {
            mc::SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"打开黑名单文件(blacklist.txt)失败,请检查dll目录下是否有这个文件\"}]}");
            return blacklist;
        }

        std::string line;
        if (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string keyword;
            while (std::getline(ss, keyword, '.')) {
                if (!keyword.empty())
                    blacklist.push_back(keyword);
            }
        }

        return blacklist;
    }

    bool ContainsBlacklistKeyword(const std::string& text, const std::vector<std::string>& blacklist) {
        for (const auto& keyword : blacklist) {
            if (text.find(keyword) != std::string::npos) {
                return true;  // 命中
            }
        }
        return false;  // 没命中
    }

    bool IsNormalCharacter(uint32_t cp) {
        return (
            (cp >= 0x4E00 && cp <= 0x9FFF) ||     // 中文
            (cp >= 0x3040 && cp <= 0x30FF) ||     // 日文假名
            (cp >= 0xAC00 && cp <= 0xD7AF) ||     // 韩文音节
            (cp >= 0x0020 && cp <= 0x007E) ||     // ASCII 可见字符
            (cp >= 0x00A0 && cp <= 0x00FF) ||     // 拉丁补充
            (cp >= 0x0100 && cp <= 0x017F)        // 拉丁扩展 A
            // 你可以继续加更多范围
            );
    }

    bool HasWeirdCharacter(const std::string& str) {
        size_t i = 0;
        while (i < str.size()) {
            uint32_t cp = 0;
            unsigned char byte = str[i];
            size_t len = 0;

            if ((byte & 0x80) == 0) {
                cp = byte;
                len = 1;
            }
            else if ((byte & 0xE0) == 0xC0) {
                if (i + 1 >= str.size()) return false;
                cp = ((byte & 0x1F) << 6) | (str[i + 1] & 0x3F);
                len = 2;
            }
            else if ((byte & 0xF0) == 0xE0) {
                if (i + 2 >= str.size()) return false;
                cp = ((byte & 0x0F) << 12) | ((str[i + 1] & 0x3F) << 6) | (str[i + 2] & 0x3F);
                len = 3;
            }
            else if ((byte & 0xF8) == 0xF0) {
                if (i + 3 >= str.size()) return false;
                cp = ((byte & 0x07) << 18) | ((str[i + 1] & 0x3F) << 12) | ((str[i + 2] & 0x3F) << 6) | (str[i + 3] & 0x3F);
                len = 4;
            }
            else {
                return false;
            }

            if (!IsNormalCharacter(cp)) {
                return false;
            }

            i += len;
        }
        return true;
    }


}