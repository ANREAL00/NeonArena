#include <vcl.h>
#pragma hdrstop

#include "GameUpgrade.h"
#include <algorithm>
#include <System.SysUtils.hpp>


TUpgrade TUpgradeManager::CreateUpgrade(EUpgradeType type, EUpgradeRarity rarity)
{
	UnicodeString name, desc;
	float value = 0.0f;
	float commonValue = 0.0f, rareValue = 0.0f, legendaryValue = 0.0f;

	switch (type)
	{

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
		value = 1.0f;
		desc = L"Пули основной атаки пробивают врагов";
		break;
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
		commonValue = 1.0f; rareValue = 2.0f; legendaryValue = 3.0f;
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
	case EUpgradeType::MaxHealth:
		name = L"Здоровье";
		commonValue = 20.0f; rareValue = 30.0f; legendaryValue = 50.0f;
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
	case EUpgradeType::HealthRegen:
		name = L"Регенерация";
		commonValue = 2.0f; rareValue = 3.0f; legendaryValue = 5.0f;
		desc = L"Восстанавливает здоровье за пройденную волну";
		break;
	case EUpgradeType::CriticalChance:
		name = L"Критический удар";
		commonValue = 2.0f; rareValue = 4.0f; legendaryValue = 7.0f;
		desc = L"Шанс нанести двойной урон";
		break;
	case EUpgradeType::DamageReduction:
		name = L"Защита";
		commonValue = 5.0f; rareValue = 7.0f; legendaryValue = 10.0f;
		desc = L"Снижает получаемый урон";
		break;
	case EUpgradeType::Luck:
		name = L"Удача";
		commonValue = 5.0f; rareValue = 10.0f; legendaryValue = 15.0f;
		desc = L"Шанс получить двойной опыт";
		break;
	case EUpgradeType::Lifesteal:
		name = L"Вампиризм";
		commonValue = 1.0f; rareValue = 2.0f; legendaryValue = 3.0f;
		desc = L"Шанс восстановить 5 HP при убийстве врага";
		break;
	default:
		return TUpgrade(type, EUpgradeRarity::Common, L"Неизвестно", L"", 0.0f);
	}
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

std::vector<TUpgrade> TUpgradeManager::GenerateRandomUpgrades(int count, const std::vector<EUpgradeType> &excludeTypes)
{
	std::vector<TUpgrade> result;
	std::vector<EUpgradeType> allTypes = {

		EUpgradeType::Damage,
		EUpgradeType::FireRate,
		EUpgradeType::BulletRange,
		EUpgradeType::BulletSize,
		EUpgradeType::BulletSpeed,
		EUpgradeType::Pierce,

		EUpgradeType::AltDamage,
		EUpgradeType::AltFireRate,
		EUpgradeType::AltSpreadShot,
		EUpgradeType::AltBulletRange,
		EUpgradeType::AltBulletSize,
		EUpgradeType::AltBulletSpeed,

		EUpgradeType::MaxHealth,
		EUpgradeType::Speed,
		EUpgradeType::ExperienceGain,

		EUpgradeType::HealthRegen,
		EUpgradeType::CriticalChance,
		EUpgradeType::DamageReduction,
		EUpgradeType::Luck,
		EUpgradeType::Lifesteal
	};

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

	if (allTypes.empty())
	{
		return result;
	}

	for (int i = static_cast<int>(allTypes.size()) - 1; i > 0; i--)
	{
		const int j = Random(i + 1);
		std::swap(allTypes[i], allTypes[j]);
	}

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
