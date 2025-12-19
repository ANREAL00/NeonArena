#include <vcl.h>
#pragma hdrstop
//---------------------------------------------------------------------------

#include "GameSpawner.h"
#include "GameConstants.h"
#include <algorithm>
#include <cmath>
//---------------------------------------------------------------------------

TGameSpawner::TGameSpawner()
	: ScreenWidth(1920.0f),
	  ScreenHeight(1080.0f),
	  WorldWidth(NeonGame::WorldWidth),
	  WorldHeight(NeonGame::WorldHeight),
	  GroundExpSpawnTimer(0.0f),
	  LastBossWave(0),
	  BossAppearanceCount(0)
{
}
//---------------------------------------------------------------------------
void TGameSpawner::SetScreenSize(float width, float height)
{
	ScreenWidth = width;
	ScreenHeight = height;
}
//---------------------------------------------------------------------------
void TGameSpawner::SetWorldSize(float width, float height)
{
	WorldWidth = width;
	WorldHeight = height;
}
//---------------------------------------------------------------------------
TPointF TGameSpawner::SpawnEnemyPosition(const TPointF &playerPosition) const
{
	// Спавним врага за пределами видимой области игрока
	const float margin = 100.0f; // отступ от края экрана
	const float spawnDistance = std::max(ScreenWidth, ScreenHeight) / 2.0f + margin;
	
	// Случайная сторона экрана (0-3: верх, право, низ, лево)
	const int side = Random(4);
	float spawnX = 0.0f;
	float spawnY = 0.0f;
	
	switch (side)
	{
		case 0: // верх
			spawnX = playerPosition.X + (Random(static_cast<int>(ScreenWidth)) - ScreenWidth / 2.0f);
			spawnY = playerPosition.Y - spawnDistance;
			break;
		case 1: // право
			spawnX = playerPosition.X + spawnDistance;
			spawnY = playerPosition.Y + (Random(static_cast<int>(ScreenHeight)) - ScreenHeight / 2.0f);
			break;
		case 2: // низ
			spawnX = playerPosition.X + (Random(static_cast<int>(ScreenWidth)) - ScreenWidth / 2.0f);
			spawnY = playerPosition.Y + spawnDistance;
			break;
		case 3: // лево
			spawnX = playerPosition.X - spawnDistance;
			spawnY = playerPosition.Y + (Random(static_cast<int>(ScreenHeight)) - ScreenHeight / 2.0f);
			break;
	}
	
	// Ограничиваем границами мира
	spawnX = std::clamp(spawnX, 0.0f, WorldWidth);
	spawnY = std::clamp(spawnY, 0.0f, WorldHeight);
	
	return PointF(spawnX, spawnY);
}
//---------------------------------------------------------------------------
TPointF TGameSpawner::SpawnBossPosition() const
{
	// Босс спавнится в центре мира
	return PointF(WorldWidth / 2.0f, WorldHeight / 2.0f);
}
//---------------------------------------------------------------------------
bool TGameSpawner::ShouldSpawnGroundExp(float deltaTime)
{
	GroundExpSpawnTimer += deltaTime;
	if (GroundExpSpawnTimer >= GroundExpSpawnInterval)
	{
		GroundExpSpawnTimer = 0.0f;
		return true;
	}
	return false;
}
//---------------------------------------------------------------------------
TPointF TGameSpawner::SpawnGroundExpPosition() const
{
	// Случайная позиция в мире с отступом от краёв
	const float margin = 50.0f;
	const float x = margin + Random(static_cast<int>(WorldWidth - margin * 2));
	const float y = margin + Random(static_cast<int>(WorldHeight - margin * 2));
	return PointF(x, y);
}
//---------------------------------------------------------------------------
void TGameSpawner::OnBossSpawned(int waveNumber)
{
	LastBossWave = waveNumber;
	BossAppearanceCount++;
}
//---------------------------------------------------------------------------
bool TGameSpawner::ShouldSpawnBoss(int currentWave) const
{
	// Босс спавнится каждые 5 волн, начиная с 5-й
	return (currentWave > 0 && currentWave % 5 == 0 && currentWave != LastBossWave);
}
//---------------------------------------------------------------------------
void TGameSpawner::Reset()
{
	GroundExpSpawnTimer = 0.0f;
	LastBossWave = 0;
	BossAppearanceCount = 0;
}
//---------------------------------------------------------------------------



