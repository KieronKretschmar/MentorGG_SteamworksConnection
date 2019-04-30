#include "Client.h"

Client::Client(ISteamGameCoordinator* pCoordinator)
	:CbOnMessageAvailable(this, &Client::OnMessageAvailable), m_pCoordinator(pCoordinator) 
{

}

void Client::OnMessageAvailable(GCMessageAvailable_t* pMsg)
{
	//std::cout << "New GC Message is available. Retrieving.." << std::endl;

	std::vector<char> recvBuffer;
	recvBuffer.resize(pMsg->m_nMessageSize);

	uint32 msgType;
	uint32 msgSize;

	m_pCoordinator->RetrieveMessage(&msgType, recvBuffer.data(), recvBuffer.size(), &msgSize);

	if (msgType & PROTO_FLAG)
	{
		auto msg_id = msgType & (~PROTO_FLAG);

		//std::cout << "Retrieved message with id " << msg_id << std::endl;

		//WE ARE IN BOYS LETS GO!
		if (msg_id == k_EMsgGCClientWelcome)
		{
			std::cout << "Received ClientWelcome" << std::endl;
			std::cout << "Requesting Match history" << std::endl;

			CMsgGCCStrike15_v2_MatchListRequestRecentUserGames msg;
			msg.set_accountid(SteamUser()->GetSteamID().GetAccountID());

			if (SendMessageToGC(k_EMsgGCCStrike15_v2_MatchListRequestRecentUserGames, &msg) != k_EGCResultOK)
			{
				std::cout << "Couldn't request match history for some reason" << std::endl;
			}
		}

		if (msg_id == k_EMsgGCCStrike15_v2_MatchList)
		{
			CMsgGCCStrike15_v2_MatchList msg;
			msg.ParseFromArray(recvBuffer.data() + typeSize, msgSize - typeSize);

			std::cout << "Received MatchList with " << msg.matches_size() << " matches" << std::endl;

			for (int i = 0; i < msg.matches_size(); i++)
			{
				auto match = msg.matches(i);
				std::cout << "Match #" << i + 1 << std::endl;
				std::cout << "---------------" << std::endl;
				std::cout << "Time: " << match.matchtime() << std::endl;
				std::cout << "Link: " << match.roundstatsall(match.roundstatsall_size() - 1).map().c_str() << std::endl;
				
				//for (int j = 0; j < match.roundstatsall_size(); j++) {
				//	auto roundstats = match.roundstatsall(j);

				//	std::cout << roundstats.map().c_str() << std::endl;
				//}

			}
		}
	}
}

EGCResults Client::SendMessageToGC(uint32 uMsgType, google::protobuf::Message* msg)
{	
	std::vector<char> sendBuffer;
	sendBuffer.resize(msg->ByteSize() + typeSize);

	uMsgType |= PROTO_FLAG;
	((uint32*)sendBuffer.data())[0] = uMsgType;
	((uint32*)sendBuffer.data())[1] = 0;

	msg->SerializeToArray(sendBuffer.data() + typeSize, sendBuffer.size() - typeSize);

	return m_pCoordinator->SendMessageW(uMsgType, sendBuffer.data(), msg->ByteSize() + typeSize);
}