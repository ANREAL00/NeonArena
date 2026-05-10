#include <vcl.h>
#pragma hdrstop

#include "GameWorld.h"
#include "GameWorld_Utils.h"
#include <System.SysUtils.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>

using namespace NeonGame;

void TGameWorld::ApplyWaveUpdate(uint32_t waveNumber, uint8_t waveState, float cooldownRemaining,
	float runTimeSeconds, bool includesRunTime)
{
	if (!IsNetworkGame || IsServer)
		return;
	WaveManager.ApplyNetworkState(static_cast<int>(waveNumber), waveState, cooldownRemaining);
	Stats.CurrentWave = static_cast<int>(waveNumber);
	if (includesRunTime)
		Stats.RunTimeSeconds = runTimeSeconds;
}

void TGameWorld::ApplyUpgradeChoices(uint8_t playerID, bool isWaiting, const std::vector<NeonGame::TUpgradeChoiceNet> &choices)
{
	if (!IsNetworkGame || IsServer)
		return;
	if (playerID >= Players.size())
		return;
	if (playerID >= PlayerAvailableUpgrades.size() || playerID >= PlayerWaitingForUpgradeChoice.size())
		return;

	PlayerAvailableUpgrades[playerID].clear();
	if (isWaiting)
	{
		for (const auto &c : choices)
		{
			const EUpgradeType t = static_cast<EUpgradeType>(c.Type);
			const EUpgradeRarity r = static_cast<EUpgradeRarity>(c.Rarity);
			PlayerAvailableUpgrades[playerID].push_back(TUpgradeManager::CreateUpgrade(t, r));
		}
	}
	PlayerWaitingForUpgradeChoice[playerID] = isWaiting;
}

TGameWorld::TGameWorld()
	: PrimaryFireCooldown(NeonGame::PrimaryFireCooldown),
	  AltFireCooldown(NeonGame::AltFireCooldown),
	  PrimaryFireTimer(0.0f),
	  AltFireTimer(0.0f),
	  HasMousePos(false),
	  AimDirection(PointF(0.0f, -1.0f)),
	Stats{1, 0, 0.0f},
	WorldState(EWorldState::Playing),
	WaveManager(),
	LevelUpNotificationTimer(0.0f),
	LastPlayerLevel(1),
	LocalPlayerID(0),
	IsNetworkGame(false),
	IsServer(false)
{
	Reset();
}

void TGameWorld::Reset()
{

	if (!IsNetworkGame)
	{
		Players.clear();
		Players.push_back(std::make_unique<TGamePlayer>());
		LocalPlayerID = 0;
		const float startX = WorldWidth / 2.0f;
		const float startY = WorldHeight / 2.0f;
		Players[0]->SetPosition(startX, startY);
	}
	else
	{

		const float startX = WorldWidth / 2.0f;
		const float startY = WorldHeight / 2.0f;
		for (auto &player : Players)
		{
			if (player)
			{
				player->SetPosition(startX, startY);
			}
		}
	}

	Bullets.clear();
	EnemyBullets.clear();
	ThrownProjectiles.clear();
	AcidPools.clear();
	Enemies.clear();
	Boss.reset();
	ExperienceOrbs.clear();

	PrimaryFireTimer = 0.0f;
	AltFireTimer = 0.0f;
	PlayerPrimaryFireTimers.assign(Players.size(), 0.0f);
	PlayerAltFireTimers.assign(Players.size(), 0.0f);
	HasMousePos = false;
	AimDirection = PointF(0.0f, -1.0f);

	Stats.CurrentWave = 1;
	Stats.EnemiesDefeated = 0;
	Stats.RunTimeSeconds = 0.0f;

	WaveManager.Reset();
	WaveManager.StartNextWave();

	UpgradeManager.Reset();
	Spawner.Reset();
	AvailableUpgrades.clear();
	WaitingForUpgradeChoice = false;

	PlayerUpgradeManagers.clear();
	PlayerAvailableUpgrades.clear();
	PlayerWaitingForUpgradeChoice.clear();
	PlayerPendingUpgradeChoices.clear();
	if (IsNetworkGame)
	{
		PlayerUpgradeManagers.resize(Players.size());
		PlayerAvailableUpgrades.resize(Players.size());
		PlayerWaitingForUpgradeChoice.resize(Players.size(), false);
		PlayerPendingUpgradeChoices.resize(Players.size(), 0);
		for (auto &upgradeMgr : PlayerUpgradeManagers)
		{
			upgradeMgr.Reset();
		}
	}

	GroundExpSpawnTimer = 0.0f;
	LevelUpNotificationTimer = 0.0f;
	LastPlayerLevel = 1;

	ScreenWidth = 1920.0f;
	ScreenHeight = 1080.0f;
	LastBossWave = 0;
	BossAppearanceCount = 0;
	LastRegenWave = 0;
	NextEnemyNetInstanceId = 1;
	NextExpOrbNetInstanceId = 1;
	SpectateTargetPlayerID = LocalPlayerID;

	WorldState = EWorldState::Playing;
	LastProcessedWaveCompletion = 0;
	WaveManager.SetIntermissionSeconds(IsNetworkGame ? 20.0f : 5.0f);
}

void TGameWorld::InitializeNetworkGame(uint8_t localPlayerID, bool isServer)
{
	IsNetworkGame = true;
	LocalPlayerID = localPlayerID;
	IsServer = isServer;
	SpectateTargetPlayerID = localPlayerID;
	WaveManager.SetIntermissionSeconds(20.0f);
}

void TGameWorld::LeaveNetworkMatchAndReset()
{
	IsNetworkGame = false;
	IsServer = false;
	LocalPlayerID = 0;
	SpectateTargetPlayerID = 0;
	Players.clear();
	Reset();
}

void TGameWorld::TrySetNetworkClientGameOverIfAllDead()
{
	if (!IsNetworkGame || IsServer)
		return;
	if (WorldState == EWorldState::GameOver)
		return;
	if (NeonGameWorldUtils::AreAllPlayersDead(Players))
		WorldState = EWorldState::GameOver;
}

void TGameWorld::SetPlayerCount(uint8_t count)
{
	if (count < 1 || count > 4)
		return;

	Players.clear();
	const float startX = WorldWidth / 2.0f;
	const float startY = WorldHeight / 2.0f;

	PlayerPrimaryFireTimers.assign(count, 0.0f);
	PlayerAltFireTimers.assign(count, 0.0f);

	for (uint8_t i = 0; i < count; i++)
	{
		Players.push_back(std::make_unique<TGamePlayer>());
		float offsetX = (i % 2) * 50.0f - 25.0f;
		float offsetY = (i / 2) * 50.0f - 25.0f;
		Players[i]->SetPosition(startX + offsetX, startY + offsetY);
	}

	SpectateTargetPlayerID = LocalPlayerID;

	PlayerUpgradeManagers.resize(count);
	PlayerAvailableUpgrades.resize(count);
	PlayerWaitingForUpgradeChoice.resize(count, false);
	PlayerPendingUpgradeChoices.resize(count, 0);
	for (auto &upgradeMgr : PlayerUpgradeManagers)
	{
		upgradeMgr.Reset();
	}
}

void TGameWorld::QueuePendingUpgradeChoice(uint8_t playerID, int count)
{
	if (!IsNetworkGame)
		return;
	if (playerID >= PlayerPendingUpgradeChoices.size())
		return;
	if (count <= 0)
		return;
	PlayerPendingUpgradeChoices[playerID] += count;
	if (PlayerPendingUpgradeChoices[playerID] < 0)
		PlayerPendingUpgradeChoices[playerID] = 0;
}

bool TGameWorld::IsAnyPlayerPendingUpgradeChoices() const
{
	if (!IsNetworkGame)
		return false;
	const size_t n = std::min(PlayerPendingUpgradeChoices.size(), Players.size());
	for (size_t i = 0; i < n; ++i)
	{
		if (!Players[i] || !Players[i]->IsAlive())
			continue;
		if (PlayerPendingUpgradeChoices[i] > 0)
			return true;
	}
	return false;
}

void TGameWorld::EnsureUpgradeChoicePresented(uint8_t playerID)
{
	if (!IsNetworkGame)
		return;
	if (playerID >= Players.size() || playerID >= PlayerPendingUpgradeChoices.size())
		return;
	if (!Players[playerID] || !Players[playerID]->IsAlive())
		return;
	if (PlayerPendingUpgradeChoices[playerID] <= 0)
		return;
	if (playerID >= PlayerWaitingForUpgradeChoice.size() || playerID >= PlayerAvailableUpgrades.size() || playerID >= PlayerUpgradeManagers.size())
		return;
	if (PlayerWaitingForUpgradeChoice[playerID])
		return;

	const std::vector<EUpgradeType> excludeTypes = NeonGameWorldUtils::BuildExcludedUpgradeTypes(PlayerUpgradeManagers[playerID]);
	PlayerAvailableUpgrades[playerID] = TUpgradeManager::GenerateRandomUpgrades(3, excludeTypes);
	PlayerWaitingForUpgradeChoice[playerID] = true;
}

TGamePlayer* TGameWorld::GetPlayer(uint8_t playerID)
{
	if (playerID >= Players.size())
		return nullptr;
	return Players[playerID].get();
}

const TGamePlayer* TGameWorld::GetPlayer(uint8_t playerID) const
{
	if (playerID >= Players.size())
		return nullptr;
	return Players[playerID].get();
}

TGamePlayer* TGameWorld::GetLocalPlayer()
{
	return GetPlayer(LocalPlayerID);
}

const TGamePlayer* TGameWorld::GetLocalPlayer() const
{
	return GetPlayer(LocalPlayerID);
}

