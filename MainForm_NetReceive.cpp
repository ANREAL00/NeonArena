#include <vcl.h>
#pragma hdrstop

#include "MainForm.h"
#include <algorithm>
#include <cmath>

namespace
{
float MonotonicSnapshotTime(float recvWallSeconds, float lastBufferedTs, bool hasLast)
{
	if (!hasLast || recvWallSeconds > lastBufferedTs)
		return recvWallSeconds;
	return lastBufferedTs + 1.2e-4f;
}
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
