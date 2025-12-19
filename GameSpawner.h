//---------------------------------------------------------------------------
// Система спавна (Neon Arena)
//---------------------------------------------------------------------------
#ifndef GameSpawnerH
#define GameSpawnerH
//---------------------------------------------------------------------------

#include <System.Types.hpp>
#include <vector>
#include <memory>
#include "GameConstants.h"

// Forward declarations
class TEnemy;
class TBossEnemy;
class TExperienceOrb;

//---------------------------------------------------------------------------
// Менеджер спавна игровых объектов
//---------------------------------------------------------------------------
class TGameSpawner
{
private:
	float ScreenWidth;
	float ScreenHeight;
	float WorldWidth;
	float WorldHeight;
	
	// Таймеры спавна
	float GroundExpSpawnTimer;
	static constexpr float GroundExpSpawnInterval = 2.0f; // каждые 2 секунды
	
	// Счётчики для боссов
	int LastBossWave;
	int BossAppearanceCount;

public:
	TGameSpawner();
	
	// Установка размеров экрана и мира
	void SetScreenSize(float width, float height);
	void SetWorldSize(float width, float height);
	
	// Спавн врага за пределами экрана игрока
	TPointF SpawnEnemyPosition(const TPointF &playerPosition) const;
	
	// Спавн босса
	TPointF SpawnBossPosition() const;
	
	// Спавн опыта на земле
	bool ShouldSpawnGroundExp(float deltaTime);
	TPointF SpawnGroundExpPosition() const;
	
	// Управление боссами
	void OnBossSpawned(int waveNumber);
	bool ShouldSpawnBoss(int currentWave) const;
	int GetBossAppearanceCount() const { return BossAppearanceCount; }
	
	// Сброс
	void Reset();
};

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------



