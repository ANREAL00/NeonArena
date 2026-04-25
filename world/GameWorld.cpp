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


void TGameWorld::Update(float deltaTime, const TInputState &input, int canvasWidth, int canvasHeight)
{
	if (Players.empty())
		return;
	
	TGamePlayer* localPlayer = GetLocalPlayer();
	if (!localPlayer)
		return;

	localPlayer->inputUp = input.MoveUp;
	localPlayer->inputDown = input.MoveDown;
	localPlayer->inputLeft = input.MoveLeft;
	localPlayer->inputRight = input.MoveRight;
	localPlayer->isShooting = input.PrimaryFire;

	if (IsNetworkGame)
	{
		for (size_t i = 0; i < Players.size(); i++)
		{
			if (Players[i] && i < PlayerUpgradeManagers.size())
			{
				Players[i]->ApplySpeedMultiplier(PlayerUpgradeManagers[i].GetSpeedMultiplier());
				Players[i]->ApplyMaxHealthBonus(PlayerUpgradeManagers[i].GetMaxHealthBonus());
			}
		}
	}
	else
	{
		localPlayer->ApplySpeedMultiplier(UpgradeManager.GetSpeedMultiplier());
		localPlayer->ApplyMaxHealthBonus(UpgradeManager.GetMaxHealthBonus());
	}

	localPlayer->Update(deltaTime, WorldBounds());

	const bool isAuthoritativeSimulation = (!IsNetworkGame || IsServer);
	if (isAuthoritativeSimulation)
	{
		for (size_t i = 0; i < Players.size(); i++)
		{
			if (i != LocalPlayerID && Players[i])
			{
				Players[i]->Update(deltaTime, WorldBounds());
			}
		}
	}
	
	if (IsNetworkGame && WorldState != EWorldState::GameOver)
	{
		if (NeonGameWorldUtils::AreAllPlayersDead(Players))
		{
			WorldState = EWorldState::GameOver;
		}
	}

	if (WorldState == EWorldState::GameOver || (!IsNetworkGame && WorldState == EWorldState::ChoosingUpgrade))
		return;

	if (isAuthoritativeSimulation)
	{
		Stats.RunTimeSeconds += deltaTime;
		Stats.CurrentWave = WaveManager.GetCurrentWave();

		int enemiesAlive = static_cast<int>(Enemies.size());
		WaveManager.Update(deltaTime, enemiesAlive);

		if (WaveManager.GetState() == EWaveState::Completed)
		{
			const int completedWaveNumber = WaveManager.GetCurrentWave();
			if (IsNetworkGame)
			{
				const bool shouldProcessCompletion = (completedWaveNumber != LastProcessedWaveCompletion);
				if (shouldProcessCompletion)
				{
					LastProcessedWaveCompletion = completedWaveNumber;
					for (size_t i = 0; i < Players.size(); i++)
					{
						if (Players[i] && Players[i]->IsAlive())
						{
							QueuePendingUpgradeChoice(static_cast<uint8_t>(i), 1);
						}
					}
				}
			}

			if (IsNetworkGame)
			{
				for (size_t i = 0; i < Players.size(); i++)
				{
					EnsureUpgradeChoicePresented(static_cast<uint8_t>(i));
				}
			}
			else
			{
				if (!WaitingForUpgradeChoice)
				{
					const std::vector<EUpgradeType> excludeTypes = NeonGameWorldUtils::BuildExcludedUpgradeTypes(UpgradeManager);
					AvailableUpgrades = TUpgradeManager::GenerateRandomUpgrades(3, excludeTypes);
					WaitingForUpgradeChoice = true;
					WorldState = EWorldState::ChoosingUpgrade;
				}
			}
		}
	}

	Spawner.SetScreenSize(static_cast<float>(canvasWidth), static_cast<float>(canvasHeight));
	Spawner.SetWorldSize(WorldWidth, WorldHeight);
	
	EnsureValidSpectateTarget();
	const TPointF playerPos = GetCameraFollowPosition();
	Camera.Update(deltaTime, playerPos, canvasWidth, canvasHeight, WorldWidth, WorldHeight);

	if (isAuthoritativeSimulation)
	{
		UpdateShooting(deltaTime, input, canvasWidth, canvasHeight);
	}
	else if (IsNetworkGame && !IsServer)
	{
		UpdateShooting(deltaTime, input, canvasWidth, canvasHeight);
	}

	if (isAuthoritativeSimulation)
	{
		for (auto &b : Bullets)
			b.Update(deltaTime);
		NeonGameWorldUtils::EraseIf(Bullets, [](const TBullet &b) { return !b.IsAlive(); });
		
		for (auto &b : EnemyBullets)
			b.Update(deltaTime);
		NeonGameWorldUtils::EraseIf(EnemyBullets, [](const TBullet &b) { return !b.IsAlive(); });
	}
	else if (IsNetworkGame && !IsServer)
	{
		for (auto &b : Bullets)
			b.Update(deltaTime);
		for (auto &b : Bullets)
		{
			if (!b.IsAlive() || b.IsUsed())
				continue;
			for (const auto &enemyPtr : Enemies)
			{
				if (!enemyPtr || !enemyPtr->IsAlive())
					continue;
				if (GameCollision::CircleCircleCollision(b.GetPosition(), b.GetRadius(),
						enemyPtr->GetPosition(), NeonGame::EnemyRadius))
				{
					b.MarkAsUsed();
					break;
				}
			}
		}
		NeonGameWorldUtils::EraseIf(Bullets, [](const TBullet &b) { return !b.IsAlive(); });
	}

	if (isAuthoritativeSimulation)
	{
		const int currentWave = WaveManager.GetCurrentWave();
		if (Spawner.ShouldSpawnBoss(currentWave) && (!Boss || !Boss->IsAlive()))
		{
			SpawnBoss();
			Spawner.OnBossSpawned(currentWave);
		}
		
		if (!Boss || !Boss->IsAlive())
		{
			if (WaveManager.ShouldSpawnEnemy())
			{
				SpawnEnemy();
				WaveManager.OnEnemySpawned();
			}
		}

		UpdateEnemies(deltaTime);
		
		if (Boss && Boss->IsAlive())
		{
			UpdateBoss(deltaTime);
		}
		
		for (auto &proj : ThrownProjectiles)
		{
			proj.Update(deltaTime);
		}
		
		UpdateAcidPools(deltaTime);

		UpdateExperienceOrbs(deltaTime);

		if (Spawner.ShouldSpawnGroundExp(deltaTime))
		{
			const TPointF expPos = Spawner.SpawnGroundExpPosition();
			SpawnExperienceOrb(expPos, 5);
		}

		Stats.CurrentWave = WaveManager.GetCurrentWave();
		
		UpdateCollisions();
		
		UpdateThrownProjectiles(deltaTime);
	}
}

void TGameWorld::UpdateThrownProjectiles(float deltaTime)
{
	for (auto &proj : ThrownProjectiles)
	{
		if (proj.HasLanded() && !proj.WasPoolCreated())
		{
			const TPointF landingPos = proj.GetLandingPosition();
			const float minDist = PlayerRadius + 30.0f;
			const float thresholdSq = minDist * minDist;

			float closestSq = 1.0e30f;
			if (IsNetworkGame && IsServer)
			{
				for (const auto &p : Players)
				{
					if (!p || !p->IsAlive())
						continue;
					const TPointF pp = p->GetPosition();
					const float dx = landingPos.X - pp.X;
					const float dy = landingPos.Y - pp.Y;
					closestSq = (std::min)(closestSq, dx * dx + dy * dy);
				}
			}
			else
			{
				const TGamePlayer *lp = GetLocalPlayer();
				if (lp && lp->IsAlive())
				{
					const TPointF pp = lp->GetPosition();
					const float dx = landingPos.X - pp.X;
					const float dy = landingPos.Y - pp.Y;
					closestSq = dx * dx + dy * dy;
				}
			}

			if (closestSq > thresholdSq)
			{
				AcidPools.emplace_back(landingPos, 40.0f, 5.0f, 10);
				proj.MarkPoolCreated();
			}
			else
			{
				proj.MarkPoolCreated();
			}
		}
	}
	
	ThrownProjectiles.erase(
		std::remove_if(ThrownProjectiles.begin(), ThrownProjectiles.end(),
			[](const TThrownProjectile &p) { return !p.IsAlive() || p.HasLanded(); }),
		ThrownProjectiles.end());
}

void TGameWorld::UpdateAcidPools(float deltaTime)
{
	
	for (auto &pool : AcidPools)
	{
		pool.Update(deltaTime);
	}
	
	
	AcidPools.erase(
		std::remove_if(AcidPools.begin(), AcidPools.end(),
			[](const TAcidPool &p) { return !p.IsAlive(); }),
		AcidPools.end());
}