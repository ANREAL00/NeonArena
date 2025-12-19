#include <vcl.h>
#pragma hdrstop
//---------------------------------------------------------------------------

#include "WaveManager.h"
#include <algorithm>
#include <cmath>
//---------------------------------------------------------------------------

TWaveManager::TWaveManager()
	: CurrentWave(0),
	  WaveState(EWaveState::Waiting),
	  WaveCooldownTimer(0.0f),
	  WaveActiveDuration(0.0f),
	  SpeedMultiplier(1.0f),
	  DamageMultiplier(1.0f)
{
	CurrentWaveConfig = TWaveConfig{};
}
//---------------------------------------------------------------------------
void TWaveManager::Reset()
{
	CurrentWave = 0;
	WaveState = EWaveState::Waiting;
	WaveCooldownTimer = 0.0f;
	WaveActiveDuration = 0.0f;
	SpeedMultiplier = 1.0f;
	DamageMultiplier = 1.0f;
	CurrentWaveConfig = TWaveConfig{};
}
//---------------------------------------------------------------------------
void TWaveManager::CalculateWaveConfig(int waveNumber, TWaveConfig &config)
{
	// базовая формула: увеличиваем количество врагов с каждой волной
	// Волна 1: 5 врагов
	// Волна 2: 8 врагов
	// Волна 3: 12 врагов
	// и т.д. (примерно +30-50% за волну)

	config.WaveNumber = waveNumber;
	// увеличиваем количество врагов быстрее для большей сложности
	config.TotalEnemies = 6 + static_cast<int>(waveNumber * 2.0f + (waveNumber * waveNumber) * 0.4f);
	config.EnemiesSpawned = 0;

	// интервал спавна: чем больше волна, тем быстрее спавнятся враги
	// Волна 1: 1.0 сек между врагами (быстрее)
	// Волна 5: 0.6 сек
	// Волна 10: 0.3 сек
	config.SpawnInterval = std::max(0.15f, 1.0f - waveNumber * 0.07f);
	
	// задержка перед началом волны (даёт игроку время подготовиться)
	config.WaveStartDelay = (waveNumber == 1) ? 0.0f : 3.0f;
	
	// для первой волны сразу можно спавнить, для остальных - после задержки
	config.TimeUntilNextSpawn = (waveNumber == 1) ? 0.0f : config.SpawnInterval;
}
//---------------------------------------------------------------------------
void TWaveManager::Update(float deltaTime, int &enemiesAlive)
{
	switch (WaveState)
	{
		case EWaveState::Waiting:
		{
			// пауза между волнами
			WaveCooldownTimer -= deltaTime;
			if (WaveCooldownTimer <= 0.0f)
			{
				StartNextWave();
			}
			break;
		}

		case EWaveState::Spawning:
		{
			// задержка перед началом спавна
			if (CurrentWaveConfig.WaveStartDelay > 0.0f)
			{
				CurrentWaveConfig.WaveStartDelay -= deltaTime;
				// не break, продолжаем дальше, чтобы уменьшать TimeUntilNextSpawn
			}

			// спавним врагов по таймеру (только если задержка прошла)
			if (CurrentWaveConfig.WaveStartDelay <= 0.0f)
			{
				CurrentWaveConfig.TimeUntilNextSpawn -= deltaTime;
				
				// проверяем, нужно ли ещё спавнить
				if (CurrentWaveConfig.EnemiesSpawned >= CurrentWaveConfig.TotalEnemies)
				{
					// все враги заспавнены, переходим в активную фазу
					WaveState = EWaveState::Active;
					WaveActiveDuration = 0.0f; // начинаем отсчет длительности активной фазы
				}
			}
			break;
		}

		case EWaveState::Active:
		{
			// отслеживаем длительность активной фазы волны
			WaveActiveDuration += deltaTime;
			
			// если волна длится слишком долго, увеличиваем множители
			if (WaveActiveDuration > WaveDurationThreshold)
			{
				const float excessTime = WaveActiveDuration - WaveDurationThreshold;
				// множители растут постепенно: +2% за каждую секунду после порога
				SpeedMultiplier = 1.0f + (excessTime * MultiplierGrowthRate);
				DamageMultiplier = 1.0f + (excessTime * MultiplierGrowthRate);
			}
			else
			{
				// если волна еще не превысила порог, множители остаются базовыми
				SpeedMultiplier = 1.0f;
				DamageMultiplier = 1.0f;
			}
			
			// проверяем, все ли враги убиты
			if (enemiesAlive == 0 && CurrentWaveConfig.EnemiesSpawned >= CurrentWaveConfig.TotalEnemies)
			{
				WaveState = EWaveState::Completed;
				WaveCooldownTimer = 7.0f; // пауза 7 секунд перед следующей волной
				// сбрасываем временные усиления
				WaveActiveDuration = 0.0f;
				SpeedMultiplier = 1.0f;
				DamageMultiplier = 1.0f;
				// НЕ инкрементируем CurrentWave здесь - это делается в StartNextWave()
			}
			break;
		}

		case EWaveState::Completed:
		{
			// завершена, ждём паузу
			WaveCooldownTimer -= deltaTime;
			if (WaveCooldownTimer <= 0.0f)
			{
				StartNextWave();
			}
			break;
		}
	}
}
//---------------------------------------------------------------------------
bool TWaveManager::ShouldSpawnEnemy() const
{
	// можно спавнить, если:
	// 1. Волна в состоянии Spawning или Active
	// 2. Ещё не все враги заспавнены
	// 3. Таймер до следующего спавна истёк
	if (WaveState != EWaveState::Spawning && WaveState != EWaveState::Active)
		return false;

	if (CurrentWaveConfig.EnemiesSpawned >= CurrentWaveConfig.TotalEnemies)
		return false;

	if (CurrentWaveConfig.TimeUntilNextSpawn > 0.0f)
		return false;

	return true;
}
//---------------------------------------------------------------------------
void TWaveManager::OnEnemySpawned()
{
	CurrentWaveConfig.EnemiesSpawned++;
	// сбрасываем таймер для следующего спавна
	CurrentWaveConfig.TimeUntilNextSpawn = CurrentWaveConfig.SpawnInterval;
}
//---------------------------------------------------------------------------
void TWaveManager::StartNextWave()
{
	CurrentWave++;
	CalculateWaveConfig(CurrentWave, CurrentWaveConfig);
	WaveState = EWaveState::Spawning;
	WaveCooldownTimer = 0.0f;
	// сбрасываем длительность и множители при начале новой волны
	WaveActiveDuration = 0.0f;
	SpeedMultiplier = 1.0f;
	DamageMultiplier = 1.0f;
}
//---------------------------------------------------------------------------

