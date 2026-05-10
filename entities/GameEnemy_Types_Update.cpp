#include <vcl.h>
#pragma hdrstop

#include "GameEnemy.h"
#include "core\GameConstants.h"
#include "GameProjectile.h"
#include <algorithm>
#include <cmath>

using namespace NeonGame;

TBasicEnemy::TBasicEnemy(const TPointF &spawnPos)
{
	position = spawnPos;
	baseSpeed = 95.0f;
	baseHealth = 100;
	speed = baseSpeed;
	scaledBaseSpeed = baseSpeed;
	health = baseHealth;
}

void TBasicEnemy::Update(float deltaTime, const TPointF &playerPos)
{
	MoveTowards(playerPos, speed, deltaTime);
}

TFastEnemy::TFastEnemy(const TPointF &spawnPos)
{
	position = spawnPos;
	baseSpeed = 180.0f;
	baseHealth = 50;
	speed = baseSpeed;
	scaledBaseSpeed = baseSpeed;
	health = baseHealth;
}

void TFastEnemy::Update(float deltaTime, const TPointF &playerPos)
{
	MoveTowards(playerPos, speed, deltaTime);
}

TThrowerEnemy::TThrowerEnemy(const TPointF &spawnPos)
{
	position = spawnPos;
	baseSpeed = 70.0f;
	baseHealth = 120;
	speed = baseSpeed;
	scaledBaseSpeed = baseSpeed;
	health = baseHealth;
	throwTimer = 0.0f;
	throwCooldown = 2.5f;
}

void TThrowerEnemy::Update(float deltaTime, const TPointF &playerPos)
{
	throwTimer -= deltaTime;
	MoveTowards(playerPos, speed, deltaTime);
}

TZigzagEnemy::TZigzagEnemy(const TPointF &spawnPos)
{
	position = spawnPos;
	lastPosition = spawnPos;
	baseSpeed = 140.0f;
	baseHealth = 150;
	speed = baseSpeed;
	scaledBaseSpeed = baseSpeed;
	health = baseHealth;
	teleportTimer = 0.0f;
	teleportCooldown = 3.5f;
	blinkTimer = 0.0f;
	isBlinking = false;
}

void TZigzagEnemy::Update(float deltaTime, const TPointF &playerPos)
{
	teleportTimer -= deltaTime;

	if (teleportTimer <= 0.5f && teleportTimer > 0.0f && !isBlinking)
	{
		isBlinking = true;
		blinkTimer = 0.5f;
	}

	if (teleportTimer <= 0.0f)
	{
		TPointF dir(playerPos.X - position.X, playerPos.Y - position.Y);
		const float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);

		if (len > 0.001f)
		{
			const float teleportDistance = std::max(150.0f, len * 0.7f);
			dir.X /= len;
			dir.Y /= len;

			lastPosition = position;
			position.X += dir.X * teleportDistance;
			position.Y += dir.Y * teleportDistance;

			position.X = std::clamp(position.X, 50.0f, WorldWidth - 50.0f);
			position.Y = std::clamp(position.Y, 50.0f, WorldHeight - 50.0f);
		}

		teleportTimer = teleportCooldown;
		isBlinking = false;
		blinkTimer = 0.0f;
	}

	if (isBlinking)
	{
		blinkTimer -= deltaTime;
		if (blinkTimer <= 0.0f)
		{
			blinkTimer = 0.0f;
		}
	}

	const float moveSpeed = isBlinking ? speed * 0.3f : speed;
	MoveTowards(playerPos, moveSpeed, deltaTime);
}

TKamikazeEnemy::TKamikazeEnemy(const TPointF &spawnPos)
{
	position = spawnPos;
	baseSpeed = 240.0f;
	baseHealth = 30;
	speed = baseSpeed;
	scaledBaseSpeed = baseSpeed;
	health = baseHealth;
}

void TKamikazeEnemy::Update(float deltaTime, const TPointF &playerPos)
{
	MoveTowards(playerPos, speed, deltaTime);
}

TShootingEnemy::TShootingEnemy(const TPointF &spawnPos)
{
	position = spawnPos;
	baseSpeed = 60.0f;
	baseHealth = 120;
	speed = baseSpeed;
	scaledBaseSpeed = baseSpeed;
	health = baseHealth;
	shootTimer = 0.0f;
	shootCooldown = 2.0f;
}

void TShootingEnemy::Update(float deltaTime, const TPointF &playerPos)
{
	shootTimer -= deltaTime;
	MoveTowards(playerPos, speed, deltaTime);
}
