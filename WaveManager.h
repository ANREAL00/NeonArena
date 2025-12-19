//---------------------------------------------------------------------------
// Менеджер волн (Neon Arena)
//---------------------------------------------------------------------------
#ifndef WaveManagerH
#define WaveManagerH
//---------------------------------------------------------------------------

#include <System.Types.hpp>
#include <vector>
#include <memory>

class TEnemy;

//---------------------------------------------------------------------------
enum class EWaveState
{
	Waiting,      // пауза между волнами
	Spawning,     // идёт спавн врагов
	Active,       // волна активна, враги на карте
	Completed     // все враги убиты, волна завершена
};

//---------------------------------------------------------------------------
struct TWaveConfig
{
	int WaveNumber;
	int TotalEnemies;
	int EnemiesSpawned;
	float SpawnInterval;      // интервал между спавном врагов
	float TimeUntilNextSpawn; // таймер до следующего спавна
	float WaveStartDelay;     // задержка перед началом волны
};

//---------------------------------------------------------------------------
class TWaveManager
{
private:
	int CurrentWave;
	EWaveState WaveState;
	TWaveConfig CurrentWaveConfig;
	float WaveCooldownTimer;  // таймер паузы между волнами
	float WaveActiveDuration; // длительность активной фазы волны
	float SpeedMultiplier;    // временный множитель скорости врагов
	float DamageMultiplier;   // временный множитель урона врагов
	static constexpr float WaveDurationThreshold = 15.0f; // порог длительности волны (секунды)
	static constexpr float MultiplierGrowthRate = 0.02f;  // скорость роста множителей за секунду

	// расчёт параметров волны на основе номера
	void CalculateWaveConfig(int waveNumber, TWaveConfig &config);

public:
	TWaveManager();
	void Reset();
	void Update(float deltaTime, int &enemiesAlive);
	bool ShouldSpawnEnemy() const;
	void OnEnemySpawned();
	void StartNextWave();

	// геттеры
	int GetCurrentWave() const { return CurrentWave; }
	EWaveState GetState() const { return WaveState; }
	const TWaveConfig &GetConfig() const { return CurrentWaveConfig; }
	bool IsWaveActive() const { return WaveState == EWaveState::Active || WaveState == EWaveState::Spawning; }
	float GetSpeedMultiplier() const { return SpeedMultiplier; }
	float GetDamageMultiplier() const { return DamageMultiplier; }
};

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

