#include <vcl.h>
#pragma hdrstop

#include "GameWorld.h"
#include <algorithm>

using namespace NeonGame;

void TGameWorld::SpawnEnemy()
{
	TGamePlayer* localPlayer = GetLocalPlayer();
	if (!localPlayer)
		return;

	const TPointF playerPos = localPlayer->GetPosition();
	const float spawnOffset = 100.0f;

	const int side = Random(4);
	TPointF pos;

	switch (side)
	{
		case 0:
			pos = PointF(
				playerPos.X - ScreenWidth / 2.0f - spawnOffset,
				playerPos.Y + (Random(static_cast<int>(ScreenHeight)) - ScreenHeight / 2.0f));
			break;
		case 1:
			pos = PointF(
				playerPos.X + ScreenWidth / 2.0f + spawnOffset,
				playerPos.Y + (Random(static_cast<int>(ScreenHeight)) - ScreenHeight / 2.0f));
			break;
		case 2:
			pos = PointF(
				playerPos.X + (Random(static_cast<int>(ScreenWidth)) - ScreenWidth / 2.0f),
				playerPos.Y - ScreenHeight / 2.0f - spawnOffset);
			break;
		default:
			pos = PointF(
				playerPos.X + (Random(static_cast<int>(ScreenWidth)) - ScreenWidth / 2.0f),
				playerPos.Y + ScreenHeight / 2.0f + spawnOffset);
			break;
	}

	pos.X = std::clamp(pos.X, 50.0f, WorldWidth - 50.0f);
	pos.Y = std::clamp(pos.Y, 50.0f, WorldHeight - 50.0f);

	const int wave = WaveManager.GetCurrentWave();
	int enemyType = 0;

	if (wave < 3)
	{
		enemyType = 0;
	}
	else if (wave < 6)
	{
		enemyType = Random(2);
	}
	else if (wave < 8)
	{
		const int rand = Random(4);
		if (rand == 0)
			enemyType = 0;
		else if (rand == 1)
			enemyType = 1;
		else if (rand == 2)
			enemyType = 5;
		else
			enemyType = 4;
	}
	else if (wave < 11)
	{
		const int rand = Random(5);
		if (rand == 0)
			enemyType = 0;
		else if (rand == 1)
			enemyType = 1;
		else if (rand == 2)
			enemyType = 3;
		else if (rand == 3)
			enemyType = 4;
		else
			enemyType = 5;
	}
	else if (wave < 13)
	{
		const int rand = Random(5);
		if (rand == 0)
			enemyType = 0;
		else if (rand == 1)
			enemyType = 1;
		else if (rand == 2)
			enemyType = 2;
		else if (rand == 3)
			enemyType = 3;
		else
			enemyType = 5;
	}
	else
	{
		const int rand = Random(6);
		if (rand == 0)
			enemyType = 0;
		else if (rand == 1)
			enemyType = 1;
		else if (rand == 2)
			enemyType = 2;
		else if (rand == 3)
			enemyType = 3;
		else if (rand == 4)
			enemyType = 4;
		else
			enemyType = 5;
	}

	std::unique_ptr<TEnemy> newEnemy;
	switch (enemyType)
	{
		case 0:
			newEnemy = std::make_unique<TBasicEnemy>(pos);
			break;
		case 1:
			newEnemy = std::make_unique<TFastEnemy>(pos);
			break;
		case 2:
			newEnemy = std::make_unique<TThrowerEnemy>(pos);
			break;
		case 3:
			newEnemy = std::make_unique<TZigzagEnemy>(pos);
			break;
		case 4:
			newEnemy = std::make_unique<TKamikazeEnemy>(pos);
			break;
		case 5:
			newEnemy = std::make_unique<TShootingEnemy>(pos);
			break;
		default:
			newEnemy = std::make_unique<TBasicEnemy>(pos);
			break;
	}

	if (wave > 12)
	{
		const int scalingLevel = (wave - 12) / 4;
		if (scalingLevel > 0)
		{
			const float healthMultiplier = 1.0f + (scalingLevel * 0.25f);
			const float speedMultiplier = 1.0f + (scalingLevel * 0.15f);
			newEnemy->ApplyScaling(healthMultiplier, speedMultiplier);
		}
	}

	newEnemy->SetNetInstanceId(NextEnemyNetInstanceId++);
	Enemies.push_back(std::move(newEnemy));
}

void TGameWorld::SpawnBoss()
{
	const TPointF bossPos = Spawner.SpawnBossPosition();
	const int appearanceLevel = Spawner.GetBossAppearanceCount();
	Boss = std::make_unique<TBossEnemy>(bossPos, appearanceLevel);
}

void TGameWorld::UpdateBoss(float deltaTime)
{
	TGamePlayer* targetPlayer = GetLocalPlayer();

	if (IsNetworkGame && IsServer)
	{
		TGamePlayer* bestPlayer = nullptr;
		size_t bestPlayerIdx = 0;
		float bestDistSq = 1.0e30f;
		const TPointF bossPos = Boss ? Boss->GetPosition() : TPointF(WorldWidth * 0.5f, WorldHeight * 0.5f);
		for (size_t i = 0; i < Players.size(); ++i)
		{
			TGamePlayer* p = Players[i].get();
			if (!p || !p->IsAlive())
				continue;
			const TPointF pp = p->GetPosition();
			const float dx = pp.X - bossPos.X;
			const float dy = pp.Y - bossPos.Y;
			const float d2 = dx * dx + dy * dy;
			if (!bestPlayer || d2 < bestDistSq - 1.0e-3f || (std::fabs(d2 - bestDistSq) <= 1.0e-3f && i < bestPlayerIdx))
			{
				bestDistSq = d2;
				bestPlayer = p;
				bestPlayerIdx = i;
			}
		}
		if (bestPlayer)
			targetPlayer = bestPlayer;
	}
	else if (IsNetworkGame && !IsServer)
	{
		TGamePlayer* hostPlayer = GetPlayer(0);
		if (hostPlayer)
			targetPlayer = hostPlayer;
	}
	if (!Boss || !Boss->IsAlive() || !targetPlayer)
		return;

	const TPointF playerPos = targetPlayer->GetPosition();

	const float speedMultiplier = WaveManager.GetSpeedMultiplier();
	Boss->ApplyTemporarySpeedMultiplier(speedMultiplier);

	Boss->Update(deltaTime, playerPos);

	const float damageMultiplier = WaveManager.GetDamageMultiplier();
	Boss->CreateBullets(playerPos, EnemyBullets, damageMultiplier);

	if (!Boss->IsAlive())
	{
		const float lifestealChance = UpgradeManager.GetLifestealChancePercent() / 100.0f;
		if (lifestealChance > 0.0f && targetPlayer && targetPlayer->IsAlive())
		{
			if ((static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) < lifestealChance)
			{
				targetPlayer->Heal(5);
			}
		}

		const int expValue = 100;
		SpawnExperienceOrb(Boss->GetPosition(), expValue);
		Stats.EnemiesDefeated++;
	}
}
