#include <vcl.h>
#pragma hdrstop

#include "GameWorld.h"
#include "core\GameConstants.h"

using namespace NeonGame;

float TGameWorld::GetPlayerHealthRatio() const
{
	const TGamePlayer* localPlayer = GetLocalPlayer();
	if (!localPlayer)
		return 0.0f;
	return localPlayer->GetHealthRatio();
}

int TGameWorld::GetPlayerHealth() const
{
	const TGamePlayer* localPlayer = GetLocalPlayer();
	if (!localPlayer)
		return 0;
	return localPlayer->GetHealth();
}

int TGameWorld::GetPlayerMaxHealth() const
{
	const TGamePlayer* localPlayer = GetLocalPlayer();
	if (!localPlayer)
		return 100;
	return localPlayer->GetMaxHealth();
}

float TGameWorld::GetPlayerExperienceRatio() const
{
	const TGamePlayer* localPlayer = GetLocalPlayer();
	if (!localPlayer)
		return 0.0f;
	return localPlayer->GetExperienceRatio();
}

int TGameWorld::GetPlayerExperience() const
{
	const TGamePlayer* localPlayer = GetLocalPlayer();
	if (!localPlayer)
		return 0;
	return localPlayer->GetExperience();
}

int TGameWorld::GetPlayerExperienceToNext() const
{
	const TGamePlayer* localPlayer = GetLocalPlayer();
	if (!localPlayer)
		return 100;
	return localPlayer->GetExperienceToNextLevel();
}

int TGameWorld::GetPlayerLevel() const
{
	const TGamePlayer* localPlayer = GetLocalPlayer();
	if (!localPlayer)
		return 1;
	return localPlayer->GetLevel();
}

bool TGameWorld::IsPlayerAlive() const
{
	const TGamePlayer* localPlayer = GetLocalPlayer();
	return localPlayer && localPlayer->IsAlive();
}

TPlayerStats TGameWorld::GetPlayerStats() const
{
	TPlayerStats stats;
	stats.DamageMultiplier = UpgradeManager.GetDamageMultiplier();
	stats.FireRateMultiplier = UpgradeManager.GetFireRateMultiplier();
	stats.BulletRangeMultiplier = UpgradeManager.GetBulletRangeMultiplier();
	stats.BulletSizeMultiplier = UpgradeManager.GetBulletSizeMultiplier();
	stats.BulletSpeedMultiplier = UpgradeManager.GetBulletSpeedMultiplier();
	stats.HasPierce = UpgradeManager.GetHasPierce();
	stats.AltDamageMultiplier = UpgradeManager.GetAltDamageMultiplier();
	stats.AltFireRateMultiplier = UpgradeManager.GetAltFireRateMultiplier();
	stats.AltSpreadShotCount = UpgradeManager.GetAltSpreadShotCount();
	stats.AltBulletRangeMultiplier = UpgradeManager.GetAltBulletRangeMultiplier();
	stats.AltBulletSizeMultiplier = UpgradeManager.GetAltBulletSizeMultiplier();
	stats.AltBulletSpeedMultiplier = UpgradeManager.GetAltBulletSpeedMultiplier();
	stats.SpeedMultiplier = UpgradeManager.GetSpeedMultiplier();
	stats.ExperienceGainMultiplier = UpgradeManager.GetExperienceGainMultiplier();

	stats.HealthRegenPerWave = UpgradeManager.GetHealthRegenPerWave();
	stats.CriticalChancePercent = UpgradeManager.GetCriticalChancePercent();
	stats.DamageReductionPercent = UpgradeManager.GetDamageReductionPercent();
	stats.LuckPercent = UpgradeManager.GetLuckPercent();
	stats.LifestealChancePercent = UpgradeManager.GetLifestealChancePercent();
	return stats;
}

float TGameWorld::GetBossHealthRatio() const
{
	if (!Boss || !Boss->IsAlive())
		return 0.0f;
	return Boss->GetHealthRatio();
}

int TGameWorld::GetBossHealth() const
{
	if (!Boss || !Boss->IsAlive())
		return 0;
	return Boss->GetHealth();
}

int TGameWorld::GetBossMaxHealth() const
{
	if (!Boss || !Boss->IsAlive())
		return 0;
	return Boss->GetMaxHealth();
}

uint8_t TGameWorld::GetBossPhase() const
{
	if (!Boss || !Boss->IsAlive())
		return 0;
	return static_cast<uint8_t>(Boss->GetPhase());
}

float TGameWorld::GetPrimaryFireCooldown() const
{
	if (LocalPlayerID < PlayerPrimaryFireTimers.size())
		return PlayerPrimaryFireTimers[LocalPlayerID];
	return PrimaryFireTimer;
}

float TGameWorld::GetAltFireCooldown() const
{
	if (LocalPlayerID < PlayerAltFireTimers.size())
		return PlayerAltFireTimers[LocalPlayerID];
	return AltFireTimer;
}

float TGameWorld::GetPrimaryFireMaxCooldown() const
{
	return PrimaryFireCooldown;
}

float TGameWorld::GetAltFireMaxCooldown() const
{
	return AltFireCooldown;
}

TPointF TGameWorld::GetPlayerPosition() const
{
	const TGamePlayer* localPlayer = GetLocalPlayer();
	if (!localPlayer)
		return TPointF(0.0f, 0.0f);
	return localPlayer->GetPosition();
}

std::vector<TPointF> TGameWorld::GetEnemyPositions() const
{
	std::vector<TPointF> positions;
	for (const auto &enemy : Enemies)
	{
		if (enemy && enemy->IsAlive())
		{
			positions.push_back(enemy->GetPosition());
		}
	}
	return positions;
}

TPointF TGameWorld::GetBossPosition() const
{
	if (!Boss || !Boss->IsAlive())
		return TPointF(0.0f, 0.0f);
	return Boss->GetPosition();
}

float TGameWorld::GetPlayerHealthRatio(uint8_t playerID) const
{
	const TGamePlayer* player = GetPlayer(playerID);
	if (!player)
		return 0.0f;
	return player->GetHealthRatio();
}

int TGameWorld::GetPlayerHealth(uint8_t playerID) const
{
	const TGamePlayer* player = GetPlayer(playerID);
	if (!player)
		return 0;
	return player->GetHealth();
}

int TGameWorld::GetPlayerMaxHealth(uint8_t playerID) const
{
	const TGamePlayer* player = GetPlayer(playerID);
	if (!player)
		return 100;
	return player->GetMaxHealth();
}

float TGameWorld::GetPlayerExperienceRatio(uint8_t playerID) const
{
	const TGamePlayer* player = GetPlayer(playerID);
	if (!player)
		return 0.0f;
	return player->GetExperienceRatio();
}

int TGameWorld::GetPlayerExperience(uint8_t playerID) const
{
	const TGamePlayer* player = GetPlayer(playerID);
	if (!player)
		return 0;
	return player->GetExperience();
}

int TGameWorld::GetPlayerExperienceToNext(uint8_t playerID) const
{
	const TGamePlayer* player = GetPlayer(playerID);
	if (!player)
		return 100;
	return player->GetExperienceToNextLevel();
}

int TGameWorld::GetPlayerLevel(uint8_t playerID) const
{
	const TGamePlayer* player = GetPlayer(playerID);
	if (!player)
		return 1;
	return player->GetLevel();
}

bool TGameWorld::IsPlayerAlive(uint8_t playerID) const
{
	const TGamePlayer* player = GetPlayer(playerID);
	return player && player->IsAlive();
}

TPointF TGameWorld::GetPlayerPosition(uint8_t playerID) const
{
	const TGamePlayer* player = GetPlayer(playerID);
	if (!player)
		return TPointF(0.0f, 0.0f);
	return player->GetPosition();
}

TPlayerStats TGameWorld::GetPlayerStats(uint8_t playerID) const
{
	if (!IsNetworkGame || playerID >= PlayerUpgradeManagers.size())
		return GetPlayerStats();

	const TUpgradeManager &mgr = PlayerUpgradeManagers[playerID];
	TPlayerStats stats;
	stats.DamageMultiplier = mgr.GetDamageMultiplier();
	stats.FireRateMultiplier = mgr.GetFireRateMultiplier();
	stats.BulletRangeMultiplier = mgr.GetBulletRangeMultiplier();
	stats.BulletSizeMultiplier = mgr.GetBulletSizeMultiplier();
	stats.BulletSpeedMultiplier = mgr.GetBulletSpeedMultiplier();
	stats.HasPierce = mgr.GetHasPierce();
	stats.AltDamageMultiplier = mgr.GetAltDamageMultiplier();
	stats.AltFireRateMultiplier = mgr.GetAltFireRateMultiplier();
	stats.AltSpreadShotCount = mgr.GetAltSpreadShotCount();
	stats.AltBulletRangeMultiplier = mgr.GetAltBulletRangeMultiplier();
	stats.AltBulletSizeMultiplier = mgr.GetAltBulletSizeMultiplier();
	stats.AltBulletSpeedMultiplier = mgr.GetAltBulletSpeedMultiplier();
	stats.SpeedMultiplier = mgr.GetSpeedMultiplier();
	stats.ExperienceGainMultiplier = mgr.GetExperienceGainMultiplier();
	stats.HealthRegenPerWave = mgr.GetHealthRegenPerWave();
	stats.CriticalChancePercent = mgr.GetCriticalChancePercent();
	stats.DamageReductionPercent = mgr.GetDamageReductionPercent();
	stats.LuckPercent = mgr.GetLuckPercent();
	stats.LifestealChancePercent = mgr.GetLifestealChancePercent();
	return stats;
}

float TGameWorld::GetPlayerSpeedMultiplier(uint8_t playerID) const
{
	if (IsNetworkGame)
	{
		if (playerID < PlayerUpgradeManagers.size())
			return PlayerUpgradeManagers[playerID].GetSpeedMultiplier();
		return 1.0f;
	}
	return UpgradeManager.GetSpeedMultiplier();
}

const std::vector<TUpgrade> &TGameWorld::GetAvailableUpgrades() const
{
	if (IsNetworkGame && LocalPlayerID < PlayerAvailableUpgrades.size())
	{
		return PlayerAvailableUpgrades[LocalPlayerID];
	}
	return AvailableUpgrades;
}

bool TGameWorld::IsWaitingForUpgradeChoice() const
{
	if (IsNetworkGame && LocalPlayerID < PlayerWaitingForUpgradeChoice.size())
	{
		return PlayerWaitingForUpgradeChoice[LocalPlayerID];
	}
	return WaitingForUpgradeChoice;
}

bool TGameWorld::IsPlayerWaitingForUpgradeChoice(uint8_t playerID) const
{
	if (!IsNetworkGame)
		return WaitingForUpgradeChoice;
	if (playerID < PlayerWaitingForUpgradeChoice.size())
		return PlayerWaitingForUpgradeChoice[playerID];
	return false;
}

const std::vector<TUpgrade> &TGameWorld::GetAvailableUpgradesForPlayer(uint8_t playerID) const
{
	if (IsNetworkGame)
	{
		if (playerID < PlayerAvailableUpgrades.size())
			return PlayerAvailableUpgrades[playerID];
		static const std::vector<TUpgrade> empty;
		return empty;
	}
	return AvailableUpgrades;
}

TPointF TGameWorld::GetCameraFollowPosition() const
{
	const TGamePlayer* local = GetLocalPlayer();
	if (local && local->IsAlive())
		return local->GetPosition();
	if (IsNetworkGame && WorldState != EWorldState::GameOver)
	{
		const TGamePlayer* t = GetPlayer(SpectateTargetPlayerID);
		if (t && t->IsAlive())
			return t->GetPosition();
	}
	return TPointF(WorldWidth * 0.5f, WorldHeight * 0.5f);
}

void TGameWorld::EnsureValidSpectateTarget()
{
	if (!IsNetworkGame || WorldState == EWorldState::GameOver)
		return;
	const TGamePlayer* local = GetLocalPlayer();
	if (local && local->IsAlive())
		return;
	const TGamePlayer* cur = GetPlayer(SpectateTargetPlayerID);
	if (cur && cur->IsAlive())
		return;
	for (size_t i = 0; i < Players.size(); ++i)
	{
		if (Players[i] && Players[i]->IsAlive())
		{
			SpectateTargetPlayerID = static_cast<uint8_t>(i);
			return;
		}
	}
}

void TGameWorld::CycleSpectateTarget(int direction)
{
	if (!IsNetworkGame || WorldState == EWorldState::GameOver || direction == 0)
		return;
	const TGamePlayer* local = GetLocalPlayer();
	if (local && local->IsAlive())
		return;

	std::vector<uint8_t> alive;
	alive.reserve(Players.size());
	for (size_t i = 0; i < Players.size(); ++i)
	{
		if (Players[i] && Players[i]->IsAlive())
			alive.push_back(static_cast<uint8_t>(i));
	}
	if (alive.size() <= 1)
	{
		if (!alive.empty())
			SpectateTargetPlayerID = alive[0];
		return;
	}

	int idx = 0;
	for (size_t k = 0; k < alive.size(); ++k)
	{
		if (alive[k] == SpectateTargetPlayerID)
		{
			idx = static_cast<int>(k);
			break;
		}
	}
	const int n = static_cast<int>(alive.size());
	idx = ((idx + direction) % n + n) % n;
	SpectateTargetPlayerID = alive[static_cast<size_t>(idx)];
}

void TGameWorld::AddCameraShake(float intensity, float duration)
{
	Camera.AddShake(intensity, duration);
}
