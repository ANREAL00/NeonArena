#include <vcl.h>
#pragma hdrstop

#include "GameUpgrade.h"
#include <algorithm>
#include <System.SysUtils.hpp>

TUpgradeManager::TUpgradeManager()
	: DamageMultiplier(1.0f),
	  FireRateMultiplier(1.0f),
	  BulletRangeMultiplier(1.0f),
	  BulletSizeMultiplier(1.0f),
	  BulletSpeedMultiplier(1.0f),
	  HasPierce(false),
	  AltDamageMultiplier(1.0f),
	  AltFireRateMultiplier(1.0f),
	  AltSpreadShotCount(3),
	  AltBulletRangeMultiplier(1.0f),
	  AltBulletSizeMultiplier(1.0f),
	  AltBulletSpeedMultiplier(1.0f),
	  MaxHealthBonus(0),
	  SpeedMultiplier(1.0f),
	  ExperienceGainMultiplier(1.0f),
	  HealthRegenPerWave(0.0f),
	  CriticalChancePercent(0.0f),
	  DamageReductionPercent(0.0f),
	  LuckPercent(0.0f),
	  LifestealChancePercent(0.0f)
{
}

void TUpgradeManager::Reset()
{
	ActiveUpgrades.clear();
	DamageMultiplier = 1.0f;
	FireRateMultiplier = 1.0f;
	BulletRangeMultiplier = 1.0f;
	BulletSizeMultiplier = 1.0f;
	BulletSpeedMultiplier = 1.0f;
	HasPierce = false;
	AltDamageMultiplier = 1.0f;
	AltFireRateMultiplier = 1.0f;
	AltSpreadShotCount = 3;
	AltBulletRangeMultiplier = 1.0f;
	AltBulletSizeMultiplier = 1.0f;
	AltBulletSpeedMultiplier = 1.0f;
	MaxHealthBonus = 0;
	SpeedMultiplier = 1.0f;
	ExperienceGainMultiplier = 1.0f;
	HealthRegenPerWave = 0.0f;
	CriticalChancePercent = 0.0f;
	DamageReductionPercent = 0.0f;
	LuckPercent = 0.0f;
	LifestealChancePercent = 0.0f;
}

void TUpgradeManager::AddUpgrade(const TUpgrade &upgrade)
{

	auto it = std::find_if(ActiveUpgrades.begin(), ActiveUpgrades.end(),
		[&upgrade](const TUpgrade &u) { return u.Type == upgrade.Type; });

	if (it != ActiveUpgrades.end())
	{
		it->Level++;
		it->Value += upgrade.Value;
	}
	else
	{
		ActiveUpgrades.push_back(upgrade);
	}
	RecalculateModifiers();
}

void TUpgradeManager::RemoveUpgrade(EUpgradeType type)
{
	ActiveUpgrades.erase(
		std::remove_if(ActiveUpgrades.begin(), ActiveUpgrades.end(),
			[type](const TUpgrade &u) { return u.Type == type; }),
		ActiveUpgrades.end());
	RecalculateModifiers();
}

int TUpgradeManager::GetUpgradeLevel(EUpgradeType type) const
{
	auto it = std::find_if(ActiveUpgrades.begin(), ActiveUpgrades.end(),
		[type](const TUpgrade &u) { return u.Type == type; });
	return (it != ActiveUpgrades.end()) ? it->Level : 0;
}

bool TUpgradeManager::HasUpgrade(EUpgradeType type) const
{
	return GetUpgradeLevel(type) > 0;
}

void TUpgradeManager::RecalculateModifiers()
{
	DamageMultiplier = 1.0f;
	FireRateMultiplier = 1.0f;
	BulletRangeMultiplier = 1.0f;
	BulletSizeMultiplier = 1.0f;
	BulletSpeedMultiplier = 1.0f;
	HasPierce = false;

	AltDamageMultiplier = 1.0f;
	AltFireRateMultiplier = 1.0f;
	AltSpreadShotCount = 3;
	AltBulletRangeMultiplier = 1.0f;
	AltBulletSizeMultiplier = 1.0f;
	AltBulletSpeedMultiplier = 1.0f;

	MaxHealthBonus = 0;
	SpeedMultiplier = 1.0f;
	ExperienceGainMultiplier = 1.0f;

	HealthRegenPerWave = 0.0f;
	CriticalChancePercent = 0.0f;
	DamageReductionPercent = 0.0f;
	LuckPercent = 0.0f;
	LifestealChancePercent = 0.0f;

	for (const auto &upgrade : ActiveUpgrades)
	{
		const int level = upgrade.Level;
		const float value = upgrade.Value;

		switch (upgrade.Type)
		{
		case EUpgradeType::Damage:
			DamageMultiplier += value;
			break;
		case EUpgradeType::FireRate:
			FireRateMultiplier *= (1.0f - value);
			if (FireRateMultiplier < 0.1f)
				FireRateMultiplier = 0.1f;
			break;
		case EUpgradeType::BulletRange:
			BulletRangeMultiplier += value;
			break;
		case EUpgradeType::BulletSize:
			BulletSizeMultiplier += value;
			break;
		case EUpgradeType::BulletSpeed:
			BulletSpeedMultiplier += value;
			break;
		case EUpgradeType::Pierce:
			HasPierce = true;
			break;
		case EUpgradeType::AltDamage:
			AltDamageMultiplier += value;
			break;
		case EUpgradeType::AltFireRate:
			AltFireRateMultiplier *= (1.0f - value);
			if (AltFireRateMultiplier < 0.1f)
				AltFireRateMultiplier = 0.1f;
			break;
		case EUpgradeType::AltSpreadShot:
			AltSpreadShotCount = 3 + static_cast<int>(value);
			break;
		case EUpgradeType::AltBulletRange:
			AltBulletRangeMultiplier += value;
			break;
		case EUpgradeType::AltBulletSize:
			AltBulletSizeMultiplier += value;
			break;
		case EUpgradeType::AltBulletSpeed:
			AltBulletSpeedMultiplier += value;
			break;
		case EUpgradeType::MaxHealth:
			MaxHealthBonus += static_cast<int>(value);
			break;
		case EUpgradeType::Speed:
			SpeedMultiplier += value;
			break;
		case EUpgradeType::ExperienceGain:
			ExperienceGainMultiplier += value;
			break;
		case EUpgradeType::HealthRegen:
			HealthRegenPerWave += value;
			break;
		case EUpgradeType::CriticalChance:
			CriticalChancePercent += value;
			if (CriticalChancePercent > 100.0f)
				CriticalChancePercent = 100.0f;
			break;
		case EUpgradeType::DamageReduction:
			DamageReductionPercent += value;
			if (DamageReductionPercent > 90.0f)
				DamageReductionPercent = 90.0f;
			break;
		case EUpgradeType::Luck:
			LuckPercent += value;
			if (LuckPercent > 100.0f)
				LuckPercent = 100.0f;
			break;
		case EUpgradeType::Lifesteal:
			LifestealChancePercent += value;
			if (LifestealChancePercent > 30.0f)
				LifestealChancePercent = 30.0f;
			break;
		}
	}
}

