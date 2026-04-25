#include <vcl.h>
#pragma hdrstop

#include "GameWorld.h"
#include "GameWorld_Utils.h"
#include <algorithm>
#include <cmath>

using namespace NeonGame;

void TGameWorld::UpdateEnemies(float deltaTime)
{
	const float speedMultiplier = WaveManager.GetSpeedMultiplier();
	const float damageMultiplier = WaveManager.GetDamageMultiplier();

	for (auto &e : Enemies)
	{
		if (!e || !e->IsAlive())
			continue;

		TGamePlayer* targetPlayer = GetLocalPlayer();
		TPointF playerPos = targetPlayer ? targetPlayer->GetPosition() : TPointF(WorldWidth / 2.0f, WorldHeight / 2.0f);

		
		
		if (IsNetworkGame && IsServer)
		{
			TGamePlayer* bestPlayer = nullptr;
			size_t bestPlayerIdx = 0;
			float bestDistSq = 1.0e30f;
			const TPointF enemyPos = e->GetPosition();
			for (size_t i = 0; i < Players.size(); ++i)
			{
				TGamePlayer* p = Players[i].get();
				if (!p || !p->IsAlive())
					continue;
				const TPointF pp = p->GetPosition();
				const float dx = pp.X - enemyPos.X;
				const float dy = pp.Y - enemyPos.Y;
				const float d2 = dx * dx + dy * dy;
				
				if (!bestPlayer || d2 < bestDistSq - 1.0e-3f || (std::fabs(d2 - bestDistSq) <= 1.0e-3f && i < bestPlayerIdx))
				{
					bestDistSq = d2;
					bestPlayer = p;
					bestPlayerIdx = i;
					playerPos = pp;
				}
			}
			targetPlayer = bestPlayer ? bestPlayer : targetPlayer;
		}

		e->ApplyTemporarySpeedMultiplier(speedMultiplier);

		e->Update(deltaTime, playerPos);

		TShootingEnemy *shootingEnemy = dynamic_cast<TShootingEnemy*>(e.get());
		if (shootingEnemy)
		{
			if (shootingEnemy->GetShootTimer() <= 0.0f)
			{
				const TPointF enemyPos = shootingEnemy->GetPosition();
				const float distanceToPlayer = std::sqrt(
					(playerPos.X - enemyPos.X) * (playerPos.X - enemyPos.X) +
					(playerPos.Y - enemyPos.Y) * (playerPos.Y - enemyPos.Y));

				const int wave = WaveManager.GetCurrentWave();
				int baseEnemyBulletDamage = 12;
				float baseEnemyBulletSpeed = 350.0f;

				if (wave > 8)
				{
					const int scalingLevel = (wave - 8) / 2;
					if (scalingLevel > 0)
					{
						baseEnemyBulletDamage = static_cast<int>(baseEnemyBulletDamage * (1.0f + scalingLevel * 0.25f));
						baseEnemyBulletSpeed *= (1.0f + scalingLevel * 0.20f);
					}
				}

				baseEnemyBulletDamage = static_cast<int>(baseEnemyBulletDamage * damageMultiplier);

				const float timeToReach = (distanceToPlayer > 0.001f && baseEnemyBulletSpeed > 0.001f)
					? distanceToPlayer / baseEnemyBulletSpeed
					: 0.0f;

				TPointF predictedPlayerPos = playerPos;
				if (timeToReach > 0.0f && targetPlayer)
				{
					const bool isMoving = targetPlayer->inputUp || targetPlayer->inputDown || targetPlayer->inputLeft || targetPlayer->inputRight;

					if (isMoving)
					{
						const TPointF playerMoveDir = targetPlayer->GetFacingDirection();
						const float moveDirLen = std::sqrt(playerMoveDir.X * playerMoveDir.X + playerMoveDir.Y * playerMoveDir.Y);

						if (moveDirLen > 0.1f)
						{
							const float playerSpeed = targetPlayer->GetCurrentSpeed();

							const float predictionFactor = 0.6f;
							predictedPlayerPos.X += playerMoveDir.X * playerSpeed * timeToReach * predictionFactor;
							predictedPlayerPos.Y += playerMoveDir.Y * playerSpeed * timeToReach * predictionFactor;
						}
					}
				}

				TPointF dir(predictedPlayerPos.X - enemyPos.X,
					predictedPlayerPos.Y - enemyPos.Y);
				const float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
				if (len > 0.001f)
				{
					dir.X /= len;
					dir.Y /= len;

					const float enemyBulletRange = 400.0f;
					const float enemyBulletSize = 3.0f;

					EnemyBullets.emplace_back(enemyPos, dir,
						baseEnemyBulletDamage, baseEnemyBulletSpeed, enemyBulletRange, enemyBulletSize);

					shootingEnemy->ResetShootTimer();
				}
			}
		}

		TThrowerEnemy *throwerEnemy = dynamic_cast<TThrowerEnemy*>(e.get());
		if (throwerEnemy)
		{
			if (throwerEnemy->GetThrowTimer() <= 0.0f)
			{
				const TPointF &throwerPos = throwerEnemy->GetPosition();
				const float distance = std::sqrt((playerPos.X - throwerPos.X) * (playerPos.X - throwerPos.X) +
					(playerPos.Y - throwerPos.Y) * (playerPos.Y - throwerPos.Y));

				const float throwTime = distance / 300.0f;
				TPointF predictedPos = playerPos;

				const int wave = WaveManager.GetCurrentWave();
				int baseProjectileDamage = 20;

				if (wave > 8)
				{
					const int scalingLevel = (wave - 8) / 2;
					if (scalingLevel > 0)
					{
						baseProjectileDamage = static_cast<int>(baseProjectileDamage * (1.0f + scalingLevel * 0.25f));
					}
				}

				baseProjectileDamage = static_cast<int>(baseProjectileDamage * damageMultiplier);

				ThrownProjectiles.emplace_back(throwerPos, predictedPos, baseProjectileDamage, 300.0f);
				throwerEnemy->ResetThrowTimer();
			}
		}
	}
}

