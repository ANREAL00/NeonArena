#include <vcl.h>
#pragma hdrstop

#include "GameEnemy.h"
#include "core\GameConstants.h"
#include "GameProjectile.h"
#include <cmath>
#include <vector>

using namespace NeonGame;

void TBossEnemy::ShootFanPattern(const TPointF &playerPos, std::vector<TBullet> &bullets, float damageMultiplier)
{
	TPointF dir(playerPos.X - position.X, playerPos.Y - position.Y);
	const float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
	if (len < 0.001f)
		return;

	const float baseAngle = std::atan2(dir.Y, dir.X);
	const float spread = 0.8f;
	const int bulletCount = 7;

	const int baseBulletDamage = 12;
	int bulletDamage = static_cast<int>(baseBulletDamage * (1.0f + appearanceLevel * 0.20f));
	bulletDamage = static_cast<int>(bulletDamage * damageMultiplier);

	for (int i = 0; i < bulletCount; i++)
	{
		const float t = static_cast<float>(i) / (bulletCount - 1) - 0.5f;
		const float angle = baseAngle + t * spread;
		TPointF bulletDir(std::cos(angle), std::sin(angle));

		const float bulletSpeed = 380.0f;
		const float bulletRange = 900.0f;
		const float bulletSize = 5.0f;

		bullets.emplace_back(position, bulletDir, bulletDamage, bulletSpeed, bulletRange, bulletSize);
	}
}

void TBossEnemy::ShootCirclePattern(const TPointF &playerPos, std::vector<TBullet> &bullets, float damageMultiplier)
{
	const int bulletCount = 16;
	const float angleStep = 2.0f * 3.14159265f / bulletCount;

	const int baseBulletDamage = 10;
	int bulletDamage = static_cast<int>(baseBulletDamage * (1.0f + appearanceLevel * 0.20f));
	bulletDamage = static_cast<int>(bulletDamage * damageMultiplier);

	for (int i = 0; i < bulletCount; i++)
	{
		const float angle = i * angleStep;
		TPointF bulletDir(std::cos(angle), std::sin(angle));

		const float bulletSpeed = 340.0f;
		const float bulletRange = 900.0f;
		const float bulletSize = 4.5f;

		bullets.emplace_back(position, bulletDir, bulletDamage, bulletSpeed, bulletRange, bulletSize);
	}

	for (int i = 0; i < bulletCount; i++)
	{
		const float angle = i * angleStep + angleStep * 0.5f;
		TPointF bulletDir(std::cos(angle), std::sin(angle));

		const float bulletSpeed = 300.0f;
		const float bulletRange = 900.0f;
		const float bulletSize = 4.0f;

		bullets.emplace_back(position, bulletDir, bulletDamage, bulletSpeed, bulletRange, bulletSize);
	}
}

void TBossEnemy::ShootSpiralPattern(const TPointF &playerPos, std::vector<TBullet> &bullets, float damageMultiplier)
{
	spiralAngle += 0.4f;
	const int bulletCount = 8;
	const float angleStep = 2.0f * 3.14159265f / bulletCount;

	const int baseBulletDamage = 15;
	int bulletDamage = static_cast<int>(baseBulletDamage * (1.0f + appearanceLevel * 0.20f));
	bulletDamage = static_cast<int>(bulletDamage * damageMultiplier);

	for (int i = 0; i < bulletCount; i++)
	{
		const float angle = spiralAngle + i * angleStep;
		TPointF bulletDir(std::cos(angle), std::sin(angle));

		const float bulletSpeed = 400.0f;
		const float bulletRange = 900.0f;
		const float bulletSize = 5.5f;

		bullets.emplace_back(position, bulletDir, bulletDamage, bulletSpeed, bulletRange, bulletSize);
	}

	for (int i = 0; i < bulletCount; i++)
	{
		const float angle = -spiralAngle + i * angleStep;
		TPointF bulletDir(std::cos(angle), std::sin(angle));

		const float bulletSpeed = 360.0f;
		const float bulletRange = 900.0f;
		const float bulletSize = 5.0f;

		bullets.emplace_back(position, bulletDir, bulletDamage, bulletSpeed, bulletRange, bulletSize);
	}
}

void TBossEnemy::ShootCombinedPattern(const TPointF &playerPos, std::vector<TBullet> &bullets, float damageMultiplier)
{
	TPointF dir(playerPos.X - position.X, playerPos.Y - position.Y);
	const float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
	if (len > 0.001f)
	{
		const float baseAngle = std::atan2(dir.Y, dir.X);
		const float spread = 0.7f;
		const int fanBulletCount = 6;

		const int baseBulletDamage = 13;
		int bulletDamage = static_cast<int>(baseBulletDamage * (1.0f + appearanceLevel * 0.20f));
		bulletDamage = static_cast<int>(bulletDamage * damageMultiplier);

		for (int i = 0; i < fanBulletCount; i++)
		{
			const float t = static_cast<float>(i) / (fanBulletCount - 1) - 0.5f;
			const float angle = baseAngle + t * spread;
			TPointF bulletDir(std::cos(angle), std::sin(angle));

			bullets.emplace_back(position, bulletDir, bulletDamage, 370.0f, 900.0f, 5.0f);
		}
	}

	const int circleBulletCount = 12;
	const float angleStep = 2.0f * 3.14159265f / circleBulletCount;

	const int baseBulletDamage = 11;
	int bulletDamage = static_cast<int>(baseBulletDamage * (1.0f + appearanceLevel * 0.20f));
	bulletDamage = static_cast<int>(bulletDamage * damageMultiplier);

	for (int i = 0; i < circleBulletCount; i++)
	{
		const float angle = i * angleStep;
		TPointF bulletDir(std::cos(angle), std::sin(angle));

		bullets.emplace_back(position, bulletDir, bulletDamage, 330.0f, 900.0f, 4.5f);
	}
}

void TBossEnemy::ShootAggressivePattern(const TPointF &playerPos, std::vector<TBullet> &bullets, float damageMultiplier)
{
	spiralAngle += 0.5f;

	for (int spiral = 0; spiral < 3; spiral++)
	{
		const float spiralOffset = spiral * (2.0f * 3.14159265f / 3.0f);
		const int bulletCount = 6;
		const float angleStep = 2.0f * 3.14159265f / bulletCount;

		const int baseBulletDamage = 16;
		int bulletDamage = static_cast<int>(baseBulletDamage * (1.0f + appearanceLevel * 0.20f));
		bulletDamage = static_cast<int>(bulletDamage * damageMultiplier);

		for (int i = 0; i < bulletCount; i++)
		{
			const float angle = spiralAngle + spiralOffset + i * angleStep;
			TPointF bulletDir(std::cos(angle), std::sin(angle));

			const float speed = 420.0f + spiral * 20.0f;
			bullets.emplace_back(position, bulletDir, bulletDamage, speed, 900.0f, 5.5f);
		}
	}

	TPointF dir(playerPos.X - position.X, playerPos.Y - position.Y);
	const float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
	if (len > 0.001f)
	{
		const float baseAngle = std::atan2(dir.Y, dir.X);
		const int fanBulletCount = 5;
		const float spread = 0.5f;

		const int baseBulletDamage = 14;
		int bulletDamage = static_cast<int>(baseBulletDamage * (1.0f + appearanceLevel * 0.20f));
		bulletDamage = static_cast<int>(bulletDamage * damageMultiplier);

		for (int i = 0; i < fanBulletCount; i++)
		{
			const float t = static_cast<float>(i) / (fanBulletCount - 1) - 0.5f;
			const float angle = baseAngle + t * spread;
			TPointF bulletDir(std::cos(angle), std::sin(angle));

			bullets.emplace_back(position, bulletDir, bulletDamage, 400.0f, 900.0f, 5.0f);
		}
	}
}
