#include <vcl.h>
#pragma hdrstop

#include "MainForm.h"
#include <System.SysUtils.hpp>
#include <algorithm>
#include <chrono>
#include <cstdint>

float NetInterpolationClockSeconds()
{
	static const auto t0 = std::chrono::steady_clock::now();
	return std::chrono::duration<float>(std::chrono::steady_clock::now() - t0).count();
}

void TForm1::InitializeNetwork()
{
	if (!NetworkManager.Initialize())
	{
		return;
	}
}

void TForm1::UpdateNetwork(float deltaTime)
{
	NetworkManager.Update(deltaTime);

	if (NetworkManager.ConsumePendingReturnToLobby())
		ReturnToCoopLobby();

	if (IsNetworkGameActive && !NetworkManager.IsHosting() && NetworkManager.GetState() == NeonGame::ENetworkState::Connected)
	{
		SendPlayerInput();
	}

	ReceiveNetworkUpdates();

	NetworkUpdateTimer += deltaTime;
	if (NetworkUpdateTimer >= NetworkUpdateInterval)
	{
		NetworkUpdateTimer = 0.0f;

		if (NetworkManager.IsHosting() && World->IsNetworkGameActive())
		{
			TGameWorld::TGameStateSnapshot snapshot = World->GetGameStateSnapshot();
			snapshot.Tick = FrameNumber;

			std::vector<NeonGame::TPlayerUpdatePacket> playerUpdates;
			std::vector<NeonGame::TEnemyUpdatePacket> enemyUpdates;
			for (const auto &playerState : snapshot.Players)
			{
				NeonGame::TPlayerUpdatePacket update;
				update.Header.Type = NeonGame::EPacketType::PlayerUpdate;
				update.PlayerID = playerState.PlayerID;
				update.PositionX = playerState.PositionX;
				update.PositionY = playerState.PositionY;
				update.FacingDirectionX = playerState.FacingDirectionX;
				update.FacingDirectionY = playerState.FacingDirectionY;
				update.Health = playerState.Health;
				update.MaxHealth = playerState.MaxHealth;
				update.Level = playerState.Level;
				update.Experience = playerState.Experience;
				update.FrameNumber = snapshot.Tick;
				update.SpeedMultiplier = World->GetPlayerSpeedMultiplier(playerState.PlayerID);
				playerUpdates.push_back(update);
			}

			for (size_t i = 0; i < snapshot.Enemies.size(); ++i)
			{
				const auto &enemyState = snapshot.Enemies[i];
				NeonGame::TEnemyUpdatePacket update;
				update.Header.Type = NeonGame::EPacketType::EnemyUpdate;
				update.EnemyID = static_cast<uint16_t>(i);
				update.EnemyType = enemyState.Type;
				update.PositionX = enemyState.PositionX;
				update.PositionY = enemyState.PositionY;
				update.Health = enemyState.Health;
				update.MaxHealth = enemyState.Health;
				update.IsAlive = enemyState.IsAlive;
				update.NetInstanceId = enemyState.NetInstanceId;
				enemyUpdates.push_back(update);
			}

			NetworkManager.BroadcastGameState(playerUpdates);
			NetworkManager.BroadcastEnemyState(enemyUpdates);
			{
				NeonGame::TBossUpdatePacket bp;
				bp.Header.Type = NeonGame::EPacketType::BossUpdate;
				bp.IsAlive = World->HasActiveBoss();
				if (bp.IsAlive)
				{
					const TPointF bpos = World->GetBossPosition();
					bp.PositionX = bpos.X;
					bp.PositionY = bpos.Y;
					bp.Health = World->GetBossHealth();
					bp.MaxHealth = World->GetBossMaxHealth();
					bp.Phase = World->GetBossPhase();
				}
				NetworkManager.BroadcastBossState(bp);
			}
			{
				NetworkManager.BroadcastBulletState(World->GetBulletNetSnapshot());
			}
			{
				const std::vector<NeonGame::TExpOrbNetPacket> expOrbSnap = World->GetExperienceOrbNetSnapshot();
				NetworkManager.BroadcastExpOrbState(expOrbSnap);
			}

			{
				NeonGame::TWaveUpdatePacket wave;
				wave.Header.Type = NeonGame::EPacketType::WaveUpdate;
				wave.WaveNumber = static_cast<uint32_t>(World->GetWaveManager().GetCurrentWave());
				wave.WaveState = static_cast<uint8_t>(World->GetWaveManager().GetState());
				wave.CooldownRemaining = World->GetWaveManager().GetCooldownTimer();
				wave.RunTimeSeconds = World->GetStats().RunTimeSeconds;
				NetworkManager.BroadcastWaveState(wave);
			}

			{
				std::vector<NeonGame::TUpgradeChoicesPacket> upgradePackets;
				const uint8_t n = World->GetPlayerCount();
				upgradePackets.reserve(n);
				for (uint8_t i = 0; i < n; ++i)
				{
					NeonGame::TUpgradeChoicesPacket p;
					p.Header.Type = NeonGame::EPacketType::UpgradeChoices;
					p.PlayerID = i;
					p.IsWaiting = World->IsPlayerWaitingForUpgradeChoice(i);
					if (p.IsWaiting)
					{
						const auto &ups = World->GetAvailableUpgradesForPlayer(i);
						p.Choices.clear();
						for (size_t k = 0; k < ups.size() && k < 3; ++k)
						{
							NeonGame::TUpgradeChoiceNet c;
							c.Type = static_cast<uint8_t>(ups[k].Type);
							c.Rarity = static_cast<uint8_t>(ups[k].Rarity);
							p.Choices.push_back(c);
						}
					}
					upgradePackets.push_back(p);
				}
				NetworkManager.BroadcastUpgradeChoices(upgradePackets);
			}
		}
	}

	FrameNumber++;
}

void TForm1::SendPlayerInput()
{
	if (!IsNetworkGameActive || NetworkManager.IsHosting())
		return;

	NeonGame::TPlayerInputPacket inputPacket;
	inputPacket.Header.Type = NeonGame::EPacketType::PlayerInput;
	inputPacket.PlayerID = World->GetLocalPlayerID();
	inputPacket.InputUp = InputState.MoveUp;
	inputPacket.InputDown = InputState.MoveDown;
	inputPacket.InputLeft = InputState.MoveLeft;
	inputPacket.InputRight = InputState.MoveRight;
	inputPacket.IsShooting = InputState.PrimaryFire;
	inputPacket.IsAltShooting = InputState.AltFire;
	inputPacket.FrameNumber = FrameNumber;

	if (InputState.HasMouse)
	{
		const TPointF cameraPos = World->GetCameraPosition();
		inputPacket.MouseX = static_cast<float>(InputState.MouseClient.x) + cameraPos.X;
		inputPacket.MouseY = static_cast<float>(InputState.MouseClient.y) + cameraPos.Y;
	}
	else
	{
		const TPointF playerPos = World->GetPlayerPosition();
		inputPacket.MouseX = playerPos.X;
		inputPacket.MouseY = playerPos.Y;
	}

	NetworkManager.SendPlayerInput(inputPacket);
}
