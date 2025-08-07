#include "pch.h"
#include "mc.h"
#include "HookA.h"
#include <string>
#include <Psapi.h>
#include <Windows.h>
#include <locale>
#include <codecvt>
#pragma comment(lib, "Psapi.lib")
using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;

constexpr size_t MAX_MESSAGES = 10;                   // 限制：最多发言次数
constexpr std::chrono::seconds TIME_WINDOW(6);      // 限制：多少秒内不能发太多
constexpr std::chrono::minutes CLEANUP_THRESHOLD(5); // 清理：5分钟没发言就清除
constexpr std::chrono::seconds CLEANUP_INTERVAL(30); // 每30秒触发一次清理
bool free_thread = false;

std::unordered_map<std::string, std::deque<TimePoint>> playerMessageTimestamps;

TimePoint lastCleanupTime = Clock::now();

std::unordered_set<std::string> spamWhitelist = {
	"admin",
	"trustedUser1",
	"trustedUser2"
	// 你可以在这里继续添加白名单用户名
};


namespace mc {

	void free_threadF() {
		free_thread = true;
		while (true)
		{
			SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"欢迎使用INRbot! 更多功能在1030888791!\"}]}");
			Sleep(60000);
		}

	}

	bool IsMouseFocusOnProcess(const std::string& processName) {
		HWND foregroundWnd = GetForegroundWindow();
		if (!foregroundWnd) return false;

		DWORD pid = 0;
		GetWindowThreadProcessId(foregroundWnd, &pid);
		if (pid == 0) return false;

		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
		if (!hProcess) return false;

		char exeName[MAX_PATH] = { 0 };
		if (GetModuleBaseNameA(hProcess, NULL, exeName, MAX_PATH)) {
			std::string exeStr(exeName);
			CloseHandle(hProcess);

			// 比较是否匹配（不区分大小写）
			for (auto& c : exeStr) c = tolower(c);
			std::string lowerProcessName = processName;
			for (auto& c : lowerProcessName) c = tolower(c);

			return exeStr == lowerProcessName;
		}

		CloseHandle(hProcess);
		return false;
	}

	bool IsPlayerSpamming(const std::string& username) {

		if (fun::GetUserPermission(username) == UserPermission::admin) {
			return false; // 永远不算刷屏
		}

		if (fun::GetUserPermission(username) == UserPermission::bedrock) {
			return false; // 永远不算刷屏
		}

		TimePoint now = Clock::now();

		// 清理长时间未活跃的玩家
		if (now - lastCleanupTime > CLEANUP_INTERVAL) {
			for (auto it = playerMessageTimestamps.begin(); it != playerMessageTimestamps.end(); ) {
				if (it->second.empty() || now - it->second.back() > CLEANUP_THRESHOLD) {
					it = playerMessageTimestamps.erase(it);
				}
				else {
					++it;
				}
			}
			lastCleanupTime = now;
		}

		auto& timestamps = playerMessageTimestamps[username];

		// 清理过期记录
		while (!timestamps.empty() && (now - timestamps.front() > TIME_WINDOW)) {
			timestamps.pop_front();
		}

		timestamps.push_back(now);

		if (timestamps.size() > MAX_MESSAGES) {
			// 超过限制，清除记录
			playerMessageTimestamps.erase(username);
			return true;
		}

		return false;
	}

	bool IsMessageMax(const std::wstring& message, SIZE_T Max) {

		if (fun::GetUserPermission(fun::WStringToString(message)) == UserPermission::admin) {
			return false; // 永远不算刷屏
		}

		if (!g_Config.NoMaxMessageKick)
		{
			SIZE_T MessageLen = 0;
			MessageLen = message.length();
			if (MessageLen >= Max)
			{
				return true;
			}

			return false;
		}

		return false;
	}

	SendNethwork SendCommand(std::string message) {

		constexpr size_t MAX_TEXT_LENGTH = 0x200;

		fun::WriteLog(L"收到发送信息的请求 " + fun::StringToWString(message));

		LRESULT state = SendMessage(hwndRadio1, BM_GETCHECK, 0, 0);
		if (state != BST_CHECKED)
		{
			return SendNethwork::Nothis;
		}

		if (g_SendCommandW == nullptr)
		{
			return SendNethwork::Addr_Error;
		}

		if (Dll_authStatus == authStatus::free)
		{
			if (free_thread == false)
			{
				std::thread free_threadW(free_threadF);
				free_threadW.detach();

			}
		}

		struct SendPacket {
			const char* text_ptr; 
			void* unused;       
			int length;        
			int reserved;     
			int flag;          
		};

		char Send_can_2[80] = { 0 };

		static_assert(sizeof(SendPacket) <= sizeof(Send_can_2), "结构体太大");

		bool tooLong = message.length() > MAX_TEXT_LENGTH;

		const char* finalText = tooLong ? u8"tellraw @a {\"rawtext\":[{\"text\":\"§l§c此次发包字数过多,为了防止机器人被踢已拦截,联系管理员解决\"}]}" : message.c_str();
		int finalLength = static_cast<int>(strlen(finalText));

		// 构造数据
		SendPacket* packet = reinterpret_cast<SendPacket*>(Send_can_2);
		packet->text_ptr = finalText;              // 指向字符串内容
		packet->unused = nullptr;
		packet->length = finalLength;
		packet->reserved = 0;
		packet->flag = 0x9F;               


		g_SendCommandW(Send_this, Send_can_2);


		MH_DisableHook((void*)addr);
		return SendNethwork::Success;
	}

	QxTCW QxTc() {

		if (g_ESCW == nullptr)
		{
			return QxTCW::Addr_Error;
		}

		void* ptrAt8 = nullptr;
		memcpy(&ptrAt8, e_this + 0x8, sizeof(void*));
		if (ptrAt8 == nullptr)
		{
			return QxTCW::Nothis;
		}

		if (IsMouseFocusOnProcess("Minecraft.Windows.exe"))
		{
			return QxTCW::MouseOnTarget;
		}

		char air[0x60] = { 0 };
		g_ESCW(e_this, air);
		return QxTCW::Success;



	}

	// 宽字符串转 UTF-8 字符串
	std::string WStringToUTF8(const std::wstring& wstr) {
		std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
		return conv.to_bytes(wstr);
	}
}