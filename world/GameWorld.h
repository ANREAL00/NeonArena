#ifndef GameWorldH
#define GameWorldH

#include <cstdint>

#include <System.Types.hpp>
#include <Vcl.Graphics.hpp>
#include <memory>
#include <vector>

#include "GamePlayer.h"
#include "GameProjectile.h"
#include "GameEnemy.h"
#include "GameConstants.h"
#include "GameUIRenderer.h"
#include "WaveManager.h"
#include "GameExperience.h"
#include "GameUpgrade.h"
#include "GameInput.h"
#include "GameCamera.h"
#include "GameState.h"
#include "GameCollision.h"
#include "GameSpawner.h"
#include "GameNetwork.h"


class TGameWorld
{
private:
	
	std::vector<std::unique_ptr<TGamePlayer>> Players; 
	uint8_t LocalPlayerID; 
	bool IsNetworkGame; 
	bool IsServer; 
	
	std::vector<TBullet> Bullets;
	std::vector<TBullet> EnemyBullets; 
	std::vector<TThrownProjectile> ThrownProjectiles; 
	std::vector<TAcidPool> AcidPools; 
	std::vector<std::unique_ptr<TEnemy>> Enemies;
	std::unique_ptr<TBossEnemy> Boss; 
	std::vector<NeonGame::TBulletNetPacket> ReplicatedBullets;
	std::vector<TExperienceOrb> ExperienceOrbs;
	float PrimaryFireCooldown;
	float AltFireCooldown;
	float PrimaryFireTimer;
	float AltFireTimer;
	std::vector<float> PlayerPrimaryFireTimers;
	std::vector<float> PlayerAltFireTimers;

	TPoint MousePosScreen;
	bool HasMousePos;
	TPointF AimDirection;

	TGameCamera Camera;
	TGameStats Stats;
	EWorldState WorldState;
	TWaveManager WaveManager;
	TUpgradeManager UpgradeManager; 
	std::vector<TUpgradeManager> PlayerUpgradeManagers; 
	TGameSpawner Spawner;

	
	std::vector<std::vector<TUpgrade>> PlayerAvailableUpgrades; 
	std::vector<bool> PlayerWaitingForUpgradeChoice; 
	std::vector<int> PlayerPendingUpgradeChoices;
	int LastProcessedWaveCompletion;
	
	
	std::vector<TUpgrade> AvailableUpgrades; 
	bool WaitingForUpgradeChoice; 

	
	float GroundExpSpawnTimer;
	static constexpr float GroundExpSpawnInterval = 2.0f; 

	
	float LevelUpNotificationTimer;
	int LastPlayerLevel;

	
	float ScreenWidth;
	float ScreenHeight;
	int LastBossWave; 
	int BossAppearanceCount; 
	int LastRegenWave;
	uint32_t NextEnemyNetInstanceId = 1;
	uint32_t NextExpOrbNetInstanceId = 1;
	uint8_t SpectateTargetPlayerID = 0;

	void UpdateShooting(float deltaTime, const TInputState &input, int canvasWidth, int canvasHeight);
	void UpdateShootingForPlayer(uint8_t playerID, float deltaTime, const TInputState &input, int canvasWidth, int canvasHeight);
	void UpdateEnemies(float deltaTime);
	void UpdateBoss(float deltaTime);
	void UpdateThrownProjectiles(float deltaTime);
	void UpdateAcidPools(float deltaTime);
	void UpdateExperienceOrbs(float deltaTime);
	void SpawnExperienceOrb(const TPointF &pos, int expValue);
	void GenerateUpgradesForPlayer(uint8_t playerID);
	void QueuePendingUpgradeChoice(uint8_t playerID, int count);
	void EnsureUpgradeChoicePresented(uint8_t playerID);
	bool IsAnyPlayerPendingUpgradeChoices() const;
	void UpdateCollisions();
	void SpawnEnemy(); 
	void SpawnBoss(); 

public:
	
	TGamePlayer* GetPlayer(uint8_t playerID);
	const TGamePlayer* GetPlayer(uint8_t playerID) const;
	TGamePlayer* GetLocalPlayer();
	const TGamePlayer* GetLocalPlayer() const;
	TGameWorld();
	void Reset();
	
	
	void InitializeNetworkGame(uint8_t localPlayerID, bool isServer);
	void LeaveNetworkMatchAndReset();
	void SetPlayerCount(uint8_t count); 
	
	void Update(float deltaTime, const TInputState &input, int canvasWidth, int canvasHeight);
	void Update(float deltaTime, const std::vector<TInputState> &inputs, int canvasWidth, int canvasHeight); 
	void RenderScene(TCanvas *canvas);

	
	EWorldState GetState() const { return WorldState; }
	const TGameStats &GetStats() const { return Stats; }
	
	
	float GetPlayerHealthRatio() const;
	int GetPlayerHealth() const;
	int GetPlayerMaxHealth() const;
	float GetPlayerExperienceRatio() const;
	int GetPlayerExperience() const;
	int GetPlayerExperienceToNext() const;
	int GetPlayerLevel() const;
	bool IsPlayerAlive() const;
	
	
	float GetPlayerHealthRatio(uint8_t playerID) const;
	int GetPlayerHealth(uint8_t playerID) const;
	int GetPlayerMaxHealth(uint8_t playerID) const;
	float GetPlayerExperienceRatio(uint8_t playerID) const;
	int GetPlayerExperience(uint8_t playerID) const;
	int GetPlayerExperienceToNext(uint8_t playerID) const;
	int GetPlayerLevel(uint8_t playerID) const;
	bool IsPlayerAlive(uint8_t playerID) const;
	
	TPointF GetCameraPosition() const { return Camera.GetBasePosition(); }
	const TWaveManager &GetWaveManager() const { return WaveManager; }
	const TUpgradeManager &GetUpgradeManager() const { return UpgradeManager; }
	const std::vector<TUpgrade> &GetAvailableUpgrades() const;
	bool IsWaitingForUpgradeChoice() const;
	bool IsPlayerWaitingForUpgradeChoice(uint8_t playerID) const;
	const std::vector<TUpgrade> &GetAvailableUpgradesForPlayer(uint8_t playerID) const;
	void SelectUpgrade(int index, uint8_t playerID = 0); 
	float GetLevelUpNotificationTimer() const { return LevelUpNotificationTimer; }
	int GetLastPlayerLevel() const { return LastPlayerLevel; }
	TPlayerStats GetPlayerStats() const;
	TPlayerStats GetPlayerStats(uint8_t playerID) const;
	bool HasBoss() const { return Boss != nullptr && Boss->IsAlive(); }
	float GetBossHealthRatio() const;
	int GetBossHealth() const;
	int GetBossMaxHealth() const;
	uint8_t GetBossPhase() const;
	
	
	void AddCameraShake(float intensity, float duration);
	float GetPrimaryFireCooldown() const;
	float GetAltFireCooldown() const;
	float GetPrimaryFireMaxCooldown() const;
	float GetAltFireMaxCooldown() const;
	
	
	TPointF GetPlayerPosition() const;
	TPointF GetPlayerPosition(uint8_t playerID) const;
	std::vector<TPointF> GetEnemyPositions() const;
	TPointF GetBossPosition() const;
	bool HasActiveBoss() const { return Boss != nullptr && Boss->IsAlive(); }
	float GetPlayerSpeedMultiplier(uint8_t playerID) const;
	
	
	uint8_t GetLocalPlayerID() const { return LocalPlayerID; }
	uint8_t GetPlayerCount() const { return static_cast<uint8_t>(Players.size()); }
	bool IsNetworkGameActive() const { return IsNetworkGame; }
	bool IsServerInstance() const { return IsServer; }

	TPointF GetCameraFollowPosition() const;
	void CycleSpectateTarget(int direction);
	void EnsureValidSpectateTarget();
	void TrySetNetworkClientGameOverIfAllDead();
	
	
	struct TGameStateSnapshot
	{
		uint32_t Tick; 
		uint32_t WaveNumber;
		uint32_t EnemiesAlive;
		struct TPlayerState
		{
			uint8_t PlayerID;
			float PositionX, PositionY;
			float FacingDirectionX, FacingDirectionY;
			int32_t Health, MaxHealth;
			int32_t Level;
			int32_t Experience;
			bool IsAlive;
		};
		std::vector<TPlayerState> Players;
		
		struct TEnemyState
		{
			uint8_t Type; 
			float PositionX, PositionY;
			int32_t Health;
			bool IsAlive;
			uint32_t NetInstanceId;
		};
		std::vector<TEnemyState> Enemies;
		
		struct TBulletState
		{
			float PositionX, PositionY;
			float VelocityX, VelocityY;
			bool IsPlayerBullet;
		};
		std::vector<TBulletState> Bullets;
		
		struct TBossState
		{
			float PositionX, PositionY;
			int32_t Health, MaxHealth;
			uint8_t Phase;
			bool IsAlive;
		};
		TBossState Boss;
	};
	
	TGameStateSnapshot GetGameStateSnapshot() const;
	void ApplyGameStateSnapshot(const TGameStateSnapshot &snapshot, float interpolationFactor = 0.0f);
	void ApplyEnemyUpdates(const std::vector<NeonGame::TEnemyUpdatePacket> &updates);
	void ApplyExperienceOrbSnapshotClient(const std::vector<NeonGame::TExpOrbNetPacket> &orbs);
	void ApplyBossUpdateClient(const NeonGame::TBossUpdatePacket &boss);
	void ApplyBulletSnapshotClient(const std::vector<NeonGame::TBulletNetPacket> &bullets);
	std::vector<NeonGame::TBulletNetPacket> GetBulletNetSnapshot() const;
	std::vector<NeonGame::TExpOrbNetPacket> GetExperienceOrbNetSnapshot() const;
	void ApplyWaveUpdate(uint32_t waveNumber, uint8_t waveState, float cooldownRemaining,
		float runTimeSeconds, bool includesRunTime);
	void ApplyUpgradeChoices(uint8_t playerID, bool isWaiting, const std::vector<NeonGame::TUpgradeChoiceNet> &choices);

	size_t GetEnemySlotCount() const;
	TEnemy* GetEnemyBySlot(size_t index);
};


#endif


