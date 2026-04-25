#include <vcl.h>
#pragma hdrstop

#include "GameNetwork.h"
#include <Windows.h>

namespace NeonGame
{
void TNetworkManager::Update(float deltaTime)
{
	LastEnemyBulkEntryCount = -1;
	LastExpOrbBulkEntryCount = -1;
	uint32_t currentTime = GetTickCount();

	if (State == ENetworkState::Hosting)
	{
		AcceptNewConnection();
		ProcessClientPackets();
		CheckClientTimeouts();
	}
	else if (State == ENetworkState::Connecting)
	{
		if (currentTime - ConnectionStartTime > ConnectionTimeout)
		{
			SetError("Connection timeout");
			if (IsReconnecting)
			{
				if (TryReconnect())
					return;
			}
			State = ENetworkState::Disconnected;
			IsReconnecting = false;
			return;
		}

		std::unique_ptr<TPacket> packet = ReceivePacket(ClientSocket);
		while (packet)
		{
			if (!ValidatePacket(*packet))
			{
				PacketsLost++;
				packet = ReceivePacket(ClientSocket);
				continue;
			}

			switch (packet->Header.Type)
			{
				case EPacketType::ConnectResponse:
				{
					TConnectResponsePacket *response = static_cast<TConnectResponsePacket*>(packet.get());
					if (response->Accepted)
					{
						LocalPlayerID = response->PlayerID;
						State = ENetworkState::Connected;
						LastPacketTime = currentTime;
						IsReconnecting = false;
						ClearError();

						Clients.clear();
						Clients.resize(4);

						Clients[0].PlayerID = 0;
						Clients[0].PlayerName = "Host";
						Clients[0].IsConnected = true;
						Clients[0].LastPacketTime = currentTime;

						if (LocalPlayerID < Clients.size())
						{
							Clients[LocalPlayerID].PlayerID = LocalPlayerID;
							Clients[LocalPlayerID].PlayerName = ReconnectPlayerName;
							Clients[LocalPlayerID].IsConnected = true;
							Clients[LocalPlayerID].LastPacketTime = currentTime;
						}
					}
					else
					{
						SetError(response->Message.empty() ? "Connection rejected" : response->Message);
						State = ENetworkState::Disconnected;
					}
					break;
				}
				default:
					break;
			}

			packet = ReceivePacket(ClientSocket);
		}

		int sockErr = 0;
		if (ConsumeSocketFault(ClientSocket, sockErr))
		{
			SetError(sockErr == 0 ? "Connection closed by peer" : ("Connection lost (socket error " + std::to_string(sockErr) + ")"));
			StartReconnect();
			return;
		}
	}
	else if (State == ENetworkState::Connected)
	{
		if (currentTime - LastPacketTime > PacketTimeout)
		{
			SetError("Connection lost (timeout)");
			StartReconnect();
			return;
		}

		std::unique_ptr<TPacket> packet = ReceivePacket(ClientSocket);
		while (packet)
		{
			if (!ValidatePacket(*packet))
			{
				PacketsLost++;
				packet = ReceivePacket(ClientSocket);
				continue;
			}

			LastPacketTime = currentTime;

			uint32_t latency = currentTime - packet->Header.Timestamp;
			AverageLatency = AverageLatency * 0.9f + latency * 0.1f;

			switch (packet->Header.Type)
			{
				case EPacketType::PlayerInput:
				{
					TPlayerInputPacket *input = static_cast<TPlayerInputPacket*>(packet.get());
					ReceivedInputs.push_back(*input);
					break;
				}
				case EPacketType::GameStateUpdate:
				{
					TGameStateUpdatePacket *bulk = static_cast<TGameStateUpdatePacket*>(packet.get());
					for (const auto &p : bulk->Players)
						ReceivedPlayerUpdates.push_back(p);
					break;
				}
				case EPacketType::PlayerUpdate:
				{
					TPlayerUpdatePacket *update = static_cast<TPlayerUpdatePacket*>(packet.get());
					ReceivedPlayerUpdates.push_back(*update);
					break;
				}
				case EPacketType::EnemyUpdate:
				{
					TEnemyUpdatePacket *update = static_cast<TEnemyUpdatePacket*>(packet.get());
					ReceivedEnemyUpdates.push_back(*update);
					break;
				}
				case EPacketType::EnemyBulkUpdate:
				{
					TEnemyBulkUpdatePacket *bulk = static_cast<TEnemyBulkUpdatePacket*>(packet.get());
					LastEnemyBulkEntryCount = static_cast<int>(bulk->Enemies.size());
					for (const auto &e : bulk->Enemies)
					{
						ReceivedEnemyUpdates.push_back(e);
					}
					break;
				}
				case EPacketType::ExpOrbBulkUpdate:
				{
					TExpOrbBulkUpdatePacket *bulk = static_cast<TExpOrbBulkUpdatePacket*>(packet.get());
					LastExpOrbBulkEntryCount = static_cast<int>(bulk->Orbs.size());
					for (const auto &o : bulk->Orbs)
						ReceivedExpOrbUpdates.push_back(o);
					break;
				}
				case EPacketType::WaveUpdate:
				{
					TWaveUpdatePacket *w = static_cast<TWaveUpdatePacket*>(packet.get());
					ReceivedWaveUpdates.push_back(*w);
					break;
				}
				case EPacketType::UpgradeChoices:
				{
					TUpgradeChoicesPacket *u = static_cast<TUpgradeChoicesPacket*>(packet.get());
					ReceivedUpgradeChoices.push_back(*u);
					break;
				}
				case EPacketType::UpgradeSelect:
				{
					TUpgradeSelectPacket *u = static_cast<TUpgradeSelectPacket*>(packet.get());
					ReceivedUpgradeSelects.push_back(*u);
					break;
				}
				case EPacketType::BossUpdate:
				{
					TBossUpdatePacket *b = static_cast<TBossUpdatePacket*>(packet.get());
					ReceivedBossUpdates.push_back(*b);
					break;
				}
				case EPacketType::BulletUpdate:
				{
					TBulletBulkUpdatePacket *bb = static_cast<TBulletBulkUpdatePacket*>(packet.get());
					ReceivedBulletUpdates = bb->Bullets;
					break;
				}
				case EPacketType::ReturnToLobby:
				{
					PendingReturnToLobby = true;
					break;
				}
				case EPacketType::Disconnect:
				{
					SetError("Server disconnected");
					State = ENetworkState::Disconnected;
					break;
				}
				default:
					break;
			}

			packet = ReceivePacket(ClientSocket);
		}

		int sockErr = 0;
		if (ConsumeSocketFault(ClientSocket, sockErr))
		{
			SetError(sockErr == 0 ? "Connection closed by peer" : ("Connection lost (socket error " + std::to_string(sockErr) + ")"));
			StartReconnect();
			return;
		}
	}

	if (IsReconnecting)
	{
		ReconnectTimer += static_cast<uint32_t>(deltaTime * 1000.0f);
		if (ReconnectTimer >= ReconnectDelay)
		{
			ReconnectTimer = 0;
			if (!TryReconnect())
			{
				IsReconnecting = false;
				State = ENetworkState::Disconnected;
			}
		}
	}
}
}