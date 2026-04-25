#include <vcl.h>
#pragma hdrstop

#include "GameWorld.h"
#include "GameWorld_Utils.h"
#include "GameConstants.h"
#include <algorithm>
#include <cmath>

using namespace NeonGame;

TGameWorld::TGameStateSnapshot TGameWorld::GetGameStateSnapshot() const
{
	TGameStateSnapshot snapshot;
	snapshot.Tick = 0;
	snapshot.WaveNumber = WaveManager.GetCurrentWave();
	snapshot.EnemiesAlive = static_cast<uint32_t>(Enemies.size());

	for (size_t i = 0; i < Players.size(); i++)
	{
		if (Players[i])
		{
			TGameStateSnapshot::TPlayerState playerState;
			playerState.PlayerID = static_cast<uint8_t>(i);
			TPointF pos = Players[i]->GetPosition();
			playerState.PositionX = pos.X;
			playerState.PositionY = pos.Y;
			TPointF facing = Players[i]->GetFacingDirection();
			playerState.FacingDirectionX = facing.X;
			playerState.FacingDirectionY = facing.Y;
			playerState.Health = Players[i]->GetHealth();
			playerState.MaxHealth = Players[i]->GetMaxHealth();
			playerState.Level = Players[i]->GetLevel();
			playerState.Experience = Players[i]->GetExperience();
			playerState.IsAlive = Players[i]->IsAlive();
			snapshot.Players.push_back(playerState);
		}
	}

	for (const auto &enemy : Enemies)
	{
		TGameStateSnapshot::TEnemyState enemyState;
		if (enemy && enemy->IsAlive())
		{
			if (dynamic_cast<TBasicEnemy*>(enemy.get()))
				enemyState.Type = 0;
			else if (dynamic_cast<TFastEnemy*>(enemy.get()))
				enemyState.Type = 1;
			else if (dynamic_cast<TThrowerEnemy*>(enemy.get()))
				enemyState.Type = 2;
			else if (dynamic_cast<TZigzagEnemy*>(enemy.get()))
				enemyState.Type = 3;
			else if (dynamic_cast<TKamikazeEnemy*>(enemy.get()))
				enemyState.Type = 4;
			else if (dynamic_cast<TShootingEnemy*>(enemy.get()))
				enemyState.Type = 5;
			else
				enemyState.Type = 0;

			TPointF pos = enemy->GetPosition();
			enemyState.PositionX = pos.X;
			enemyState.PositionY = pos.Y;
			enemyState.Health = enemy->GetHealth();
			enemyState.IsAlive = true;
			enemyState.NetInstanceId = enemy->GetNetInstanceId();
		}
		else
		{
			enemyState.Type = 0;
			enemyState.PositionX = 0.0f;
			enemyState.PositionY = 0.0f;
			enemyState.Health = 0;
			enemyState.IsAlive = false;
			enemyState.NetInstanceId = 0;
		}
		snapshot.Enemies.push_back(enemyState);
	}

	if (Boss && Boss->IsAlive())
	{
		TPointF bossPos = Boss->GetPosition();
		snapshot.Boss.PositionX = bossPos.X;
		snapshot.Boss.PositionY = bossPos.Y;
		snapshot.Boss.Health = Boss->GetHealth();
		snapshot.Boss.MaxHealth = Boss->GetMaxHealth();
		snapshot.Boss.Phase = static_cast<uint8_t>(Boss->GetPhase());
		snapshot.Boss.IsAlive = true;
	}
	else
	{
		snapshot.Boss.IsAlive = false;
	}

	return snapshot;
}

void TGameWorld::ApplyBossUpdateClient(const NeonGame::TBossUpdatePacket &boss)
{
	if (!IsNetworkGame || IsServer)
		return;

	if (!boss.IsAlive)
	{
		Boss.reset();
		return;
	}

	if (!Boss)
	{
		Boss = std::make_unique<TBossEnemy>(TPointF(boss.PositionX, boss.PositionY), 0);
	}

	Boss->SetPosition(TPointF(boss.PositionX, boss.PositionY));
	Boss->SetHealth(static_cast<int>(boss.Health));
	Boss->SetPhaseFromNetwork(boss.Phase);
}

void TGameWorld::ApplyBulletSnapshotClient(const std::vector<NeonGame::TBulletNetPacket> &bullets)
{
	if (!IsNetworkGame || IsServer)
		return;
	ReplicatedBullets = bullets;
}

std::vector<NeonGame::TBulletNetPacket> TGameWorld::GetBulletNetSnapshot() const
{
	std::vector<NeonGame::TBulletNetPacket> out;
	out.reserve(Bullets.size() + EnemyBullets.size());
	for (const auto &b : Bullets)
	{
		if (!b.IsAlive())
			continue;
		const TPointF p = b.GetPosition();
		NeonGame::TBulletNetPacket nb;
		nb.PositionX = p.X;
		nb.PositionY = p.Y;
		nb.IsPlayerBullet = true;
		out.push_back(nb);
	}
	for (const auto &b : EnemyBullets)
	{
		if (!b.IsAlive())
			continue;
		const TPointF p = b.GetPosition();
		NeonGame::TBulletNetPacket nb;
		nb.PositionX = p.X;
		nb.PositionY = p.Y;
		nb.IsPlayerBullet = false;
		out.push_back(nb);
	}
	return out;
}

void TGameWorld::ApplyGameStateSnapshot(const TGameStateSnapshot &snapshot, float interpolationFactor)
{
	if (snapshot.WaveNumber != WaveManager.GetCurrentWave())
	{
	}

	for (const auto &playerState : snapshot.Players)
	{
		if (playerState.PlayerID >= Players.size())
			continue;

		if (!Players[playerState.PlayerID])
			Players[playerState.PlayerID] = std::make_unique<TGamePlayer>();

		TGamePlayer* player = Players[playerState.PlayerID].get();

		TPointF currentPos = player->GetPosition();
		TPointF targetPos(playerState.PositionX, playerState.PositionY);

		if (interpolationFactor > 0.0f && interpolationFactor < 1.0f)
		{
			TPointF interpolatedPos(
				currentPos.X + (targetPos.X - currentPos.X) * interpolationFactor,
				currentPos.Y + (targetPos.Y - currentPos.Y) * interpolationFactor
			);
			player->SetPosition(interpolatedPos.X, interpolatedPos.Y);
		}
		else
		{
			player->SetPosition(targetPos.X, targetPos.Y);
		}

		player->SetFacingDirection(TPointF(playerState.FacingDirectionX, playerState.FacingDirectionY));
	}

	if (snapshot.Boss.IsAlive)
	{
		if (!Boss)
		{
		}
	}
}

void TGameWorld::Update(float deltaTime, const std::vector<TInputState> &inputs, int canvasWidth, int canvasHeight)
{
	if (Players.empty())
		return;

	for (size_t i = 0; i < inputs.size() && i < Players.size(); i++)
	{
		if (!Players[i])
			continue;

		const TInputState &input = inputs[i];
		Players[i]->inputUp = input.MoveUp;
		Players[i]->inputDown = input.MoveDown;
		Players[i]->inputLeft = input.MoveLeft;
		Players[i]->inputRight = input.MoveRight;
		Players[i]->isShooting = input.PrimaryFire;

		if (i < PlayerUpgradeManagers.size())
		{
			Players[i]->ApplySpeedMultiplier(PlayerUpgradeManagers[i].GetSpeedMultiplier());
			Players[i]->ApplyMaxHealthBonus(PlayerUpgradeManagers[i].GetMaxHealthBonus());
		}

		Players[i]->Update(deltaTime, WorldBounds());

		UpdateShootingForPlayer(static_cast<uint8_t>(i), deltaTime, input, canvasWidth, canvasHeight);
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

	Stats.RunTimeSeconds += deltaTime;
	Stats.CurrentWave = WaveManager.GetCurrentWave();

	int enemiesAlive = static_cast<int>(Enemies.size());
	WaveManager.Update(deltaTime, enemiesAlive);

	if (WaveManager.GetState() == EWaveState::Completed)
	{
		const int completedWaveNumber = WaveManager.GetCurrentWave();
		const bool shouldProcessCompletion = (IsNetworkGame && completedWaveNumber != LastProcessedWaveCompletion);
		if (shouldProcessCompletion)
		{
			LastProcessedWaveCompletion = completedWaveNumber;
			if (!NeonGameWorldUtils::AreAllPlayersDead(Players))
			{
				const float cx = WorldWidth * 0.5f;
				const float cy = WorldHeight * 0.5f;
				for (auto &pl : Players)
				{
					if (pl && !pl->IsAlive())
						pl->NetworkRespawn(cx, cy, 20);
				}
			}
			for (size_t i = 0; i < Players.size(); i++)
			{
				if (Players[i] && Players[i]->IsAlive())
				{
					QueuePendingUpgradeChoice(static_cast<uint8_t>(i), 1);
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
	}

	Spawner.SetScreenSize(static_cast<float>(canvasWidth), static_cast<float>(canvasHeight));
	Spawner.SetWorldSize(WorldWidth, WorldHeight);

	EnsureValidSpectateTarget();
	const TPointF playerPos = GetCameraFollowPosition();
	Camera.Update(deltaTime, playerPos, canvasWidth, canvasHeight, WorldWidth, WorldHeight);

	for (auto &b : Bullets)
		b.Update(deltaTime);
	Bullets.erase(
		std::remove_if(Bullets.begin(), Bullets.end(),
			[](const TBullet &b) { return !b.IsAlive(); }),
		Bullets.end());

	for (auto &b : EnemyBullets)
		b.Update(deltaTime);
	EnemyBullets.erase(
		std::remove_if(EnemyBullets.begin(), EnemyBullets.end(),
			[](const TBullet &b) { return !b.IsAlive(); }),
		EnemyBullets.end());

	if (!IsNetworkGame || IsServer)
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
	}

	Stats.CurrentWave = WaveManager.GetCurrentWave();

	if (!IsNetworkGame || IsServer)
	{
		UpdateCollisions();
	}
	UpdateThrownProjectiles(deltaTime);
}

