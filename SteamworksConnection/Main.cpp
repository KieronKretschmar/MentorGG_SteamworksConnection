

#include "Client.h"

void ReportError(const std::string& sMessage)
{
	std::cout << sMessage << std::endl;
	std::cin.get();
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
			return 0;
		}
	}
}

int main(char* argv[])
{
	if (SteamAPI_RestartAppIfNecessary(k_uAppIdInvalid))
	{
		ReportError("What");
		return 0;
	}

	if (!SteamAPI_Init())
	{
		ReportError("SteamAPI_Init failed");
		return 0;
	}

	if (!SteamUser()->BLoggedOn())
	{
		ReportError("You are not logged into Steam :(");
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
		return 0;
	}

	Client client(coordinator);

	CreateThread(nullptr, 0, MessageLoop, (void*)coordinator, 0, nullptr);

	CMsgClientHello hello;
	hello.set_client_session_need(1);

	std::cout << "Sending Hello" << std::endl;

	if(client.SendMessageToGC(k_EMsgGCClientHello, &hello) != k_EGCResultOK)
	{
		ReportError("Failed to send Hello -_-");
		return 0;
	}
	
	std::cin.get();

	return 0;
}