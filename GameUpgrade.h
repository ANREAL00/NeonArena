//---------------------------------------------------------------------------
// Система улучшений (Neon Arena)
//---------------------------------------------------------------------------
#ifndef GameUpgradeH
#define GameUpgradeH
//---------------------------------------------------------------------------

#include <System.Classes.hpp>
#include <System.SysUtils.hpp>
#include <vector>

//---------------------------------------------------------------------------
enum class EUpgradeRarity
{
	Common,    // обычное (синий) - шанс 60%
	Rare,      // редкое (фиолетовый) - шанс 30%
	Legendary  // легендарное (золотой) - шанс 10%
};

//---------------------------------------------------------------------------
enum class EUpgradeType
{
	// Основная атака
	Damage,              // урон
	FireRate,            // скорострельность
	BulletRange,         // дальность пуль
	BulletSize,          // размер пуль
	BulletSpeed,         // скорость пуль
	Pierce,              // пробитие
	
	// Альтернативная атака
	AltDamage,           // урон альтернативной атаки
	AltFireRate,         // скорострельность альтернативной атаки
	AltSpreadShot,       // количество пуль в залпе
	AltBulletRange,      // дальность альтернативной атаки
	AltBulletSize,       // размер пуль альтернативной атаки
	AltBulletSpeed,      // скорость пуль альтернативной атаки
	
	// Общие
	MaxHealth,           // здоровье
	Speed,               // скорость движения
	ExperienceGain,      // получение опыта
	
	// Новые улучшения
	HealthRegen,         // регенерация здоровья
	CriticalChance,      // шанс критического удара
	DamageReduction,     // снижение получаемого урона
	Luck,                // шанс на двойной опыт
	Lifesteal            // вампиризм (восстановление HP при убийстве)
};

//---------------------------------------------------------------------------
struct TUpgrade
{
	EUpgradeType Type;
	EUpgradeRarity Rarity;
	UnicodeString Name;
	UnicodeString Description;
	int Level; // уровень улучшения (для стаков)
	float Value; // значение улучшения (зависит от редкости)

	TUpgrade() : Type(EUpgradeType::Damage), Rarity(EUpgradeRarity::Common), Level(0), Value(0.0f) {}
	TUpgrade(EUpgradeType type, EUpgradeRarity rarity, const UnicodeString &name, const UnicodeString &desc, float value)
		: Type(type), Rarity(rarity), Name(name), Description(desc), Level(1), Value(value) {}
};

//---------------------------------------------------------------------------
class TUpgradeManager
{
private:
	// активные улучшения игрока
	std::vector<TUpgrade> ActiveUpgrades;

	// модификаторы (вычисляются из активных улучшений)
	// Основная атака
	float DamageMultiplier;
	float FireRateMultiplier; // множитель для кулдауна (меньше = быстрее)
	float BulletRangeMultiplier;
	float BulletSizeMultiplier;
	float BulletSpeedMultiplier;
	bool HasPierce;
	
	// Альтернативная атака
	float AltDamageMultiplier;
	float AltFireRateMultiplier;
	int AltSpreadShotCount;
	float AltBulletRangeMultiplier;
	float AltBulletSizeMultiplier;
	float AltBulletSpeedMultiplier;
	
	// Общие
	int MaxHealthBonus;
	float SpeedMultiplier;
	float ExperienceGainMultiplier;
	
	// Новые улучшения
	float HealthRegenPerWave;     // регенерация HP за волну
	float CriticalChancePercent; // шанс критического удара (%)
	float DamageReductionPercent; // снижение получаемого урона (%)
	float LuckPercent;            // шанс на двойной опыт (%)
	float LifestealChancePercent; // вампиризм (шанс восстановить 5 HP за убийство, %)

	void RecalculateModifiers();

public:
	TUpgradeManager();
	void Reset();
	void AddUpgrade(const TUpgrade &upgrade);
	void RemoveUpgrade(EUpgradeType type);
	int GetUpgradeLevel(EUpgradeType type) const;
	bool HasUpgrade(EUpgradeType type) const;

	// геттеры модификаторов
	// Основная атака
	float GetDamageMultiplier() const { return DamageMultiplier; }
	float GetFireRateMultiplier() const { return FireRateMultiplier; }
	float GetBulletRangeMultiplier() const { return BulletRangeMultiplier; }
	float GetBulletSizeMultiplier() const { return BulletSizeMultiplier; }
	float GetBulletSpeedMultiplier() const { return BulletSpeedMultiplier; }
	bool GetHasPierce() const { return HasPierce; }
	
	// Альтернативная атака
	float GetAltDamageMultiplier() const { return AltDamageMultiplier; }
	float GetAltFireRateMultiplier() const { return AltFireRateMultiplier; }
	int GetAltSpreadShotCount() const { return AltSpreadShotCount; }
	float GetAltBulletRangeMultiplier() const { return AltBulletRangeMultiplier; }
	float GetAltBulletSizeMultiplier() const { return AltBulletSizeMultiplier; }
	float GetAltBulletSpeedMultiplier() const { return AltBulletSpeedMultiplier; }
	
	// Общие
	int GetMaxHealthBonus() const { return MaxHealthBonus; }
	float GetSpeedMultiplier() const { return SpeedMultiplier; }
	float GetExperienceGainMultiplier() const { return ExperienceGainMultiplier; }
	
	// Новые улучшения
	float GetHealthRegenPerWave() const { return HealthRegenPerWave; }
	float GetCriticalChancePercent() const { return CriticalChancePercent; }
	float GetDamageReductionPercent() const { return DamageReductionPercent; }
	float GetLuckPercent() const { return LuckPercent; }
	float GetLifestealChancePercent() const { return LifestealChancePercent; }

	// генерация случайных улучшений для выбора
	// excludeTypes - типы улучшений, которые уже есть у игрока (например, Pierce)
	static std::vector<TUpgrade> GenerateRandomUpgrades(int count, const std::vector<EUpgradeType> &excludeTypes = std::vector<EUpgradeType>());
	static TUpgrade CreateUpgrade(EUpgradeType type, EUpgradeRarity rarity = EUpgradeRarity::Common);
};

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

