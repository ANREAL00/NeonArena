#include <vcl.h>
#pragma hdrstop

#include "GameNetwork.h"
#include <Windows.h>
#include <algorithm>

namespace NeonGame
{
TNetworkManager::TNetworkManager()
	: State(ENetworkState::Disconnected),
	  IsHost(false),
	  LocalPlayerID(0),
	  ListenSocket(nullptr),
	  ClientSocket(nullptr),
	  NextSequence(0),
	  LastReceivedSequence(0),
	  ConnectionStartTime(0),
	  LastPacketTime(0),
	  ReconnectTimer(0),
	  IsReconnecting(false),
	  ReconnectPort(0),
	  LocalIPAddress(""),
	  PacketsSent(0),
	  PacketsReceived(0),
	  PacketsLost(0),
	  AverageLatency(0.0f)
{
	Clients.resize(4);
}

TNetworkManager::~TNetworkManager()
{
	Shutdown();
}

bool TNetworkManager::Initialize()
{
	if (!InitializeWinSock())
		return false;
	return true;
}

void TNetworkManager::Shutdown()
{
	Disconnect();
	CleanupWinSock();
}

bool TNetworkManager::StartHosting(const std::string &playerName, uint16_t port)
{
	if (State != ENetworkState::Disconnected)
		return false;

	if (!CreateListenSocket(port))
		return false;

	IsHost = true;
	State = ENetworkState::Hosting;
	LocalPlayerID = 0;

	Clients[0].PlayerID = 0;
	Clients[0].PlayerName = playerName;
	Clients[0].IsConnected = true;
	Clients[0].LastPacketTime = GetTickCount();

	return true;
}

void TNetworkManager::Disconnect()
{
	if (ListenSocket)
	{
		CloseSocket(ListenSocket);
		ListenSocket = nullptr;
	}

	if (ClientSocket)
	{
		CloseSocket(ClientSocket);
		ClientSocket = nullptr;
	}

	for (void* socket : ClientSockets)
	{
		CloseSocket(socket);
	}
	ClientSockets.clear();

	State = ENetworkState::Disconnected;
	IsHost = false;
	Clients.clear();
	Clients.resize(4);
	ReceivedInputs.clear();
	ReceivedPlayerUpdates.clear();
	ReceivedEnemyUpdates.clear();
	ReceivedExpOrbUpdates.clear();
	ReceivedWaveUpdates.clear();
	ReceivedUpgradeChoices.clear();
	ReceivedUpgradeSelects.clear();
	LastEnemyBulkEntryCount = -1;
	LastExpOrbBulkEntryCount = -1;
	PendingReturnToLobby = false;
}

void TNetworkManager::EndClientGameSession()
{
	if (IsHost)
		return;

	if (ClientSocket)
	{
		CloseSocket(ClientSocket);
		ClientSocket = nullptr;
	}

	State = ENetworkState::Disconnected;
	ReceivedInputs.clear();
	ReceivedPlayerUpdates.clear();
	ReceivedEnemyUpdates.clear();
	ReceivedExpOrbUpdates.clear();
	ReceivedWaveUpdates.clear();
	ReceivedUpgradeChoices.clear();
	ReceivedUpgradeSelects.clear();
	LastEnemyBulkEntryCount = -1;
	LastExpOrbBulkEntryCount = -1;
	PendingReturnToLobby = false;
	LastReceivedSequence = 0;
	NextSequence = 0;
	ClearError();
}

bool TNetworkManager::BroadcastGameState(const std::vector<TPlayerUpdatePacket> &players)
{
	if (!IsHost || State != ENetworkState::Hosting)
		return false;

	TGameStateUpdatePacket bulk;
	bulk.Header.Type = EPacketType::GameStateUpdate;
	bulk.Players = players;

	for (void* clientSocket : ClientSockets)
	{
		SendPacket(clientSocket, bulk);
	}

	return true;
}

bool TNetworkManager::BroadcastEnemyState(const std::vector<TEnemyUpdatePacket> &enemies)
{
	if (!IsHost || State != ENetworkState::Hosting)
		return false;

	TEnemyBulkUpdatePacket bulk;
	bulk.Header.Type = EPacketType::EnemyBulkUpdate;
	bulk.Enemies = enemies;

	for (void* clientSocket : ClientSockets)
	{
		SendPacket(clientSocket, bulk);
	}

	return true;
}

bool TNetworkManager::BroadcastExpOrbState(const std::vector<TExpOrbNetPacket> &orbs)
{
	if (!IsHost || State != ENetworkState::Hosting)
		return false;

	TExpOrbBulkUpdatePacket bulk;
	bulk.Header.Type = EPacketType::ExpOrbBulkUpdate;
	bulk.Orbs = orbs;

	for (void* clientSocket : ClientSockets)
	{
		SendPacket(clientSocket, bulk);
	}

	return true;
}

bool TNetworkManager::BroadcastWaveState(const TWaveUpdatePacket &wave)
{
	if (!IsHost || State != ENetworkState::Hosting)
		return false;

	for (void* clientSocket : ClientSockets)
	{
		SendPacket(clientSocket, wave);
	}

	return true;
}

bool TNetworkManager::BroadcastUpgradeChoices(const std::vector<TUpgradeChoicesPacket> &choices)
{
	if (!IsHost || State != ENetworkState::Hosting)
		return false;

	for (void* clientSocket : ClientSockets)
	{
		for (const auto &p : choices)
		{
			SendPacket(clientSocket, p);
		}
	}

	return true;
}

bool TNetworkManager::BroadcastBossState(const TBossUpdatePacket &boss)
{
	if (!IsHost || State != ENetworkState::Hosting)
		return false;

	for (void* clientSocket : ClientSockets)
	{
		if (clientSocket)
			SendPacket(clientSocket, boss);
	}
	return true;
}

bool TNetworkManager::BroadcastBulletState(const std::vector<TBulletNetPacket> &bullets)
{
	if (!IsHost || State != ENetworkState::Hosting)
		return false;

	TBulletBulkUpdatePacket bulk;
	bulk.Header.Type = EPacketType::BulletUpdate;
	bulk.Bullets = bullets;

	for (void* clientSocket : ClientSockets)
	{
		if (clientSocket)
			SendPacket(clientSocket, bulk);
	}
	return true;
}

std::string TNetworkManager::GetPlayerName(uint8_t playerID) const
{
	if (playerID < Clients.size() && Clients[playerID].IsConnected)
	{
		return Clients[playerID].PlayerName;
	}
	return std::string();
}

bool TNetworkManager::BroadcastReturnToLobby()
{
	if (!IsHost || State != ENetworkState::Hosting)
		return false;

	TSignalPacket sig;
	sig.Header.Type = EPacketType::ReturnToLobby;
	for (void *cs : ClientSockets)
	{
		if (cs)
			SendPacket(cs, sig);
	}
	return true;
}

bool TNetworkManager::SendReturnToLobbyRequest()
{
	if (IsHost || State != ENetworkState::Connected || !ClientSocket)
		return false;

	TSignalPacket sig;
	sig.Header.Type = EPacketType::RequestReturnToLobby;
	return SendPacket(ClientSocket, sig);
}

bool TNetworkManager::ConsumePendingReturnToLobby()
{
	if (!PendingReturnToLobby)
		return false;
	PendingReturnToLobby = false;
	return true;
}
}

