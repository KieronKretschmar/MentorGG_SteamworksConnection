#pragma once

#define _SCL_SECURE_NO_WARNINGS
#include <Windows.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#pragma warning(push, 3)
#pragma warning(disable: 4996)
#include "steamworks_sdk\public\steam\steam_api.h"
#include "steamworks_sdk\public\steam\isteamgamecoordinator.h"
#include "protobuf\cstrike15_gcmessages.pb.h"
#include "protobuf\gcsdk_gcmessages.pb.h"
#include "protobuf\gcsystemmsgs.pb.h"
#pragma warning(pop)

#pragma comment(lib, "steamworks_sdk\\redistributable_bin\\steam_api.lib")
#pragma comment(lib, "protobuf\\libprotobuf.lib")

constexpr uint32 PROTO_FLAG = (1 << 31);

class Client {
private:
	CCallback<Client, GCMessageAvailable_t, false> CbOnMessageAvailable;
	ISteamGameCoordinator* m_pCoordinator;
	static const uint32 typeSize = 2 * sizeof(uint32);
	bool m_bIsReady;
	bool m_bInWait;
	HANDLE m_hPipeOut;

	void OnMessageAvailable(GCMessageAvailable_t* pMsg);
public:
	Client(ISteamGameCoordinator*);
	EGCResults SendMessageToGC(uint32 uMsgType, google::protobuf::Message* msg);
	bool IsReady() { return m_bIsReady; }
	bool InWait() { return m_bInWait; }
	void Wait() { m_bInWait = true; }
	void SetPipeHandle(HANDLE hPipe) { m_hPipeOut = hPipe; }
};