//---------------------------------------------------------------------------
// Игровой мир (Neon Arena)
//---------------------------------------------------------------------------
#ifndef GameWorldH
#define GameWorldH
//---------------------------------------------------------------------------

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

//---------------------------------------------------------------------------
class TGameWorld
{
private:
	// Поддержка нескольких игроков (до 4)
	std::vector<std::unique_ptr<TGamePlayer>> Players; // массив игроков (0-3)
	uint8_t LocalPlayerID; // ID локального игрока (0-3)
	bool IsNetworkGame; // является ли игра сетевой
	bool IsServer; // является ли этот экземпляр сервером
	
	// Хранение входных данных для каждого игрока (для сервера)
	std::vector<TInputState> PlayerInputs; // индекс = playerID
	
	std::vector<TBullet> Bullets;
	std::vector<TBullet> EnemyBullets; // пули врагов
	std::vector<TThrownProjectile> ThrownProjectiles; // снаряды метателей
	std::vector<TAcidPool> AcidPools; // лужи кислоты
	std::vector<std::unique_ptr<TEnemy>> Enemies;
	uint32_t NextEnemyID; // счетчик для уникальных ID врагов
	std::unique_ptr<TBossEnemy> Boss; // босс (может быть только один)
	std::vector<TExperienceOrb> ExperienceOrbs;
	float PrimaryFireCooldown;
	float AltFireCooldown;

	TPoint MousePosScreen;
	bool HasMousePos;

	TGameCamera Camera;
	TGameStats Stats;
	EWorldState WorldState;
	TWaveManager WaveManager;
	TUpgradeManager UpgradeManager; // для обратной совместимости (одиночная игра)
	std::vector<TUpgradeManager> PlayerUpgradeManagers; // по одному на каждого игрока (для мультиплеера)
	TGameSpawner Spawner;

	// состояние выбора улучшения для каждого игрока
	std::vector<std::vector<TUpgrade>> PlayerAvailableUpgrades; // для каждого игрока свои варианты
	std::vector<bool> PlayerWaitingForUpgradeChoice; // для каждого игрока свой флаг
	
	// состояние выбора улучшения для одиночной игры (для обратной совместимости)
	std::vector<TUpgrade> AvailableUpgrades; // 3 варианта на выбор (одиночная игра)
	bool WaitingForUpgradeChoice; // флаг ожидания выбора (одиночная игра)

	// спавн опыта на земле
	float GroundExpSpawnTimer;
	static constexpr float GroundExpSpawnInterval = 2.0f; // каждые 2 секунды

	// визуальный фидбек level up
	float LevelUpNotificationTimer;
	int LastPlayerLevel;

	// размеры экрана для спавна врагов
	float ScreenWidth;
	float ScreenHeight;
	int LastBossWave; // последняя волна, на которой был босс
	int BossAppearanceCount; // количество появлений босса (для масштабирования)
	int LastRegenWave; // последняя волна, для которой была применена регенерация

	void UpdateShooting(float deltaTime, const TInputState &input, int canvasWidth, int canvasHeight);
	void UpdateShootingForPlayer(uint8_t playerID, float deltaTime, const TInputState &input, int canvasWidth, int canvasHeight);
	void UpdateEnemies(float deltaTime);
	void UpdateBoss(float deltaTime);
	void UpdateThrownProjectiles(float deltaTime);
	
	// Общий метод для завершения обновления мира (волны, враги, камера, коллизии)
	void FinalizeUpdate(float deltaTime, const TInputState &input, int canvasWidth, int canvasHeight);
	void UpdateAcidPools(float deltaTime);
	void UpdateExperienceOrbs(float deltaTime);
	void UpdateCollisions();
	void SpawnEnemy(); // спавн за пределами экрана игрока
	void SpawnBoss(); // спавн босса

public:
	// Вспомогательные методы для работы с игроками
	TGamePlayer* GetPlayer(uint8_t playerID);
	const TGamePlayer* GetPlayer(uint8_t playerID) const;
	TGamePlayer* GetLocalPlayer();
	const TGamePlayer* GetLocalPlayer() const;
	TGameWorld();
	void Reset();
	
	// Инициализация сетевой игры
	void InitializeNetworkGame(uint8_t localPlayerID, bool isServer);
	void SetPlayerCount(uint8_t count); // установить количество игроков (1-4)
	
	void Update(float deltaTime, const TInputState &input, int canvasWidth, int canvasHeight);
	void Update(float deltaTime, const std::vector<TInputState> &inputs, int canvasWidth, int canvasHeight); // для сервера с несколькими вводами
	void RenderScene(TCanvas *canvas);

	// геттеры
	EWorldState GetState() const { return WorldState; }
	const TGameStats &GetStats() const { return Stats; }
	void SetPlayerInput(uint8_t playerID, const TInputState &input); // Для сервера: установить ввод игрока
	
	// Геттеры для локального игрока (для обратной совместимости)
	float GetPlayerHealthRatio() const;
	int GetPlayerHealth() const;
	int GetPlayerMaxHealth() const;
	float GetPlayerExperienceRatio() const;
	int GetPlayerExperience() const;
	int GetPlayerExperienceToNext() const;
	int GetPlayerLevel() const;
	bool IsPlayerAlive() const;
	
	// Геттеры для конкретного игрока
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
	void SelectUpgrade(int index, uint8_t playerID = 0); // выбор улучшения по индексу (0-2) для конкретного игрока
	void GenerateUpgradesForPlayer(uint8_t playerID); // генерация улучшений для игрока (для клиентов)
	float GetLevelUpNotificationTimer() const { return LevelUpNotificationTimer; }
	int GetLastPlayerLevel() const { return LastPlayerLevel; }
	TPlayerStats GetPlayerStats() const;
	TPlayerStats GetPlayerStats(uint8_t playerID) const;
	bool HasBoss() const { return Boss != nullptr && Boss->IsAlive(); }
	float GetBossHealthRatio() const;
	int GetBossHealth() const;
	int GetBossMaxHealth() const;
	
	// для визуальных эффектов
	void AddCameraShake(float intensity, float duration);
	float GetPrimaryFireCooldown() const;
	float GetAltFireCooldown() const;
	
	// для мини-карты
	TPointF GetPlayerPosition() const;
	TPointF GetPlayerPosition(uint8_t playerID) const;
	std::vector<TPointF> GetEnemyPositions() const;
	TPointF GetBossPosition() const;
	bool HasActiveBoss() const { return Boss != nullptr && Boss->IsAlive(); }
	
	// геттеры для сущностей
	std::vector<std::unique_ptr<TEnemy>>& GetEnemies() { return Enemies; }
	TBossEnemy* GetBoss() { return Boss.get(); }
	
	// Сетевые методы
	uint8_t GetLocalPlayerID() const { return LocalPlayerID; }
	uint8_t GetPlayerCount() const { return static_cast<uint8_t>(Players.size()); }
	bool IsNetworkGameActive() const { return IsNetworkGame; }
	bool IsServerInstance() const { return IsServer; }
	
	// Получение состояния игры для синхронизации
	struct TGameStateSnapshot
	{
		uint32_t Tick;
		uint32_t WaveNumber;
		uint32_t EnemiesAlive;
		uint8_t WorldState; // добавлен для синхронизации состояния игры
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
			uint8_t Type; // тип врага
			uint32_t EnemyID; // уникальный ID
			float PositionX, PositionY;
			int32_t Health;
			bool IsAlive;
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

		struct TExperienceOrbState
		{
			float PositionX, PositionY;
			int32_t Value;
			bool IsActive;
		};
		std::vector<TExperienceOrbState> ExperienceOrbs;
	};
	
	TGameStateSnapshot GetGameStateSnapshot() const;
	void ApplyGameStateSnapshot(const TGameStateSnapshot &snapshot, float interpolationFactor = 0.0f);
};

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

