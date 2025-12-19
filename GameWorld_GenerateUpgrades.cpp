//---------------------------------------------------------------------------
void TGameWorld::GenerateUpgradesForPlayer(uint8_t playerID)
{
	if (!IsNetworkGame || playerID >= Players.size())
		return;
		
	if (PlayerWaitingForUpgradeChoice[playerID])
		return; // уже ждёт выбора
		
	// Генерируем улучшения для этого игрока
	std::vector<EUpgradeType> excludeTypes;
	if (PlayerUpgradeManagers[playerID].HasUpgrade(EUpgradeType::Pierce))
		excludeTypes.push_back(EUpgradeType::Pierce);
		
	PlayerAvailableUpgrades[playerID] = TUpgradeManager::GenerateRandomUpgrades(3, excludeTypes);
	PlayerWaitingForUpgradeChoice[playerID] = true;
}
//---------------------------------------------------------------------------
