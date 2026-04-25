#ifndef GameWorldUtilsH
#define GameWorldUtilsH

#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

#include "GameUpgrade.h"
#include "GamePlayer.h"

namespace NeonGameWorldUtils
{
	inline bool AreAllPlayersDead(const std::vector<std::unique_ptr<TGamePlayer>> &players)
	{
		for (const auto &player : players)
		{
			if (player && player->IsAlive())
				return false;
		}
		return true;
	}

	inline float GetLocalDamageReductionPercent(
		bool isNetworkGame,
		uint8_t localPlayerId,
		const std::vector<TUpgradeManager> &playerUpgradeManagers,
		const TUpgradeManager &upgradeManager)
	{
		if (isNetworkGame && localPlayerId < playerUpgradeManagers.size())
			return playerUpgradeManagers[localPlayerId].GetDamageReductionPercent();
		return upgradeManager.GetDamageReductionPercent();
	}

	inline int ApplyDamageReduction(int damage, float reductionPercent)
	{
		const float reduction = std::clamp(reductionPercent / 100.0f, 0.0f, 0.95f);
		return static_cast<int>(damage * (1.0f - reduction));
	}

	template <typename TVector, typename TPredicate>
	inline void EraseIf(TVector &v, TPredicate pred)
	{
		v.erase(std::remove_if(v.begin(), v.end(), pred), v.end());
	}

	inline std::vector<EUpgradeType> BuildExcludedUpgradeTypes(const TUpgradeManager &upgradeManager)
	{
		std::vector<EUpgradeType> excludeTypes;
		if (upgradeManager.HasUpgrade(EUpgradeType::Pierce))
		{
			excludeTypes.push_back(EUpgradeType::Pierce);
		}
		return excludeTypes;
	}
}

#endif

