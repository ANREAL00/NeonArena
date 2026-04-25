#include <vcl.h>
#pragma hdrstop

#include "GameWorld.h"

using namespace NeonGame;

void TGameWorld::GenerateUpgradesForPlayer(uint8_t playerID)
{
	if (!IsNetworkGame || playerID >= Players.size())
		return;
		
	if (PlayerWaitingForUpgradeChoice[playerID])
		return; 

	std::vector<EUpgradeType> excludeTypes;
	if (PlayerUpgradeManagers[playerID].HasUpgrade(EUpgradeType::Pierce))
		excludeTypes.push_back(EUpgradeType::Pierce);
		
	PlayerAvailableUpgrades[playerID] = TUpgradeManager::GenerateRandomUpgrades(3, excludeTypes);
	PlayerWaitingForUpgradeChoice[playerID] = true;
}

