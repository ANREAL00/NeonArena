#include <vcl.h>
#pragma hdrstop

#include "GameWorld.h"
#include "GameWorld_Utils.h"
#include <algorithm>
#include <cmath>

using namespace NeonGame;

void TGameWorld::UpdateShooting(float deltaTime, const TInputState &input, int canvasWidth, int canvasHeight)
{
	UpdateShootingForPlayer(LocalPlayerID, deltaTime, input, canvasWidth, canvasHeight);
}

void TGameWorld::UpdateShootingForPlayer(uint8_t playerID, float deltaTime, const TInputState &input, int canvasWidth, int canvasHeight)
{
	TGamePlayer* player = GetPlayer(playerID);
	if (!player || !player->IsAlive())
		return;

	if (PlayerPrimaryFireTimers.size() <= playerID)
	{
		PlayerPrimaryFireTimers.resize(static_cast<size_t>(playerID) + 1u, 0.0f);
		PlayerAltFireTimers.resize(static_cast<size_t>(playerID) + 1u, 0.0f);
	}
	float &pTimer = PlayerPrimaryFireTimers[playerID];
	float &aTimer = PlayerAltFireTimers[playerID];
	pTimer = std::max(0.0f, pTimer - deltaTime);
	aTimer = std::max(0.0f, aTimer - deltaTime);

	const TUpgradeManager &upgrades =
		(IsNetworkGame && playerID < PlayerUpgradeManagers.size())
			? PlayerUpgradeManagers[playerID]
			: UpgradeManager;

	const TPointF playerPos = player->GetPosition();

	if (input.HasMouse)
	{
		TPointF mouseWorld;
		if (IsNetworkGame && playerID != LocalPlayerID)
		{
			mouseWorld = TPointF(input.AimWorldX, input.AimWorldY);
		}
		else
		{
			const float mouseX = static_cast<float>(input.MouseClient.x);
			const float mouseY = static_cast<float>(input.MouseClient.y);
			const TPointF cameraPos = Camera.GetBasePosition();
			mouseWorld = TPointF(mouseX + cameraPos.X, mouseY + cameraPos.Y);
		}

		AimDirection = PointF(mouseWorld.X - playerPos.X, mouseWorld.Y - playerPos.Y);
		float len = std::sqrt(AimDirection.X * AimDirection.X + AimDirection.Y * AimDirection.Y);
		if (len > 0.0001f)
		{
			AimDirection.X /= len;
			AimDirection.Y /= len;
		}
		else
		{
			AimDirection = PointF(0.0f, -1.0f);
		}

		player->SetFacingDirection(AimDirection);
	}

	const float effectiveFireCooldown = PrimaryFireCooldown * upgrades.GetFireRateMultiplier();
	if (input.PrimaryFire && input.HasMouse && pTimer <= 0.0f)
	{
		const int baseDamage = 15;
		const int effectiveDamage = static_cast<int>(baseDamage * upgrades.GetDamageMultiplier());

		const float baseSpeed = 520.0f;
		const float baseRange = 200.0f;
		const float baseSize = 4.0f;

		const float effectiveSpeed = baseSpeed * (1.0f + upgrades.GetBulletSpeedMultiplier());
		const float effectiveRange = baseRange * (1.0f + upgrades.GetBulletRangeMultiplier());
		const float effectiveSize = baseSize * (1.0f + upgrades.GetBulletSizeMultiplier());

		Bullets.emplace_back(playerPos, AimDirection, effectiveDamage,
			effectiveSpeed, effectiveRange, effectiveSize);
		pTimer = effectiveFireCooldown;
	}

	const float effectiveAltFireCooldown = AltFireCooldown * upgrades.GetAltFireRateMultiplier();
	if (input.AltFire && input.HasMouse && aTimer <= 0.0f)
	{
		const int count = upgrades.GetAltSpreadShotCount();
		const float spread = 0.25f;
		const int baseDamage = 10;
		const int effectiveDamage = static_cast<int>(baseDamage * upgrades.GetAltDamageMultiplier());

		const float baseSpeed = 520.0f;
		const float baseRange = 120.0f;
		const float baseSize = 4.0f;

		const float effectiveSpeed = baseSpeed * (1.0f + upgrades.GetAltBulletSpeedMultiplier());
		const float effectiveRange = baseRange * (1.0f + upgrades.GetAltBulletRangeMultiplier());
		const float effectiveSize = baseSize * (1.0f + upgrades.GetAltBulletSizeMultiplier());

		for (int i = 0; i < count; ++i)
		{
			const float t = (count > 1) ? (static_cast<float>(i) / (count - 1) - 0.5f) : 0.0f;
			const float angleOffset = t * spread;

			const float baseAngle = std::atan2(AimDirection.Y, AimDirection.X);
			const float ang = baseAngle + angleOffset;
			TPointF dir(std::cos(ang), std::sin(ang));

			Bullets.emplace_back(playerPos, dir, effectiveDamage,
				effectiveSpeed, effectiveRange, effectiveSize);
		}

		player->ApplyShotgunRecoil(AimDirection.X, AimDirection.Y);

		aTimer = effectiveAltFireCooldown;
	}
}
