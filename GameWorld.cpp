#include <vcl.h>
#pragma hdrstop
//---------------------------------------------------------------------------

#include "GameWorld.h"
#include <System.SysUtils.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
//---------------------------------------------------------------------------

using namespace NeonGame;

TGameWorld::TGameWorld()
	: PrimaryFireCooldown(NeonGame::PrimaryFireCooldown), // используем константу
	  AltFireCooldown(NeonGame::AltFireCooldown),
	  HasMousePos(false),
	Stats{1, 0, 0.0f},
	WorldState(EWorldState::Playing),
	WaveManager(),
	LevelUpNotificationTimer(0.0f),
	LastPlayerLevel(1),
	LocalPlayerID(0),
	IsNetworkGame(false),
	IsServer(false),
	NextEnemyID(1)
{
	Reset();
}
//---------------------------------------------------------------------------
void TGameWorld::Reset()
{
	// Если это не сетевая игра, создаем одного игрока
	if (!IsNetworkGame)
	{
		Players.clear();
		Players.push_back(std::make_unique<TGamePlayer>());
		LocalPlayerID = 0;
		const float startX = WorldWidth / 2.0f;
		const float startY = WorldHeight / 2.0f;
		Players[0]->SetPosition(startX, startY);
	}
	else
	{
		// В сетевой игре создаём слоты только для подключенных игроков
		const size_t playerCount = GetPlayerCount();
		if (Players.size() != playerCount)
		{
			Players.clear();
			for (size_t i = 0; i < playerCount; i++)
			{
				Players.push_back(std::make_unique<TGamePlayer>());
			}
		}
		
		const float startX = WorldWidth / 2.0f;
		const float startY = WorldHeight / 2.0f;
		for (size_t i = 0; i < Players.size(); i++)
		{
			if (Players[i])
			{
				float offsetX = (i % 2) * 50.0f - 25.0f;
				float offsetY = (i / 2) * 50.0f - 25.0f;
				Players[i]->SetPosition(startX + offsetX, startY + offsetY);
				Players[i]->SetHealth(Players[i]->GetMaxHealth()); // Сбрасываем здоровье до макс
			}
		}
	}

	Bullets.clear();
	EnemyBullets.clear();
	ThrownProjectiles.clear();
	AcidPools.clear();
	Enemies.clear();
	Boss.reset();
	ExperienceOrbs.clear();

	HasMousePos = false;

	Stats.CurrentWave = 1;
	Stats.EnemiesDefeated = 0;
	Stats.RunTimeSeconds = 0.0f;

	WaveManager.Reset();
	WaveManager.StartNextWave(); // начинаем первую волну

	UpgradeManager.Reset();
	Spawner.Reset();
	AvailableUpgrades.clear();
	WaitingForUpgradeChoice = false;
	
	// Инициализируем улучшения для каждого игрока
	PlayerUpgradeManagers.clear();
	PlayerAvailableUpgrades.clear();
	PlayerWaitingForUpgradeChoice.clear();
	if (IsNetworkGame)
	{
		PlayerUpgradeManagers.resize(Players.size());
		PlayerAvailableUpgrades.resize(Players.size());
		PlayerWaitingForUpgradeChoice.resize(Players.size(), false);
		PlayerInputs.resize(Players.size());
		for (auto &upgradeMgr : PlayerUpgradeManagers)
		{
			upgradeMgr.Reset();
		}
	}

	GroundExpSpawnTimer = 0.0f;
	LevelUpNotificationTimer = 0.0f;
	LastPlayerLevel = 1;

	ScreenWidth = 1920.0f;
	ScreenHeight = 1080.0f;
	LastBossWave = 0;
	BossAppearanceCount = 0;
	LastRegenWave = 0;

	WorldState = EWorldState::Playing;
}

//---------------------------------------------------------------------------
void TGameWorld::InitializeNetworkGame(uint8_t localPlayerID, bool isServer)
{
	IsNetworkGame = true;
	LocalPlayerID = localPlayerID;
	IsServer = isServer;
	
	// Принудительно создаем слоты, если их нет
	if (Players.size() < 4)
	{
		// Сохраняем существующих игроков
		std::vector<std::unique_ptr<TGamePlayer>> oldPlayers = std::move(Players);
		Players.clear();
		for (int i = 0; i < 4; i++)
		{
			if (i < oldPlayers.size() && oldPlayers[i])
				Players.push_back(std::move(oldPlayers[i]));
			else
				Players.push_back(std::make_unique<TGamePlayer>());
		}
	}
	
	// Устанавливаем начальные позиции для всех игроков
	const float startX = WorldWidth / 2.0f;
	const float startY = WorldHeight / 2.0f;
	for (size_t i = 0; i < Players.size(); i++)
	{
		if (Players[i])
		{
			float offsetX = (i % 2) * 50.0f - 25.0f;
			float offsetY = (i / 2) * 50.0f - 25.0f;
			Players[i]->SetPosition(startX + offsetX, startY + offsetY);
			Players[i]->SetHealth(Players[i]->GetMaxHealth());
		}
	}
}

//---------------------------------------------------------------------------
void TGameWorld::SetPlayerCount(uint8_t count)
{
	if (count < 1 || count > 4)
		return;
	
	Players.clear();
	const float startX = WorldWidth / 2.0f;
	const float startY = WorldHeight / 2.0f;
	
	// Создаем игроков с небольшим смещением, чтобы они не спавнились в одной точке
	for (uint8_t i = 0; i < count; i++)
	{
		Players.push_back(std::make_unique<TGamePlayer>());
		float offsetX = (i % 2) * 50.0f - 25.0f;
		float offsetY = (i / 2) * 50.0f - 25.0f;
		Players[i]->SetPosition(startX + offsetX, startY + offsetY);
	}
	
	// Инициализируем улучшения для каждого игрока
	PlayerUpgradeManagers.resize(count);
	PlayerAvailableUpgrades.resize(count);
	PlayerWaitingForUpgradeChoice.resize(count, false);
	for (auto &upgradeMgr : PlayerUpgradeManagers)
	{
		upgradeMgr.Reset();
	}
}

//---------------------------------------------------------------------------
TGamePlayer* TGameWorld::GetPlayer(uint8_t playerID)
{
	if (playerID >= Players.size())
		return nullptr;
	return Players[playerID].get();
}

//---------------------------------------------------------------------------
const TGamePlayer* TGameWorld::GetPlayer(uint8_t playerID) const
{
	if (playerID >= Players.size())
		return nullptr;
	return Players[playerID].get();
}

//---------------------------------------------------------------------------
TGamePlayer* TGameWorld::GetLocalPlayer()
{
	return GetPlayer(LocalPlayerID);
}

//---------------------------------------------------------------------------
const TGamePlayer* TGameWorld::GetLocalPlayer() const
{
	return GetPlayer(LocalPlayerID);
}

//---------------------------------------------------------------------------
void TGameWorld::Update(float deltaTime, const TInputState &input, int canvasWidth, int canvasHeight)
{
	// Для обратной совместимости: если один игрок, используем его
	if (Players.empty())
		return;
	
	TGamePlayer* localPlayer = GetLocalPlayer();
	if (!localPlayer)
		return;

	// передаём ввод локальному игроку
	localPlayer->inputUp = input.MoveUp;
	localPlayer->inputDown = input.MoveDown;
	localPlayer->inputLeft = input.MoveLeft;
	localPlayer->inputRight = input.MoveRight;
	localPlayer->isShooting = input.PrimaryFire;

	// применяем модификаторы улучшений к игрокам
	if (IsNetworkGame)
	{
		// В сетевой игре каждый игрок имеет свой UpgradeManager
		for (size_t i = 0; i < Players.size(); i++)
		{
			if (Players[i] && i < PlayerUpgradeManagers.size())
			{
				Players[i]->ApplySpeedMultiplier(PlayerUpgradeManagers[i].GetSpeedMultiplier());
				Players[i]->ApplyMaxHealthBonus(PlayerUpgradeManagers[i].GetMaxHealthBonus());
			}
		}
	}
	else
	{
		// Одиночная игра - используем общий UpgradeManager
		localPlayer->ApplySpeedMultiplier(UpgradeManager.GetSpeedMultiplier());
		localPlayer->ApplyMaxHealthBonus(UpgradeManager.GetMaxHealthBonus());
	}

	// Обновляем локального игрока
	localPlayer->Update(deltaTime, WorldBounds());
	
	// Обновляем остальных игроков (для сетевой игры)
	for (size_t i = 0; i < Players.size(); i++)
	{
		if (i != LocalPlayerID && Players[i])
		{
			Players[i]->Update(deltaTime, WorldBounds());
		}
	}
	
	
	// Обновляем стрельбу для всех игроков (на сервере) или локального игрока (клиент/одиночная)
	if (IsNetworkGame && IsServer)
	{
		// Обрабатываем стрельбу для всех игроков на основе их входных данных
		for (size_t i = 0; i < Players.size(); i++)
		{
			if (Players[i] && Players[i]->IsAlive() && i < PlayerInputs.size())
			{
				UpdateShootingForPlayer(static_cast<uint8_t>(i), deltaTime, PlayerInputs[i], canvasWidth, canvasHeight);
			}
		}
	}
	else
	{
		// Обновляем стрельбу для локального игрока
		UpdateShooting(deltaTime, input, canvasWidth, canvasHeight);
	}

	// Завершаем обновление мира (общая логика для всех режимов)
	FinalizeUpdate(deltaTime, input, canvasWidth, canvasHeight);
}

//---------------------------------------------------------------------------
void TGameWorld::FinalizeUpdate(float deltaTime, const TInputState &input, int canvasWidth, int canvasHeight)
{
	// Проверяем, все ли игроки мертвы
	if (IsNetworkGame && WorldState != EWorldState::GameOver)
	{
		bool allDead = true;
		for (const auto &player : Players)
		{
			if (player && player->IsAlive())
			{
				allDead = false;
				break;
			}
		}
		if (allDead)
		{
			WorldState = EWorldState::GameOver;
		}
	}

	// если Game Over или выбор улучшения - не обновляем игру дальше
	if (WorldState == EWorldState::GameOver || WorldState == EWorldState::ChoosingUpgrade)
		return;

	// обновляем статистику
	Stats.RunTimeSeconds += deltaTime;
	Stats.CurrentWave = WaveManager.GetCurrentWave();

	// обновляем менеджер волн
	int enemiesAlive = static_cast<int>(Enemies.size());
	WaveManager.Update(deltaTime, enemiesAlive);

	// проверяем, завершена ли волна и нужно ли предложить улучшение
	if (WaveManager.GetState() == EWaveState::Completed)
	{
		// Возрождаем мертвых игроков после завершения волны с 5 HP
		if (IsNetworkGame)
		{
			for (size_t i = 0; i < Players.size(); i++)
			{
				if (Players[i] && !Players[i]->IsAlive())
				{
					Players[i]->Heal(5);
				}
			}
		}
		
		// Предлагаем улучшения
		if (IsNetworkGame)
		{
			bool anyPlayerWaiting = false;
			for (size_t i = 0; i < Players.size(); i++)
			{
				if (Players[i] && Players[i]->IsAlive() && !PlayerWaitingForUpgradeChoice[i])
				{
					// Подсчитываем общее количество улучшений: отложенные (за уровни) + 1 (за волну)
					const int totalUpgrades = Players[i]->GetPendingUpgradeCount() + 1;
					
					// Генерируем улучшения для каждого отложенного уровня + за волну
					std::vector<EUpgradeType> excludeTypes;
					if (PlayerUpgradeManagers[i].HasUpgrade(EUpgradeType::Pierce))
						excludeTypes.push_back(EUpgradeType::Pierce);
					
					// Генерируем все улучшения сразу (по 3 варианта на каждое)
					PlayerAvailableUpgrades[i] = TUpgradeManager::GenerateRandomUpgrades(3, excludeTypes);
					PlayerWaitingForUpgradeChoice[i] = true;
					anyPlayerWaiting = true;
					
					// Очищаем счётчик отложенных улучшений
					Players[i]->ClearPendingUpgrades();
				}
			}
			if (anyPlayerWaiting && WorldState != EWorldState::ChoosingUpgrade)
				WorldState = EWorldState::ChoosingUpgrade;
		}
		else
		{
			if (!WaitingForUpgradeChoice)
			{
				std::vector<EUpgradeType> excludeTypes;
				if (UpgradeManager.HasUpgrade(EUpgradeType::Pierce))
					excludeTypes.push_back(EUpgradeType::Pierce);
					
				AvailableUpgrades = TUpgradeManager::GenerateRandomUpgrades(3, excludeTypes);
				WaitingForUpgradeChoice = true;
				WorldState = EWorldState::ChoosingUpgrade;
			}
		}
	}

	// Обновляем размеры для спавнера
	Spawner.SetScreenSize(static_cast<float>(canvasWidth), static_cast<float>(canvasHeight));
	Spawner.SetWorldSize(WorldWidth, WorldHeight);
	
	// обновляем камеру (используем позицию локального игрока)
	const TPointF playerPos = GetLocalPlayer() ? GetLocalPlayer()->GetPosition() : TPointF(WorldWidth / 2.0f, WorldHeight / 2.0f);
	Camera.Update(deltaTime, playerPos, canvasWidth, canvasHeight, WorldWidth, WorldHeight);

	// обновляем пули игрока
	for (auto &b : Bullets)
		b.Update(deltaTime);
	Bullets.erase(
		std::remove_if(Bullets.begin(), Bullets.end(),
			[](const TBullet &b) { return !b.IsAlive(); }),
		Bullets.end());
	
	// обновляем пули врагов
	for (auto &b : EnemyBullets)
		b.Update(deltaTime);
	EnemyBullets.erase(
		std::remove_if(EnemyBullets.begin(), EnemyBullets.end(),
			[](const TBullet &b) { return !b.IsAlive(); }),
		EnemyBullets.end());

	// проверяем, нужно ли заспавнить босса (только для сервера или одиночной игры)
	const int currentWave = WaveManager.GetCurrentWave();
	if ((!IsNetworkGame || IsServer) && Spawner.ShouldSpawnBoss(currentWave) && (!Boss || !Boss->IsAlive()))
	{
		SpawnBoss();
		Spawner.OnBossSpawned(currentWave);
	}
	
	// спавн врагов через систему волн (только для сервера или одиночной игры)
	if ((!IsNetworkGame || IsServer) && (!Boss || !Boss->IsAlive()))
	{
		if (WaveManager.ShouldSpawnEnemy())
		{
			SpawnEnemy();
			WaveManager.OnEnemySpawned();
		}
	}

	// обновляем врагов, босса, снаряды и т.д.
	UpdateEnemies(deltaTime);
	if (Boss && Boss->IsAlive())
		UpdateBoss(deltaTime);
		
	for (auto &proj : ThrownProjectiles)
		proj.Update(deltaTime);
		
	UpdateAcidPools(deltaTime);
	UpdateExperienceOrbs(deltaTime);

	if ((!IsNetworkGame || IsServer) && Spawner.ShouldSpawnGroundExp(deltaTime))
	{
		const TPointF expPos = Spawner.SpawnGroundExpPosition();
		ExperienceOrbs.emplace_back(expPos, 5);
	}

	Stats.CurrentWave = WaveManager.GetCurrentWave();
	UpdateCollisions();
	UpdateThrownProjectiles(deltaTime);
}

void TGameWorld::UpdateShooting(float deltaTime, const TInputState &input, int canvasWidth, int canvasHeight)
{
	// Обновляем стрельбу для локального игрока
	UpdateShootingForPlayer(LocalPlayerID, deltaTime, input, canvasWidth, canvasHeight);
}

//---------------------------------------------------------------------------
void TGameWorld::UpdateShootingForPlayer(uint8_t playerID, float deltaTime, const TInputState &input, int canvasWidth, int canvasHeight)
{
	TGamePlayer* player = GetPlayer(playerID);
	if (!player)
		return;

	TPointF aimDir(0.0f, -1.0f);
	const TPointF playerPos = player->GetPosition();

	// получаем правильный менеджер улучшений
	TUpgradeManager& upgrades = (IsNetworkGame && playerID < PlayerUpgradeManagers.size())
		? PlayerUpgradeManagers[playerID]
		: UpgradeManager;

	// вычисляем направление на курсор
	if (input.HasMouse)
	{
		const float mouseX = static_cast<float>(input.MouseClient.x);
		const float mouseY = static_cast<float>(input.MouseClient.y);
		const TPointF cameraPos = Camera.GetBasePosition();
		const TPointF mouseWorld(mouseX + cameraPos.X, mouseY + cameraPos.Y);

		aimDir = PointF(mouseWorld.X - playerPos.X, mouseWorld.Y - playerPos.Y);
		float len = std::sqrt(aimDir.X * aimDir.X + aimDir.Y * aimDir.Y);
		if (len > 0.0001f)
		{
			aimDir.X /= len;
			aimDir.Y /= len;
		}
		else
		{
			aimDir = PointF(0.0f, -1.0f);
		}

		player->SetFacingDirection(aimDir);
	}
	else
	{
		aimDir = player->GetFacingDirection();
	}

	// PRIMARY FIRE
	const float effectiveFireCooldown = PrimaryFireCooldown * upgrades.GetFireRateMultiplier();
	if (input.PrimaryFire && input.HasMouse && player->GetPrimaryFireTimer() <= 0.0f)
	{
		const int baseDamage = 15;
		const int effectiveDamage = static_cast<int>(baseDamage * upgrades.GetDamageMultiplier());
		
		const float baseSpeed = 520.0f;
		const float baseRange = 200.0f;
		const float baseSize = 4.0f;
		
		const float effectiveSpeed = baseSpeed * (1.0f + upgrades.GetBulletSpeedMultiplier());
		const float effectiveRange = baseRange * (1.0f + upgrades.GetBulletRangeMultiplier());
		const float effectiveSize = baseSize * (1.0f + upgrades.GetBulletSizeMultiplier());
		
		Bullets.emplace_back(playerPos, aimDir, effectiveDamage, 
			effectiveSpeed, effectiveRange, effectiveSize, playerID);
		player->SetPrimaryFireTimer(effectiveFireCooldown);
	}

	// ALT FIRE
	const float effectiveAltFireCooldown = AltFireCooldown * upgrades.GetAltFireRateMultiplier();
	if (input.AltFire && input.HasMouse && player->GetAltFireTimer() <= 0.0f)
	{
		const int count = upgrades.GetAltSpreadShotCount();
		const float spread = 0.25f;
		const int baseDamage = 10;
		const int effectiveDamage = static_cast<int>(baseDamage * upgrades.GetAltDamageMultiplier());

		const float baseSpeed = 520.0f;
		const float baseRange = 120.0f;
		const float baseSize = 4.0f;
		
		const float effectiveSpeed = baseSpeed * (1.0f + upgrades.GetAltBulletSpeedMultiplier());
		const float effectiveRange = baseRange * (1.0f + upgrades.GetAltBulletRangeMultiplier());
		const float effectiveSize = baseSize * (1.0f + upgrades.GetAltBulletSizeMultiplier());

		for (int i = 0; i < count; ++i)
		{
			const float t = (count > 1) ? (static_cast<float>(i) / (count - 1) - 0.5f) : 0.0f;
			const float angleOffset = t * spread;

			const float baseAngle = std::atan2(aimDir.Y, aimDir.X);
			const float ang = baseAngle + angleOffset;
			TPointF dir(std::cos(ang), std::sin(ang));

			Bullets.emplace_back(playerPos, dir, effectiveDamage,
				effectiveSpeed, effectiveRange, effectiveSize, playerID);
		}

		player->SetAltFireTimer(effectiveAltFireCooldown);
	}
}
//---------------------------------------------------------------------------
void TGameWorld::UpdateEnemies(float deltaTime)
{
	// получаем временные множители из менеджера волн
	const float speedMultiplier = WaveManager.GetSpeedMultiplier();
	const float damageMultiplier = WaveManager.GetDamageMultiplier();
	
	for (auto &e : Enemies)
	{
		if (!e || !e->IsAlive())
			continue;

		// Находим ближайшего живого игрока
		TGamePlayer* targetPlayer = nullptr;
		float minDistanceSq = 1e10f;
		
		for (const auto &playerPtr : Players)
		{
			if (playerPtr && playerPtr->IsAlive())
			{
				TPointF pos = playerPtr->GetPosition();
				TPointF ePos = e->GetPosition();
				float dx = pos.X - ePos.X;
				float dy = pos.Y - ePos.Y;
				float distSq = dx * dx + dy * dy;
				if (distSq < minDistanceSq)
				{
					minDistanceSq = distSq;
					targetPlayer = playerPtr.get();
				}
			}
		}

		if (!targetPlayer)
			continue;

		const TPointF playerPos = targetPlayer->GetPosition();
		
		// применяем временный множитель скорости (если волна длится слишком долго)
		e->ApplyTemporarySpeedMultiplier(speedMultiplier);
			
		e->Update(deltaTime, playerPos);
		
		// проверяем, стреляет ли враг
		TShootingEnemy *shootingEnemy = dynamic_cast<TShootingEnemy*>(e.get());
		if (shootingEnemy)
		{
			if (shootingEnemy->GetShootTimer() <= 0.0f)
			{
				// предсказываем позицию игрока
				const TPointF enemyPos = shootingEnemy->GetPosition();
				const float distanceToPlayer = std::sqrt(
					(playerPos.X - enemyPos.X) * (playerPos.X - enemyPos.X) +
					(playerPos.Y - enemyPos.Y) * (playerPos.Y - enemyPos.Y));
				
				// создаём вражескую пулю
				const int wave = WaveManager.GetCurrentWave();
				int baseEnemyBulletDamage = 12; // увеличен базовый урон (было 8)
				float baseEnemyBulletSpeed = 350.0f; // увеличена скорость (было 300)
				
				// масштабируем урон и скорость пуль стреляющих врагов после 8 волны
				if (wave > 8)
				{
					const int scalingLevel = (wave - 8) / 2; // чаще масштабирование
					if (scalingLevel > 0)
					{
						baseEnemyBulletDamage = static_cast<int>(baseEnemyBulletDamage * (1.0f + scalingLevel * 0.25f));
						baseEnemyBulletSpeed *= (1.0f + scalingLevel * 0.20f);
					}
				}
				
				// применяем временный множитель урона (если волна длится слишком долго)
				baseEnemyBulletDamage = static_cast<int>(baseEnemyBulletDamage * damageMultiplier);
				
				// вычисляем время полета пули до игрока
				const float timeToReach = (distanceToPlayer > 0.001f && baseEnemyBulletSpeed > 0.001f) 
					? distanceToPlayer / baseEnemyBulletSpeed 
					: 0.0f;
				
				// предсказываем позицию игрока через время полета пули
				TPointF predictedPlayerPos = playerPos;
				if (timeToReach > 0.0f)
				{
					// проверяем, движется ли игрок сейчас (проверяем нажатые клавиши)
					const bool isMoving = targetPlayer->inputUp || targetPlayer->inputDown || targetPlayer->inputLeft || targetPlayer->inputRight;
					
					if (isMoving)
					{
						// если игрок движется, используем предсказание
						const TPointF playerMoveDir = targetPlayer->GetFacingDirection();
						const float moveDirLen = std::sqrt(playerMoveDir.X * playerMoveDir.X + playerMoveDir.Y * playerMoveDir.Y);
						
						// если направление валидное
						if (moveDirLen > 0.1f)
						{
							const float playerSpeed = targetPlayer->GetCurrentSpeed();
							// предсказываем смещение игрока
							const float predictionFactor = 0.6f; // коэффициент предсказания (можно настроить)
							predictedPlayerPos.X += playerMoveDir.X * playerSpeed * timeToReach * predictionFactor;
							predictedPlayerPos.Y += playerMoveDir.Y * playerSpeed * timeToReach * predictionFactor;
						}
					}
					// если игрок стоит (isMoving == false), стреляем в текущую позицию (predictedPlayerPos = playerPos)
				}
				
				// стреляем в предсказанную позицию (или текущую, если игрок стоит)
				TPointF dir(predictedPlayerPos.X - enemyPos.X, 
					predictedPlayerPos.Y - enemyPos.Y);
				const float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
				if (len > 0.001f)
				{
					dir.X /= len;
					dir.Y /= len;
					
					const float enemyBulletRange = 400.0f;
					const float enemyBulletSize = 3.0f;
					
					EnemyBullets.emplace_back(enemyPos, dir, 
						baseEnemyBulletDamage, baseEnemyBulletSpeed, enemyBulletRange, enemyBulletSize);
						
					shootingEnemy->ResetShootTimer();
				}
			}
		}
		
		// проверяем, метает ли враг снаряды
		TThrowerEnemy *throwerEnemy = dynamic_cast<TThrowerEnemy*>(e.get());
		if (throwerEnemy)
		{
			if (throwerEnemy->GetThrowTimer() <= 0.0f)
			{
				// метаем снаряд в игрока (предсказываем позицию)
				const TPointF &throwerPos = throwerEnemy->GetPosition();
				const float distance = std::sqrt((playerPos.X - throwerPos.X) * (playerPos.X - throwerPos.X) + 
					(playerPos.Y - throwerPos.Y) * (playerPos.Y - throwerPos.Y));
				
				// предсказываем позицию игрока (простое предсказание)
				const float throwTime = distance / 300.0f; // примерное время полета
				TPointF predictedPos = playerPos;
				
				const int wave = WaveManager.GetCurrentWave();
				int baseProjectileDamage = 20;
				
				// масштабируем урон снарядов метателей
				if (wave > 8)
				{
					const int scalingLevel = (wave - 8) / 2;
					if (scalingLevel > 0)
					{
						baseProjectileDamage = static_cast<int>(baseProjectileDamage * (1.0f + scalingLevel * 0.25f));
					}
				}
				
				// применяем временный множитель урона
				baseProjectileDamage = static_cast<int>(baseProjectileDamage * damageMultiplier);
				
				ThrownProjectiles.emplace_back(throwerPos, predictedPos, baseProjectileDamage, 300.0f);
				throwerEnemy->ResetThrowTimer();
			}
		}
	}
}
//---------------------------------------------------------------------------
void TGameWorld::UpdateCollisions()
{
	// коллизии вражеских пуль, луж и контактный урон для ВСЕХ игроков
	for (const auto &playerPtr : Players)
	{
		if (!playerPtr || !playerPtr->IsAlive())
			continue;

		TGamePlayer* player = playerPtr.get();
		const TPointF playerPos = player->GetPosition();
		const uint8_t pID = playerPtr == Players[LocalPlayerID] ? LocalPlayerID : 255; // Упрощенно для ID

		// В сетевой игре нам нужно знать реальный ID игрока для UpgradeManager
		// Найдем индекс игрока в векторе
		uint8_t realPlayerID = 0;
		for (uint8_t i = 0; i < Players.size(); i++) {
			if (Players[i].get() == player) {
				realPlayerID = i;
				break;
			}
		}

		TUpgradeManager& playerUpgrades = (IsNetworkGame && realPlayerID < PlayerUpgradeManagers.size())
			? PlayerUpgradeManagers[realPlayerID]
			: UpgradeManager;

		// 1. Коллизии вражеских пуль
		if (!player->IsInvulnerable())
		{
			for (auto &b : EnemyBullets)
			{
				if (!b.IsAlive() || b.IsUsed())
					continue;
				
				const TPointF &bp = b.GetPosition();
				const float bulletRadius = b.GetRadius();
				const float hitRadius = PlayerRadius + bulletRadius;
				const float hitRadiusSq = hitRadius * hitRadius;
				const float dx = bp.X - playerPos.X;
				const float dy = bp.Y - playerPos.Y;
				if (dx * dx + dy * dy <= hitRadiusSq)
				{
					int damage = b.GetDamage();
					float damageReduction = playerUpgrades.GetDamageReductionPercent() / 100.0f;
					damage = static_cast<int>(damage * (1.0f - damageReduction));
					player->TakeDamage(damage);
					b.MarkAsUsed();
					
					// шейк камеры только для локального игрока
					if (realPlayerID == LocalPlayerID)
						Camera.AddShake(5.0f, 0.15f);

					break; 
				}
			}
		}

		// 2. Коллизии луж кислоты
		if (!player->IsInvulnerable())
		{
			for (auto &pool : AcidPools)
			{
				if (!pool.IsAlive() || !pool.CanDealDamage())
					continue;
				
				const TPointF &poolPos = pool.GetPosition();
				const float poolRadius = pool.GetRadius();
				const float hitRadius = PlayerRadius + poolRadius;
				const float hitRadiusSq = hitRadius * hitRadius;
				const float dx = poolPos.X - playerPos.X;
				const float dy = poolPos.Y - playerPos.Y;
				if (dx * dx + dy * dy <= hitRadiusSq)
				{
					int damage = pool.GetDamage();
					float damageReduction = playerUpgrades.GetDamageReductionPercent() / 100.0f;
					damage = static_cast<int>(damage * (1.0f - damageReduction));
					player->TakeDamage(damage);
					pool.ResetDamageCooldown();

					if (realPlayerID == LocalPlayerID)
						Camera.AddShake(5.0f, 0.15f);
					break;
				}
			}
		}

		// 3. Контактный урон от врагов
		if (!player->IsInvulnerable())
		{
			const float contactRadius = PlayerRadius + EnemyRadius;
			const float contactRadiusSq = contactRadius * contactRadius;
			for (auto &e : Enemies)
			{
				if (!e || !e->IsAlive())
					continue;

				const TPointF &ep = e->GetPosition();
				const float dx = ep.X - playerPos.X;
				const float dy = ep.Y - playerPos.Y;
				if (dx * dx + dy * dy <= contactRadiusSq)
				{
					int damage = EnemyContactDamage;
					const float damageReduction = playerUpgrades.GetDamageReductionPercent() / 100.0f;
					damage = static_cast<int>(damage * (1.0f - damageReduction));
					player->TakeDamage(damage);
					
					if (realPlayerID == LocalPlayerID)
						Camera.AddShake(5.0f, 0.15f);
				}
			}

			// 4. Контактный урон от босса
			if (Boss && Boss->IsAlive())
			{
				const TPointF &bossPos = Boss->GetPosition();
				const float bossRadius = EnemyRadius * 2.5f; 
				const float bossContactRadius = PlayerRadius + bossRadius;
				const float bossContactRadiusSq = bossContactRadius * bossContactRadius;
				const float dx = bossPos.X - playerPos.X;
				const float dy = bossPos.Y - playerPos.Y;
				if (dx * dx + dy * dy <= bossContactRadiusSq)
				{
					int bossDamage = EnemyContactDamage * (2 + Spawner.GetBossAppearanceCount());
					const float damageReduction = playerUpgrades.GetDamageReductionPercent() / 100.0f;
					bossDamage = static_cast<int>(bossDamage * (1.0f - damageReduction));
					player->TakeDamage(bossDamage);
					if (!Boss->IsStunned())
						Boss->StartStun(0.25f);
					
					if (realPlayerID == LocalPlayerID)
						Camera.AddShake(6.0f, 0.2f);
				}
			}
		}

		// Проверяем Game Over (если все игроки мертвы)
		bool allDead = true;
		for (const auto &p : Players)
		{
			if (p && p->IsAlive())
			{
				allDead = false;
				break;
			}
		}
		if (allDead)
		{
			WorldState = EWorldState::GameOver;
			return;
		}
	}
	
	// коллизии пуль игрока с боссом
	if (Boss && Boss->IsAlive())
	{
		const TPointF &bossPos = Boss->GetPosition();
		const float bossRadius = EnemyRadius * 2.5f; // радиус босса
		
		for (auto &b : Bullets)
		{
			if (!b.IsAlive() || b.IsUsed())
				continue;
			
			const TPointF &bp = b.GetPosition();
			const float bulletRadius = b.GetRadius();
			const float hitRadius = bossRadius + bulletRadius;
			const float hitRadiusSq = hitRadius * hitRadius;
			const float dx = bp.X - bossPos.X;
			const float dy = bp.Y - bossPos.Y;
			if (dx * dx + dy * dy <= hitRadiusSq)
			{
				// получаем менеджер улучшений владельца пули
				const uint8_t ownerID = b.GetOwnerID();
				TUpgradeManager& ownerUpgrades = (IsNetworkGame && ownerID < PlayerUpgradeManagers.size())
					? PlayerUpgradeManagers[ownerID]
					: UpgradeManager;

				// проверяем критический удар
				int damage = b.GetDamage();
				const float critChance = ownerUpgrades.GetCriticalChancePercent() / 100.0f;
				if (critChance > 0.0f && (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) < critChance)
				{
					damage *= 2; 
				}
				Boss->ApplyDamage(damage);
				b.MarkAsUsed();
				
				// шейк камеры только если это локальный игрок или мы хост
				Camera.AddShake(3.0f, 0.1f);
				if (!Boss->IsAlive())
				{
					// проверяем вампиризм при убийстве босса для ВЛАДЕЛЬЦА пули
					const float lifestealChance = ownerUpgrades.GetLifestealChancePercent() / 100.0f;
					if (lifestealChance > 0.0f)
					{
						TGamePlayer* owner = GetPlayer(ownerID);
						if (owner && owner->IsAlive())
						{
							if ((static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) < lifestealChance)
							{
								owner->Heal(5);
							}
						}
					}
					
					// босс убит, создаём орб опыта
					const int expValue = 100;
					ExperienceOrbs.emplace_back(bossPos, expValue);
					Stats.EnemiesDefeated++;
					// сильный шейк при убийстве босса
					Camera.AddShake(8.0f, 0.3f);
				}
				// break; // Убираем break, чтобы несколько пуль могли попасть в босса в одном кадре
			}
		}
	}


	// коллизии пуля-враг
	const bool hasPierce = UpgradeManager.GetHasPierce();
	for (auto &e : Enemies)
	{
		if (!e || !e->IsAlive())
			continue;

		for (auto &b : Bullets)
		{
			if (!b.IsAlive() || b.IsUsed())
				continue;

			const TPointF &ep = e->GetPosition();
			const TPointF &bp = b.GetPosition();
			const float bulletRadius = b.GetRadius();
			const float hitRadius = EnemyRadius + bulletRadius;
			const float hitRadiusSq = hitRadius * hitRadius;
			const float dx = ep.X - bp.X;
			const float dy = ep.Y - bp.Y;
			if (dx * dx + dy * dy <= hitRadiusSq)
			{
				const uint8_t ownerID = b.GetOwnerID();
				TUpgradeManager& ownerUpgrades = (IsNetworkGame && ownerID < PlayerUpgradeManagers.size())
					? PlayerUpgradeManagers[ownerID]
					: UpgradeManager;

				int damage = b.GetDamage();
				const float critChance = ownerUpgrades.GetCriticalChancePercent() / 100.0f;
				if (critChance > 0.0f && (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) < critChance)
				{
					damage *= 2;
				}
				e->ApplyDamage(damage);
				
				// Проверяем вампиризм ПРИ ПОПАДАНИИ, если враг умер
				if (!e->IsAlive())
				{
					const float lifestealChance = ownerUpgrades.GetLifestealChancePercent() / 100.0f;
					if (lifestealChance > 0.0f)
					{
						TGamePlayer* owner = GetPlayer(ownerID);
						if (owner && owner->IsAlive())
						{
							if ((static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) < lifestealChance)
							{
								owner->Heal(1); // за обычного врага лечим на 1
							}
						}
					}
				}

				if (!ownerUpgrades.GetHasPierce())
				{
					b.MarkAsUsed();
					break;
				}
			}
		}
	}

	Enemies.erase(
		std::remove_if(Enemies.begin(), Enemies.end(),
			[this](const std::unique_ptr<TEnemy> &e)
			{
				if (!e)
					return true;
				if (!e->IsAlive())
				{
					Stats.EnemiesDefeated++;
					// Орб опыта падает для всех
					ExperienceOrbs.emplace_back(e->GetPosition(), e->GetBaseExperienceValue());
					return true;
				}
				return false;
			}),
		Enemies.end());
}
//---------------------------------------------------------------------------
void TGameWorld::SpawnEnemy()
{
	TGamePlayer* localPlayer = GetLocalPlayer();
	if (!localPlayer)
		return;

	// спавним врага за пределами экрана игрока
	const TPointF playerPos = localPlayer->GetPosition();
	
	// спавним на расстоянии от края экрана
	const float spawnOffset = 100.0f; // отступ от края экрана
	
	const int side = Random(4);
	TPointF pos;
	
	switch (side)
	{
		case 0: // слева от экрана
			pos = PointF(
				playerPos.X - ScreenWidth / 2.0f - spawnOffset,
				playerPos.Y + (Random(static_cast<int>(ScreenHeight)) - ScreenHeight / 2.0f));
			break;
		case 1: // справа от экрана
			pos = PointF(
				playerPos.X + ScreenWidth / 2.0f + spawnOffset,
				playerPos.Y + (Random(static_cast<int>(ScreenHeight)) - ScreenHeight / 2.0f));
			break;
		case 2: // сверху от экрана
			pos = PointF(
				playerPos.X + (Random(static_cast<int>(ScreenWidth)) - ScreenWidth / 2.0f),
				playerPos.Y - ScreenHeight / 2.0f - spawnOffset);
			break;
		default: // снизу от экрана
			pos = PointF(
				playerPos.X + (Random(static_cast<int>(ScreenWidth)) - ScreenWidth / 2.0f),
				playerPos.Y + ScreenHeight / 2.0f + spawnOffset);
			break;
	}
	
	// ограничиваем позицию границами мира
	pos.X = std::clamp(pos.X, 50.0f, WorldWidth - 50.0f);
	pos.Y = std::clamp(pos.Y, 50.0f, WorldHeight - 50.0f);

	// выбираем тип врага в зависимости от волны
	const int wave = WaveManager.GetCurrentWave();
	int enemyType = 0;
	
	if (wave < 3)
	{
		// волны 1-2: базовые, быстрые, метатели, телепортирующиеся, стреляющие
		enemyType = 0; // TBasicEnemy
	}
	else if (wave < 6)
	{
		// волны 3-5: обычные и желтые быстрые
		enemyType = Random(2); // 0 = TBasicEnemy, 1 = TFastEnemy
	}
	else if (wave < 8)
	{
		// волны 6-7: обычные, желтые быстрые, зеленые стреляющие и красные быстрые (камикадзе)
		const int rand = Random(4);
		if (rand == 0)
			enemyType = 0; // TBasicEnemy
		else if (rand == 1)
			enemyType = 1; // TFastEnemy
		else if (rand == 2)
			enemyType = 5; // TShootingEnemy
		else
			enemyType = 4; // TKamikazeEnemy (красные быстрые)
	}
	else if (wave < 11)
	{
		// волны 8-10: обычные, желтые быстрые, зеленые стреляющие, красные быстрые и телепортирующиеся
		const int rand = Random(5);
		if (rand == 0)
			enemyType = 0; // TBasicEnemy
		else if (rand == 1)
			enemyType = 1; // TFastEnemy
		else if (rand == 2)
			enemyType = 3; // TZigzagEnemy (телепортирующиеся)
		else if (rand == 3)
			enemyType = 4; // TKamikazeEnemy (красные быстрые)
		else
			enemyType = 5; // TShootingEnemy
	}
	else if (wave < 13)
	{
		// волны 11-12: все типы кроме камикадзе (или с камикадзе, но реже)
		const int rand = Random(5);
		if (rand == 0)
			enemyType = 0; // TBasicEnemy
		else if (rand == 1)
			enemyType = 1; // TFastEnemy
		else if (rand == 2)
			enemyType = 2; // TThrowerEnemy
		else if (rand == 3)
			enemyType = 3; // TZigzagEnemy (телепортирующиеся)
		else
			enemyType = 5; // TShootingEnemy
	}
	else
	{
		// волны 13+: все типы врагов
		// 0 = TBasicEnemy, 1 = TFastEnemy, 2 = TThrowerEnemy, 3 = TZigzagEnemy, 4 = TKamikazeEnemy, 5 = TShootingEnemy
		const int rand = Random(6);
		if (rand == 0)
			enemyType = 0; // TBasicEnemy
		else if (rand == 1)
			enemyType = 1; // TFastEnemy
		else if (rand == 2)
			enemyType = 2; // TThrowerEnemy
		else if (rand == 3)
			enemyType = 3; // TZigzagEnemy (телепортирующиеся)
		else if (rand == 4)
			enemyType = 4; // TKamikazeEnemy (красные быстрые)
		else
			enemyType = 5; // TShootingEnemy
	}
	
	// создаём врага
	std::unique_ptr<TEnemy> newEnemy;
	switch (enemyType)
	{
		case 0:
			newEnemy = std::make_unique<TBasicEnemy>(pos);
			break;
		case 1:
			newEnemy = std::make_unique<TFastEnemy>(pos);
			break;
		case 2:
			newEnemy = std::make_unique<TThrowerEnemy>(pos);
			break;
		case 3:
			newEnemy = std::make_unique<TZigzagEnemy>(pos);
			break;
		case 4:
			newEnemy = std::make_unique<TKamikazeEnemy>(pos);
			break;
		case 5:
			newEnemy = std::make_unique<TShootingEnemy>(pos);
			break;
		default:
			newEnemy = std::make_unique<TBasicEnemy>(pos);
			break;
	}
	
	// применяем более сильное масштабирование после 12 волны (каждые 4 волны)
	if (wave > 12)
	{
		const int scalingLevel = (wave - 12) / 4; // уровень усиления (0, 1, 2, 3...)
		if (scalingLevel > 0)
		{
			// увеличиваем HP на 25% за уровень, скорость на 15% за уровень
			const float healthMultiplier = 1.0f + (scalingLevel * 0.25f);
			const float speedMultiplier = 1.0f + (scalingLevel * 0.15f);
			newEnemy->ApplyScaling(healthMultiplier, speedMultiplier);
		}
	}
	
	newEnemy->SetEnemyID(NextEnemyID++);
	Enemies.push_back(std::move(newEnemy));
}
//---------------------------------------------------------------------------
void TGameWorld::SpawnBoss()
{
	const TPointF bossPos = Spawner.SpawnBossPosition();
	const int appearanceLevel = Spawner.GetBossAppearanceCount();
	Boss = std::make_unique<TBossEnemy>(bossPos, appearanceLevel);
	Boss->SetEnemyID(0); // у босса всегда ID 0 для простоты
}
//---------------------------------------------------------------------------
void TGameWorld::UpdateBoss(float deltaTime)
{
	TGamePlayer* localPlayer = GetLocalPlayer();
	if (!Boss || !Boss->IsAlive() || !localPlayer)
		return;

	const TPointF playerPos = localPlayer->GetPosition();
	
	// применяем временный множитель скорости к боссу (если волна длится слишком долго)
	const float speedMultiplier = WaveManager.GetSpeedMultiplier();
	Boss->ApplyTemporarySpeedMultiplier(speedMultiplier);
	
	Boss->Update(deltaTime, playerPos);
	
	// создаём пули босса с применением временного множителя урона
	const float damageMultiplier = WaveManager.GetDamageMultiplier();
	Boss->CreateBullets(playerPos, EnemyBullets, damageMultiplier);
	
	// если босс убит, создаём орб опыта
	if (!Boss->IsAlive())
	{
		// проверяем вампиризм при убийстве босса
		const float lifestealChance = UpgradeManager.GetLifestealChancePercent() / 100.0f;
		if (lifestealChance > 0.0f && localPlayer && localPlayer->IsAlive())
		{
			if ((static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) < lifestealChance)
			{
				localPlayer->Heal(5); // восстанавливаем 5 HP
			}
		}
		
		const int expValue = 100; // много опыта за босса
		ExperienceOrbs.emplace_back(Boss->GetPosition(), expValue);
		Stats.EnemiesDefeated++;
	}
}
//---------------------------------------------------------------------------
void TGameWorld::UpdateThrownProjectiles(float deltaTime)
{
	// создаем лужи для приземлившихся снарядов (только если не попали в игрока)
	TGamePlayer* localPlayer = GetLocalPlayer();
	if (localPlayer)
	{
		const TPointF playerPos = localPlayer->GetPosition();
		const float playerRadiusSq = PlayerRadius * PlayerRadius;
		
		for (auto &proj : ThrownProjectiles)
		{
		// если снаряд приземлился, создаем лужу (только один раз)
		// снаряды теперь не наносят урон при попадании, просто пролетают и приземляются
		if (proj.HasLanded() && !proj.WasPoolCreated())
		{
			// дополнительная проверка: убеждаемся, что лужа не создается в позиции игрока
			const TPointF landingPos = proj.GetLandingPosition();
			const float dx = landingPos.X - playerPos.X;
			const float dy = landingPos.Y - playerPos.Y;
			const float distSq = dx * dx + dy * dy;
			const float minDist = PlayerRadius + 30.0f; // минимальное расстояние от игрока (увеличено)
			
			// создаем лужу только если она не в позиции игрока и не слишком близко
			if (distSq > minDist * minDist)
			{
				AcidPools.emplace_back(landingPos, 40.0f, 5.0f, 10);
				proj.MarkPoolCreated();
			}
			else
			{
				// если лужа слишком близко к игроку, помечаем как созданную, но не создаем
				proj.MarkPoolCreated();
			}
		}
		}
	}
	
	// удаляем приземлившиеся или истекшие снаряды
	ThrownProjectiles.erase(
		std::remove_if(ThrownProjectiles.begin(), ThrownProjectiles.end(),
			[](const TThrownProjectile &p) { return !p.IsAlive() || p.HasLanded(); }),
		ThrownProjectiles.end());
}
//---------------------------------------------------------------------------
void TGameWorld::UpdateAcidPools(float deltaTime)
{
	// обновляем лужи
	for (auto &pool : AcidPools)
	{
		pool.Update(deltaTime);
	}
	
	// удаляем истекшие лужи
	AcidPools.erase(
		std::remove_if(AcidPools.begin(), AcidPools.end(),
			[](const TAcidPool &p) { return !p.IsAlive(); }),
		AcidPools.end());
}
//---------------------------------------------------------------------------
void TGameWorld::UpdateExperienceOrbs(float deltaTime)
{
	TGamePlayer* localPlayer = GetLocalPlayer();
	if (!localPlayer)
		return;

	const TPointF playerPos = localPlayer->GetPosition();

	// обновляем орбы
	for (auto &orb : ExperienceOrbs)
	{
		orb.Update(deltaTime, playerPos);
	}

	// удаляем собранные и истёкшие орбы, одновременно добавляя опыт
	ExperienceOrbs.erase(
		std::remove_if(ExperienceOrbs.begin(), ExperienceOrbs.end(),
			[this, playerPos, localPlayer](const TExperienceOrb &orb)
			{
				if (!orb.IsAlive())
					return true;
				// если орб был подобран
				if (orb.CheckPickup(playerPos, PlayerRadius))
				{
					int baseExp = orb.GetExperienceValue();
					// применяем множитель опыта
					baseExp = static_cast<int>(baseExp * UpgradeManager.GetExperienceGainMultiplier());
					
					// проверяем удачу (шанс получить двойной опыт)
					const float luckChance = UpgradeManager.GetLuckPercent() / 100.0f;
					if (luckChance > 0.0f && (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) < luckChance)
					{
						baseExp *= 2; // удача удваивает опыт
					}
					
					if (localPlayer)
					{
						localPlayer->AddExperience(baseExp);
					}
					return true;
				}
				return false;
			}),
		ExperienceOrbs.end());

	// проверяем прокачку для каждого игрока
	if (IsNetworkGame)
	{
		// В сетевой игре проверяем level up для каждого игрока отдельно
		for (size_t i = 0; i < Players.size(); i++)
		{
			if (Players[i] && Players[i]->CheckLevelUp())
			{
				// визуальный фидбек level up (только для локального игрока)
				if (i == LocalPlayerID)
				{
					LevelUpNotificationTimer = 2.0f; // показываем 2 секунды
					LastPlayerLevel = Players[i]->GetLevel();
				}
				
				// В кооперативе откладываем улучшения до конца волны
				Players[i]->IncrementPendingUpgrades();
			}
		}
	}
	else
	{
		// Одиночная игра
		if (localPlayer && localPlayer->CheckLevelUp())
		{
			// визуальный фидбек level up
			LevelUpNotificationTimer = 2.0f; // показываем 2 секунды
			LastPlayerLevel = localPlayer->GetLevel();

			// предлагаем улучшение при level up
			if (!WaitingForUpgradeChoice)
			{
				// собираем список уже имеющихся улучшений для исключения
				std::vector<EUpgradeType> excludeTypes;
				if (UpgradeManager.HasUpgrade(EUpgradeType::Pierce))
				{
					excludeTypes.push_back(EUpgradeType::Pierce);
				}
				AvailableUpgrades = TUpgradeManager::GenerateRandomUpgrades(3, excludeTypes);
				WaitingForUpgradeChoice = true;
				WorldState = EWorldState::ChoosingUpgrade;
			}
		}
	}

	// уменьшаем таймер уведомления
	if (LevelUpNotificationTimer > 0.0f)
	{
		LevelUpNotificationTimer -= deltaTime;
		if (LevelUpNotificationTimer < 0.0f)
			LevelUpNotificationTimer = 0.0f;
	}
}
//---------------------------------------------------------------------------
void TGameWorld::RenderScene(TCanvas *canvas)
{
	if (!canvas)
		return;

	// применяем шейк камеры
	TPointF camera = Camera.GetBasePosition();
	const TPointF shakeOffset = Camera.GetShakeOffset();
	camera.X += shakeOffset.X;
	camera.Y += shakeOffset.Y;

	const int canvasWidth = canvas->ClipRect.Width();
	const int canvasHeight = canvas->ClipRect.Height();

	// === ФОН С ПАТТЕРНАМИ ===
	// Базовый темный фон
	canvas->Brush->Color = static_cast<TColor>(RGB(5, 5, 15)); // очень темно-синий
	canvas->Brush->Style = bsSolid;
	canvas->FillRect(TRect(0, 0, canvasWidth, canvasHeight));

	// === ДЕКОРАТИВНЫЕ СТРУКТУРНЫЕ ЭЛЕМЕНТЫ ===
	// Платформы и блоки (сдержанные, не мешают геймплею)
	const int platformSpacing = 320;
	canvas->Pen->Color = static_cast<TColor>(RGB(15, 25, 50));
	canvas->Pen->Width = 2;
	canvas->Brush->Color = static_cast<TColor>(RGB(10, 15, 30));
	canvas->Brush->Style = bsSolid;
	
	for (float px = 0.0f; px <= WorldWidth; px += platformSpacing)
	{
		for (float py = 0.0f; py <= WorldHeight; py += platformSpacing)
		{
			// Смещение для разнообразия
			const float offsetX = (static_cast<int>(px / platformSpacing) % 3) * 40.0f;
			const float offsetY = (static_cast<int>(py / platformSpacing) % 3) * 40.0f;
			
			const float sx = px + offsetX - camera.X;
			const float sy = py + offsetY - camera.Y;
			
			if (sx > -100.0f && sx < canvasWidth + 100.0f && sy > -100.0f && sy < canvasHeight + 100.0f)
			{
				const float size = 60.0f;
				const int left = static_cast<int>(std::round(sx - size * 0.5f));
				const int top = static_cast<int>(std::round(sy - size * 0.5f));
				const int right = static_cast<int>(std::round(sx + size * 0.5f));
				const int bottom = static_cast<int>(std::round(sy + size * 0.5f));
				
				// Рисуем платформу
				canvas->Rectangle(left, top, right, bottom);
				
				// Неоновый контур
				canvas->Pen->Color = static_cast<TColor>(RGB(20, 40, 80));
				canvas->Rectangle(left - 1, top - 1, right + 1, bottom + 1);
				canvas->Pen->Color = static_cast<TColor>(RGB(15, 25, 50));
			}
		}
	}
	
	// === УЛУЧШЕННАЯ СЕТКА ===
	const int gridStep = 160;
	
	// Основные линии сетки (очень сдержанные)
	canvas->Pen->Color = static_cast<TColor>(RGB(8, 12, 25));
	canvas->Pen->Width = 1;
	
	for (float wx = 0.0f; wx <= WorldWidth; wx += gridStep)
	{
		const float sx = wx - camera.X;
		if (sx < 0.0f || sx > canvasWidth)
			continue;
		canvas->MoveTo(static_cast<int>(std::round(sx)), 0);
		canvas->LineTo(static_cast<int>(std::round(sx)), canvasHeight);
	}

	for (float wy = 0.0f; wy <= WorldHeight; wy += gridStep)
	{
		const float sy = wy - camera.Y;
		if (sy < 0.0f || sy > canvasHeight)
			continue;
		canvas->MoveTo(0, static_cast<int>(std::round(sy)));
		canvas->LineTo(canvasWidth, static_cast<int>(std::round(sy)));
	}
	
	// Вторичные линии (еще более тонкие)
	canvas->Pen->Color = static_cast<TColor>(RGB(5, 8, 15));
	canvas->Pen->Width = 1;
	
	for (float wx = gridStep * 0.5f; wx <= WorldWidth; wx += gridStep)
	{
		const float sx = wx - camera.X;
		if (sx < 0.0f || sx > canvasWidth)
			continue;
		canvas->MoveTo(static_cast<int>(std::round(sx)), 0);
		canvas->LineTo(static_cast<int>(std::round(sx)), canvasHeight);
	}

	for (float wy = gridStep * 0.5f; wy <= WorldHeight; wy += gridStep)
	{
		const float sy = wy - camera.Y;
		if (sy < 0.0f || sy > canvasHeight)
			continue;
		canvas->MoveTo(0, static_cast<int>(std::round(sy)));
		canvas->LineTo(canvasWidth, static_cast<int>(std::round(sy)));
	}
	
	// === ДЕКОРАТИВНЫЕ НЕОНОВЫЕ ЛИНИИ ===
	// Горизонтальные и вертикальные линии для структуры
	canvas->Pen->Color = static_cast<TColor>(RGB(15, 30, 60));
	canvas->Pen->Width = 1;
	
	const int lineSpacing = 480;
	for (float lx = 0.0f; lx <= WorldWidth; lx += lineSpacing)
	{
		const float sx = lx - camera.X;
		if (sx >= -10.0f && sx <= canvasWidth + 10.0f)
		{
			// Рисуем пунктирную линию
			for (int y = 0; y < canvasHeight; y += 20)
			{
				canvas->MoveTo(static_cast<int>(std::round(sx)), y);
				canvas->LineTo(static_cast<int>(std::round(sx)), y + 10);
			}
		}
	}
	
	for (float ly = 0.0f; ly <= WorldHeight; ly += lineSpacing)
	{
		const float sy = ly - camera.Y;
		if (sy >= -10.0f && sy <= canvasHeight + 10.0f)
		{
			// Рисуем пунктирную линию
			for (int x = 0; x < canvasWidth; x += 20)
			{
				canvas->MoveTo(x, static_cast<int>(std::round(sy)));
				canvas->LineTo(x + 10, static_cast<int>(std::round(sy)));
			}
		}
	}
	
	// === ТОЧКИ НА ПЕРЕСЕЧЕНИЯХ (ОЧЕНЬ СДЕРЖАННЫЕ) ===
	canvas->Brush->Color = static_cast<TColor>(RGB(12, 20, 40));
	canvas->Brush->Style = bsSolid;
	canvas->Pen->Color = static_cast<TColor>(RGB(18, 30, 55));
	canvas->Pen->Width = 1;
	
	for (float wx = 0.0f; wx <= WorldWidth; wx += gridStep)
	{
		for (float wy = 0.0f; wy <= WorldHeight; wy += gridStep)
		{
			const float sx = wx - camera.X;
			const float sy = wy - camera.Y;
			
			if (sx >= -5.0f && sx <= canvasWidth + 5.0f && sy >= -5.0f && sy <= canvasHeight + 5.0f)
			{
				const int pointSize = 2;
				const int left = static_cast<int>(std::round(sx - pointSize));
				const int top = static_cast<int>(std::round(sy - pointSize));
				const int right = static_cast<int>(std::round(sx + pointSize));
				const int bottom = static_cast<int>(std::round(sy + pointSize));
				canvas->Ellipse(left, top, right, bottom);
			}
		}
	}

	// рисуем лужи кислоты (под врагами, но над сеткой)
	for (const auto &pool : AcidPools)
	{
		if (pool.IsAlive())
		{
			pool.Draw(canvas, camera);
		}
	}
	
	// рисуем врагов (поверх луж)
	for (const auto &e : Enemies)
		if (e)
			e->Draw(canvas, camera);

	// рисуем орбы опыта
	for (const auto &orb : ExperienceOrbs)
		orb.Draw(canvas, camera);

	// рисуем пули игрока
	for (const auto &b : Bullets)
		b.Draw(canvas, camera);
	
	// рисуем пули врагов (другим цветом)
	for (const auto &b : EnemyBullets)
	{
		if (!b.IsAlive() || b.IsUsed())
			continue;
		
		// временно меняем цвет для вражеских пуль
		const float screenX = b.GetPosition().X - camera.X;
		const float screenY = b.GetPosition().Y - camera.Y;
		const float r = b.GetRadius();
		const int left = static_cast<int>(std::round(screenX - r));
		const int top = static_cast<int>(std::round(screenY - r));
		const int right = static_cast<int>(std::round(screenX + r));
		const int bottom = static_cast<int>(std::round(screenY + r));
		
		canvas->Brush->Color = static_cast<TColor>(RGB(255, 100, 100)); // красный
		canvas->Pen->Color = static_cast<TColor>(RGB(255, 150, 150));
		canvas->Pen->Width = 1;
		canvas->Ellipse(left, top, right, bottom);
	}
	
	// рисуем снаряды метателей
	for (const auto &proj : ThrownProjectiles)
	{
		if (proj.IsAlive() && !proj.HasLanded())
		{
			proj.Draw(canvas, camera);
		}
	}
	
	// рисуем босса
	if (Boss && Boss->IsAlive())
	{
		Boss->Draw(canvas, camera);
	}

	// рисуем игрока поверх
	// Рисуем всех игроков
	for (const auto &player : Players)
	{
		if (player)
		{
			player->Draw(canvas, camera);
		}
	}
	
	// применяем шейк камеры к позиции отрисовки
	// (шейк уже применён в UpdateCamera через ShakeOffset)
}
//---------------------------------------------------------------------------
float TGameWorld::GetPlayerHealthRatio() const
{
	const TGamePlayer* localPlayer = GetLocalPlayer();
	if (!localPlayer)
		return 0.0f;
	return localPlayer->GetHealthRatio();
}
//---------------------------------------------------------------------------
int TGameWorld::GetPlayerHealth() const
{
	const TGamePlayer* localPlayer = GetLocalPlayer();
	if (!localPlayer)
		return 0;
	return localPlayer->GetHealth();
}
//---------------------------------------------------------------------------
int TGameWorld::GetPlayerMaxHealth() const
{
	const TGamePlayer* localPlayer = GetLocalPlayer();
	if (!localPlayer)
		return 100;
	return localPlayer->GetMaxHealth();
}
//---------------------------------------------------------------------------
float TGameWorld::GetPlayerExperienceRatio() const
{
	const TGamePlayer* localPlayer = GetLocalPlayer();
	if (!localPlayer)
		return 0.0f;
	return localPlayer->GetExperienceRatio();
}
//---------------------------------------------------------------------------
int TGameWorld::GetPlayerExperience() const
{
	const TGamePlayer* localPlayer = GetLocalPlayer();
	if (!localPlayer)
		return 0;
	return localPlayer->GetExperience();
}
//---------------------------------------------------------------------------
int TGameWorld::GetPlayerExperienceToNext() const
{
	const TGamePlayer* localPlayer = GetLocalPlayer();
	if (!localPlayer)
		return 100;
	return localPlayer->GetExperienceToNextLevel();
}
//---------------------------------------------------------------------------
int TGameWorld::GetPlayerLevel() const
{
	const TGamePlayer* localPlayer = GetLocalPlayer();
	if (!localPlayer)
		return 1;
	return localPlayer->GetLevel();
}
//---------------------------------------------------------------------------
bool TGameWorld::IsPlayerAlive() const
{
	const TGamePlayer* localPlayer = GetLocalPlayer();
	return localPlayer && localPlayer->IsAlive();
}
//---------------------------------------------------------------------------
void TGameWorld::SelectUpgrade(int index, uint8_t playerID)
{
	if (IsNetworkGame)
	{
		// В сетевой игре каждый игрок выбирает улучшения независимо
		if (playerID >= PlayerAvailableUpgrades.size() || playerID >= PlayerUpgradeManagers.size())
			return;
		
		if (index < 0 || index >= static_cast<int>(PlayerAvailableUpgrades[playerID].size()))
			return;
		
		// добавляем выбранное улучшение для конкретного игрока
		const TUpgrade &selected = PlayerAvailableUpgrades[playerID][index];
		PlayerUpgradeManagers[playerID].AddUpgrade(selected);
		
		// применяем улучшения к конкретному игроку
		TGamePlayer* player = GetPlayer(playerID);
		if (player)
		{
			player->ApplySpeedMultiplier(PlayerUpgradeManagers[playerID].GetSpeedMultiplier());
			player->ApplyMaxHealthBonus(PlayerUpgradeManagers[playerID].GetMaxHealthBonus());
		}
		
		// очищаем доступные улучшения для этого игрока
		PlayerAvailableUpgrades[playerID].clear();
		PlayerWaitingForUpgradeChoice[playerID] = false;
		
		// Проверяем, все ли ЖИВЫЕ игроки выбрали улучшения
		bool allChose = true;
		for (size_t i = 0; i < PlayerWaitingForUpgradeChoice.size(); i++)
		{
			// Проверяем только живых игроков (пропускаем пустые слоты)
			if (Players[i] && Players[i]->IsAlive())
			{
				if (PlayerWaitingForUpgradeChoice[i])
				{
					allChose = false;
					break;
				}
			}
		}
		
		// Если все выбрали, продолжаем игру
		if (allChose && WorldState == EWorldState::ChoosingUpgrade)
		{
			WorldState = EWorldState::Playing;
			// начинаем следующую волну только если волна была завершена
			if (WaveManager.GetState() == EWaveState::Completed)
			{
				WaveManager.StartNextWave();
			}
		}
	}
	else
	{
		// Одиночная игра - используем старую логику
		if (index < 0 || index >= static_cast<int>(AvailableUpgrades.size()))
			return;

		// добавляем выбранное улучшение
		const TUpgrade &selected = AvailableUpgrades[index];
		UpgradeManager.AddUpgrade(selected);

		// применяем улучшения ко всем игрокам
		for (auto &player : Players)
		{
			if (player)
			{
				player->ApplySpeedMultiplier(UpgradeManager.GetSpeedMultiplier());
				player->ApplyMaxHealthBonus(UpgradeManager.GetMaxHealthBonus());
			}
		}

		// очищаем доступные улучшения и продолжаем игру
		AvailableUpgrades.clear();
		WaitingForUpgradeChoice = false;
		WorldState = EWorldState::Playing;

		// начинаем следующую волну только если волна была завершена
		// (не при level up во время активной волны)
		if (WaveManager.GetState() == EWaveState::Completed)
		{
			WaveManager.StartNextWave();
		}
	}
}
//---------------------------------------------------------------------------
void TGameWorld::GenerateUpgradesForPlayer(uint8_t playerID)
{
	if (!IsNetworkGame || playerID >= Players.size())
		return;
		
	if (PlayerWaitingForUpgradeChoice[playerID])
		return; // уже ждёт выбора
		
	// Генерируем улучшения для этого игрока
	std::vector<EUpgradeType> excludeTypes;
	if (PlayerUpgradeManagers[playerID].HasUpgrade(EUpgradeType::Pierce))
		excludeTypes.push_back(EUpgradeType::Pierce);
		
	PlayerAvailableUpgrades[playerID] = TUpgradeManager::GenerateRandomUpgrades(3, excludeTypes);
	PlayerWaitingForUpgradeChoice[playerID] = true;
}
//---------------------------------------------------------------------------
TPlayerStats TGameWorld::GetPlayerStats() const
{
	TPlayerStats stats;
	stats.DamageMultiplier = UpgradeManager.GetDamageMultiplier();
	stats.FireRateMultiplier = UpgradeManager.GetFireRateMultiplier();
	stats.BulletRangeMultiplier = UpgradeManager.GetBulletRangeMultiplier();
	stats.BulletSizeMultiplier = UpgradeManager.GetBulletSizeMultiplier();
	stats.BulletSpeedMultiplier = UpgradeManager.GetBulletSpeedMultiplier();
	stats.HasPierce = UpgradeManager.GetHasPierce();
	stats.AltDamageMultiplier = UpgradeManager.GetAltDamageMultiplier();
	stats.AltFireRateMultiplier = UpgradeManager.GetAltFireRateMultiplier();
	stats.AltSpreadShotCount = UpgradeManager.GetAltSpreadShotCount();
	stats.AltBulletRangeMultiplier = UpgradeManager.GetAltBulletRangeMultiplier();
	stats.AltBulletSizeMultiplier = UpgradeManager.GetAltBulletSizeMultiplier();
	stats.AltBulletSpeedMultiplier = UpgradeManager.GetAltBulletSpeedMultiplier();
	stats.SpeedMultiplier = UpgradeManager.GetSpeedMultiplier();
	stats.ExperienceGainMultiplier = UpgradeManager.GetExperienceGainMultiplier();
	// Новые улучшения
	stats.HealthRegenPerWave = UpgradeManager.GetHealthRegenPerWave();
	stats.CriticalChancePercent = UpgradeManager.GetCriticalChancePercent();
	stats.DamageReductionPercent = UpgradeManager.GetDamageReductionPercent();
	stats.LuckPercent = UpgradeManager.GetLuckPercent();
	stats.LifestealChancePercent = UpgradeManager.GetLifestealChancePercent();
	return stats;
}
//---------------------------------------------------------------------------
float TGameWorld::GetBossHealthRatio() const
{
	if (!Boss || !Boss->IsAlive())
		return 0.0f;
	return Boss->GetHealthRatio();
}
//---------------------------------------------------------------------------
int TGameWorld::GetBossHealth() const
{
	if (!Boss || !Boss->IsAlive())
		return 0;
	return Boss->GetHealth();
}
//---------------------------------------------------------------------------
int TGameWorld::GetBossMaxHealth() const
{
	if (!Boss || !Boss->IsAlive())
		return 0;
	return Boss->GetMaxHealth();
}
float TGameWorld::GetPrimaryFireCooldown() const
{
	const TGamePlayer* player = GetLocalPlayer();
	return player ? player->GetPrimaryFireTimer() : 0.0f;
}
//---------------------------------------------------------------------------
float TGameWorld::GetAltFireCooldown() const
{
	const TGamePlayer* player = GetLocalPlayer();
	return player ? player->GetAltFireTimer() : 0.0f;
}
//---------------------------------------------------------------------------
TPointF TGameWorld::GetPlayerPosition() const
{
	const TGamePlayer* localPlayer = GetLocalPlayer();
	if (!localPlayer)
		return TPointF(0.0f, 0.0f);
	return localPlayer->GetPosition();
}
//---------------------------------------------------------------------------
std::vector<TPointF> TGameWorld::GetEnemyPositions() const
{
	std::vector<TPointF> positions;
	for (const auto &enemy : Enemies)
	{
		if (enemy && enemy->IsAlive())
		{
			positions.push_back(enemy->GetPosition());
		}
	}
	return positions;
}
//---------------------------------------------------------------------------
TPointF TGameWorld::GetBossPosition() const
{
	if (!Boss || !Boss->IsAlive())
		return TPointF(0.0f, 0.0f);
	return Boss->GetPosition();
}

//---------------------------------------------------------------------------
// Перегрузки методов для работы с конкретным игроком
//---------------------------------------------------------------------------
float TGameWorld::GetPlayerHealthRatio(uint8_t playerID) const
{
	const TGamePlayer* player = GetPlayer(playerID);
	if (!player)
		return 0.0f;
	return player->GetHealthRatio();
}

int TGameWorld::GetPlayerHealth(uint8_t playerID) const
{
	const TGamePlayer* player = GetPlayer(playerID);
	if (!player)
		return 0;
	return player->GetHealth();
}

int TGameWorld::GetPlayerMaxHealth(uint8_t playerID) const
{
	const TGamePlayer* player = GetPlayer(playerID);
	if (!player)
		return 100;
	return player->GetMaxHealth();
}

float TGameWorld::GetPlayerExperienceRatio(uint8_t playerID) const
{
	const TGamePlayer* player = GetPlayer(playerID);
	if (!player)
		return 0.0f;
	return player->GetExperienceRatio();
}

int TGameWorld::GetPlayerExperience(uint8_t playerID) const
{
	const TGamePlayer* player = GetPlayer(playerID);
	if (!player)
		return 0;
	return player->GetExperience();
}

int TGameWorld::GetPlayerExperienceToNext(uint8_t playerID) const
{
	const TGamePlayer* player = GetPlayer(playerID);
	if (!player)
		return 100;
	return player->GetExperienceToNextLevel();
}

int TGameWorld::GetPlayerLevel(uint8_t playerID) const
{
	const TGamePlayer* player = GetPlayer(playerID);
	if (!player)
		return 1;
	return player->GetLevel();
}

bool TGameWorld::IsPlayerAlive(uint8_t playerID) const
{
	const TGamePlayer* player = GetPlayer(playerID);
	return player && player->IsAlive();
}

TPointF TGameWorld::GetPlayerPosition(uint8_t playerID) const
{
	const TGamePlayer* player = GetPlayer(playerID);
	if (!player)
		return TPointF(0.0f, 0.0f);
	return player->GetPosition();
}

TPlayerStats TGameWorld::GetPlayerStats(uint8_t playerID) const
{
	// Пока все игроки используют общий UpgradeManager
	// В будущем можно сделать индивидуальные улучшения
	return GetPlayerStats();
}

//---------------------------------------------------------------------------
// Методы для синхронизации состояния игры
//---------------------------------------------------------------------------
TGameWorld::TGameStateSnapshot TGameWorld::GetGameStateSnapshot() const
{
	TGameStateSnapshot snapshot;
	snapshot.Tick = 0; // будет установлен сервером
	snapshot.WaveNumber = WaveManager.GetCurrentWave();
	snapshot.EnemiesAlive = static_cast<uint32_t>(Enemies.size());
	snapshot.WorldState = static_cast<uint8_t>(WorldState);
	
	// Состояния игроков
	for (size_t i = 0; i < Players.size(); i++)
	{
		if (Players[i])
		{
			TGameStateSnapshot::TPlayerState playerState;
			playerState.PlayerID = static_cast<uint8_t>(i);
			TPointF pos = Players[i]->GetPosition();
			playerState.PositionX = pos.X;
			playerState.PositionY = pos.Y;
			TPointF facing = Players[i]->GetFacingDirection();
			playerState.FacingDirectionX = facing.X;
			playerState.FacingDirectionY = facing.Y;
			playerState.Health = Players[i]->GetHealth();
			playerState.MaxHealth = Players[i]->GetMaxHealth();
			playerState.Level = Players[i]->GetLevel();
			playerState.Experience = Players[i]->GetExperience();
			playerState.IsAlive = Players[i]->IsAlive();
			snapshot.Players.push_back(playerState);
		}
	}
	
	// Состояния врагов
	for (const auto &enemy : Enemies)
	{
		if (enemy && enemy->IsAlive())
		{
			TGameStateSnapshot::TEnemyState enemyState;
			enemyState.Type = static_cast<uint8_t>(enemy->GetType());
			enemyState.EnemyID = enemy->GetEnemyID();
			TPointF pos = enemy->GetPosition();
			enemyState.PositionX = pos.X;
			enemyState.PositionY = pos.Y;
			enemyState.Health = enemy->GetHealth();
			enemyState.IsAlive = true;
			snapshot.Enemies.push_back(enemyState);
		}
	}
	
	// Состояния сфер опыта
	for (const auto &orb : ExperienceOrbs)
	{
		if (orb.IsAlive())
		{
			TGameStateSnapshot::TExperienceOrbState orbState;
			TPointF orbPos = orb.GetPosition();
			orbState.PositionX = orbPos.X;
			orbState.PositionY = orbPos.Y;
			orbState.Value = orb.GetExperienceValue();
			orbState.IsActive = true;
			snapshot.ExperienceOrbs.push_back(orbState);
		}
	}
	
	// Состояния пуль
	// ... (синхронизация пуль пока упрощена)
	
	// Состояние босса
	if (Boss && Boss->IsAlive())
	{
		TPointF bossPos = Boss->GetPosition();
		snapshot.Boss.PositionX = bossPos.X;
		snapshot.Boss.PositionY = bossPos.Y;
		snapshot.Boss.Health = Boss->GetHealth();
		snapshot.Boss.MaxHealth = Boss->GetMaxHealth();
		snapshot.Boss.Phase = static_cast<uint8_t>(Boss->GetPhase());
		snapshot.Boss.IsAlive = true;
	}
	else
	{
		snapshot.Boss.IsAlive = false;
	}
	
	return snapshot;
}

//---------------------------------------------------------------------------
void TGameWorld::ApplyGameStateSnapshot(const TGameStateSnapshot &snapshot, float interpolationFactor)
{
	// Применяем общее состояние мира
	WorldState = static_cast<EWorldState>(snapshot.WorldState);
	
	// Применяем состояние волны
	if (snapshot.WaveNumber != WaveManager.GetCurrentWave())
	{
		// В простейшем случае просто устанавливаем номер волны
		// В полной версии нужно сбрасывать состояние WaveManager
	}
	
	// Применяем состояния игроков
	for (const auto &playerState : snapshot.Players)
	{
		uint8_t id = playerState.PlayerID;
		
		// Если вектор слишком мал, расширяем его
		if (id >= Players.size())
		{
			size_t oldSize = Players.size();
			Players.resize(id + 1);
			for (size_t i = oldSize; i <= id; i++)
			{
				if (!Players[i])
					Players[i] = std::make_unique<TGamePlayer>();
			}
		}
		
		if (!Players[id])
			Players[id] = std::make_unique<TGamePlayer>();
		
		// У старой версии игрока ID может отсутствовать, поэтому проверяем
		TGamePlayer* player = Players[id].get();
		
		// Проверяем на появление только что созданного "мертвого" игрока
		if (player->GetMaxHealth() <= 0)
			player->SetMaxHealth(playerState.MaxHealth > 0 ? playerState.MaxHealth : 100);

		// Если это НЕ локальный игрок, мы больше НЕ устанавливаем позицию здесь.
		// Веб-движок (InterpolateRemotePlayers) сделает это плавно.
		// Но мы все равно синхронизируем статы
		if (id == LocalPlayerID)
		{
			// Для локального игрока позиция устанавливается только если в ReceiveNetworkUpdates 
			// произошла значительная коррекция. Здесь ничего не трогаем.
		}
		else
		{
			// Позицию не трогаем — за нее отвечает интерполяция
		}
		
		player->SetFacingDirection(TPointF(playerState.FacingDirectionX, playerState.FacingDirectionY));
		
		// Синхронизируем здоровье, уровень, опыт
		player->SetHealth(playerState.Health);
		player->SetMaxHealth(playerState.MaxHealth);
		player->SetLevel(playerState.Level);
		player->SetExperience(playerState.Experience);
	}
	
	// Применяем состояния врагов через ID-matching
	std::vector<std::unique_ptr<TEnemy>> newEnemies;
	for (const auto &enemyState : snapshot.Enemies)
	{
		// Пытаемся найти существующего врага по ID
		bool found = false;
		for (size_t i = 0; i < Enemies.size(); i++)
		{
			if (Enemies[i] && Enemies[i]->GetEnemyID() == enemyState.EnemyID)
			{
				// Нашли — переносим его в новый список
				Enemies[i]->SetHealth(enemyState.Health);
				// Позицию пока не трогаем (будет интерполяция в MainForm)
				newEnemies.push_back(std::move(Enemies[i]));
				found = true;
				break;
			}
		}
		
		if (!found)
		{
			// Новый враг
			std::unique_ptr<TEnemy> enemy;
			TPointF pos(enemyState.PositionX, enemyState.PositionY);
			
			switch (static_cast<EEnemyType>(enemyState.Type))
			{
				case EEnemyType::Fast:     enemy = std::make_unique<TFastEnemy>(pos); break;
				case EEnemyType::Thrower:  enemy = std::make_unique<TThrowerEnemy>(pos); break;
				case EEnemyType::Zigzag:   enemy = std::make_unique<TZigzagEnemy>(pos); break;
				case EEnemyType::Kamikaze: enemy = std::make_unique<TKamikazeEnemy>(pos); break;
				case EEnemyType::Shooting: enemy = std::make_unique<TShootingEnemy>(pos); break;
				default:                   enemy = std::make_unique<TBasicEnemy>(pos); break;
			}
			
			if (enemy)
			{
				enemy->SetEnemyID(enemyState.EnemyID);
				enemy->SetHealth(enemyState.Health);
				newEnemies.push_back(std::move(enemy));
			}
		}
	}
	Enemies = std::move(newEnemies);
	
	// Применяем состояние босса
	if (snapshot.Boss.IsAlive)
	{
		if (!Boss)
		{
			// Создаем босса (упрощенно)
			Boss = std::make_unique<TBossEnemy>(TPointF(snapshot.Boss.PositionX, snapshot.Boss.PositionY));
			Boss->SetEnemyID(0);
		}
		// Позицию не трогаем — за нее отвечает интерполяция (ID 0)
		Boss->SetHealth(snapshot.Boss.Health);
		// Phase синхронизируется через пакет
	}
	else if (Boss)
	{
		Boss = nullptr;
	}

	// Применяем состояния сфер опыта
	ExperienceOrbs.clear();
	for (const auto &orbState : snapshot.ExperienceOrbs)
	{
		if (orbState.IsActive)
		{
			ExperienceOrbs.push_back(TExperienceOrb(TPointF(orbState.PositionX, orbState.PositionY), orbState.Value));
		}
	}
}

//---------------------------------------------------------------------------
// Обновление для сервера с несколькими вводами
//---------------------------------------------------------------------------
void TGameWorld::Update(float deltaTime, const std::vector<TInputState> &inputs, int canvasWidth, int canvasHeight)
{
	// Применяем вводы к соответствующим игрокам
	for (size_t i = 0; i < inputs.size() && i < Players.size(); i++)
	{
		if (!Players[i])
			continue;
		
		const TInputState &input = inputs[i];
		Players[i]->inputUp = input.MoveUp;
		Players[i]->inputDown = input.MoveDown;
		Players[i]->inputLeft = input.MoveLeft;
		Players[i]->inputRight = input.MoveRight;
		Players[i]->isShooting = input.PrimaryFire;
		
		// Применяем модификаторы в зависимости от UpgradeManager игрока
		if (i < PlayerUpgradeManagers.size())
		{
			Players[i]->ApplySpeedMultiplier(PlayerUpgradeManagers[i].GetSpeedMultiplier());
			Players[i]->ApplyMaxHealthBonus(PlayerUpgradeManagers[i].GetMaxHealthBonus());
		}
		
		// Обновляем игрока
		Players[i]->Update(deltaTime, WorldBounds());
		
		// Обновляем стрельбу для этого игрока
		UpdateShootingForPlayer(static_cast<uint8_t>(i), deltaTime, input, canvasWidth, canvasHeight);
	}
	
	// Завершаем обновление мира (волны, враги, камера, коллизии)
	// В качестве input передаем ввод хоста (ID 0)
	FinalizeUpdate(deltaTime, inputs[0], canvasWidth, canvasHeight);
}

//---------------------------------------------------------------------------
const std::vector<TUpgrade> &TGameWorld::GetAvailableUpgrades() const
{
	if (IsNetworkGame && LocalPlayerID < PlayerAvailableUpgrades.size())
	{
		return PlayerAvailableUpgrades[LocalPlayerID];
	}
	return AvailableUpgrades;
}

//---------------------------------------------------------------------------
bool TGameWorld::IsWaitingForUpgradeChoice() const
{
	if (IsNetworkGame && LocalPlayerID < PlayerWaitingForUpgradeChoice.size())
	{
		return PlayerWaitingForUpgradeChoice[LocalPlayerID];
	}
	return WaitingForUpgradeChoice;
}

//---------------------------------------------------------------------------
void TGameWorld::SetPlayerInput(uint8_t playerID, const TInputState &input)
{
	if (playerID < PlayerInputs.size())
	{
		PlayerInputs[playerID] = input;
	}
}
//---------------------------------------------------------------------------
