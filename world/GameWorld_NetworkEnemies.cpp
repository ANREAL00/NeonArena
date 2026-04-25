#include <vcl.h>
#pragma hdrstop

#include "GameWorld.h"
#include <algorithm>

using namespace NeonGame;

namespace
{
std::unique_ptr<TEnemy> CreateEnemyByType(uint8_t type, const TPointF &position)
{
	switch (type)
	{
		case 0: return std::make_unique<TBasicEnemy>(position);
		case 1: return std::make_unique<TFastEnemy>(position);
		case 2: return std::make_unique<TThrowerEnemy>(position);
		case 3: return std::make_unique<TZigzagEnemy>(position);
		case 4: return std::make_unique<TKamikazeEnemy>(position);
		case 5: return std::make_unique<TShootingEnemy>(position);
		default: return std::make_unique<TBasicEnemy>(position);
	}
}

uint8_t DetectEnemyType(const TEnemy *enemy)
{
	if (dynamic_cast<const TBasicEnemy*>(enemy)) return 0;
	if (dynamic_cast<const TFastEnemy*>(enemy)) return 1;
	if (dynamic_cast<const TThrowerEnemy*>(enemy)) return 2;
	if (dynamic_cast<const TZigzagEnemy*>(enemy)) return 3;
	if (dynamic_cast<const TKamikazeEnemy*>(enemy)) return 4;
	if (dynamic_cast<const TShootingEnemy*>(enemy)) return 5;
	return 0;
}
}

void TGameWorld::ApplyEnemyUpdates(const std::vector<TEnemyUpdatePacket> &updates)
{
	if (!IsNetworkGame || IsServer)
	{
		return;
	}

	if (updates.empty())
	{
		Enemies.clear();
		return;
	}

	size_t maxEnemyID = 0;
	for (const auto &update : updates)
	{
		maxEnemyID = std::max(maxEnemyID, static_cast<size_t>(update.EnemyID));
	}
	const size_t slotCount = maxEnemyID + 1;
	if (Enemies.size() < slotCount)
	{
		Enemies.resize(slotCount);
	}

	for (const auto &update : updates)
	{
		const size_t enemyID = static_cast<size_t>(update.EnemyID);
		if (enemyID >= Enemies.size())
			continue;

		if (!update.IsAlive)
		{
			if (Enemies[enemyID])
			{
				Enemies[enemyID].reset();
			}
			continue;
		}

		const bool needsCreate = !Enemies[enemyID] ||
			DetectEnemyType(Enemies[enemyID].get()) != update.EnemyType;
		if (needsCreate)
		{
			Enemies[enemyID] = CreateEnemyByType(update.EnemyType, TPointF(update.PositionX, update.PositionY));
		}

		Enemies[enemyID]->SetNetInstanceId(update.NetInstanceId);
		Enemies[enemyID]->SetPosition(TPointF(update.PositionX, update.PositionY));
		Enemies[enemyID]->SetHealth(update.Health);
	}

	
	

	if (Enemies.size() != slotCount)
	{
		Enemies.resize(slotCount);
	}
}

size_t TGameWorld::GetEnemySlotCount() const
{
	return Enemies.size();
}


TEnemy* TGameWorld::GetEnemyBySlot(size_t index)
{
	if (index >= Enemies.size())
		return nullptr;
	return Enemies[index].get();
}
