//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "MainForm.h"
#include <System.SysUtils.hpp>
#include <algorithm>
#include <chrono>
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm1 *Form1;
//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
	: TForm(Owner),
	  LastTick(0),
	  HasLastTick(false),
	  WorldState(EWorldState::MainMenu),
	  IsNetworkGameActive(false),
	  FrameNumber(0),
	  NetworkUpdateTimer(0.0f)
{
	World = std::make_unique<TGameWorld>();
	InputState = TInputState{};
	UIState = TGameUIState{};
	UIState.ShowStatsPanel = false; // инициализируем явно
	
	// Инициализируем сеть
	if (NetworkManager.Initialize())
	{
		// Сеть готова к использованию
	}
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormCreate(TObject *Sender)
{
	KeyPreview = true;
	DoubleBuffered = true;

	// полноэкранный режим
	WindowState = wsMaximized;
	BorderStyle = bsNone;

	World->Reset();
	WorldState = EWorldState::MainMenu;

	LastTick = Now();

	if (GameTimer)
		GameTimer->Enabled = true;
}
//---------------------------------------------------------------------------
void TForm1::ResetGame()
{
	World->Reset();
	WorldState = EWorldState::Playing;
	UIState.OverlayAlpha = 0.0f;
	UIState.RestartButtonRect = Rect(0, 0, 0, 0);
	UIState.MenuButtonRect = Rect(0, 0, 0, 0);
	UIState.UpgradeButtonRects.clear();
	UIState.ShowStatsPanel = false; // панель характеристик скрыта по умолчанию
	UIState.UpgradeButtonHovers.clear();
	UIState.UpgradeButtonHoverTime.clear();
	UIState.StartButtonHover = false;
	UIState.ExitButtonHover = false;
	UIState.ResumeButtonHover = false;
	UIState.PauseMenuButtonHover = false;
	
	// Сбрасываем сетевые данные
	IsNetworkGameActive = false;
	FrameNumber = 0;
	NetworkUpdateTimer = 0.0f;
	PredictedStates.clear();
	InterpolatedPlayers.clear();
	InterpolationClock = 0.0f;
	PersistentInputs.clear();
	PersistentInputs.resize(4);
	
	// Сбрасываем UI состояние меню кооператива
	UIState.CreateGameButtonHover = false;
	UIState.JoinGameButtonHover = false;
	UIState.BackButtonHover = false;
	UIState.StartGameButtonHover = false;
	UIState.IPAddress = "127.0.0.1"; // IP по умолчанию
	UIState.IPInputFocused = false;
	UIState.PlayerNames.clear();
	UIState.PlayerReady.clear();
}

//---------------------------------------------------------------------------
// Сетевые методы
//---------------------------------------------------------------------------
void TForm1::InitializeNetwork()
{
	if (!NetworkManager.Initialize())
	{
		// Ошибка инициализации сети
		return;
	}
}

//---------------------------------------------------------------------------
void TForm1::UpdateNetwork(float deltaTime)
{
	// Обновляем сетевой менеджер
	NetworkManager.Update(deltaTime);
	
	// Отправляем ввод локального игрока (если мы клиент)
	if (IsNetworkGameActive && !NetworkManager.IsHosting() && NetworkManager.GetState() == NeonGame::ENetworkState::Connected)
	{
		SendPlayerInput();
	}
	
	// Получаем обновления от сети
	ReceiveNetworkUpdates(deltaTime);
	
	// Обновляем таймер для периодической отправки состояния (сервер)
	NetworkUpdateTimer += deltaTime;
	if (NetworkUpdateTimer >= NetworkUpdateInterval)
	{
		NetworkUpdateTimer = 0.0f;
		
		// Сервер отправляет состояние игры всем клиентам
		if (NetworkManager.IsHosting() && World->IsNetworkGameActive())
		{
			TGameWorld::TGameStateSnapshot snapshot = World->GetGameStateSnapshot();
			snapshot.Tick = FrameNumber;
			
			// 1. Отправляем обновления игроков
			std::vector<NeonGame::TPlayerUpdatePacket> playerUpdates;
			for (const auto &playerState : snapshot.Players)
			{
				NeonGame::TPlayerUpdatePacket update;
				update.Header.Type = NeonGame::EPacketType::PlayerUpdate;
				update.PlayerID = playerState.PlayerID;
				update.PositionX = playerState.PositionX;
				update.PositionY = playerState.PositionY;
				update.FacingDirectionX = playerState.FacingDirectionX;
				update.FacingDirectionY = playerState.FacingDirectionY;
				update.Health = playerState.Health;
				update.MaxHealth = playerState.MaxHealth;
				update.Level = playerState.Level;
				update.Experience = playerState.Experience;
				update.FrameNumber = snapshot.Tick;
				playerUpdates.push_back(update);
			}
			NetworkManager.BroadcastGameState(playerUpdates);
			
			// 2. Отправляем обновления врагов
			if (!snapshot.Enemies.empty())
			{
				NeonGame::TEnemyUpdatePacket enemyPacket;
				enemyPacket.Header.Type = NeonGame::EPacketType::EnemyUpdate;
				enemyPacket.FrameNumber = snapshot.Tick;
				for (const auto &e : snapshot.Enemies)
				{
					NeonGame::TEnemyUpdatePacket::TEnemyData data;
					data.Type = e.Type;
					data.EnemyID = e.EnemyID;
					data.PositionX = e.PositionX;
					data.PositionY = e.PositionY;
					data.Health = e.Health;
					data.IsAlive = e.IsAlive;
					enemyPacket.Enemies.push_back(data);
				}
				NetworkManager.BroadcastPacket(enemyPacket);
			}
			
			// 3. Отправляем обновление босса
			if (snapshot.Boss.IsAlive)
			{
				NeonGame::TBossUpdatePacket bossPacket;
				bossPacket.Header.Type = NeonGame::EPacketType::BossUpdate;
				bossPacket.PositionX = snapshot.Boss.PositionX;
				bossPacket.PositionY = snapshot.Boss.PositionY;
				bossPacket.Health = snapshot.Boss.Health;
				bossPacket.MaxHealth = snapshot.Boss.MaxHealth;
				bossPacket.Phase = snapshot.Boss.Phase;
				bossPacket.IsAlive = snapshot.Boss.IsAlive;
				bossPacket.FrameNumber = snapshot.Tick;
				NetworkManager.BroadcastPacket(bossPacket);
			}
			
			// 4. Отправляем сферы опыта
			if (!snapshot.ExperienceOrbs.empty())
			{
				NeonGame::TExperienceOrbUpdatePacket orbPacket;
				orbPacket.Header.Type = NeonGame::EPacketType::ExperienceOrbUpdate;
				orbPacket.FrameNumber = snapshot.Tick;
				for (const auto &o : snapshot.ExperienceOrbs)
				{
					NeonGame::TExperienceOrbUpdatePacket::TExperienceOrbData data;
					data.PositionX = o.PositionX;
					data.PositionY = o.PositionY;
					data.Value = o.Value;
					data.IsActive = o.IsActive;
					orbPacket.Orbs.push_back(data);
				}
				NetworkManager.BroadcastPacket(orbPacket);
			}
			
			// 5. Отправляем общее состояние
			NeonGame::TGameStateUpdatePacket statePacket;
			statePacket.Header.Type = NeonGame::EPacketType::GameStateUpdate;
			statePacket.WaveNumber = snapshot.WaveNumber;
			statePacket.EnemiesAlive = snapshot.EnemiesAlive;
			statePacket.WorldState = static_cast<uint8_t>(WorldState);
			statePacket.FrameNumber = snapshot.Tick;
			NetworkManager.BroadcastPacket(statePacket);
		}
	}
	
	FrameNumber++;
}

//---------------------------------------------------------------------------
void TForm1::SendPlayerInput()
{
	if (!IsNetworkGameActive || NetworkManager.IsHosting())
		return;
	
	// Создаем пакет ввода
	NeonGame::TPlayerInputPacket inputPacket;
	inputPacket.Header.Type = NeonGame::EPacketType::PlayerInput;
	inputPacket.PlayerID = World->GetLocalPlayerID();
	inputPacket.InputUp = InputState.MoveUp;
	inputPacket.InputDown = InputState.MoveDown;
	inputPacket.InputLeft = InputState.MoveLeft;
	inputPacket.InputRight = InputState.MoveRight;
	inputPacket.IsShooting = InputState.PrimaryFire;
	inputPacket.FrameNumber = FrameNumber;
	
	// Конвертируем позицию мыши в мировые координаты
	if (InputState.HasMouse)
	{
		const TPointF cameraPos = World->GetCameraPosition();
		inputPacket.MouseX = static_cast<float>(InputState.MouseClient.x) + cameraPos.X;
		inputPacket.MouseY = static_cast<float>(InputState.MouseClient.y) + cameraPos.Y;
	}
	else
	{
		// Используем текущую позицию игрока
		const TPointF playerPos = World->GetPlayerPosition();
		inputPacket.MouseX = playerPos.X;
		inputPacket.MouseY = playerPos.Y;
	}
	
	// Отправляем на сервер
	NetworkManager.SendPlayerInput(inputPacket);
}

//---------------------------------------------------------------------------
void TForm1::ReceiveNetworkUpdates(float deltaTime)
{
	if (NetworkManager.GetState() == NeonGame::ENetworkState::Disconnected)
		return;
	
	// Если мы клиент, получаем обновления от сервера
	if (!NetworkManager.IsHosting())
	{
		// 1. Получаем все пакеты от сервера
		std::unique_ptr<NeonGame::TGameStateUpdatePacket> gameStateUpdate = NetworkManager.GetReceivedGameStateUpdate();
		std::vector<NeonGame::TPlayerUpdatePacket> playerUpdates = NetworkManager.GetReceivedPlayerUpdates();
		std::vector<NeonGame::TEnemyUpdatePacket> enemyUpdates = NetworkManager.GetReceivedEnemyUpdates();
		std::vector<NeonGame::TBossUpdatePacket> bossUpdates = NetworkManager.GetReceivedBossUpdates();
		std::vector<NeonGame::TExperienceOrbUpdatePacket> orbUpdates = NetworkManager.GetReceivedOrbUpdates();
		
		// 2. Обрабатываем переход в состояние Playing (старт игры)
		if (gameStateUpdate)
		{
			if (gameStateUpdate->WorldState == static_cast<uint8_t>(EWorldState::Playing) && !IsNetworkGameActive)
			{
				World->InitializeNetworkGame(NetworkManager.GetLocalPlayerID(), false);
				ResetGame();
				IsNetworkGameActive = true;
				WorldState = EWorldState::Playing;
			}
			
			// Синхронизируем WorldState с сервером (для ChoosingUpgrade и других состояний)
			if (IsNetworkGameActive)
			{
				EWorldState newState = static_cast<EWorldState>(gameStateUpdate->WorldState);
				
				// Если переходим в ChoosingUpgrade, генерируем улучшения локально
				if (newState == EWorldState::ChoosingUpgrade && WorldState != EWorldState::ChoosingUpgrade)
				{
					// Клиент генерирует свои улучшения локально для отображения UI
					// Выбор будет отправлен на сервер через TPlayerUpgradePacket
					if (!World->IsWaitingForUpgradeChoice())
					{
						World->GenerateUpgradesForPlayer(World->GetLocalPlayerID());
					}
				}
				
				WorldState = newState;
			}
		}
		
		if (!IsNetworkGameActive)
			return;

		// 3. Подготавливаем снапшот мира для синхронизации
		TGameWorld::TGameStateSnapshot worldSnapshot = World->GetGameStateSnapshot();
		bool hasUpdates = (gameStateUpdate != nullptr || !playerUpdates.empty() || !enemyUpdates.empty() || !bossUpdates.empty() || !orbUpdates.empty());
		
		// 4. Обрабатываем обновления игроков (интерполяция + статы)
		if (InterpolatedPlayers.size() < World->GetPlayerCount())
		{
			InterpolatedPlayers.resize(World->GetPlayerCount());
		}
		
		float currentTime = InterpolationClock;
		TGamePlayer* localPlayer = World->GetLocalPlayer();

		if (!playerUpdates.empty())
		{
			// Обновляем слоты игроков в worldSnapshot
			for (const auto &update : playerUpdates)
			{
				if (update.PlayerID >= InterpolatedPlayers.size())
					continue;
				
				// Обработка расхождений для локального игрока
				if (update.PlayerID == World->GetLocalPlayerID() && localPlayer)
				{
					TPointF serverPos(update.PositionX, update.PositionY);
					TPointF clientPos = localPlayer->GetPosition();
					float distance = std::sqrt(std::pow(serverPos.X - clientPos.X, 2) + std::pow(serverPos.Y - clientPos.Y, 2));
					
					const float ignoreDeviation = 5.0f;
					const float snapDeviation = 150.0f;
					const float correctionSpeed = 5.0f;
					
					if (distance > snapDeviation)
					{
						localPlayer->SetPosition(serverPos.X, serverPos.Y);
					}
					else if (distance > ignoreDeviation)
					{
						float correctionFactor = std::min(0.5f, correctionSpeed * deltaTime);
						localPlayer->SetPosition(
							clientPos.X + (serverPos.X - clientPos.X) * correctionFactor,
							clientPos.Y + (serverPos.Y - clientPos.Y) * correctionFactor
						);
					}
					
					TPointF serverDir(update.FacingDirectionX, update.FacingDirectionY);
					localPlayer->SetFacingDirection(serverDir);
				}
				
				// Добавляем в буфер интерполяции
				TInterpolatedPlayer &interp = InterpolatedPlayers[update.PlayerID];
				TPlayerStateSnapshot interpSnapshot;
				interpSnapshot.Position = TPointF(update.PositionX, update.PositionY);
				interpSnapshot.FacingDirection = TPointF(update.FacingDirectionX, update.FacingDirectionY);
				interpSnapshot.Health = static_cast<float>(update.Health);
				interpSnapshot.MaxHealth = static_cast<float>(update.MaxHealth);
				interpSnapshot.Tick = update.FrameNumber;
				interpSnapshot.Timestamp = currentTime;
				
				interp.StateBuffer.push_back(interpSnapshot);
				if (interp.StateBuffer.size() > TInterpolatedPlayer::MaxBufferSize)
					interp.StateBuffer.erase(interp.StateBuffer.begin());
				
				interp.LastUpdateTime = currentTime;
				interp.HasValidState = true;
				
				// Обновляем данные в worldSnapshot для ApplyGameStateSnapshot
				for (auto &ps : worldSnapshot.Players)
				{
					if (ps.PlayerID == update.PlayerID)
					{
						ps.PositionX = update.PositionX;
						ps.PositionY = update.PositionY;
						ps.FacingDirectionX = update.FacingDirectionX;
						ps.FacingDirectionY = update.FacingDirectionY;
						ps.Health = update.Health;
						ps.MaxHealth = update.MaxHealth;
						ps.Level = update.Level;
						ps.Experience = update.Experience;
						ps.IsAlive = (update.Health > 0);
						break;
					}
				}
			}
		}

		// 5. Обновляем состояние мира, врагов и босса в снапшоте
		if (gameStateUpdate)
		{
			worldSnapshot.WaveNumber = gameStateUpdate->WaveNumber;
			worldSnapshot.EnemiesAlive = gameStateUpdate->EnemiesAlive;
			worldSnapshot.WorldState = gameStateUpdate->WorldState;
		}
		
		if (!enemyUpdates.empty())
		{
			// Сначала добавляем ВСЕ полученные пакеты в буферы интерполяции
			for (const auto &packet : enemyUpdates)
			{
				for (const auto &e : packet.Enemies)
				{
					TEnemyStateSnapshot enemySnap;
					enemySnap.EnemyID = e.EnemyID;
					enemySnap.Position = TPointF(e.PositionX, e.PositionY);
					enemySnap.Tick = packet.FrameNumber;
					enemySnap.Timestamp = InterpolationClock;
					
					auto &interp = InterpolatedEnemies[e.EnemyID];
					interp.EnemyID = e.EnemyID;
					interp.IsActive = true;
					interp.StateBuffer.push_back(enemySnap);
					if (interp.StateBuffer.size() > TInterpolatedEnemy::MaxBufferSize)
						interp.StateBuffer.erase(interp.StateBuffer.begin());
				}
			}
			
			// Затем обновляем текущий список врагов в мире на основе ПОСЛЕДНЕГО состояния
			const auto &lastEnemyUpdate = enemyUpdates.back();
			worldSnapshot.Enemies.clear();
			for (const auto &e : lastEnemyUpdate.Enemies)
			{
				TGameWorld::TGameStateSnapshot::TEnemyState enemyState;
				enemyState.Type = e.Type;
				enemyState.EnemyID = e.EnemyID;
				enemyState.PositionX = e.PositionX;
				enemyState.PositionY = e.PositionY;
				enemyState.Health = e.Health;
				enemyState.IsAlive = e.IsAlive;
				worldSnapshot.Enemies.push_back(enemyState);
			}
		}
		
		if (!bossUpdates.empty())
		{
			// Сначала добавляем ВСЕ полученные пакеты в буферы интерполяции
			for (const auto &packet : bossUpdates)
			{
				if (packet.IsAlive)
				{
					TEnemyStateSnapshot bossSnap;
					bossSnap.EnemyID = 0;
					bossSnap.Position = TPointF(packet.PositionX, packet.PositionY);
					bossSnap.Tick = packet.FrameNumber;
					bossSnap.Timestamp = InterpolationClock;
					
					auto &interp = InterpolatedEnemies[0];
					interp.EnemyID = 0;
					interp.IsActive = true;
					interp.StateBuffer.push_back(bossSnap);
					if (interp.StateBuffer.size() > TInterpolatedEnemy::MaxBufferSize)
						interp.StateBuffer.erase(interp.StateBuffer.begin());
				}
			}

			// Затем обновляем снапшот мира на основе ПОСЛЕДНЕГО состояния
			const auto &lastBossUpdate = bossUpdates.back();
			worldSnapshot.Boss.PositionX = lastBossUpdate.PositionX;
			worldSnapshot.Boss.PositionY = lastBossUpdate.PositionY;
			worldSnapshot.Boss.Health = lastBossUpdate.Health;
			worldSnapshot.Boss.MaxHealth = lastBossUpdate.MaxHealth;
			worldSnapshot.Boss.Phase = lastBossUpdate.Phase;
			worldSnapshot.Boss.IsAlive = lastBossUpdate.IsAlive;
		}

		// 6. Обрабатываем обновления сфер опыта
		if (!orbUpdates.empty())
		{
			// Берем последний пакет сфер
			const auto &lastOrbUpdate = orbUpdates.back();
			worldSnapshot.ExperienceOrbs.clear();
			for (const auto &o : lastOrbUpdate.Orbs)
			{
				TGameWorld::TGameStateSnapshot::TExperienceOrbState orbState;
				orbState.PositionX = o.PositionX;
				orbState.PositionY = o.PositionY;
				orbState.Value = o.Value;
				orbState.IsActive = o.IsActive;
				worldSnapshot.ExperienceOrbs.push_back(orbState);
			}
		}
		
		// 7. Применяем накопленный снапшот к миру
		if (hasUpdates)
		{
			World->ApplyGameStateSnapshot(worldSnapshot);
		}
	}
}

//---------------------------------------------------------------------------
void TForm1::ApplyClientSidePrediction(float deltaTime)
{
	if (!IsNetworkGameActive || NetworkManager.IsHosting())
		return;
	
	// Сохраняем текущее предсказанное состояние
	TGamePlayer* localPlayer = World->GetLocalPlayer();
	if (!localPlayer)
		return;
	
	TPredictedState predicted;
	predicted.Position = localPlayer->GetPosition();
	predicted.FacingDirection = localPlayer->GetFacingDirection();
	predicted.FrameNumber = FrameNumber;
	
	PredictedStates.push_back(predicted);
	
	// Ограничиваем размер истории (храним последние 60 кадров)
	if (PredictedStates.size() > 60)
	{
		PredictedStates.erase(PredictedStates.begin());
	}
	
	// Расхождения обрабатываются в ReceiveNetworkUpdates()
}

//---------------------------------------------------------------------------
void TForm1::InterpolateRemotePlayers(float deltaTime)
{
	if (!IsNetworkGameActive || NetworkManager.IsHosting())
		return;
	
	// Инициализируем интерполяцию для других игроков, если нужно
	if (InterpolatedPlayers.size() < World->GetPlayerCount())
	{
		InterpolatedPlayers.resize(World->GetPlayerCount());
	}
	
	static float interpolationDelay = 0.15f; // задержка интерполяции (150ms) для сглаживания
	
	// Интерполируем позиции других игроков
	for (uint8_t i = 0; i < World->GetPlayerCount(); i++)
	{
		if (i == World->GetLocalPlayerID())
			continue; // Пропускаем локального игрока
		
		TGamePlayer* player = World->GetPlayer(i);
		if (!player)
			continue;
		
		TInterpolatedPlayer &interp = InterpolatedPlayers[i];
		
		if (!interp.HasValidState || interp.StateBuffer.empty())
			continue;

		// Целевое время воспроизведения (с задержкой для сглаживания)
		float targetPlaybackTime = InterpolationClock - interpolationDelay;
		
		// Если данных слишком мало, просто берем последнее состояние
		if (interp.StateBuffer.size() < 2)
		{
			const auto &snapshot = interp.StateBuffer.back();
			player->SetPosition(snapshot.Position.X, snapshot.Position.Y);
			player->SetFacingDirection(snapshot.FacingDirection);
			continue;
		}
		
		// Находим два ближайших состояния для интерполяции вокруг targetPlaybackTime
		size_t olderIdx = 0;
		size_t newerIdx = 0;
		bool foundPair = false;
		
		for (size_t j = 0; j < interp.StateBuffer.size() - 1; j++)
		{
			if (interp.StateBuffer[j].Timestamp <= targetPlaybackTime && 
			    interp.StateBuffer[j + 1].Timestamp >= targetPlaybackTime)
			{
				olderIdx = j;
				newerIdx = j + 1;
				foundPair = true;
				break;
			}
		}
		
		if (foundPair)
		{
			const auto &olderState = interp.StateBuffer[olderIdx];
			const auto &newerState = interp.StateBuffer[newerIdx];
			
			// Вычисляем коэффициент интерполяции
			float t = 0.0f;
			float span = newerState.Timestamp - olderState.Timestamp;
			if (span > 0.0001f)
			{
				t = (targetPlaybackTime - olderState.Timestamp) / span;
				t = std::max(0.0f, std::min(1.0f, t));
			}
			
			// Интерполируем позицию
			TPointF interpolatedPos(
				olderState.Position.X + (newerState.Position.X - olderState.Position.X) * t,
				olderState.Position.Y + (newerState.Position.Y - olderState.Position.Y) * t
			);
			
			// Интерполируем направление
			TPointF interpolatedDir(
				olderState.FacingDirection.X + (newerState.FacingDirection.X - olderState.FacingDirection.X) * t,
				olderState.FacingDirection.Y + (newerState.FacingDirection.Y - olderState.FacingDirection.Y) * t
			);
			
			player->SetPosition(interpolatedPos.X, interpolatedPos.Y);
			player->SetFacingDirection(interpolatedDir);
		}
		else if (targetPlaybackTime > interp.StateBuffer.back().Timestamp)
		{
			// Экстраполяция или просто последнее состояние (если отстаем)
			const auto &snapshot = interp.StateBuffer.back();
			player->SetPosition(snapshot.Position.X, snapshot.Position.Y);
			player->SetFacingDirection(snapshot.FacingDirection);
		}
		else if (targetPlaybackTime < interp.StateBuffer.front().Timestamp)
		{
			// Слишком старое время (может быть в начале игры)
			const auto &snapshot = interp.StateBuffer.front();
			player->SetPosition(snapshot.Position.X, snapshot.Position.Y);
			player->SetFacingDirection(snapshot.FacingDirection);
		}
		
		// Удаляем слишком старые состояния (которым более 1 секунды)
		// Оставляем как минимум два последних состояния для возможной интерполяции
		while (interp.StateBuffer.size() > 2 && 
		       (InterpolationClock - interp.StateBuffer[0].Timestamp) > 1.0f)
		{
			interp.StateBuffer.erase(interp.StateBuffer.begin());
		}
	}
}
//---------------------------------------------------------------------------
void TForm1::InterpolateEnemies(float deltaTime)
{
	if (!IsNetworkGameActive || NetworkManager.IsHosting())
		return;

	const float interpolationDelay = 0.150f; // 150ms задержка для плавности
	const float targetTime = InterpolationClock - interpolationDelay;

	// Обновляем каждого врага в мире
	auto &enemies = World->GetEnemies();
	
	// Очищаем неактивных врагов из интерполятора
	for (auto it = InterpolatedEnemies.begin(); it != InterpolatedEnemies.end(); )
	{
		bool found = false;
		for (const auto &e : enemies)
		{
			if (e->GetEnemyID() == it->first)
			{
				found = true;
				break;
			}
		}
		
		if (!found)
			it = InterpolatedEnemies.erase(it);
		else
			++it;
	}

	for (auto &e : enemies)
	{
		uint32_t id = e->GetEnemyID();
		if (InterpolatedEnemies.find(id) == InterpolatedEnemies.end())
			continue;

		auto &interp = InterpolatedEnemies[id];
		if (interp.StateBuffer.size() < 2)
			continue;

		// Находим два состояния для интерполяции
		const TEnemyStateSnapshot* s1 = nullptr;
		const TEnemyStateSnapshot* s2 = nullptr;

		for (size_t i = 0; i < interp.StateBuffer.size() - 1; i++)
		{
			if (interp.StateBuffer[i].Timestamp <= targetTime && interp.StateBuffer[i+1].Timestamp >= targetTime)
			{
				s1 = &interp.StateBuffer[i];
				s2 = &interp.StateBuffer[i+1];
				break;
			}
		}

		if (s1 && s2)
		{
			float duration = s2->Timestamp - s1->Timestamp;
			float t = (duration > 0) ? (targetTime - s1->Timestamp) / duration : 1.0f;

			TPointF pos(
				s1->Position.X + (s2->Position.X - s1->Position.X) * t,
				s1->Position.Y + (s2->Position.Y - s1->Position.Y) * t
			);
			e->SetPosition(pos);
		}
		else if (targetTime > interp.StateBuffer.back().Timestamp)
		{
			// Экстраполяция или просто установка последней точки
			e->SetPosition(interp.StateBuffer.back().Position);
		}
		
		// Удаляем слишком старые состояния
		while (interp.StateBuffer.size() > 2 && interp.StateBuffer[1].Timestamp < targetTime - 0.5f)
		{
			interp.StateBuffer.erase(interp.StateBuffer.begin());
		}
	}

	// Обновляем босса отдельно
	TBossEnemy* boss = World->GetBoss();
	if (boss && boss->IsAlive())
	{
		uint32_t id = boss->GetEnemyID();
		if (InterpolatedEnemies.count(id) > 0)
		{
			auto &interp = InterpolatedEnemies[id];
			if (interp.StateBuffer.size() >= 2)
			{
				const TEnemyStateSnapshot* s1 = nullptr;
				const TEnemyStateSnapshot* s2 = nullptr;

				for (size_t i = 0; i < interp.StateBuffer.size() - 1; i++)
				{
					if (interp.StateBuffer[i].Timestamp <= targetTime && interp.StateBuffer[i + 1].Timestamp >= targetTime)
					{
						s1 = &interp.StateBuffer[i];
						s2 = &interp.StateBuffer[i + 1];
						break;
					}
				}

				if (s1 && s2)
				{
					float duration = s2->Timestamp - s1->Timestamp;
					float t = (duration > 0) ? (targetTime - s1->Timestamp) / duration : 1.0f;
					boss->SetPosition(TPointF(
						s1->Position.X + (s2->Position.X - s1->Position.X) * t,
						s1->Position.Y + (s2->Position.Y - s1->Position.Y) * t
					));
				}
			}
		}
	}
}

//---------------------------------------------------------------------------
void TForm1::UpdateGame(double deltaSeconds)
{
	if (!World)
		return;

	const float dt = static_cast<float>(deltaSeconds);
	
	// Обновляем сеть:
	// - когда сетевая игра уже активна
	// - а также пока мы находимся в состояниях Hosting/Connecting/Connected (меню кооператива)
	if (IsNetworkGameActive || NetworkManager.GetState() != NeonGame::ENetworkState::Disconnected)
	{
		UpdateNetwork(dt);
	}

	// обновляем состояние мира только если игра активна
	// (не перезаписываем MainMenu и Paused)
	EWorldState previousState = WorldState;
	if (WorldState == EWorldState::Playing || WorldState == EWorldState::ChoosingUpgrade)
	{
		// Если это сетевая игра и мы сервер, собираем вводы от всех игроков
		if (IsNetworkGameActive && NetworkManager.IsHosting())
		{
			// Ввод локального игрока (хоста)
			PersistentInputs[0] = InputState;
			
			// Получаем вводы от других игроков и обновляем сохраненные состояния
			std::vector<NeonGame::TPlayerInputPacket> receivedInputs = NetworkManager.GetReceivedInputs();
			for (const auto &inputPacket : receivedInputs)
			{
				if (inputPacket.PlayerID < 4)
				{
					TInputState &state = PersistentInputs[inputPacket.PlayerID];
					state.MoveUp = inputPacket.InputUp;
					state.MoveDown = inputPacket.InputDown;
					state.MoveLeft = inputPacket.InputLeft;
					state.MoveRight = inputPacket.InputRight;
					state.PrimaryFire = inputPacket.IsShooting;
					
					// ВАЖНО: клиент отправляет мировые координаты, но нам нужны экранные
					// Конвертируем обратно в экранные координаты относительно камеры
					const TPointF cameraPos = World->GetCameraPosition();
					state.MouseClient = TPoint(
						static_cast<int>(inputPacket.MouseX - cameraPos.X), 
						static_cast<int>(inputPacket.MouseY - cameraPos.Y)
					);
					state.HasMouse = true;
					
					// Сохраняем ввод для обработки стрельбы
					World->SetPlayerInput(inputPacket.PlayerID, state);
				}
			}
			
			// Сохраняем ввод хоста для обработки стрельбы
			World->SetPlayerInput(0, PersistentInputs[0]);
			
			// Обновляем мир с вводами всех игроков (используем сохраненные)
			World->Update(dt, PersistentInputs, GameCanvas->Width, GameCanvas->Height);
		}
		else if (IsNetworkGameActive && !NetworkManager.IsHosting())
		{
			// Клиент: обновляем часы интерполяции
			InterpolationClock += dt;
			
			// Интерполируем позиции других игроков и врагов ДО обновления мира
			InterpolateRemotePlayers(dt);
			InterpolateEnemies(dt);
			
			// Применяем client-side prediction для локального игрока
			ApplyClientSidePrediction(dt);
			
			// Обновляем мир с локальным вводом (предсказание)
			World->Update(dt, InputState, GameCanvas->Width, GameCanvas->Height);
		}
		else
		{
			// Одиночная игра
			World->Update(dt, InputState, GameCanvas->Width, GameCanvas->Height);
		}
		
		WorldState = World->GetState();
		
		// если только что перешли в GameOver, обновляем рекорды
		if (previousState != EWorldState::GameOver && WorldState == EWorldState::GameOver)
		{
			// Получаем актуальные статистики из мира
			// Важно: Stats уже должны быть обновлены в World->Update() перед переходом в GameOver
			const TGameStats &stats = World->GetStats();
			
			// Обновляем рекорды с текущими результатами
			RecordsManager.UpdateRecords(stats.CurrentWave, stats.EnemiesDefeated, stats.RunTimeSeconds);
		}
	}

	// обновляем анимацию Game Over оверлея
	if (WorldState == EWorldState::GameOver)
	{
		UIState.OverlayAlpha = std::min(1.0f, UIState.OverlayAlpha + dt * 1.8f);
		}
		else
		{
		UIState.OverlayAlpha = std::max(0.0f, UIState.OverlayAlpha - dt * 2.5f);
		}

	// обновляем анимацию наведения на кнопки улучшений
	if (WorldState == EWorldState::ChoosingUpgrade)
	{
		if (UIState.UpgradeButtonHoverTime.size() < 3)
			UIState.UpgradeButtonHoverTime.resize(3, 0.0f);
		
		// проверяем hover по текущей позиции мыши (даже если мышь не двигается)
		if (InputState.HasMouse && GameCanvas)
	{
			const int X = InputState.MouseClient.x;
			const int Y = InputState.MouseClient.y;
			
			// проверяем hover с учетом увеличения кнопки при анимации (максимальный scale = 1.05)
			const float maxScale = 1.05f;
			const int buttonWidth = 380;
			const int maxOffset = static_cast<int>((buttonWidth * maxScale - buttonWidth) / 2);
			
			for (size_t i = 0; i < UIState.UpgradeButtonRects.size() && i < UIState.UpgradeButtonHovers.size(); i++)
			{
				// расширяем rect для проверки hover с учетом максимального увеличения
				TRect expandedRect = UIState.UpgradeButtonRects[i];
				expandedRect.Left -= maxOffset;
				expandedRect.Top -= maxOffset;
				expandedRect.Right += maxOffset;
				expandedRect.Bottom += maxOffset;
				
				UIState.UpgradeButtonHovers[i] = PointInRect(expandedRect, X, Y);
			}
	}

		// обновляем анимацию на основе текущего состояния hover
		for (size_t i = 0; i < UIState.UpgradeButtonHovers.size() && i < UIState.UpgradeButtonHoverTime.size(); i++)
	{
			if (UIState.UpgradeButtonHovers[i])
		{
				// плавно увеличиваем до 1.0
				UIState.UpgradeButtonHoverTime[i] = std::min(1.0f, UIState.UpgradeButtonHoverTime[i] + dt * 8.0f);
			}
			else
			{
				// плавно уменьшаем до 0.0
				UIState.UpgradeButtonHoverTime[i] = std::max(0.0f, UIState.UpgradeButtonHoverTime[i] - dt * 8.0f);
			}
		}
		// постоянно перерисовываем для плавной анимации
		GameCanvas->Repaint();
	}

	// обновляем мир с текущим вводом только если игра активна
	// (World->Update() уже вызывается выше в условии Playing/ChoosingUpgrade)
	if (WorldState == EWorldState::Playing)
	{
		// WorldState уже обновлен выше
	}
	else if (WorldState == EWorldState::MainMenu || WorldState == EWorldState::Paused)
	{
		// обновляем только для перерисовки при наведении
		GameCanvas->Repaint();
	}
}
//---------------------------------------------------------------------------
void TForm1::RenderGame()
{
	if (!GameCanvas || !World)
		return;

	TCanvas *canvas = GameCanvas->Canvas;
	canvas->Brush->Color = clBlack;
	canvas->FillRect(GameCanvas->ClientRect);

	// главное меню
	if (WorldState == EWorldState::MainMenu)
	{
		UiRenderer.DrawMainMenu(canvas, RecordsManager.GetRecords(), UIState);
		return;
	}
	
	// меню кооператива
	if (WorldState == EWorldState::CoopMenu)
	{
		UiRenderer.DrawCoopMenu(canvas, UIState, &NetworkManager);
		return;
	}

	// рисуем игровую сцену (для всех остальных состояний)
	World->RenderScene(canvas);

	// рисуем HUD
	if (WorldState == EWorldState::Playing)
	{
		UiRenderer.DrawHud(canvas,
			World->GetPlayerHealthRatio(),
			World->GetPlayerHealth(),
			World->GetPlayerMaxHealth(),
			World->GetPlayerExperienceRatio(),
			World->GetPlayerExperience(),
			World->GetPlayerExperienceToNext(),
			World->GetPlayerLevel(),
			World->GetStats(),
			UIState.ShowStatsPanel);

		// отображение характеристик
		UiRenderer.DrawPlayerStats(canvas, World->GetPlayerStats(), UIState);

		// уведомление о level up
		if (World->GetLevelUpNotificationTimer() > 0.0f)
		{
			UiRenderer.DrawLevelUpNotification(canvas,
				World->GetLevelUpNotificationTimer(),
				World->GetLastPlayerLevel());
		}

		// рисуем полоску HP босса, если он есть
		if (World->HasBoss())
		{
			UiRenderer.DrawBossHealthBar(canvas,
				World->GetBossHealthRatio(),
				World->GetBossHealth(),
				World->GetBossMaxHealth());
		}
		
		// индикаторы перезарядки
		UiRenderer.DrawCooldownIndicators(canvas,
			World->GetPrimaryFireCooldown(),
			World->GetAltFireCooldown(),
			NeonGame::PrimaryFireCooldown,
			NeonGame::AltFireCooldown);
		
		// мини-карта
		if (World->IsNetworkGameActive())
		{
			// Для сетевой игры - отображаем всех игроков
			std::vector<TPointF> playerPositions;
			for (uint8_t i = 0; i < World->GetPlayerCount(); i++)
			{
				playerPositions.push_back(World->GetPlayerPosition(i));
			}
			UiRenderer.DrawMinimap(canvas,
				playerPositions,
				World->GetEnemyPositions(),
				World->GetBossPosition(),
				World->HasActiveBoss(),
				NeonGame::WorldWidth,
				NeonGame::WorldHeight,
				World->GetLocalPlayerID());
			
			// Отображаем имена игроков
			std::vector<std::string> playerNames;
			if (NetworkManager.IsHosting())
			{
				const auto &clients = NetworkManager.GetClients();
				for (const auto &client : clients)
				{
					if (client.IsConnected)
						playerNames.push_back(client.PlayerName);
				}
			}
			UiRenderer.DrawPlayerNames(canvas, playerPositions, playerNames, World->GetCameraPosition(), World->GetLocalPlayerID());
		}
		else
		{
			// Для одиночной игры - обычная мини-карта
			UiRenderer.DrawMinimap(canvas,
				World->GetPlayerPosition(),
				World->GetEnemyPositions(),
				World->GetBossPosition(),
				World->HasActiveBoss(),
				NeonGame::WorldWidth,
				NeonGame::WorldHeight);
		}
		}

	// меню паузы
	if (WorldState == EWorldState::Paused)
	{
		UiRenderer.DrawPauseMenu(canvas, UIState);
	}

	// меню выбора улучшения
	if (WorldState == EWorldState::ChoosingUpgrade)
	{
		const auto &upgrades = World->GetAvailableUpgrades();
		UiRenderer.DrawUpgradeMenu(canvas, upgrades, UIState);
	}

	// экран Game Over
	if (WorldState == EWorldState::GameOver)
	{
		UiRenderer.DrawGameOver(canvas, World->GetStats(), RecordsManager.GetRecords(), UIState);
	}
}
//---------------------------------------------------------------------------
void __fastcall TForm1::GameCanvasPaint(TObject *Sender)
{
	RenderGame();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::GameTimerTimer(TObject *Sender)
{
	const TDateTime now = Now();
	double deltaSeconds = 0.0;

	if (!HasLastTick)
	{
		deltaSeconds = GameTimer->Interval / 1000.0;
		HasLastTick = true;
	}
	else
	{
		const double deltaDays = static_cast<double>(now - LastTick);
		deltaSeconds = deltaDays * 86400.0;
	}

	LastTick = now;

	UpdateGame(deltaSeconds);
	GameCanvas->Repaint();
}
//---------------------------------------------------------------------------
// Простая проверка корректности IPv4-адреса формата x.x.x.x (0-255)
static bool IsValidIPv4(const std::string &ip)
{
	if (ip.empty())
		return false;

	int parts = 0;
	int value = -1;
	bool hasDigit = false;

	for (size_t i = 0; i <= ip.size(); ++i)
	{
		char c = (i < ip.size()) ? ip[i] : '.'; // добавляем финальную точку для обработки последнего сегмента

		if (c == '.')
		{
			if (!hasDigit)
				return false; // пустой сегмент

			if (value < 0 || value > 255)
				return false;

			parts++;
			value = -1;
			hasDigit = false;
		}
		else if (c >= '0' && c <= '9')
		{
			int digit = c - '0';
			if (!hasDigit)
			{
				value = digit;
				hasDigit = true;
			}
			else
			{
				value = value * 10 + digit;
				if (value > 255)
					return false;
			}
		}
		else
		{
			return false; // недопустимый символ
		}
	}

	return parts == 4; // должно быть ровно 4 сегмента
}

void __fastcall TForm1::FormKeyDown(TObject *Sender, WORD &Key, TShiftState Shift)
{
	// выход из полноэкранного режима по ESC (только в игре)
	if (Key == VK_ESCAPE && WorldState == EWorldState::Playing)
	{
		WorldState = EWorldState::Paused;
		return;
	}
	
	// выход из паузы по ESC
	if (Key == VK_ESCAPE && WorldState == EWorldState::Paused)
	{
		WorldState = EWorldState::Playing;
		return;
	}

	// главное меню
	if (WorldState == EWorldState::MainMenu)
	{
		// обработка нажатий на кнопки будет в GameCanvasMouseDown
		return;
	}

	// меню кооператива - обработка ввода IP адреса
	if (WorldState == EWorldState::CoopMenu)
	{
		if (UIState.IPInputFocused)
		{
			// ENTER — пытаемся подключиться к игре
			if (Key == VK_RETURN)
			{
				if (!UIState.IPAddress.empty())
				{
					// Перед подключением проверяем корректность IP, чтобы избежать крашей
					if (!IsValidIPv4(UIState.IPAddress))
					{
						Application->MessageBox(L"Неверный IP-адрес. Используйте формат вида 192.168.0.10", L"Ошибка", MB_OK | MB_ICONWARNING);
					}
					else
					{
						if (NetworkManager.ConnectToGame(UIState.IPAddress, "Player1", 7777))
						{
							// Подключение началось
						}
						else
						{
							// Можно дополнительно вывести текст ошибки из NetworkManager при желании
							Application->MessageBox(L"Не удалось подключиться к хосту.", L"Ошибка подключения", MB_OK | MB_ICONERROR);
						}
					}
				}
				UIState.IPInputFocused = false;
				GameCanvas->Repaint();
				return;
			}

			// ESC — выходим из режима ввода
			if (Key == VK_ESCAPE)
			{
				UIState.IPInputFocused = false;
				GameCanvas->Repaint();
				return;
			}

			// BACKSPACE — удаляем последний символ
			if (Key == VK_BACK)
			{
				if (!UIState.IPAddress.empty())
				{
					UIState.IPAddress.pop_back();
					GameCanvas->Repaint();
				}
				return;
			}

			// Добавление символов IP (только цифры и точка), ограничение длины
			if (UIState.IPAddress.length() < 15)
			{
				// Цифры (верхний ряд и numpad)
				if ((Key >= '0' && Key <= '9') || (Key >= VK_NUMPAD0 && Key <= VK_NUMPAD9))
				{
					char ch = '0';
					if (Key >= '0' && Key <= '9')
						ch = static_cast<char>(Key);
					else
						ch = static_cast<char>('0' + (Key - VK_NUMPAD0));

					UIState.IPAddress += ch;
					GameCanvas->Repaint();
				}
				// Точка с основной клавиатуры и с numpad
				else if (Key == VK_OEM_PERIOD || Key == VK_DECIMAL)
				{
					UIState.IPAddress += '.';
					GameCanvas->Repaint();
				}
			}
			return;
		}
		return;
	}

	if (WorldState == EWorldState::GameOver)
	{
		if (Key == 'R')
		{
			ResetGame();
		}
		return;
	}
	
	// меню паузы
	if (WorldState == EWorldState::Paused)
	{
		if (Key == VK_RETURN || Key == VK_SPACE)
		{
			WorldState = EWorldState::Playing;
		}
		// обработка нажатий на кнопки будет в GameCanvasMouseDown
		return;
	}

	// выбор улучшения
	if (WorldState == EWorldState::ChoosingUpgrade)
	{
		uint8_t localPlayerID = World->GetLocalPlayerID();
		if (Key == '1' || Key == VK_NUMPAD1)
		{
			World->SelectUpgrade(0, localPlayerID);
		}
		else if (Key == '2' || Key == VK_NUMPAD2)
		{
			World->SelectUpgrade(1, localPlayerID);
		}
		else if (Key == '3' || Key == VK_NUMPAD3)
		{
			World->SelectUpgrade(2, localPlayerID);
		}
		return;
	}

	// переключение панели характеристик по TAB (только в игре)
	if (WorldState == EWorldState::Playing && (Key == VK_TAB || Key == 9))
	{
		UIState.ShowStatsPanel = !UIState.ShowStatsPanel;
		Key = 0; // предотвращаем стандартную обработку TAB
		return;
	}

	// обновляем состояние ввода
	switch (Key)
	{
		case 'W': InputState.MoveUp = true; break;
		case 'S': InputState.MoveDown = true; break;
		case 'A': InputState.MoveLeft = true; break;
		case 'D': InputState.MoveRight = true; break;
		case VK_SPACE: InputState.PrimaryFire = true; break;
		default: break;
	}
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormKeyUp(TObject *Sender, WORD &Key, TShiftState Shift)
{
	if (WorldState == EWorldState::GameOver)
		return;

	// переключение панели характеристик по TAB (только в игре)
	if (WorldState == EWorldState::Playing && (Key == VK_TAB || Key == 9))
	{
		UIState.ShowStatsPanel = !UIState.ShowStatsPanel;
		Key = 0; // предотвращаем стандартную обработку TAB
		return;
	}

	switch (Key)
	{
		case 'W': InputState.MoveUp = false; break;
		case 'S': InputState.MoveDown = false; break;
		case 'A': InputState.MoveLeft = false; break;
		case 'D': InputState.MoveRight = false; break;
		case VK_SPACE: InputState.PrimaryFire = false; break;
		default: break;
	}
}
//---------------------------------------------------------------------------
void __fastcall TForm1::GameCanvasMouseMove(TObject *Sender, TShiftState Shift, int X, int Y)
{
	InputState.MouseClient = Point(X, Y);
	InputState.HasMouse = true;

	if (WorldState == EWorldState::MainMenu)
	{
		// проверяем hover только если кнопки уже инициализированы (после первого рендера)
		if (UIState.StartButtonRect.Width() > 0 && UIState.ExitButtonRect.Width() > 0)
		{
			const bool newStartHover = PointInRect(UIState.StartButtonRect, X, Y);
			const bool newCoopHover = PointInRect(UIState.CoopButtonRect, X, Y);
			const bool newExitHover = PointInRect(UIState.ExitButtonRect, X, Y);
			if (newStartHover != UIState.StartButtonHover || 
			    newCoopHover != UIState.CoopButtonHover || 
			    newExitHover != UIState.ExitButtonHover)
			{
				UIState.StartButtonHover = newStartHover;
				UIState.CoopButtonHover = newCoopHover;
				UIState.ExitButtonHover = newExitHover;
				GameCanvas->Repaint();
			}
		}
		else
		{
			// принудительно перерисовываем для инициализации кнопок
			GameCanvas->Repaint();
		}
	}
	else if (WorldState == EWorldState::CoopMenu)
	{
		bool needRepaint = false;
		
		const bool newCreateHover = PointInRect(UIState.CreateGameButtonRect, X, Y);
		const bool newJoinHover = PointInRect(UIState.JoinGameButtonRect, X, Y);
		const bool newBackHover = PointInRect(UIState.BackButtonRect, X, Y);
		bool newStartHover = false;
		
		if (NetworkManager.IsHosting() && UIState.StartGameButtonRect.Width() > 0)
		{
			newStartHover = PointInRect(UIState.StartGameButtonRect, X, Y);
		}
		
		if (newCreateHover != UIState.CreateGameButtonHover ||
		    newJoinHover != UIState.JoinGameButtonHover ||
		    newBackHover != UIState.BackButtonHover ||
		    newStartHover != UIState.StartGameButtonHover)
		{
			UIState.CreateGameButtonHover = newCreateHover;
			UIState.JoinGameButtonHover = newJoinHover;
			UIState.BackButtonHover = newBackHover;
			UIState.StartGameButtonHover = newStartHover;
			needRepaint = true;
		}
		
		if (needRepaint)
		{
			GameCanvas->Repaint();
		}
	}
	else if (WorldState == EWorldState::Paused)
	{
		const bool newResumeHover = PointInRect(UIState.ResumeButtonRect, X, Y);
		const bool newPauseMenuHover = PointInRect(UIState.PauseMenuButtonRect, X, Y);
		if (newResumeHover != UIState.ResumeButtonHover || newPauseMenuHover != UIState.PauseMenuButtonHover)
		{
			UIState.ResumeButtonHover = newResumeHover;
			UIState.PauseMenuButtonHover = newPauseMenuHover;
			GameCanvas->Repaint();
		}
	}
	else if (WorldState == EWorldState::GameOver)
	{
		const bool newRestartHover = PointInRect(UIState.RestartButtonRect, X, Y);
		const bool newMenuHover = PointInRect(UIState.MenuButtonRect, X, Y);
		if (newRestartHover != UIState.RestartButtonHover || newMenuHover != UIState.MenuButtonHover)
		{
			UIState.RestartButtonHover = newRestartHover;
			UIState.MenuButtonHover = newMenuHover;
			GameCanvas->Repaint();
		}
	}
	else if (WorldState == EWorldState::ChoosingUpgrade)
	{
		// hover проверяется в UpdateGame для постоянного обновления
		// здесь просто обновляем позицию мыши
	}
	else
	{
		UIState.RestartButtonHover = false;
		UIState.MenuButtonHover = false;
		UIState.StartButtonHover = false;
		UIState.ExitButtonHover = false;
		UIState.ResumeButtonHover = false;
		UIState.PauseMenuButtonHover = false;
	}
}
//---------------------------------------------------------------------------
void __fastcall TForm1::GameCanvasMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if (WorldState == EWorldState::MainMenu && Button == mbLeft)
	{
		// проверяем клики только если кнопки инициализированы
		if (UIState.StartButtonRect.Width() > 0 && UIState.ExitButtonRect.Width() > 0)
		{
			if (PointInRect(UIState.StartButtonRect, X, Y))
			{
				ResetGame();
				WorldState = EWorldState::Playing;
				return;
			}
			else if (PointInRect(UIState.CoopButtonRect, X, Y))
			{
				// Переход в меню кооператива
				WorldState = EWorldState::CoopMenu;
				return;
			}
			else if (PointInRect(UIState.ExitButtonRect, X, Y))
			{
				Application->Terminate();
				return;
			}
		}
		return;
	}
	
	if (WorldState == EWorldState::Paused && Button == mbLeft)
	{
		if (PointInRect(UIState.ResumeButtonRect, X, Y))
		{
			WorldState = EWorldState::Playing;
		}
		else if (PointInRect(UIState.PauseMenuButtonRect, X, Y))
		{
			WorldState = EWorldState::MainMenu;
		}
		return;
	}

	if (WorldState == EWorldState::GameOver && Button == mbLeft)
	{
		if (PointInRect(UIState.RestartButtonRect, X, Y))
		{
			ResetGame();
		}
		else if (PointInRect(UIState.MenuButtonRect, X, Y))
		{
			// перезагружаем рекорды при возврате в главное меню
			RecordsManager.Load();
			WorldState = EWorldState::MainMenu;
		}
		return;
	}

	if (WorldState == EWorldState::ChoosingUpgrade && Button == mbLeft)
	{
		uint8_t localPlayerID = World->GetLocalPlayerID();
		for (size_t i = 0; i < UIState.UpgradeButtonRects.size(); i++)
		{
			if (PointInRect(UIState.UpgradeButtonRects[i], X, Y))
			{
				World->SelectUpgrade(static_cast<int>(i), localPlayerID);
				return;
			}
		}
		return;
	}

	// Обработка меню кооператива
	if (WorldState == EWorldState::CoopMenu && Button == mbLeft)
	{
		// Кнопка "Назад"
		if (PointInRect(UIState.BackButtonRect, X, Y))
		{
			NetworkManager.Disconnect();
			WorldState = EWorldState::MainMenu;
			return;
		}
		
		// Кнопка "Создать игру"
		if (PointInRect(UIState.CreateGameButtonRect, X, Y))
		{
			if (NetworkManager.StartHosting("Player1", 7777))
			{
				// Игра создана, можно начать
			}
			return;
		}
		
		// Кнопка "Присоединиться к игре"
		if (PointInRect(UIState.JoinGameButtonRect, X, Y))
		{
			// Переключаемся в режим ввода IP
			UIState.IPInputFocused = true;
			return;
		}
		
		// Кнопка "Начать игру" (только для хоста)
		if (NetworkManager.IsHosting() && PointInRect(UIState.StartGameButtonRect, X, Y))
		{
			// Начинаем игру
			World->InitializeNetworkGame(0, true); // хост имеет ID 0
			ResetGame();
			IsNetworkGameActive = true;
			WorldState = EWorldState::Playing;
			
			// Сразу отправляем пакет о смене состояния, чтобы клиенты не ждали таймера
			NeonGame::TGameStateUpdatePacket statePacket;
			statePacket.Header.Type = NeonGame::EPacketType::GameStateUpdate;
			statePacket.WaveNumber = World->GetWaveManager().GetCurrentWave();
			statePacket.EnemiesAlive = 0;
			statePacket.WorldState = static_cast<uint8_t>(WorldState);
			statePacket.FrameNumber = FrameNumber;
			NetworkManager.BroadcastPacket(statePacket);
			
			return;
		}
		
		// Клик по полю ввода IP
		if (PointInRect(UIState.IPInputRect, X, Y))
		{
			UIState.IPInputFocused = true;
			return;
		}
		else
		{
			UIState.IPInputFocused = false;
		}
		
		return;
	}

	if (WorldState == EWorldState::Playing)
	{
		if (Button == mbLeft)
			InputState.PrimaryFire = true;
		else if (Button == mbRight)
			InputState.AltFire = true;
	}
}
//---------------------------------------------------------------------------
void __fastcall TForm1::GameCanvasMouseUp(TObject *Sender, TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if (WorldState == EWorldState::GameOver)
		return;

	if (Button == mbLeft)
		InputState.PrimaryFire = false;
	else if (Button == mbRight)
		InputState.AltFire = false;
}
//---------------------------------------------------------------------------
