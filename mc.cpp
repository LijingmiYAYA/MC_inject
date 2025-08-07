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

constexpr size_t MAX_MESSAGES = 10;                   // ���ƣ���෢�Դ���
constexpr std::chrono::seconds TIME_WINDOW(6);      // ���ƣ��������ڲ��ܷ�̫��
constexpr std::chrono::minutes CLEANUP_THRESHOLD(5); // ����5����û���Ծ����
constexpr std::chrono::seconds CLEANUP_INTERVAL(30); // ÿ30�봥��һ������
bool free_thread = false;

std::unordered_map<std::string, std::deque<TimePoint>> playerMessageTimestamps;

TimePoint lastCleanupTime = Clock::now();

std::unordered_set<std::string> spamWhitelist = {
	"admin",
	"trustedUser1",
	"trustedUser2"
	// ����������������Ӱ������û���
};


namespace mc {

	void free_threadF() {
		free_thread = true;
		while (true)
		{
			SendCommand(u8"tellraw @a {\"rawtext\":[{\"text\":\"��ӭʹ��INRbot! ���๦����1030888791!\"}]}");
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

			// �Ƚ��Ƿ�ƥ�䣨�����ִ�Сд��
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
			return false; // ��Զ����ˢ��
		}

		if (fun::GetUserPermission(username) == UserPermission::bedrock) {
			return false; // ��Զ����ˢ��
		}

		TimePoint now = Clock::now();

		// ����ʱ��δ��Ծ�����
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

		// ������ڼ�¼
		while (!timestamps.empty() && (now - timestamps.front() > TIME_WINDOW)) {
			timestamps.pop_front();
		}

		timestamps.push_back(now);

		if (timestamps.size() > MAX_MESSAGES) {
			// �������ƣ������¼
			playerMessageTimestamps.erase(username);
			return true;
		}

		return false;
	}

	bool IsMessageMax(const std::wstring& message, SIZE_T Max) {

		if (fun::GetUserPermission(fun::WStringToString(message)) == UserPermission::admin) {
			return false; // ��Զ����ˢ��
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

		fun::WriteLog(L"�յ�������Ϣ������ " + fun::StringToWString(message));

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

		static_assert(sizeof(SendPacket) <= sizeof(Send_can_2), "�ṹ��̫��");

		bool tooLong = message.length() > MAX_TEXT_LENGTH;

		const char* finalText = tooLong ? u8"tellraw @a {\"rawtext\":[{\"text\":\"��l��c�˴η�����������,Ϊ�˷�ֹ�����˱���������,��ϵ����Ա���\"}]}" : message.c_str();
		int finalLength = static_cast<int>(strlen(finalText));

		// ��������
		SendPacket* packet = reinterpret_cast<SendPacket*>(Send_can_2);
		packet->text_ptr = finalText;              // ָ���ַ�������
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

	// ���ַ���ת UTF-8 �ַ���
	std::string WStringToUTF8(const std::wstring& wstr) {
		std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
		return conv.to_bytes(wstr);
	}
}