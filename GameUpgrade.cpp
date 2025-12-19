#include <vcl.h>
#pragma hdrstop
//---------------------------------------------------------------------------

#include "GameUpgrade.h"
#include <algorithm>
#include <System.SysUtils.hpp>

//---------------------------------------------------------------------------
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

//---------------------------------------------------------------------------
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

//---------------------------------------------------------------------------
void TUpgradeManager::AddUpgrade(const TUpgrade &upgrade)
{
	// ищем, есть ли уже такое улучшение
	auto it = std::find_if(ActiveUpgrades.begin(), ActiveUpgrades.end(),
		[&upgrade](const TUpgrade &u) { return u.Type == upgrade.Type; });

	if (it != ActiveUpgrades.end())
	{
		// улучшение уже есть - увеличиваем уровень и суммируем value
		// (для правильного накопления эффектов разных редкостей)
		it->Level++;
		it->Value += upgrade.Value; // суммируем value для правильного накопления
	}
	else
	{
		// новое улучшение
		ActiveUpgrades.push_back(upgrade);
	}

	RecalculateModifiers();
}

//---------------------------------------------------------------------------
void TUpgradeManager::RemoveUpgrade(EUpgradeType type)
{
	ActiveUpgrades.erase(
		std::remove_if(ActiveUpgrades.begin(), ActiveUpgrades.end(),
			[type](const TUpgrade &u) { return u.Type == type; }),
		ActiveUpgrades.end());
	RecalculateModifiers();
}

//---------------------------------------------------------------------------
int TUpgradeManager::GetUpgradeLevel(EUpgradeType type) const
{
	auto it = std::find_if(ActiveUpgrades.begin(), ActiveUpgrades.end(),
		[type](const TUpgrade &u) { return u.Type == type; });
	return (it != ActiveUpgrades.end()) ? it->Level : 0;
}

//---------------------------------------------------------------------------
bool TUpgradeManager::HasUpgrade(EUpgradeType type) const
{
	return GetUpgradeLevel(type) > 0;
}

//---------------------------------------------------------------------------
void TUpgradeManager::RecalculateModifiers()
{
	// Основная атака
	DamageMultiplier = 1.0f;
	FireRateMultiplier = 1.0f;
	BulletRangeMultiplier = 1.0f;
	BulletSizeMultiplier = 1.0f;
	BulletSpeedMultiplier = 1.0f;
	HasPierce = false;
	
	// Альтернативная атака
	AltDamageMultiplier = 1.0f;
	AltFireRateMultiplier = 1.0f;
	AltSpreadShotCount = 3;
	AltBulletRangeMultiplier = 1.0f;
	AltBulletSizeMultiplier = 1.0f;
	AltBulletSpeedMultiplier = 1.0f;
	
	// Общие
	MaxHealthBonus = 0;
	SpeedMultiplier = 1.0f;
	ExperienceGainMultiplier = 1.0f;
	
	// Новые улучшения
	HealthRegenPerWave = 0.0f;
	CriticalChancePercent = 0.0f;
	DamageReductionPercent = 0.0f;
	LuckPercent = 0.0f;
	LifestealChancePercent = 0.0f;

	for (const auto &upgrade : ActiveUpgrades)
	{
		const int level = upgrade.Level;
		const float value = upgrade.Value; // базовое значение улучшения
		
		switch (upgrade.Type)
		{
		// Основная атака
		case EUpgradeType::Damage:
			// value уже содержит сумму всех значений улучшений
			DamageMultiplier += value;
			break;
		case EUpgradeType::FireRate:
			// аддитивное накопление процентов уменьшения кулдауна
			// value уже содержит сумму всех значений улучшений
			FireRateMultiplier *= (1.0f - value);
			if (FireRateMultiplier < 0.1f)
				FireRateMultiplier = 0.1f;
			break;
		case EUpgradeType::BulletRange:
			// value уже содержит сумму всех значений улучшений
			BulletRangeMultiplier += value;
			break;
		case EUpgradeType::BulletSize:
			// value уже содержит сумму всех значений улучшений
			BulletSizeMultiplier += value;
			break;
		case EUpgradeType::BulletSpeed:
			// value уже содержит сумму всех значений улучшений
			BulletSpeedMultiplier += value;
			break;
		case EUpgradeType::Pierce:
			HasPierce = true;
			break;
		
		// Альтернативная атака
		case EUpgradeType::AltDamage:
			// value уже содержит сумму всех значений улучшений
			AltDamageMultiplier += value;
			break;
		case EUpgradeType::AltFireRate:
			// аддитивное накопление процентов уменьшения кулдауна
			// value уже содержит сумму всех значений улучшений
			AltFireRateMultiplier *= (1.0f - value);
			if (AltFireRateMultiplier < 0.1f)
				AltFireRateMultiplier = 0.1f;
			break;
		case EUpgradeType::AltSpreadShot:
			// value уже содержит сумму всех значений улучшений (количество пуль)
			AltSpreadShotCount = 3 + static_cast<int>(value);
			break;
		case EUpgradeType::AltBulletRange:
			// value уже содержит сумму всех значений улучшений
			AltBulletRangeMultiplier += value;
			break;
		case EUpgradeType::AltBulletSize:
			// value уже содержит сумму всех значений улучшений
			AltBulletSizeMultiplier += value;
			break;
		case EUpgradeType::AltBulletSpeed:
			// value уже содержит сумму всех значений улучшений
			AltBulletSpeedMultiplier += value;
			break;
		
		// Общие
		case EUpgradeType::MaxHealth:
			// value уже содержит сумму всех значений улучшений (HP)
			MaxHealthBonus += static_cast<int>(value);
			break;
		case EUpgradeType::Speed:
			// value уже содержит сумму всех значений улучшений
			SpeedMultiplier += value;
			break;
		case EUpgradeType::ExperienceGain:
			// value уже содержит сумму всех значений улучшений
			ExperienceGainMultiplier += value;
			break;
		
		// Новые улучшения
		case EUpgradeType::HealthRegen:
			// value уже содержит сумму всех значений улучшений (HP за волну)
			HealthRegenPerWave += value;
			break;
		case EUpgradeType::CriticalChance:
			// value уже содержит сумму всех значений улучшений (%)
			CriticalChancePercent += value;
			if (CriticalChancePercent > 100.0f)
				CriticalChancePercent = 100.0f; // максимум 100%
			break;
		case EUpgradeType::DamageReduction:
			// value уже содержит сумму всех значений улучшений (%)
			DamageReductionPercent += value;
			if (DamageReductionPercent > 90.0f)
				DamageReductionPercent = 90.0f; // максимум 90% защиты
			break;
		case EUpgradeType::Luck:
			// value уже содержит сумму всех значений улучшений (%)
			LuckPercent += value;
			if (LuckPercent > 100.0f)
				LuckPercent = 100.0f; // максимум 100%
			break;
		case EUpgradeType::Lifesteal:
			// value уже содержит сумму всех значений улучшений (% шанс)
			LifestealChancePercent += value;
			if (LifestealChancePercent > 30.0f)
				LifestealChancePercent = 30.0f; // максимум 30% шанс
			break;
		}
	}
}

//---------------------------------------------------------------------------
TUpgrade TUpgradeManager::CreateUpgrade(EUpgradeType type, EUpgradeRarity rarity)
{
	UnicodeString name, desc;
	float value = 0.0f;
	
	// определяем значения в зависимости от редкости
	float commonValue = 0.0f, rareValue = 0.0f, legendaryValue = 0.0f;
	
	switch (type)
	{
	// Основная атака
	case EUpgradeType::Damage:
		name = L"Урон";
		commonValue = 0.10f; rareValue = 0.15f; legendaryValue = 0.20f;
		desc = L"Увеличивает урон основной атаки";
		break;
	case EUpgradeType::FireRate:
		name = L"Скорострельность";
		commonValue = 0.10f; rareValue = 0.15f; legendaryValue = 0.20f;
		desc = L"Уменьшает кулдаун основной атаки";
		break;
	case EUpgradeType::BulletRange:
		name = L"Дальность";
		commonValue = 0.15f; rareValue = 0.25f; legendaryValue = 0.40f;
		desc = L"Увеличивает дальность основной атаки";
		break;
	case EUpgradeType::BulletSize:
		name = L"Размер пуль";
		commonValue = 0.20f; rareValue = 0.35f; legendaryValue = 0.50f;
		desc = L"Увеличивает размер пуль основной атаки";
		break;
	case EUpgradeType::BulletSpeed:
		name = L"Скорость пуль";
		commonValue = 0.15f; rareValue = 0.25f; legendaryValue = 0.40f;
		desc = L"Увеличивает скорость пуль основной атаки";
		break;
	case EUpgradeType::Pierce:
		name = L"Пробитие";
		value = 1.0f; // не зависит от редкости
		desc = L"Пули основной атаки пробивают врагов";
		break;
	
	// Альтернативная атака
	case EUpgradeType::AltDamage:
		name = L"Урон залпа";
		commonValue = 0.10f; rareValue = 0.15f; legendaryValue = 0.20f;
		desc = L"Увеличивает урон альтернативной атаки";
		break;
	case EUpgradeType::AltFireRate:
		name = L"Скорость залпа";
		commonValue = 0.10f; rareValue = 0.15f; legendaryValue = 0.20f;
		desc = L"Уменьшает кулдаун альтернативной атаки";
		break;
	case EUpgradeType::AltSpreadShot:
		name = L"Множественный залп";
		commonValue = 1.0f; rareValue = 2.0f; legendaryValue = 3.0f; // количество пуль
		desc = L"Увеличивает количество пуль в залпе";
		break;
	case EUpgradeType::AltBulletRange:
		name = L"Дальность залпа";
		commonValue = 0.15f; rareValue = 0.25f; legendaryValue = 0.40f;
		desc = L"Увеличивает дальность альтернативной атаки";
		break;
	case EUpgradeType::AltBulletSize:
		name = L"Размер залпа";
		commonValue = 0.20f; rareValue = 0.35f; legendaryValue = 0.50f;
		desc = L"Увеличивает размер пуль альтернативной атаки";
		break;
	case EUpgradeType::AltBulletSpeed:
		name = L"Скорость залпа";
		commonValue = 0.15f; rareValue = 0.25f; legendaryValue = 0.40f;
		desc = L"Увеличивает скорость пуль альтернативной атаки";
		break;
	
	// Общие
	case EUpgradeType::MaxHealth:
		name = L"Здоровье";
		commonValue = 20.0f; rareValue = 30.0f; legendaryValue = 50.0f; // HP
		desc = L"Увеличивает максимальное здоровье";
		break;
	case EUpgradeType::Speed:
		name = L"Скорость";
		commonValue = 0.05f; rareValue = 0.07f; legendaryValue = 0.10f;
		desc = L"Увеличивает скорость движения";
		break;
	case EUpgradeType::ExperienceGain:
		name = L"Опыт";
		commonValue = 0.15f; rareValue = 0.25f; legendaryValue = 0.40f;
		desc = L"Увеличивает получаемый опыт";
		break;
	
	// Новые улучшения
	case EUpgradeType::HealthRegen:
		name = L"Регенерация";
		commonValue = 2.0f; rareValue = 3.0f; legendaryValue = 5.0f; // HP за волну
		desc = L"Восстанавливает здоровье за пройденную волну";
		break;
	case EUpgradeType::CriticalChance:
		name = L"Критический удар";
		commonValue = 2.0f; rareValue = 4.0f; legendaryValue = 7.0f; // процент
		desc = L"Шанс нанести двойной урон";
		break;
	case EUpgradeType::DamageReduction:
		name = L"Защита";
		commonValue = 5.0f; rareValue = 7.0f; legendaryValue = 10.0f; // процент
		desc = L"Снижает получаемый урон";
		break;
	case EUpgradeType::Luck:
		name = L"Удача";
		commonValue = 5.0f; rareValue = 10.0f; legendaryValue = 15.0f; // процент
		desc = L"Шанс получить двойной опыт";
		break;
	case EUpgradeType::Lifesteal:
		name = L"Вампиризм";
		commonValue = 1.0f; rareValue = 2.0f; legendaryValue = 3.0f; // процент шанса
		desc = L"Шанс восстановить 5 HP при убийстве врага";
		break;
	default:
		return TUpgrade(type, EUpgradeRarity::Common, L"Неизвестно", L"", 0.0f);
	}
	
	// выбираем значение в зависимости от редкости
	if (value == 0.0f)
	{
		switch (rarity)
		{
		case EUpgradeRarity::Common:
			value = commonValue;
			break;
		case EUpgradeRarity::Rare:
			value = rareValue;
			break;
		case EUpgradeRarity::Legendary:
			value = legendaryValue;
			break;
		}
	}
	
	// добавляем информацию о редкости в описание
	UnicodeString rarityText;
	switch (rarity)
	{
	case EUpgradeRarity::Common:
		rarityText = L"";
		break;
	case EUpgradeRarity::Rare:
		rarityText = L" [Редкое]";
		break;
	case EUpgradeRarity::Legendary:
		rarityText = L" [Легендарное]";
		break;
	}
	
	// форматируем описание с процентом
	UnicodeString finalDesc = desc;
	if (type == EUpgradeType::MaxHealth)
	{
		finalDesc = desc + L" на " + IntToStr(static_cast<int>(value)) + L" HP" + rarityText;
	}
	else if (type == EUpgradeType::AltSpreadShot)
	{
		finalDesc = desc + L" на " + IntToStr(static_cast<int>(value)) + L" пуль" + rarityText;
	}
	else if (type == EUpgradeType::Pierce)
	{
		finalDesc = desc + rarityText;
	}
	else if (type == EUpgradeType::HealthRegen)
	{
		finalDesc = desc + L" на " + IntToStr(static_cast<int>(value)) + L" HP" + rarityText;
	}
	else if (type == EUpgradeType::CriticalChance || type == EUpgradeType::DamageReduction || 
	         type == EUpgradeType::Luck)
	{
		finalDesc = desc + L" на " + IntToStr(static_cast<int>(value)) + L"%" + rarityText;
	}
	else if (type == EUpgradeType::Lifesteal)
	{
		finalDesc = desc + L" (" + IntToStr(static_cast<int>(value)) + L"% шанс)" + rarityText;
	}
	else
	{
		finalDesc = desc + L" на " + IntToStr(static_cast<int>(value * 100)) + L"%" + rarityText;
	}
	
	return TUpgrade(type, rarity, name, finalDesc, value);
}

//---------------------------------------------------------------------------
std::vector<TUpgrade> TUpgradeManager::GenerateRandomUpgrades(int count, const std::vector<EUpgradeType> &excludeTypes)
{
	std::vector<TUpgrade> result;
	std::vector<EUpgradeType> allTypes = {
		// Основная атака
		EUpgradeType::Damage,
		EUpgradeType::FireRate,
		EUpgradeType::BulletRange,
		EUpgradeType::BulletSize,
		EUpgradeType::BulletSpeed,
		EUpgradeType::Pierce,
		
		// Альтернативная атака
		EUpgradeType::AltDamage,
		EUpgradeType::AltFireRate,
		EUpgradeType::AltSpreadShot,
		EUpgradeType::AltBulletRange,
		EUpgradeType::AltBulletSize,
		EUpgradeType::AltBulletSpeed,
		
		// Общие
		EUpgradeType::MaxHealth,
		EUpgradeType::Speed,
		EUpgradeType::ExperienceGain,
		
		// Новые улучшения
		EUpgradeType::HealthRegen,
		EUpgradeType::CriticalChance,
		EUpgradeType::DamageReduction,
		EUpgradeType::Luck,
		EUpgradeType::Lifesteal
	};

	// исключаем типы, которые уже есть у игрока
	for (auto it = allTypes.begin(); it != allTypes.end();)
	{
		bool shouldExclude = false;
		for (EUpgradeType excludeType : excludeTypes)
		{
			if (*it == excludeType)
			{
				shouldExclude = true;
				break;
			}
		}
		if (shouldExclude)
		{
			it = allTypes.erase(it);
		}
		else
		{
			++it;
		}
	}

	// если после исключения осталось меньше типов, чем нужно, возвращаем что есть
	if (allTypes.empty())
	{
		return result;
	}

	// перемешиваем типы используя Random() из VCL
	// Fisher-Yates shuffle
	for (int i = static_cast<int>(allTypes.size()) - 1; i > 0; i--)
	{
		const int j = Random(i + 1);
		std::swap(allTypes[i], allTypes[j]);
	}

	// генерируем улучшения с учетом редкости
	// шансы: Common 60%, Rare 30%, Legendary 10%
	for (int i = 0; i < count && i < static_cast<int>(allTypes.size()); i++)
	{
		EUpgradeRarity rarity;
		const int roll = Random(100);
		if (roll < 60)
			rarity = EUpgradeRarity::Common;
		else if (roll < 90)
			rarity = EUpgradeRarity::Rare;
		else
			rarity = EUpgradeRarity::Legendary;
		
		result.push_back(CreateUpgrade(allTypes[i], rarity));
	}

	return result;
}

//---------------------------------------------------------------------------

