#include <vcl.h>
#pragma hdrstop

#include "MainForm.h"
#include <System.SysUtils.hpp>
#include <algorithm>
#include <cstdint>

void TForm1::ResetGame()
{
	World->Reset();
	WorldState = EWorldState::Playing;
	UIState.OverlayAlpha = 0.0f;
	UIState.RestartButtonRect = Rect(0, 0, 0, 0);
	UIState.MenuButtonRect = Rect(0, 0, 0, 0);
	UIState.UpgradeButtonRects.clear();
	UIState.ShowStatsPanel = false;
	UIState.UpgradeButtonHovers.clear();
	UIState.UpgradeButtonHoverTime.clear();
	UIState.StartButtonHover = false;
	UIState.ExitButtonHover = false;
	UIState.ResumeButtonHover = false;
	UIState.PauseMenuButtonHover = false;

	IsNetworkGameActive = false;
	FrameNumber = 0;
	NetworkUpdateTimer = 0.0f;
	PredictedStates.clear();
	InterpolatedPlayers.clear();
	InterpolatedEnemies.clear();
	InterpEnemyLastNetInstanceId.clear();
	HostRemoteAuthValid = false;
	HostRemoteAuthPos.clear();
	HostRemoteAuthFacing.clear();

	UIState.CreateGameButtonHover = false;
	UIState.JoinGameButtonHover = false;
	UIState.BackButtonHover = false;
	UIState.StartGameButtonHover = false;
	
	UIState.IPAddress.clear();
	UIState.IPInputFocused = false;
	UIState.PlayerNames.clear();
	UIState.PlayerReady.clear();

	HostLastSimulatedInputs.assign(4, TInputState());
}

void TForm1::ReturnToCoopLobby()
{
	if (!World)
		return;

	if (!NetworkManager.IsHosting())
		NetworkManager.EndClientGameSession();

	World->LeaveNetworkMatchAndReset();
	IsNetworkGameActive = false;
	FrameNumber = 0;
	NetworkUpdateTimer = 0.0f;
	PredictedStates.clear();
	InterpolatedPlayers.clear();
	InterpolatedEnemies.clear();
	InterpEnemyLastNetInstanceId.clear();
	HostRemoteAuthValid = false;
	HostRemoteAuthPos.clear();
	HostRemoteAuthFacing.clear();
	HostLastSimulatedInputs.assign(4, TInputState());
	UIState.ShowStatsPanel = false;
	WorldState = EWorldState::CoopMenu;
}

void TForm1::UpdateGame(double deltaSeconds)
{
	if (!World)
		return;

	const float dt = static_cast<float>(deltaSeconds);

	if (IsNetworkGameActive || NetworkManager.GetState() != NeonGame::ENetworkState::Disconnected)
	{
		UpdateNetwork(dt);
	}

	const EWorldState previousState = WorldState;
	if (WorldState == EWorldState::Playing || WorldState == EWorldState::ChoosingUpgrade)
	{
		if (IsNetworkGameActive && NetworkManager.IsHosting())
		{
			if (HostLastSimulatedInputs.size() != 4)
				HostLastSimulatedInputs.assign(4, TInputState());

			std::vector<TInputState> playerInputs = HostLastSimulatedInputs;
			playerInputs[0] = InputState;

			std::vector<NeonGame::TPlayerInputPacket> receivedInputs = NetworkManager.GetReceivedInputs();
			for (const auto &inputPacket : receivedInputs)
			{
				if (inputPacket.PlayerID < 4 && inputPacket.PlayerID != 0)
				{
					TInputState &state = playerInputs[inputPacket.PlayerID];
					state.MoveUp = inputPacket.InputUp;
					state.MoveDown = inputPacket.InputDown;
					state.MoveLeft = inputPacket.InputLeft;
					state.MoveRight = inputPacket.InputRight;
					state.PrimaryFire = inputPacket.IsShooting;
					state.AltFire = inputPacket.IsAltShooting;
					state.AimWorldX = inputPacket.MouseX;
					state.AimWorldY = inputPacket.MouseY;
					state.HasMouse = true;
				}
			}

			HostLastSimulatedInputs = playerInputs;

			RestoreHostRemoteAuthoritativePositions();

			{
				std::vector<NeonGame::TUpgradeSelectPacket> selections = NetworkManager.GetReceivedUpgradeSelects();
				for (const auto &sel : selections)
				{
					if (sel.PlayerID < World->GetPlayerCount() && sel.ChoiceIndex < 3)
					{
						World->SelectUpgrade(static_cast<int>(sel.ChoiceIndex), sel.PlayerID);
					}
				}
			}

			World->Update(dt, playerInputs, GameCanvas->Width, GameCanvas->Height);

			HostFeedRemoteSnapshotsAfterSim();
			InterpolateRemotePlayers(dt);
		}
		else if (IsNetworkGameActive && !NetworkManager.IsHosting())
		{
			ApplyClientSidePrediction(dt);

			InterpolateNetworkEnemies(dt);

			World->Update(dt, InputState, GameCanvas->Width, GameCanvas->Height);

			InterpolateRemotePlayers(dt);
		}
		else
		{
			World->Update(dt, InputState, GameCanvas->Width, GameCanvas->Height);
		}

		WorldState = World->GetState();
	}

	if (World->GetState() == EWorldState::GameOver)
		WorldState = EWorldState::GameOver;

	if (previousState != EWorldState::GameOver && WorldState == EWorldState::GameOver)
	{
		const TGameStats &stats = World->GetStats();
		RecordsManager.UpdateRecords(stats.CurrentWave, stats.EnemiesDefeated, stats.RunTimeSeconds);
		RecordsManager.Save();
	}
}

void TForm1::RenderGame()
{
	if (!GameCanvas || !World)
		return;

	TCanvas *canvas = GameCanvas->Canvas;
	if (!canvas)
		return;

	canvas->Brush->Style = bsSolid;
	canvas->Brush->Color = static_cast<TColor>(RGB(0, 0, 0));
	canvas->FillRect(GameCanvas->ClientRect);

	if (WorldState == EWorldState::MainMenu)
	{
		UiRenderer.DrawMainMenu(canvas, RecordsManager.GetRecords(), UIState);
		return;
	}

	if (WorldState == EWorldState::CoopMenu)
	{
		UiRenderer.DrawCoopMenu(canvas, UIState, &NetworkManager);
		return;
	}

	World->RenderScene(canvas);

	if (WorldState == EWorldState::Playing || WorldState == EWorldState::Paused || WorldState == EWorldState::ChoosingUpgrade || WorldState == EWorldState::GameOver)
	{
		const TGameStats &stats = World->GetStats();
		const int localPlayerId = World->GetLocalPlayerID();
		const auto localStats = World->GetPlayerStats(localPlayerId);

		UiRenderer.DrawHud(canvas,
			World->GetPlayerHealthRatio(),
			World->GetPlayerHealth(),
			World->GetPlayerMaxHealth(),
			World->GetPlayerExperienceRatio(),
			World->GetPlayerExperience(),
			World->GetPlayerExperienceToNext(),
			World->GetPlayerLevel(),
			stats,
			UIState.ShowStatsPanel);

		UiRenderer.DrawPlayerStats(canvas, localStats, UIState);

		if (World->HasActiveBoss())
		{
			UiRenderer.DrawBossHealthBar(canvas, World->GetBossHealthRatio(), World->GetBossHealth(), World->GetBossMaxHealth());
		}

		UiRenderer.DrawCooldownIndicators(canvas,
			World->GetPrimaryFireCooldown(),
			World->GetAltFireCooldown(),
			World->GetPrimaryFireMaxCooldown(),
			World->GetAltFireMaxCooldown());

		std::vector<TPointF> playerPositions;
		playerPositions.reserve(World->GetPlayerCount());
		for (uint8_t i = 0; i < World->GetPlayerCount(); i++)
		{
			TGamePlayer* p = World->GetPlayer(i);
			playerPositions.push_back(p ? p->GetPosition() : TPointF(0, 0));
		}

		UiRenderer.DrawMinimap(canvas, playerPositions, World->GetEnemyPositions(),
			World->GetBossPosition(), World->HasActiveBoss(),
			NeonGame::WorldWidth, NeonGame::WorldHeight,
			World->GetLocalPlayerID());

		if (IsNetworkGameActive)
		{
			std::vector<std::string> playerNames;
			playerNames.reserve(World->GetPlayerCount());
			for (uint8_t i = 0; i < World->GetPlayerCount(); i++)
				playerNames.push_back(NetworkManager.GetPlayerName(i));

			UiRenderer.DrawPlayerNames(canvas, playerPositions, playerNames, World->GetCameraPosition(), World->GetLocalPlayerID());
		}
	}

	if (WorldState == EWorldState::Paused)
	{
		UiRenderer.DrawPauseMenu(canvas, UIState);
	}

	if (WorldState == EWorldState::ChoosingUpgrade ||
		(WorldState == EWorldState::Playing && World->IsNetworkGameActive() && World->IsWaitingForUpgradeChoice()))
	{
		const auto &upgrades = World->GetAvailableUpgrades();
		UiRenderer.DrawUpgradeMenu(canvas, upgrades, UIState);
	}

	if (WorldState == EWorldState::GameOver)
	{
		UiRenderer.DrawGameOver(canvas, World->GetStats(), RecordsManager.GetRecords(), UIState, IsNetworkGameActive);
	}
}

void __fastcall TForm1::GameCanvasPaint(TObject *Sender)
{
	RenderGame();
}

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

