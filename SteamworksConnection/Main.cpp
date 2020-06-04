#define _CRT_SECURE_NO_WARNINGS

#include "Client.h"
#include <sstream>
#include <chrono> 
#include <ctime> 
#include <iomanip>

void ReportError(const std::string& sMessage)
{
	auto now = std::chrono::system_clock::now();
	auto now_c = std::chrono::system_clock::to_time_t(now);
	std::cout << "[" << GetCurrentProcessId() << "]: " << std::put_time(std::localtime(&now_c), "%c") << "> " << sMessage << std::endl;
}

void SimpleLog(const std::string& sMessage)
{
	auto now = std::chrono::system_clock::now();
	auto now_c = std::chrono::system_clock::to_time_t(now);
	std::cout << "[" << GetCurrentProcessId() << "]: " << std::put_time(std::localtime(&now_c), "%c") << "> " << sMessage << std::endl;
}

DWORD WINAPI MessageLoop(void* pArgs)
{
	auto coordinator = (ISteamGameCoordinator*)pArgs;

	while (true)
	{
		try {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			Steam_RunCallbacks(GetHSteamPipe(), false);
		}
		catch (std::exception& e)
		{
			ReportError(e.what());
			SimpleLog("Shutting down MessageLoop");
			return 0;
		}
	}
}

int main(char* argv[])
{
	if (SteamAPI_RestartAppIfNecessary(k_uAppIdInvalid))
	{
		ReportError("What");
		SimpleLog("Shutting down");
		return 0;
	}

	if (!SteamAPI_Init())
	{
		ReportError("SteamAPI_Init failed");
		SimpleLog("Shutting down");
		return 0;
	}

	if (!SteamUser()->BLoggedOn())
	{
		ReportError("You are not logged into Steam :(");
		SimpleLog("Shutting down");
		return 0;
	}
	else
	{
		std::cout << "Current logged in user: " << SteamFriends()->GetPersonaName() << std::endl;
	}

	auto coordinator = (ISteamGameCoordinator*)SteamClient()
		->GetISteamGenericInterface(GetHSteamUser(), GetHSteamPipe(), STEAMGAMECOORDINATOR_INTERFACE_VERSION);
	if ( coordinator == nullptr )
	{
		ReportError("Couldn't grab Coordinator");
		SimpleLog("Shutting down");
		return 0;
	}

	Client client(coordinator);

	CreateThread(nullptr, 0, MessageLoop, (void*)coordinator, 0, nullptr);

	HANDLE hPipeIn;
	char buffer[1024];
	DWORD dwRead;

	// Let everyone access pipe
	// Initialize a security descriptor.  
	PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR,
		SECURITY_DESCRIPTOR_MIN_LENGTH);

	InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);

	// NULL means Everyone can acces the pipe
	SetSecurityDescriptorDacl(pSD, TRUE, NULL, FALSE);

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = pSD;
	sa.bInheritHandle = FALSE;

	hPipeIn = CreateNamedPipe(TEXT("\\\\.\\pipe\\ShareCodePipe"),
		PIPE_ACCESS_DUPLEX,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,   // FILE_FLAG_FIRST_PIPE_INSTANCE is not needed but forces CreateNamedPipe(..) to fail if the pipe already exists...
		1,
		1024 * 16,
		1024 * 16,
		NMPWAIT_USE_DEFAULT_WAIT,
		&sa);

	CMsgClientHello hello;
	hello.set_client_session_need(1);

	SimpleLog("Sending Hello");

	if (client.SendMessageToGC(k_EMsgGCClientHello, &hello) != k_EGCResultOK)
	{
		ReportError("Failed to send Hello -_-");
		SimpleLog("Shutting down");
		return 0;
	}

	while (!client.IsReady()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	SimpleLog("Is Ready");

	while (hPipeIn != INVALID_HANDLE_VALUE)
	{
		//They connected
		if (ConnectNamedPipe(hPipeIn, NULL) != FALSE)
		{
			client.SetPipeHandle(hPipeIn);

			while (true)
			{
				if (ReadFile(hPipeIn, buffer, sizeof(buffer) - 1, &dwRead, NULL))
				{ 
					buffer[dwRead] = '\0';

					std::stringstream ss(buffer);
					std::string s;
					std::vector<std::string> vSharecode;

					while (std::getline(ss, s, '|')) {
						vSharecode.push_back(s);
					}

					SimpleLog("\nShareCode data received: \n");
					printf("m: %s\no: %s\nt: %s\n", vSharecode[0].c_str(), vSharecode[1].c_str(), vSharecode[2].c_str());

					CMsgGCCStrike15_v2_MatchListRequestFullGameInfo fgi;

					fgi.set_matchid(strtoull(vSharecode[0].c_str(), NULL, 0));
					fgi.set_outcomeid(strtoull(vSharecode[1].c_str(), NULL, 0));
					fgi.set_token(atoi(vSharecode[2].c_str()));

					client.Wait();
					auto res = client.SendMessageToGC(k_EMsgGCCStrike15_v2_MatchListRequestFullGameInfo, &fgi);
					if (res != k_EGCResultOK)
					{
						client.ClearWait();
						std::cout << "failed to send message to GC: " << res << std::endl;
						std::string sPipeMsg = "--demo UNKNOWN_ERROR (" + std::to_string(res) + ")\n";
						DWORD dwWritten;
						WriteFile(hPipeIn, sPipeMsg.c_str(), sPipeMsg.length(), &dwWritten, nullptr);
					}

					while (client.InWait()) {
						Sleep(50);
					}
				}
				else
				{
					std::cout << "ReadFile failed with error code: " << GetLastError() << std::endl;

					DisconnectNamedPipe(hPipeIn);

					std::cout << "Disconnected named pipe" << std::endl;
					SimpleLog("Shutting down");
					return 0;
				}
			}
		}
	}

	SimpleLog("Shutting down");
	return 0;
}