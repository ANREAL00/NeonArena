#include <vcl.h>
#pragma hdrstop

#include "GameWorld.h"
#include "GameWorld_Utils.h"
#include <System.SysUtils.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>

using namespace NeonGame;

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
