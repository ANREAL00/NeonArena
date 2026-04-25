#include <vcl.h>
#pragma hdrstop

#include "GameWorld.h"
#include <algorithm>
#include <cmath>

using namespace NeonGame;

void TGameWorld::SpawnExperienceOrb(const TPointF &pos, int expValue)
{
	uint32_t nid = 0u;
	if (IsNetworkGame && IsServer)
		nid = NextExpOrbNetInstanceId++;
	ExperienceOrbs.emplace_back(pos, expValue, nid);
}

std::vector<TExpOrbNetPacket> TGameWorld::GetExperienceOrbNetSnapshot() const
{
	std::vector<TExpOrbNetPacket> out;
	out.reserve(ExperienceOrbs.size());
	for (const auto &orb : ExperienceOrbs)
	{
		if (!orb.IsAlive())
			continue;
		TExpOrbNetPacket p{};
		p.NetInstanceId = orb.GetNetInstanceId();
		const TPointF pos = orb.GetPosition();
		p.PositionX = pos.X;
		p.PositionY = pos.Y;
		p.ExpValue = orb.GetExperienceValue();
		p.Lifetime = orb.GetLifetime();
		out.push_back(p);
	}
	return out;
}

void TGameWorld::ApplyExperienceOrbSnapshotClient(const std::vector<TExpOrbNetPacket> &orbs)
{
	if (!IsNetworkGame || IsServer)
		return;

	ExperienceOrbs.clear();
	ExperienceOrbs.reserve(orbs.size());
	for (const auto &o : orbs)
	{
		ExperienceOrbs.emplace_back(
			TPointF(o.PositionX, o.PositionY),
			o.ExpValue,
			o.NetInstanceId,
			o.Lifetime);
	}
}

void TGameWorld::UpdateExperienceOrbs(float deltaTime)
{
	if (IsNetworkGame && !IsServer)
		return;

	TGamePlayer *localPlayer = GetLocalPlayer();
	if (!IsNetworkGame)
	{
		if (!localPlayer)
			return;

		const TPointF playerPos = localPlayer->GetPosition();

		for (auto &orb : ExperienceOrbs)
			orb.Update(deltaTime, playerPos);

		ExperienceOrbs.erase(
			std::remove_if(ExperienceOrbs.begin(), ExperienceOrbs.end(),
				[this, playerPos, localPlayer](const TExperienceOrb &orb) {
					if (!orb.IsAlive())
						return true;

					if (orb.CheckPickup(playerPos, PlayerRadius))
					{
						int baseExp = orb.GetExperienceValue();
						baseExp = static_cast<int>(baseExp * UpgradeManager.GetExperienceGainMultiplier());

						const float luckChance = UpgradeManager.GetLuckPercent() / 100.0f;
						if (luckChance > 0.0f && (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) < luckChance)
							baseExp *= 2;

						if (localPlayer)
							localPlayer->AddExperience(baseExp);
						return true;
					}
					return false;
				}),
			ExperienceOrbs.end());
	}
	else
	{
		for (auto &orb : ExperienceOrbs)
		{
			TPointF attractPos = orb.GetPosition();
			float bestDistSq = 1.0e30f;
			for (size_t i = 0; i < Players.size(); ++i)
			{
				if (!Players[i] || !Players[i]->IsAlive())
					continue;
				const TPointF p = Players[i]->GetPosition();
				const float opx = orb.GetPosition().X;
				const float opy = orb.GetPosition().Y;
				const float dx = p.X - opx;
				const float dy = p.Y - opy;
				const float d2 = dx * dx + dy * dy;
				if (d2 < bestDistSq)
				{
					bestDistSq = d2;
					attractPos = p;
				}
			}
			orb.Update(deltaTime, attractPos);
		}

		ExperienceOrbs.erase(
			std::remove_if(ExperienceOrbs.begin(), ExperienceOrbs.end(),
				[this](const TExperienceOrb &orb) {
					if (!orb.IsAlive())
						return true;

					int bestIdx = -1;
					float bestPickDistSq = 1.0e30f;
					for (size_t i = 0; i < Players.size(); ++i)
					{
						if (!Players[i] || !Players[i]->IsAlive())
							continue;
						if (!orb.CheckPickup(Players[i]->GetPosition(), PlayerRadius))
							continue;
						const TPointF p = Players[i]->GetPosition();
						const float opx = orb.GetPosition().X;
						const float opy = orb.GetPosition().Y;
						const float dx = p.X - opx;
						const float dy = p.Y - opy;
						const float d2 = dx * dx + dy * dy;
						if (d2 < bestPickDistSq)
						{
							bestPickDistSq = d2;
							bestIdx = static_cast<int>(i);
						}
					}

					if (bestIdx < 0)
						return false;

					TGamePlayer *collector = Players[static_cast<size_t>(bestIdx)].get();
					const TUpgradeManager &upgrades =
						(static_cast<size_t>(bestIdx) < PlayerUpgradeManagers.size())
							? PlayerUpgradeManagers[static_cast<size_t>(bestIdx)]
							: UpgradeManager;

					int baseExp = orb.GetExperienceValue();
					baseExp = static_cast<int>(baseExp * upgrades.GetExperienceGainMultiplier());
					const float luckChance = upgrades.GetLuckPercent() / 100.0f;
					if (luckChance > 0.0f && (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) < luckChance)
						baseExp *= 2;

					if (collector)
						collector->AddExperience(baseExp);
					return true;
				}),
			ExperienceOrbs.end());
	}

	if (IsNetworkGame)
	{
		for (size_t i = 0; i < Players.size(); i++)
		{
			if (Players[i] && Players[i]->CheckLevelUp())
			{
				if (i == LocalPlayerID)
				{
					LevelUpNotificationTimer = 2.0f;
					LastPlayerLevel = Players[i]->GetLevel();
				}

				QueuePendingUpgradeChoice(static_cast<uint8_t>(i), 1);
			}
		}
	}
	else
	{
		if (localPlayer && localPlayer->CheckLevelUp())
		{
			LevelUpNotificationTimer = 2.0f;
			LastPlayerLevel = localPlayer->GetLevel();

			if (!WaitingForUpgradeChoice)
			{
				std::vector<EUpgradeType> excludeTypes;
				if (UpgradeManager.HasUpgrade(EUpgradeType::Pierce))
					excludeTypes.push_back(EUpgradeType::Pierce);
				AvailableUpgrades = TUpgradeManager::GenerateRandomUpgrades(3, excludeTypes);
				WaitingForUpgradeChoice = true;
				WorldState = EWorldState::ChoosingUpgrade;
			}
		}
	}

	if (LevelUpNotificationTimer > 0.0f)
	{
		LevelUpNotificationTimer -= deltaTime;
		if (LevelUpNotificationTimer < 0.0f)
			LevelUpNotificationTimer = 0.0f;
	}
}
