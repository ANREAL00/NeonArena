

#ifndef WaveManagerH
#define WaveManagerH

#include <System.Types.hpp>
#include <vector>
#include <memory>

class TEnemy;

enum class EWaveState
{
	Waiting,
	Spawning,
	Active,
	Completed
};

struct TWaveConfig
{
	int WaveNumber;
	int TotalEnemies;
	int EnemiesSpawned;
	float SpawnInterval;
	float TimeUntilNextSpawn;
	float WaveStartDelay;
};

class TWaveManager
{
private:
	int CurrentWave;
	EWaveState WaveState;
	TWaveConfig CurrentWaveConfig;
	float WaveCooldownTimer;
	float WaveActiveDuration;
	float SpeedMultiplier;
	float DamageMultiplier;
	float IntermissionSeconds;
	static constexpr float WaveDurationThreshold = 15.0f;
	static constexpr float MultiplierGrowthRate = 0.02f;

	void CalculateWaveConfig(int waveNumber, TWaveConfig &config);

public:
	TWaveManager();
	void Reset();
	void Update(float deltaTime, int &enemiesAlive);
	bool ShouldSpawnEnemy() const;
	void OnEnemySpawned();
	void StartNextWave();
	void SetIntermissionSeconds(float seconds);
	float GetCooldownTimer() const { return WaveCooldownTimer; }
	void ApplyNetworkState(int waveNumber, uint8_t waveState, float cooldownRemaining);

	int GetCurrentWave() const { return CurrentWave; }
	EWaveState GetState() const { return WaveState; }
	const TWaveConfig &GetConfig() const { return CurrentWaveConfig; }
	bool IsWaveActive() const { return WaveState == EWaveState::Active || WaveState == EWaveState::Spawning; }
	float GetSpeedMultiplier() const { return SpeedMultiplier; }
	float GetDamageMultiplier() const { return DamageMultiplier; }
};

#endif
