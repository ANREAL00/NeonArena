#include <vcl.h>
#pragma hdrstop

#include "GameEnemy.h"
#include "GameConstants.h"
#include "GameProjectile.h"
#include <cmath>
#include <vector>
#include <algorithm>

using namespace NeonGame;

TBossEnemy::TBossEnemy(const TPointF &spawnPos, int appearanceLevel)
{
	position = spawnPos;
	this->appearanceLevel = appearanceLevel;
	
	baseSpeed = 50.0f;
	baseHealth = 1000;
	baseShootCooldown = 1.5f;
	
	const float healthMultiplier = 1.0f + (appearanceLevel * 0.30f);
	const float speedMultiplier = 1.0f + (appearanceLevel * 0.10f);
	const float shootSpeedMultiplier = 1.0f - (appearanceLevel * 0.15f); 
	const float shootSpeedFinal = std::max(0.1f, baseShootCooldown * shootSpeedMultiplier); 
	
	baseHealth = static_cast<int>(baseHealth * healthMultiplier);
	baseSpeed = baseSpeed * speedMultiplier;
	
	speed = baseSpeed;
	scaledBaseSpeed = baseSpeed;
	health = baseHealth;
	maxHealth = baseHealth;
	currentPhase = EBossPhase::Phase1;
	phaseTransitionTimer = 0.0f;
	shootTimer = 0.0f;
	shootCooldown = shootSpeedFinal;
	movementTimer = 0.0f;
	spiralAngle = 0.0f;
	stunTimer = 0.0f;
}

void TBossEnemy::UpdatePhase(float deltaTime, const TPointF &playerPos)
{
	
	const float healthRatio = static_cast<float>(health) / static_cast<float>(maxHealth);
	
	if (healthRatio > 0.80f && currentPhase != EBossPhase::Phase1)
	{
		currentPhase = EBossPhase::Phase1;
		phaseTransitionTimer = 1.0f; 
		shootCooldown = baseShootCooldown * 0.9f; 
	}
	else if (healthRatio > 0.60f && healthRatio <= 0.80f && currentPhase != EBossPhase::Phase2)
	{
		currentPhase = EBossPhase::Phase2;
		phaseTransitionTimer = 1.0f;
		shootCooldown = baseShootCooldown * 0.75f; 
	}
	else if (healthRatio > 0.40f && healthRatio <= 0.60f && currentPhase != EBossPhase::Phase3)
	{
		currentPhase = EBossPhase::Phase3;
		phaseTransitionTimer = 1.0f;
		shootCooldown = baseShootCooldown * 0.6f; 
	}
	else if (healthRatio > 0.20f && healthRatio <= 0.40f && currentPhase != EBossPhase::Phase4)
	{
		currentPhase = EBossPhase::Phase4;
		phaseTransitionTimer = 1.0f;
		shootCooldown = baseShootCooldown * 0.5f; 
	}
	else if (healthRatio <= 0.20f && currentPhase != EBossPhase::Phase5)
	{
		currentPhase = EBossPhase::Phase5;
		phaseTransitionTimer = 1.0f;
		shootCooldown = baseShootCooldown * 0.35f; 
	}
	
	if (phaseTransitionTimer > 0.0f)
		phaseTransitionTimer -= deltaTime;
}

void TBossEnemy::CreateBullets(const TPointF &playerPos, std::vector<TBullet> &bullets, float damageMultiplier)
{
	if (phaseTransitionTimer > 0.0f)
		return; 
	
	if (shootTimer > 0.0f)
		return; 
		
	switch (currentPhase)
	{
		case EBossPhase::Phase1:
			ShootFanPattern(playerPos, bullets, damageMultiplier);
			break;
		case EBossPhase::Phase2:
			ShootCirclePattern(playerPos, bullets, damageMultiplier);
			break;
		case EBossPhase::Phase3:
			ShootSpiralPattern(playerPos, bullets, damageMultiplier);
			break;
		case EBossPhase::Phase4:
			ShootCombinedPattern(playerPos, bullets, damageMultiplier);
			break;
		case EBossPhase::Phase5:
			ShootAggressivePattern(playerPos, bullets, damageMultiplier);
			break;
	}
	
	shootTimer = shootCooldown; 
}

void TBossEnemy::Update(float deltaTime, const TPointF &playerPos)
{
	UpdatePhase(deltaTime, playerPos);
	
	if (stunTimer > 0.0f)
	{
		stunTimer -= deltaTime;
		if (stunTimer < 0.0f)
			stunTimer = 0.0f;
	}
	
	shootTimer -= deltaTime;
	movementTimer += deltaTime;
	
	if (stunTimer > 0.0f)
	{
		return;
	}
	
	if (currentPhase == EBossPhase::Phase3)
	{
		
		const float circleRadius = 200.0f;
		const float circleSpeed = 0.5f;
		const float angle = movementTimer * circleSpeed;
		
		TPointF offset(std::cos(angle) * circleRadius, std::sin(angle) * circleRadius);
		TPointF targetPos = playerPos;
		targetPos.X += offset.X;
		targetPos.Y += offset.Y;
		
		TPointF dir(targetPos.X - position.X, targetPos.Y - position.Y);
		const float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
		if (len > 0.001f)
		{
			dir.X /= len;
			dir.Y /= len;
			position.X += dir.X * speed * deltaTime;
			position.Y += dir.Y * speed * deltaTime;
		}
	}
	else if (currentPhase == EBossPhase::Phase4)
	{
		
		const float zigzagAmplitude = 100.0f;
		const float zigzagSpeed = 2.0f;
		const float zigzagOffset = std::sin(movementTimer * zigzagSpeed) * zigzagAmplitude;
		
		TPointF dir(playerPos.X - position.X, playerPos.Y - position.Y);
		const float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
		if (len > 0.001f)
		{
			
			TPointF perp(-dir.Y, dir.X);
			const float perpLen = std::sqrt(perp.X * perp.X + perp.Y * perp.Y);
			if (perpLen > 0.001f)
			{
				perp.X /= perpLen;
				perp.Y /= perpLen;
			}
			
			dir.X /= len;
			dir.Y /= len;
			
			TPointF moveDir(dir.X, dir.Y);
			
			if (len > 50.0f)
			{
				moveDir.X += perp.X * (zigzagOffset / len);
				moveDir.Y += perp.Y * (zigzagOffset / len);
			}
			
			const float moveLen = std::sqrt(moveDir.X * moveDir.X + moveDir.Y * moveDir.Y);
			if (moveLen > 0.001f)
			{
				moveDir.X /= moveLen;
				moveDir.Y /= moveLen;
			}
			
			position.X += moveDir.X * speed * 1.2f * deltaTime; 
			position.Y += moveDir.Y * speed * 1.2f * deltaTime;
		}
	}
	else if (currentPhase == EBossPhase::Phase5)
	{
		
		TPointF dir(playerPos.X - position.X, playerPos.Y - position.Y);
		const float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
		if (len > 0.001f)
		{
			dir.X /= len;
			dir.Y /= len;
			position.X += dir.X * speed * 1.8f * deltaTime; 
			position.Y += dir.Y * speed * 1.8f * deltaTime;
		}
	}
	else
	{
		
		TPointF dir(playerPos.X - position.X, playerPos.Y - position.Y);
		const float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
		if (len > 0.001f)
		{
			dir.X /= len;
			dir.Y /= len;
			position.X += dir.X * speed * deltaTime;
			position.Y += dir.Y * speed * deltaTime;
		}
	}
}

void TBossEnemy::ApplyDamage(int dmg)
{
	health -= dmg;
	if (health < 0)
		health = 0;
}

float TBossEnemy::GetHealthRatio() const
{
	if (maxHealth <= 0)
		return 0.0f;
	return static_cast<float>(health) / static_cast<float>(maxHealth);
}

void TBossEnemy::ApplyTemporarySpeedMultiplier(float multiplier)
{
	
	float phaseMultiplier = 1.0f;
	if (currentPhase == EBossPhase::Phase3)
	{
		phaseMultiplier = 1.5f;
	}
	else if (currentPhase == EBossPhase::Phase4)
	{
		phaseMultiplier = 1.2f; 
	}
	else if (currentPhase == EBossPhase::Phase5)
	{
		phaseMultiplier = 1.8f; 
	}
	
	speed = scaledBaseSpeed * phaseMultiplier * multiplier;
}

void TBossEnemy::SetPhaseFromNetwork(uint8_t phase)
{
	if (phase > static_cast<uint8_t>(EBossPhase::Phase5))
		phase = static_cast<uint8_t>(EBossPhase::Phase5);
	currentPhase = static_cast<EBossPhase>(phase);
}

void TBossEnemy::StartStun(float duration)
{
	stunTimer = duration;
}