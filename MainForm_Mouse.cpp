#include <vcl.h>
#pragma hdrstop

#include "MainForm.h"
#include <System.SysUtils.hpp>

namespace
{
bool IsValidIPv4(const std::string &ip)
{
	if (ip.empty())
		return false;

	int parts = 0;
	int value = -1;
	bool hasDigit = false;

	for (size_t i = 0; i <= ip.size(); ++i)
	{
		char c = (i < ip.size()) ? ip[i] : '.';

		if (c == '.')
		{
			if (!hasDigit)
				return false;
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
			return false;
		}
	}

	return parts == 4;
}
}

void __fastcall TForm1::GameCanvasMouseMove(TObject *Sender, TShiftState Shift, int X, int Y)
{
	InputState.MouseClient = Point(X, Y);
	InputState.HasMouse = true;

	if (WorldState == EWorldState::MainMenu)
	{
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

void __fastcall TForm1::GameCanvasMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if (WorldState == EWorldState::MainMenu && Button == mbLeft)
	{
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
			if (IsNetworkGameActive)
			{
				if (NetworkManager.IsHosting())
				{
					NetworkManager.BroadcastReturnToLobby();
					ReturnToCoopLobby();
				}
				else
					NetworkManager.SendReturnToLobbyRequest();
			}
			else
				ResetGame();
		}
		else if (PointInRect(UIState.MenuButtonRect, X, Y))
		{
			RecordsManager.Load();
			WorldState = EWorldState::MainMenu;
		}
		return;
	}

	if (WorldState == EWorldState::ChoosingUpgrade && Button == mbLeft && World)
	{
		for (size_t i = 0; i < UIState.UpgradeButtonRects.size(); i++)
		{
			if (PointInRect(UIState.UpgradeButtonRects[i], X, Y))
			{
				const uint8_t localPlayerID = World->GetLocalPlayerID();
				if (World->IsNetworkGameActive() && !NetworkManager.IsHosting())
					NetworkManager.SendUpgradeSelect(static_cast<uint8_t>(i));
				else
					World->SelectUpgrade(static_cast<int>(i), localPlayerID);
				return;
			}
		}
		return;
	}

	if (WorldState == EWorldState::Playing && Button == mbLeft && World && World->IsNetworkGameActive() && World->IsWaitingForUpgradeChoice())
	{
		for (size_t i = 0; i < UIState.UpgradeButtonRects.size(); i++)
		{
			if (PointInRect(UIState.UpgradeButtonRects[i], X, Y))
			{
				const uint8_t localPlayerID = World->GetLocalPlayerID();
				if (!NetworkManager.IsHosting())
					NetworkManager.SendUpgradeSelect(static_cast<uint8_t>(i));
				else
					World->SelectUpgrade(static_cast<int>(i), localPlayerID);
				return;
			}
		}
	}

	if (WorldState == EWorldState::CoopMenu && Button == mbLeft)
	{
		if (PointInRect(UIState.BackButtonRect, X, Y))
		{
			NetworkManager.Disconnect();
			WorldState = EWorldState::MainMenu;
			return;
		}

		if (PointInRect(UIState.CreateGameButtonRect, X, Y))
		{
			if (NetworkManager.StartHosting("Player1", 7777))
			{
			}
			return;
		}

		if (PointInRect(UIState.JoinGameButtonRect, X, Y))
		{
			if (!UIState.IPAddress.empty())
			{
				if (!IsValidIPv4(UIState.IPAddress))
				{
					Application->MessageBox(L"Неверный IP-адрес. Используйте формат вида 192.168.0.10", L"Ошибка", MB_OK | MB_ICONWARNING);
				}
				else
				{
					if (!NetworkManager.ConnectToGame(UIState.IPAddress, "Player1", 7777))
					{
						Application->MessageBox(L"Не удалось подключиться к хосту.", L"Ошибка подключения", MB_OK | MB_ICONERROR);
					}
				}
			}
			else
			{
				UIState.IPInputFocused = true;
			}
			return;
		}

		if (NetworkManager.IsHosting() && PointInRect(UIState.StartGameButtonRect, X, Y))
		{
			ResetGame();

			World->InitializeNetworkGame(0, true);

			uint8_t playerCount = 1;
			const auto &clients = NetworkManager.GetClients();
			for (const auto &client : clients)
			{
				if (client.IsConnected && client.PlayerID != 0)
				{
					playerCount++;
				}
			}

			World->SetPlayerCount(playerCount);

			IsNetworkGameActive = true;
			WorldState = EWorldState::Playing;
			return;
		}

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
		if (World && World->IsNetworkGameActive() && !World->IsPlayerAlive())
			return;
		if (Button == mbLeft)
			InputState.PrimaryFire = true;
		else if (Button == mbRight)
			InputState.AltFire = true;
	}
}

void __fastcall TForm1::GameCanvasMouseUp(TObject *Sender, TMouseButton Button, TShiftState Shift, int X, int Y)
{
	if (WorldState == EWorldState::GameOver)
		return;

	if (Button == mbLeft)
		InputState.PrimaryFire = false;
	else if (Button == mbRight)
		InputState.AltFire = false;
}
