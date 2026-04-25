#include <vcl.h>
#pragma hdrstop

#include "MainForm.h"
#include <System.SysUtils.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>

namespace
{
float SmoothInterpT(float t)
{
	t = std::max(0.0f, std::min(1.0f, t));
	return t * t * (3.0f - 2.0f * t);
}

float MonotonicSnapshotTime(float recvWallSeconds, float lastBufferedTs, bool hasLast)
{
	if (!hasLast || recvWallSeconds > lastBufferedTs)
		return recvWallSeconds;
	return lastBufferedTs + 1.2e-4f;
}

float ShortestAngleDeltaRad(float fromRad, float toRad)
{
	float d = toRad - fromRad;
	const float twoPi = 6.2831853f;
	while (d > 3.14159265f)
		d -= twoPi;
	while (d < -3.14159265f)
		d += twoPi;
	return d;
}

}

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

void TForm1::ReceiveNetworkUpdates()
{
	if (!NetworkManager.IsHosting())
	{
		std::vector<NeonGame::TPlayerUpdatePacket> updates = NetworkManager.GetReceivedPlayerUpdates();
		std::vector<NeonGame::TWaveUpdatePacket> waveUpdates = NetworkManager.GetReceivedWaveUpdates();
		std::vector<NeonGame::TUpgradeChoicesPacket> upgradeChoices = NetworkManager.GetReceivedUpgradeChoices();
		std::vector<NeonGame::TBossUpdatePacket> bossUpdates = NetworkManager.GetReceivedBossUpdates();
		std::vector<NeonGame::TBulletNetPacket> bulletUpdates = NetworkManager.GetReceivedBulletUpdates();
		const int lastEnemyBulkCount = NetworkManager.GetLastEnemyBulkEntryCount();
		const int lastOrbBulkCount = NetworkManager.GetLastExpOrbBulkEntryCount();
		std::vector<NeonGame::TEnemyUpdatePacket> enemyUpdates = NetworkManager.GetReceivedEnemyUpdates();
		std::vector<NeonGame::TExpOrbNetPacket> orbUpdates = NetworkManager.GetReceivedExpOrbUpdates();
		std::vector<NeonGame::TPlayerUpdatePacket> latestPlayerUpdates;
		std::vector<NeonGame::TEnemyUpdatePacket> latestEnemyUpdates;

		if (!updates.empty())
		{
			uint8_t maxPlayerID = 0;
			for (const auto &update : updates)
			{
				maxPlayerID = std::max(maxPlayerID, update.PlayerID);
			}
			std::vector<bool> hasPlayer(maxPlayerID + 1, false);
			std::vector<NeonGame::TPlayerUpdatePacket> playerByID(maxPlayerID + 1);
			for (const auto &update : updates)
			{
				playerByID[update.PlayerID] = update;
				hasPlayer[update.PlayerID] = true;
			}
			for (size_t i = 0; i < playerByID.size(); ++i)
			{
				if (hasPlayer[i])
				{
					latestPlayerUpdates.push_back(playerByID[i]);
				}
			}
		}

		if (!enemyUpdates.empty())
		{
			uint16_t maxEnemyID = 0;
			for (const auto &update : enemyUpdates)
			{
				maxEnemyID = std::max(maxEnemyID, update.EnemyID);
			}
			std::vector<bool> hasEnemy(maxEnemyID + 1, false);
			std::vector<NeonGame::TEnemyUpdatePacket> enemyByID(maxEnemyID + 1);
			for (const auto &update : enemyUpdates)
			{
				enemyByID[update.EnemyID] = update;
				hasEnemy[update.EnemyID] = true;
			}
			for (size_t i = 0; i < enemyByID.size(); ++i)
			{
				if (hasEnemy[i])
				{
					latestEnemyUpdates.push_back(enemyByID[i]);
				}
			}
		}

		if (latestPlayerUpdates.size() > 0)
		{
			uint8_t maxPlayerID = 0;
			for (const auto &update : latestPlayerUpdates)
			{
				if (update.PlayerID > maxPlayerID)
					maxPlayerID = update.PlayerID;
			}

			const int playerCountInt = std::max(
				static_cast<int>(maxPlayerID) + 1,
				static_cast<int>(NetworkManager.GetLocalPlayerID()) + 1);
			const uint8_t playerCount = static_cast<uint8_t>(playerCountInt);

			if (!IsNetworkGameActive)
			{
				ResetGame();
				World->InitializeNetworkGame(NetworkManager.GetLocalPlayerID(), false);
				World->SetPlayerCount(playerCount);
				IsNetworkGameActive = true;
				WorldState = EWorldState::Playing;
			}
			else if (!World->IsNetworkGameActive() || World->GetPlayerCount() < playerCount)
			{
				if (!World->IsNetworkGameActive())
				{
					World->InitializeNetworkGame(NetworkManager.GetLocalPlayerID(), false);
				}
				World->SetPlayerCount(playerCount);
			}
		}

		if (!IsNetworkGameActive)
			return;

		if (!waveUpdates.empty())
		{
			const NeonGame::TWaveUpdatePacket &w = waveUpdates.back();
			World->ApplyWaveUpdate(w.WaveNumber, w.WaveState, w.CooldownRemaining,
				w.RunTimeSeconds, w.IncludesRunTimeSeconds);
		}

		if (!bossUpdates.empty())
		{
			World->ApplyBossUpdateClient(bossUpdates.back());
		}

		if (!bulletUpdates.empty())
		{
			World->ApplyBulletSnapshotClient(bulletUpdates);
		}

		for (const auto &u : upgradeChoices)
		{
			World->ApplyUpgradeChoices(u.PlayerID, u.IsWaiting, u.Choices);
		}

		const float currentTime = NetInterpolationClockSeconds();

		if (!latestEnemyUpdates.empty())
		{
			World->ApplyEnemyUpdates(latestEnemyUpdates);

			uint16_t maxEnemyID = 0;
			for (const auto &u : latestEnemyUpdates)
				maxEnemyID = std::max(maxEnemyID, u.EnemyID);
			const size_t needSlots = static_cast<size_t>(maxEnemyID) + 1;
			if (InterpolatedEnemies.size() < needSlots)
				InterpolatedEnemies.resize(needSlots);
			if (InterpEnemyLastNetInstanceId.size() < needSlots)
				InterpEnemyLastNetInstanceId.resize(needSlots, 0);
			if (InterpEnemyLastNetInstanceId.size() > needSlots)
				InterpEnemyLastNetInstanceId.resize(needSlots);
			if (InterpolatedEnemies.size() > needSlots)
				InterpolatedEnemies.resize(needSlots);

			for (const auto &update : latestEnemyUpdates)
			{
				const size_t id = static_cast<size_t>(update.EnemyID);
				if (id >= InterpolatedEnemies.size())
					continue;
				TInterpolatedEnemySlot &slot = InterpolatedEnemies[id];
				if (!update.IsAlive)
				{
					slot.StateBuffer.clear();
					slot.HasValidState = false;
					if (id < InterpEnemyLastNetInstanceId.size())
						InterpEnemyLastNetInstanceId[id] = 0;
					continue;
				}
				if (id < InterpEnemyLastNetInstanceId.size() && update.NetInstanceId != 0)
				{
					const uint32_t prevNet = InterpEnemyLastNetInstanceId[id];
					if (prevNet != 0 && prevNet != update.NetInstanceId)
						slot.StateBuffer.clear();
					InterpEnemyLastNetInstanceId[id] = update.NetInstanceId;
				}
				else if (update.NetInstanceId == 0 && !slot.StateBuffer.empty())
				{
					const TPointF lastPos = slot.StateBuffer.back().Position;
					const float dx = update.PositionX - lastPos.X;
					const float dy = update.PositionY - lastPos.Y;
					if (dx * dx + dy * dy > 42000.0f)
						slot.StateBuffer.clear();
				}
				TEnemyNetSnapshot snap;
				snap.Position = TPointF(update.PositionX, update.PositionY);
				const bool hasTs = !slot.StateBuffer.empty();
				const float lastTs = hasTs ? slot.StateBuffer.back().Timestamp : 0.0f;
				snap.Timestamp = MonotonicSnapshotTime(currentTime, lastTs, hasTs);
				slot.StateBuffer.push_back(snap);
				if (slot.StateBuffer.size() > TInterpolatedEnemySlot::MaxBufferSize)
					slot.StateBuffer.erase(slot.StateBuffer.begin());
				slot.LastUpdateTime = snap.Timestamp;
				slot.HasValidState = true;
			}
		}
		else if (lastEnemyBulkCount == 0)
		{
			World->ApplyEnemyUpdates(std::vector<NeonGame::TEnemyUpdatePacket>());
			InterpolatedEnemies.clear();
			InterpEnemyLastNetInstanceId.clear();
		}

		if (lastOrbBulkCount >= 0)
			World->ApplyExperienceOrbSnapshotClient(orbUpdates);

		if (World->GetPlayerCount() > 0 && InterpolatedPlayers.size() < World->GetPlayerCount())
		{
			InterpolatedPlayers.resize(World->GetPlayerCount());
		}

		TGamePlayer* localPlayer = World->GetLocalPlayer();

		for (const auto &update : latestPlayerUpdates)
		{
			if (update.PlayerID >= InterpolatedPlayers.size())
				continue;

			if (update.PlayerID == World->GetLocalPlayerID() && localPlayer)
			{
				const int prevHealth = localPlayer->GetHealth();
				localPlayer->SyncHealthFromNetwork(update.Health, update.MaxHealth);
				if (localPlayer->IsAlive() && update.Health < prevHealth)
				{
					const int dmg = prevHealth - update.Health;
					const float intens = (std::min)(10.0f, 4.0f + static_cast<float>(dmg) * 0.14f);
					const float dur = (std::min)(0.26f, 0.11f + static_cast<float>(dmg) * 0.009f);
					World->AddCameraShake(intens, dur);
				}
				localPlayer->ApplyAuthorityProgress(update.Level, update.Experience);
				localPlayer->ApplySpeedMultiplier(update.SpeedMultiplier);

				const TPointF serverPos(update.PositionX, update.PositionY);
				const TPointF clientPos = localPlayer->GetPosition();
				const float dx = serverPos.X - clientPos.X;
				const float dy = serverPos.Y - clientPos.Y;
				const float distance = std::sqrt(dx * dx + dy * dy);

				const float maxDeviation = 560.0f;

				if (distance > maxDeviation)
				{
					static float lastHardCorrectionTime = -1000.0f;
					const float hardCorrectionCooldown = 0.75f;
					if ((currentTime - lastHardCorrectionTime) < hardCorrectionCooldown)
					{
						continue;
					}
					lastHardCorrectionTime = currentTime;

					TPointF sdir(update.FacingDirectionX, update.FacingDirectionY);
					const float sdl = std::sqrt(sdir.X * sdir.X + sdir.Y * sdir.Y);
					if (sdl > 0.001f)
					{
						sdir.X /= sdl;
						sdir.Y /= sdl;
					}
					else
					{
						sdir = TPointF(0.0f, -1.0f);
					}

					localPlayer->SetPosition(serverPos.X, serverPos.Y);
					localPlayer->SetFacingDirection(sdir);
				}
				else if (distance > 16.0f)
				{
					static float lastSoftCorrectionTime = -1000.0f;
					const float softCorrectionMinInterval = 0.045f;
					if ((currentTime - lastSoftCorrectionTime) < softCorrectionMinInterval && distance < 72.0f)
					{
						continue;
					}
					lastSoftCorrectionTime = currentTime;

					TPointF sdir(update.FacingDirectionX, update.FacingDirectionY);
					const float sdl = std::sqrt(sdir.X * sdir.X + sdir.Y * sdir.Y);
					if (sdl > 0.001f)
					{
						sdir.X /= sdl;
						sdir.Y /= sdl;
					}
					else
					{
						sdir = TPointF(0.0f, -1.0f);
					}

					const float k = (std::min)(0.085f, 0.012f + distance * 0.0035f);
					localPlayer->SetPosition(clientPos.X + dx * k, clientPos.Y + dy * k);

					TPointF cdir = localPlayer->GetFacingDirection();
					float fx = cdir.X + (sdir.X - cdir.X) * k;
					float fy = cdir.Y + (sdir.Y - cdir.Y) * k;
					const float fl = std::sqrt(fx * fx + fy * fy);
					if (fl > 0.001f)
					{
						fx /= fl;
						fy /= fl;
						localPlayer->SetFacingDirection(TPointF(fx, fy));
					}

				}

				continue;
			}

			TGamePlayer *remotePlayer = World->GetPlayer(update.PlayerID);
			if (remotePlayer)
				remotePlayer->SyncHealthFromNetwork(update.Health, update.MaxHealth);

			TInterpolatedPlayer &interp = InterpolatedPlayers[update.PlayerID];

			TPlayerStateSnapshot snapshot;
			snapshot.Position = TPointF(update.PositionX, update.PositionY);
			snapshot.FacingDirection = TPointF(update.FacingDirectionX, update.FacingDirectionY);
			snapshot.Health = update.Health;
			snapshot.MaxHealth = update.MaxHealth;
			snapshot.Tick = update.FrameNumber;
			{
				const bool hasTs = !interp.StateBuffer.empty();
				const float lastTs = hasTs ? interp.StateBuffer.back().Timestamp : 0.0f;
				snapshot.Timestamp = MonotonicSnapshotTime(currentTime, lastTs, hasTs);
			}

			interp.StateBuffer.push_back(snapshot);

			if (interp.StateBuffer.size() > TInterpolatedPlayer::MaxBufferSize)
			{
				interp.StateBuffer.erase(interp.StateBuffer.begin());
			}

			interp.LastUpdateTime = snapshot.Timestamp;
			interp.HasValidState = true;
		}

		World->EnsureValidSpectateTarget();
		World->TrySetNetworkClientGameOverIfAllDead();
	}
}

void TForm1::ApplyClientSidePrediction(float deltaTime)
{
	if (!IsNetworkGameActive || NetworkManager.IsHosting())
		return;

	TGamePlayer* localPlayer = World->GetLocalPlayer();
	if (!localPlayer)
		return;

	TPredictedState predicted;
	predicted.Position = localPlayer->GetPosition();
	predicted.FacingDirection = localPlayer->GetFacingDirection();
	predicted.FrameNumber = FrameNumber;

	PredictedStates.push_back(predicted);

	if (PredictedStates.size() > 60)
	{
		PredictedStates.erase(PredictedStates.begin());
	}
}

void TForm1::InterpolateRemotePlayers(float deltaTime)
{
	(void)deltaTime;
	if (!IsNetworkGameActive)
		return;

	if (InterpolatedPlayers.size() < World->GetPlayerCount())
	{
		InterpolatedPlayers.resize(World->GetPlayerCount());
	}

	for (uint8_t i = 0; i < World->GetPlayerCount(); i++)
	{
		if (i == World->GetLocalPlayerID())
			continue;

		TGamePlayer* player = World->GetPlayer(i);
		if (!player)
			continue;

		TInterpolatedPlayer &interp = InterpolatedPlayers[i];

		if (!interp.HasValidState || interp.StateBuffer.size() < 2)
		{
			if (interp.StateBuffer.size() == 1)
			{
				const auto &snapshot = interp.StateBuffer.back();
				player->SetPosition(snapshot.Position.X, snapshot.Position.Y);
				player->SetFacingDirection(snapshot.FacingDirection);
			}
			continue;
		}

		float interpolationDelay = 0.085f;
		{
			const size_t sz = interp.StateBuffer.size();
			const float lastPairDt = interp.StateBuffer[sz - 1].Timestamp - interp.StateBuffer[sz - 2].Timestamp;
			const float span = interp.StateBuffer.back().Timestamp - interp.StateBuffer.front().Timestamp;
			float estDt = std::max(lastPairDt, span / static_cast<float>((sz > 1) ? (sz - 1) : 1));
			const uint32_t tickNew = interp.StateBuffer[sz - 1].Tick;
			const uint32_t tickOld = interp.StateBuffer[sz - 2].Tick;
			uint32_t tickDelta = (tickNew > tickOld) ? (tickNew - tickOld) : 1u;
			if (tickDelta == 0)
				tickDelta = 1u;
			const float tickBasedDt = static_cast<float>(tickDelta) * TForm1::NetworkUpdateInterval;
			estDt = std::max(estDt, tickBasedDt * 0.88f);

			if (!NetworkManager.IsHosting())
				interpolationDelay = std::min(0.098f, std::max(0.036f, estDt * 1.16f));
			else
				interpolationDelay = std::min(0.11f, std::max(0.045f, estDt * 1.35f));
		}

		const float nowClock = NetInterpolationClockSeconds();
		float targetTime = nowClock - interpolationDelay;

		size_t olderIdx = 0;
		size_t newerIdx = 0;
		bool foundPair = false;

		for (size_t j = 0; j < interp.StateBuffer.size() - 1; j++)
		{
			if (interp.StateBuffer[j].Timestamp <= targetTime &&
				interp.StateBuffer[j + 1].Timestamp >= targetTime)
			{
				olderIdx = j;
				newerIdx = j + 1;
				foundPair = true;
				break;
			}
		}

		if (!foundPair)
		{
			if (interp.StateBuffer.size() >= 2)
			{
				olderIdx = interp.StateBuffer.size() - 2;
				newerIdx = interp.StateBuffer.size() - 1;
				foundPair = true;
			}
		}

		if (foundPair)
		{
			const auto &olderState = interp.StateBuffer[olderIdx];
			const auto &newerState = interp.StateBuffer[newerIdx];

			float t = 0.0f;
			if (newerState.Timestamp > olderState.Timestamp)
			{
				t = (targetTime - olderState.Timestamp) / (newerState.Timestamp - olderState.Timestamp);
				t = std::max(0.0f, std::min(1.0f, t));
			}

			TPointF interpolatedPos(
				olderState.Position.X + (newerState.Position.X - olderState.Position.X) * t,
				olderState.Position.Y + (newerState.Position.Y - olderState.Position.Y) * t
			);

			const float a0 = std::atan2(olderState.FacingDirection.Y, olderState.FacingDirection.X);
			const float a1 = std::atan2(newerState.FacingDirection.Y, newerState.FacingDirection.X);
			const float aInterp = a0 + t * ShortestAngleDeltaRad(a0, a1);
			TPointF interpolatedDir(std::cos(aInterp), std::sin(aInterp));

			player->SetPosition(interpolatedPos.X, interpolatedPos.Y);
			player->SetFacingDirection(interpolatedDir);
		}
		else
		{
			const auto &snapshot = interp.StateBuffer.back();
			player->SetPosition(snapshot.Position.X, snapshot.Position.Y);
			player->SetFacingDirection(snapshot.FacingDirection);
		}

		const float pruneClock = NetInterpolationClockSeconds();
		interp.StateBuffer.erase(
			std::remove_if(interp.StateBuffer.begin(), interp.StateBuffer.end(),
				[pruneClock](const TPlayerStateSnapshot &s) {
					return (pruneClock - s.Timestamp) > 0.85f;
				}),
			interp.StateBuffer.end()
		);
	}
}

void TForm1::RestoreHostRemoteAuthoritativePositions()
{
	if (!IsNetworkGameActive || !NetworkManager.IsHosting() || !World || !HostRemoteAuthValid)
		return;

	const uint8_t n = World->GetPlayerCount();
	const uint8_t localId = World->GetLocalPlayerID();

	for (uint8_t i = 0; i < n; ++i)
	{
		if (i == localId)
			continue;
		if (i >= HostRemoteAuthPos.size() || i >= HostRemoteAuthFacing.size())
			continue;
		TGamePlayer *p = World->GetPlayer(i);
		if (!p)
			continue;
		const TPointF &pos = HostRemoteAuthPos[i];
		const TPointF &face = HostRemoteAuthFacing[i];
		p->SetPosition(pos.X, pos.Y);
		p->SetFacingDirection(face);
	}
}

void TForm1::HostFeedRemoteSnapshotsAfterSim()
{
	if (!IsNetworkGameActive || !NetworkManager.IsHosting() || !World)
		return;

	const uint8_t n = World->GetPlayerCount();
	const uint8_t localId = World->GetLocalPlayerID();
	const float currentTime = NetInterpolationClockSeconds();

	if (InterpolatedPlayers.size() < n)
		InterpolatedPlayers.resize(n);
	HostRemoteAuthPos.resize(n);
	HostRemoteAuthFacing.resize(n);

	bool hadRemote = false;
	for (uint8_t i = 0; i < n; ++i)
	{
		if (i == localId)
			continue;
		TGamePlayer *p = World->GetPlayer(i);
		if (!p)
			continue;
		hadRemote = true;

		const TPointF pos = p->GetPosition();
		TPointF face = p->GetFacingDirection();
		const float fl = std::sqrt(face.X * face.X + face.Y * face.Y);
		if (fl > 0.001f)
		{
			face.X /= fl;
			face.Y /= fl;
		}
		else
		{
			face = TPointF(0.0f, -1.0f);
		}

		HostRemoteAuthPos[i] = pos;
		HostRemoteAuthFacing[i] = face;

		TInterpolatedPlayer &interp = InterpolatedPlayers[i];
		TPlayerStateSnapshot snap;
		snap.Position = pos;
		snap.FacingDirection = face;
		snap.Health = static_cast<float>(p->GetHealth());
		snap.MaxHealth = static_cast<float>(p->GetMaxHealth());
		snap.Tick = FrameNumber;
		{
			const bool hasTs = !interp.StateBuffer.empty();
			const float lastTs = hasTs ? interp.StateBuffer.back().Timestamp : 0.0f;
			snap.Timestamp = MonotonicSnapshotTime(currentTime, lastTs, hasTs);
		}
		interp.StateBuffer.push_back(snap);
		if (interp.StateBuffer.size() > TInterpolatedPlayer::MaxBufferSize)
			interp.StateBuffer.erase(interp.StateBuffer.begin());
		interp.LastUpdateTime = snap.Timestamp;
		interp.HasValidState = true;
	}

	if (hadRemote)
		HostRemoteAuthValid = true;
}

void TForm1::InterpolateNetworkEnemies(float deltaTime)
{
	(void)deltaTime;

	if (!IsNetworkGameActive || NetworkManager.IsHosting())
		return;

	const size_t slotCount = World->GetEnemySlotCount();
	if (InterpolatedEnemies.size() != slotCount)
		InterpolatedEnemies.resize(slotCount);

	for (size_t i = 0; i < slotCount; ++i)
	{
		TEnemy *enemy = World->GetEnemyBySlot(i);
		if (!enemy)
		{
			if (i < InterpolatedEnemies.size())
			{
				InterpolatedEnemies[i].StateBuffer.clear();
				InterpolatedEnemies[i].HasValidState = false;
			}
			continue;
		}

		if (i >= InterpolatedEnemies.size())
			continue;

		TInterpolatedEnemySlot &interp = InterpolatedEnemies[i];

		if (!interp.HasValidState || interp.StateBuffer.empty())
			continue;

		if (interp.StateBuffer.size() < 2)
		{
			const auto &snapshot = interp.StateBuffer.back();
			enemy->SetPosition(snapshot.Position);
			continue;
		}

		float enemyInterpolationDelay = 0.085f;
		{
			const size_t esz = interp.StateBuffer.size();
			const float lastPairDt = interp.StateBuffer[esz - 1].Timestamp - interp.StateBuffer[esz - 2].Timestamp;
			const float span = interp.StateBuffer.back().Timestamp - interp.StateBuffer.front().Timestamp;
			const float estDt = std::max(lastPairDt, span / static_cast<float>((esz > 1) ? (esz - 1) : 1));
			enemyInterpolationDelay = std::min(0.11f, std::max(0.045f, estDt * 1.35f));
		}

		const float nowEnemyClock = NetInterpolationClockSeconds();
		float targetTime = nowEnemyClock - enemyInterpolationDelay;

		size_t olderIdx = 0;
		size_t newerIdx = 0;
		bool foundPair = false;

		for (size_t j = 0; j < interp.StateBuffer.size() - 1; j++)
		{
			if (interp.StateBuffer[j].Timestamp <= targetTime &&
				interp.StateBuffer[j + 1].Timestamp >= targetTime)
			{
				olderIdx = j;
				newerIdx = j + 1;
				foundPair = true;
				break;
			}
		}

		if (!foundPair)
		{
			if (interp.StateBuffer.size() >= 2)
			{
				olderIdx = interp.StateBuffer.size() - 2;
				newerIdx = interp.StateBuffer.size() - 1;
				foundPair = true;
			}
		}

		if (foundPair)
		{
			const auto &olderState = interp.StateBuffer[olderIdx];
			const auto &newerState = interp.StateBuffer[newerIdx];

			float t = 0.0f;
			if (newerState.Timestamp > olderState.Timestamp)
			{
				t = (targetTime - olderState.Timestamp) / (newerState.Timestamp - olderState.Timestamp);
				t = std::max(0.0f, std::min(1.0f, t));
			}
			const float st = SmoothInterpT(t);

			TPointF interpolatedPos(
				olderState.Position.X + (newerState.Position.X - olderState.Position.X) * st,
				olderState.Position.Y + (newerState.Position.Y - olderState.Position.Y) * st
			);
			enemy->SetPosition(interpolatedPos);
		}
		else
		{
			enemy->SetPosition(interp.StateBuffer.back().Position);
		}

		const float pruneEnemyClock = NetInterpolationClockSeconds();
		interp.StateBuffer.erase(
			std::remove_if(interp.StateBuffer.begin(), interp.StateBuffer.end(),
				[pruneEnemyClock](const TEnemyNetSnapshot &s) {
					return (pruneEnemyClock - s.Timestamp) > 0.85f;
				}),
			interp.StateBuffer.end()
		);
	}
}