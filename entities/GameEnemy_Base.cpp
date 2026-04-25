#include <vcl.h>
#pragma hdrstop

#include "GameEnemy.h"
#include "GameConstants.h"
#include "GameProjectile.h"
#include <algorithm>

using namespace NeonGame;

void TEnemy::ApplyDamage(int dmg)
{
	health -= dmg;
}

void TEnemy::ApplyScaling(float healthMultiplier, float speedMultiplier)
{
	health = static_cast<int>(baseHealth * healthMultiplier);
	scaledBaseSpeed = baseSpeed * speedMultiplier;
	speed = scaledBaseSpeed;
}

void TEnemy::ApplyTemporarySpeedMultiplier(float multiplier)
{
	speed = scaledBaseSpeed * multiplier;
}

