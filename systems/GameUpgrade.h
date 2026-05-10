#ifndef GameUpgradeH
#define GameUpgradeH

#include <System.Classes.hpp>
#include <System.SysUtils.hpp>
#include <vector>

enum class EUpgradeRarity
{
	Common,
	Rare,
	Legendary
};

enum class EUpgradeType
{

	Damage,
	FireRate,
	BulletRange,
	BulletSize,
	BulletSpeed,
	Pierce,

	AltDamage,
	AltFireRate,
	AltSpreadShot,
	AltBulletRange,
	AltBulletSize,
	AltBulletSpeed,

	MaxHealth,
	Speed,
	ExperienceGain,

	HealthRegen,
	CriticalChance,
	DamageReduction,
	Luck,
	Lifesteal
};

struct TUpgrade
{
	EUpgradeType Type;
	EUpgradeRarity Rarity;
	UnicodeString Name;
	UnicodeString Description;
	int Level;
	float Value;

	TUpgrade() : Type(EUpgradeType::Damage), Rarity(EUpgradeRarity::Common), Level(0), Value(0.0f) {}
	TUpgrade(EUpgradeType type, EUpgradeRarity rarity, const UnicodeString &name, const UnicodeString &desc, float value)
		: Type(type), Rarity(rarity), Name(name), Description(desc), Level(1), Value(value) {}
};

class TUpgradeManager
{
private:

	std::vector<TUpgrade> ActiveUpgrades;

	float DamageMultiplier;
	float FireRateMultiplier;
	float BulletRangeMultiplier;
	float BulletSizeMultiplier;
	float BulletSpeedMultiplier;
	bool HasPierce;

	float AltDamageMultiplier;
	float AltFireRateMultiplier;
	int AltSpreadShotCount;
	float AltBulletRangeMultiplier;
	float AltBulletSizeMultiplier;
	float AltBulletSpeedMultiplier;

	int MaxHealthBonus;
	float SpeedMultiplier;
	float ExperienceGainMultiplier;

	float HealthRegenPerWave;
	float CriticalChancePercent;
	float DamageReductionPercent;
	float LuckPercent;
	float LifestealChancePercent;

	void RecalculateModifiers();

public:
	TUpgradeManager();
	void Reset();
	void AddUpgrade(const TUpgrade &upgrade);
	void RemoveUpgrade(EUpgradeType type);
	int GetUpgradeLevel(EUpgradeType type) const;
	bool HasUpgrade(EUpgradeType type) const;

	float GetDamageMultiplier() const { return DamageMultiplier; }
	float GetFireRateMultiplier() const { return FireRateMultiplier; }
	float GetBulletRangeMultiplier() const { return BulletRangeMultiplier; }
	float GetBulletSizeMultiplier() const { return BulletSizeMultiplier; }
	float GetBulletSpeedMultiplier() const { return BulletSpeedMultiplier; }
	bool GetHasPierce() const { return HasPierce; }

	float GetAltDamageMultiplier() const { return AltDamageMultiplier; }
	float GetAltFireRateMultiplier() const { return AltFireRateMultiplier; }
	int GetAltSpreadShotCount() const { return AltSpreadShotCount; }
	float GetAltBulletRangeMultiplier() const { return AltBulletRangeMultiplier; }
	float GetAltBulletSizeMultiplier() const { return AltBulletSizeMultiplier; }
	float GetAltBulletSpeedMultiplier() const { return AltBulletSpeedMultiplier; }

	int GetMaxHealthBonus() const { return MaxHealthBonus; }
	float GetSpeedMultiplier() const { return SpeedMultiplier; }
	float GetExperienceGainMultiplier() const { return ExperienceGainMultiplier; }

	float GetHealthRegenPerWave() const { return HealthRegenPerWave; }
	float GetCriticalChancePercent() const { return CriticalChancePercent; }
	float GetDamageReductionPercent() const { return DamageReductionPercent; }
	float GetLuckPercent() const { return LuckPercent; }
	float GetLifestealChancePercent() const { return LifestealChancePercent; }

	static std::vector<TUpgrade> GenerateRandomUpgrades(int count, const std::vector<EUpgradeType> &excludeTypes = std::vector<EUpgradeType>());
	static TUpgrade CreateUpgrade(EUpgradeType type, EUpgradeRarity rarity = EUpgradeRarity::Common);
};

#endif
