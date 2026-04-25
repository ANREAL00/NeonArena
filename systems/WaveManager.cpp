#include <vcl.h>
#pragma hdrstop

#include "WaveManager.h"
#include <algorithm>
#include <cmath>

TWaveManager::TWaveManager()
	: CurrentWave(0),
	  WaveState(EWaveState::Waiting),
	  WaveCooldownTimer(0.0f),
	  WaveActiveDuration(0.0f),
	  SpeedMultiplier(1.0f),
	  DamageMultiplier(1.0f),
	  IntermissionSeconds(5.0f)
{
	CurrentWaveConfig = TWaveConfig{};
}

void TWaveManager::Reset()
{
	CurrentWave = 0;
	WaveState = EWaveState::Waiting;
	WaveCooldownTimer = 0.0f;
	WaveActiveDuration = 0.0f;
	SpeedMultiplier = 1.0f;
	DamageMultiplier = 1.0f;
	IntermissionSeconds = 5.0f;
	CurrentWaveConfig = TWaveConfig{};
}

void TWaveManager::SetIntermissionSeconds(float seconds)
{
	if (seconds < 0.0f)
		seconds = 0.0f;
	IntermissionSeconds = seconds;
}

void TWaveManager::ApplyNetworkState(int waveNumber, uint8_t waveState, float cooldownRemaining)
{
	if (waveNumber < 0)
		waveNumber = 0;
	CurrentWave = waveNumber;
	WaveState = static_cast<EWaveState>(waveState);
	WaveCooldownTimer = cooldownRemaining;
}

void TWaveManager::CalculateWaveConfig(int waveNumber, TWaveConfig &config)
{
	config.WaveNumber = waveNumber;
	config.TotalEnemies = 6 + static_cast<int>(waveNumber * 2.0f + (waveNumber * waveNumber) * 0.4f);
	config.EnemiesSpawned = 0;
	config.SpawnInterval = std::max(0.15f, 1.0f - waveNumber * 0.07f);
	config.WaveStartDelay = (waveNumber == 1) ? 0.0f : 3.0f;
	config.TimeUntilNextSpawn = (waveNumber == 1) ? 0.0f : config.SpawnInterval;
}

void TWaveManager::Update(float deltaTime, int &enemiesAlive)
{
	switch (WaveState)
	{
		case EWaveState::Waiting:
		{
			
			WaveCooldownTimer -= deltaTime;
			if (WaveCooldownTimer <= 0.0f)
			{
				StartNextWave();
			}
			break;
		}

		case EWaveState::Spawning:
		{
			
			if (CurrentWaveConfig.WaveStartDelay > 0.0f)
			{
				CurrentWaveConfig.WaveStartDelay -= deltaTime;
				
			}

			
			if (CurrentWaveConfig.WaveStartDelay <= 0.0f)
			{
				CurrentWaveConfig.TimeUntilNextSpawn -= deltaTime;
				
				
				if (CurrentWaveConfig.EnemiesSpawned >= CurrentWaveConfig.TotalEnemies)
				{
					WaveState = EWaveState::Active;
					WaveActiveDuration = 0.0f; 
				}
			}
			break;
		}

		case EWaveState::Active:
		{
			WaveActiveDuration += deltaTime;
			
			
			if (WaveActiveDuration > WaveDurationThreshold)
			{
				const float excessTime = WaveActiveDuration - WaveDurationThreshold;
				
				SpeedMultiplier = 1.0f + (excessTime * MultiplierGrowthRate);
				DamageMultiplier = 1.0f + (excessTime * MultiplierGrowthRate);
			}
			else
			{
				SpeedMultiplier = 1.0f;
				DamageMultiplier = 1.0f;
			}
			
			
			if (enemiesAlive == 0 && CurrentWaveConfig.EnemiesSpawned >= CurrentWaveConfig.TotalEnemies)
			{
				WaveState = EWaveState::Completed;
				WaveCooldownTimer = IntermissionSeconds; 
				
				WaveActiveDuration = 0.0f;
				SpeedMultiplier = 1.0f;
				DamageMultiplier = 1.0f;
				
			}
			break;
		}

		case EWaveState::Completed:
		{
			WaveCooldownTimer -= deltaTime;
			if (WaveCooldownTimer <= 0.0f)
			{
				StartNextWave();
			}
			break;
		}
	}
}

bool TWaveManager::ShouldSpawnEnemy() const
{
	if (WaveState != EWaveState::Spawning && WaveState != EWaveState::Active)
		return false;

	if (CurrentWaveConfig.EnemiesSpawned >= CurrentWaveConfig.TotalEnemies)
		return false;

	if (CurrentWaveConfig.TimeUntilNextSpawn > 0.0f)
		return false;

	return true;
}

void TWaveManager::OnEnemySpawned()
{
	CurrentWaveConfig.EnemiesSpawned++;
	
	CurrentWaveConfig.TimeUntilNextSpawn = CurrentWaveConfig.SpawnInterval;
}

void TWaveManager::StartNextWave()
{
	CurrentWave++;
	CalculateWaveConfig(CurrentWave, CurrentWaveConfig);
	WaveState = EWaveState::Spawning;
	WaveCooldownTimer = 0.0f;
	
	WaveActiveDuration = 0.0f;
	SpeedMultiplier = 1.0f;
	DamageMultiplier = 1.0f;
}