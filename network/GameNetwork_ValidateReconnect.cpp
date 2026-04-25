#include <vcl.h>
#pragma hdrstop

#include "GameNetwork.h"

namespace NeonGame
{
bool TNetworkManager::ValidatePacket(const TPacket &packet) const
{
	if (!ValidatePacketHeader(packet.Header))
		return false;

	switch (packet.Header.Type)
	{
		case EPacketType::PlayerInput:
		{
			const TPlayerInputPacket *input = dynamic_cast<const TPlayerInputPacket*>(&packet);
			if (!input)
				return false;

			if (input->PlayerID >= 4)
				return false;
			break;
		}
		case EPacketType::PlayerUpdate:
		{
			const TPlayerUpdatePacket *update = dynamic_cast<const TPlayerUpdatePacket*>(&packet);
			if (!update)
				return false;
			if (update->PlayerID >= 4)
				return false;

			if (update->Health < 0 || update->MaxHealth < 0 || update->Level < 1)
				return false;
			break;
		}
		case EPacketType::EnemyBulkUpdate:
		{
			const TEnemyBulkUpdatePacket *bulk = dynamic_cast<const TEnemyBulkUpdatePacket*>(&packet);
			if (!bulk)
				return false;
			if (bulk->Enemies.size() > 3000u)
				return false;
			break;
		}
		case EPacketType::ExpOrbBulkUpdate:
		{
			const TExpOrbBulkUpdatePacket *bulk = dynamic_cast<const TExpOrbBulkUpdatePacket*>(&packet);
			if (!bulk)
				return false;
			if (bulk->Orbs.size() > 2000u)
				return false;
			break;
		}
		default:
			break;
	}

	return true;
}

bool TNetworkManager::ValidatePacketHeader(const TPacketHeader &header) const
{
	if (static_cast<uint16_t>(header.Type) < 1 ||
		static_cast<uint16_t>(header.Type) > static_cast<uint16_t>(EPacketType::BossUpdate))
		return false;

	if (header.Size > 65535)
		return false;

	if (header.Sequence <= LastReceivedSequence && LastReceivedSequence > 0)
	{
	}

	return true;
}

void TNetworkManager::StartReconnect()
{
	if (IsReconnecting || IsHost)
		return;

	IsReconnecting = true;
	ReconnectTimer = 0;

	if (ClientSocket)
	{
		CloseSocket(ClientSocket);
		ClientSocket = nullptr;
	}

	State = ENetworkState::Disconnected;
}

bool TNetworkManager::TryReconnect()
{
	if (!IsReconnecting)
		return false;

	if (ConnectToGame(ReconnectHostIP, ReconnectPlayerName, ReconnectPort))
	{
		return true;
	}

	return false;
}
}

