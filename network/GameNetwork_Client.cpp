#include <vcl.h>
#pragma hdrstop

#include "GameNetwork.h"
#include <Windows.h>

namespace NeonGame
{
bool TNetworkManager::ConnectToGame(const std::string &hostIP, const std::string &playerName, uint16_t port)
{
	if (State != ENetworkState::Disconnected && State != ENetworkState::Connecting)
		return false;

	ClearError();
	State = ENetworkState::Connecting;
	ConnectionStartTime = GetTickCount();
	IsReconnecting = false;

	ReconnectHostIP = hostIP;
	ReconnectPort = port;
	ReconnectPlayerName = playerName;

	if (!ConnectToHost(hostIP, port))
	{
		SetError("Failed to connect to host");
		State = ENetworkState::Disconnected;
		return false;
	}

	TConnectRequestPacket request;
	request.Header.Type = EPacketType::ConnectRequest;
	request.PlayerName = playerName;
	request.ProtocolVersion = 1;

	if (!SendPacket(ClientSocket, request))
	{
		SetError("Failed to send connection request");
		State = ENetworkState::Disconnected;
		return false;
	}

	IsHost = false;
	LastPacketTime = GetTickCount();

	return true;
}

bool TNetworkManager::SendPlayerInput(const TPlayerInputPacket &input)
{
	if (State != ENetworkState::Connected && State != ENetworkState::Hosting)
		return false;

	if (IsHost)
	{
		ReceivedInputs.push_back(input);
		return true;
	}

	TPlayerInputPacket packet = input;
	packet.PlayerID = LocalPlayerID;
	return SendPacket(ClientSocket, packet);
}

bool TNetworkManager::SendUpgradeSelect(uint8_t choiceIndex)
{
	if (State != ENetworkState::Connected || IsHost)
		return false;
	if (choiceIndex > 2)
		return false;

	TUpgradeSelectPacket p;
	p.Header.Type = EPacketType::UpgradeSelect;
	p.PlayerID = LocalPlayerID;
	p.ChoiceIndex = choiceIndex;
	return SendPacket(ClientSocket, p);
}

std::vector<TPlayerInputPacket> TNetworkManager::GetReceivedInputs()
{
	std::vector<TPlayerInputPacket> inputs = ReceivedInputs;
	ReceivedInputs.clear();
	return inputs;
}

std::vector<TPlayerUpdatePacket> TNetworkManager::GetReceivedPlayerUpdates()
{
	std::vector<TPlayerUpdatePacket> updates = ReceivedPlayerUpdates;
	ReceivedPlayerUpdates.clear();
	return updates;
}

std::vector<TEnemyUpdatePacket> TNetworkManager::GetReceivedEnemyUpdates()
{
	std::vector<TEnemyUpdatePacket> updates = ReceivedEnemyUpdates;
	ReceivedEnemyUpdates.clear();
	return updates;
}

std::vector<TExpOrbNetPacket> TNetworkManager::GetReceivedExpOrbUpdates()
{
	std::vector<TExpOrbNetPacket> out = ReceivedExpOrbUpdates;
	ReceivedExpOrbUpdates.clear();
	return out;
}

std::vector<TWaveUpdatePacket> TNetworkManager::GetReceivedWaveUpdates()
{
	std::vector<TWaveUpdatePacket> out = ReceivedWaveUpdates;
	ReceivedWaveUpdates.clear();
	return out;
}

std::vector<TUpgradeChoicesPacket> TNetworkManager::GetReceivedUpgradeChoices()
{
	std::vector<TUpgradeChoicesPacket> out = ReceivedUpgradeChoices;
	ReceivedUpgradeChoices.clear();
	return out;
}

std::vector<TUpgradeSelectPacket> TNetworkManager::GetReceivedUpgradeSelects()
{
	std::vector<TUpgradeSelectPacket> out = ReceivedUpgradeSelects;
	ReceivedUpgradeSelects.clear();
	return out;
}

std::vector<TBossUpdatePacket> TNetworkManager::GetReceivedBossUpdates()
{
	std::vector<TBossUpdatePacket> out = ReceivedBossUpdates;
	ReceivedBossUpdates.clear();
	return out;
}

std::vector<TBulletNetPacket> TNetworkManager::GetReceivedBulletUpdates()
{
	std::vector<TBulletNetPacket> out = ReceivedBulletUpdates;
	ReceivedBulletUpdates.clear();
	return out;
}
}