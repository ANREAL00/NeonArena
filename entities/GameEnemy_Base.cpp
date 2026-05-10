#include <vcl.h>
#pragma hdrstop

#include "GameEnemy.h"
#include "core\GameConstants.h"
#include "GameProjectile.h"
#include <algorithm>
#include <cmath>

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

void TEnemy::MoveTowards(const TPointF &target, float spd, float dt)
{
	const float dx = target.X - position.X;
	const float dy = target.Y - position.Y;
	const float lenSq = dx * dx + dy * dy;
	if (lenSq > 0.000001f)
	{
		const float invLen = 1.0f / std::sqrt(lenSq);
		position.X += dx * invLen * spd * dt;
		position.Y += dy * invLen * spd * dt;
	}
}
