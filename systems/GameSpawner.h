

#ifndef GameSpawnerH
#define GameSpawnerH

#include <System.Types.hpp>
#include <vector>
#include <memory>
#include "core\GameConstants.h"

class TEnemy;
class TBossEnemy;
class TExperienceOrb;

class TGameSpawner
{
private:
	float ScreenWidth;
	float ScreenHeight;
	float WorldWidth;
	float WorldHeight;

	float GroundExpSpawnTimer;
	static constexpr float GroundExpSpawnInterval = 2.0f;

	int LastBossWave;
	int BossAppearanceCount;

public:
	TGameSpawner();

	void SetScreenSize(float width, float height);
	void SetWorldSize(float width, float height);

	TPointF SpawnEnemyPosition(const TPointF &playerPosition) const;

	TPointF SpawnBossPosition() const;

	bool ShouldSpawnGroundExp(float deltaTime);
	TPointF SpawnGroundExpPosition() const;

	void OnBossSpawned(int waveNumber);
	bool ShouldSpawnBoss(int currentWave) const;
	int GetBossAppearanceCount() const { return BossAppearanceCount; }

	void Reset();
};

#endif
