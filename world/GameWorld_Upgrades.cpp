#include <vcl.h>
#pragma hdrstop

#include "GameWorld.h"

using namespace NeonGame;

void TGameWorld::SelectUpgrade(int index, uint8_t playerID)
{
	if (IsNetworkGame)
	{
		if (playerID >= PlayerAvailableUpgrades.size() || playerID >= PlayerUpgradeManagers.size())
			return;

		if (index < 0 || index >= static_cast<int>(PlayerAvailableUpgrades[playerID].size()))
			return;

		const TUpgrade &selected = PlayerAvailableUpgrades[playerID][index];
		PlayerUpgradeManagers[playerID].AddUpgrade(selected);

		TGamePlayer* player = GetPlayer(playerID);
		if (player)
		{
			player->ApplySpeedMultiplier(PlayerUpgradeManagers[playerID].GetSpeedMultiplier());
			player->ApplyMaxHealthBonus(PlayerUpgradeManagers[playerID].GetMaxHealthBonus());
		}

		PlayerAvailableUpgrades[playerID].clear();
		PlayerWaitingForUpgradeChoice[playerID] = false;

		if (playerID < PlayerPendingUpgradeChoices.size() && PlayerPendingUpgradeChoices[playerID] > 0)
		{
			PlayerPendingUpgradeChoices[playerID] -= 1;
			if (PlayerPendingUpgradeChoices[playerID] < 0)
				PlayerPendingUpgradeChoices[playerID] = 0;
		}

		EnsureUpgradeChoicePresented(playerID);
	}
	else
	{
		if (index < 0 || index >= static_cast<int>(AvailableUpgrades.size()))
			return;

		const TUpgrade &selected = AvailableUpgrades[index];
		UpgradeManager.AddUpgrade(selected);

		for (auto &player : Players)
		{
			if (player)
			{
				player->ApplySpeedMultiplier(UpgradeManager.GetSpeedMultiplier());
				player->ApplyMaxHealthBonus(UpgradeManager.GetMaxHealthBonus());
			}
		}

		AvailableUpgrades.clear();
		WaitingForUpgradeChoice = false;
		WorldState = EWorldState::Playing;

		if (WaveManager.GetState() == EWaveState::Completed)
		{
			WaveManager.StartNextWave();
		}
	}
}
