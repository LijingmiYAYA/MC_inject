#pragma once
#include <string>
#include <unordered_map>
#include <deque>
#include <chrono>
#include <unordered_set>

// 定义白名单，存储允许不限刷屏的用户名
extern std::unordered_set<std::string> spamWhitelist;

enum class SendNethwork {
    Success = 0,
    Nothis = -1,
    Timeout = -2,
    Addr_Error = -3,
    TooLong = -4,
};

enum class QxTCW{
    Success = 0,
    Nothis = -1,
    Addr_Error = -2,
    MouseOnTarget =-3,
};

enum class authStatus {
    free = 0,
    paid = 1,
    extreme = 3,
    null = 4,
    not_found = -1,
};

extern authStatus Dll_authStatus;

namespace mc {
    SendNethwork SendCommand(std::string message);
    bool IsPlayerSpamming(const std::string& username);
    QxTCW QxTc();
    bool IsMessageMax(const std::wstring& message, SIZE_T Max);
    std::string WStringToUTF8(const std::wstring& wstr);
}