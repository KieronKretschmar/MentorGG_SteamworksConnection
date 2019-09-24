#include "Client.h"

Client::Client(ISteamGameCoordinator* pCoordinator)
	:CbOnMessageAvailable(this, &Client::OnMessageAvailable), m_pCoordinator(pCoordinator) 
{
	m_bIsReady = false;
	m_bInWait = false;
}

void Client::OnMessageAvailable(GCMessageAvailable_t* pMsg)
{
	std::vector<char> recvBuffer;
	recvBuffer.resize(pMsg->m_nMessageSize);

	uint32 msgType;
	uint32 msgSize;

	m_pCoordinator->RetrieveMessage(&msgType, recvBuffer.data(), recvBuffer.size(), &msgSize);

	if (msgType & PROTO_FLAG)
	{
		auto msg_id = msgType & (~PROTO_FLAG);

		//WE ARE IN BOYS LETS GO!
		if (msg_id == k_EMsgGCClientWelcome)
		{
			m_bIsReady = true;

			std::cout << "Received ClientWelcome" << std::endl;
			//std::cout << "Requesting Match history" << std::endl;

			//CMsgGCCStrike15_v2_MatchListRequestRecentUserGames msg;
			//msg.set_accountid(SteamUser()->GetSteamID().GetAccountID());

			//if (SendMessageToGC(k_EMsgGCCStrike15_v2_MatchListRequestRecentUserGames, &msg) != k_EGCResultOK)
			//{
			//	std::cout << "Couldn't request match history for some reason" << std::endl;
			//}
		}

		if (msg_id == k_EMsgGCCStrike15_v2_MatchList)
		{
			CMsgGCCStrike15_v2_MatchList msg;
			msg.ParseFromArray(recvBuffer.data() + typeSize, msgSize - typeSize);
			DWORD dwWritten;

			std::cout << "Received MatchList with " << msg.matches_size() << " matches" << std::endl;

			std::string sPipeMsg = "--nodemo\n";

			if (!msg.matches_size())
			{
				WriteFile(m_hPipeOut,
					sPipeMsg.c_str(),
					sPipeMsg.size(),   // = length + '\0'
					&dwWritten,
					NULL);
			}
			else
			{
				for (int i = 0; i < msg.matches_size(); i++)
				{
					auto match = msg.matches(i);
					auto link = match.roundstatsall(match.roundstatsall_size() - 1).map();
					auto time = match.matchtime();

					std::cout << "Match #" << i + 1 << std::endl;
					std::cout << "---------------" << std::endl;
					std::cout << "Time: " << time << std::endl;
					std::cout << "Link: " << link.c_str() << std::endl;

					sPipeMsg = "--demo " + link + "|" + std::to_string(time) + "\n";

					WriteFile(m_hPipeOut,
						sPipeMsg.c_str(),
						sPipeMsg.size(),   // = length + '\0'
						&dwWritten,
						NULL);

					break;
				}
			}

			m_bInWait = false;
		}
	}
}

EGCResults Client::SendMessageToGC(uint32 uMsgType, google::protobuf::Message* msg)
{	
	std::vector<char> sendBuffer;
	sendBuffer.resize(msg->ByteSize() + typeSize);

	uMsgType |= PROTO_FLAG;

	auto data = (uint32*)sendBuffer.data();

	data[0] = uMsgType;
	data[1] = 0;

	msg->SerializeToArray(sendBuffer.data() + typeSize, sendBuffer.size() - typeSize);

	return m_pCoordinator->SendMessageW(uMsgType, sendBuffer.data(), msg->ByteSize() + typeSize);
}