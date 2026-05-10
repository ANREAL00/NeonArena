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

void __fastcall TForm1::FormKeyDown(TObject *Sender, WORD &Key, TShiftState Shift)
{
	if (Key == VK_ESCAPE && WorldState == EWorldState::Playing)
	{
		WorldState = EWorldState::Paused;
		return;
	}

	if (Key == VK_ESCAPE && WorldState == EWorldState::Paused)
	{
		WorldState = EWorldState::Playing;
		return;
	}

	if (WorldState == EWorldState::MainMenu)
	{
		return;
	}

	if (WorldState == EWorldState::CoopMenu)
	{
		if (UIState.IPInputFocused)
		{
			if (Key == VK_RETURN)
			{
				if (!UIState.IPAddress.empty())
				{
					if (!IsValidIPv4(UIState.IPAddress))
					{
						Application->MessageBox(L"Неверный IP-адрес. Используйте формат вида 192.168.0.10", L"Ошибка", MB_OK | MB_ICONWARNING);
					}
					else
					{
						if (NetworkManager.ConnectToGame(UIState.IPAddress, "Player1", 7777))
						{
						}
						else
						{
							Application->MessageBox(L"Не удалось подключиться к хосту.", L"Ошибка подключения", MB_OK | MB_ICONERROR);
						}
					}
				}
				UIState.IPInputFocused = false;
				GameCanvas->Repaint();
				return;
			}

			if (Key == VK_ESCAPE)
			{
				UIState.IPInputFocused = false;
				GameCanvas->Repaint();
				return;
			}

			if (Key == VK_BACK)
			{
				if (!UIState.IPAddress.empty())
				{
					UIState.IPAddress.pop_back();
					GameCanvas->Repaint();
				}
				return;
			}

			if (UIState.IPAddress.length() < 15)
			{
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
		return;
	}

	if (WorldState == EWorldState::Paused)
	{
		if (Key == VK_RETURN || Key == VK_SPACE)
		{
			WorldState = EWorldState::Playing;
		}

		return;
	}

	if (WorldState == EWorldState::ChoosingUpgrade)
	{
		uint8_t localPlayerID = World->GetLocalPlayerID();
		if (Key == '1' || Key == VK_NUMPAD1)
		{
			if (!(World->IsNetworkGameActive() && !NetworkManager.IsHosting()))
				World->SelectUpgrade(0, localPlayerID);
			else
				NetworkManager.SendUpgradeSelect(0);
		}
		else if (Key == '2' || Key == VK_NUMPAD2)
		{
			if (!(World->IsNetworkGameActive() && !NetworkManager.IsHosting()))
				World->SelectUpgrade(1, localPlayerID);
			else
				NetworkManager.SendUpgradeSelect(1);
		}
		else if (Key == '3' || Key == VK_NUMPAD3)
		{
			if (!(World->IsNetworkGameActive() && !NetworkManager.IsHosting()))
				World->SelectUpgrade(2, localPlayerID);
			else
				NetworkManager.SendUpgradeSelect(2);
		}
		return;
	}

	if (WorldState == EWorldState::Playing && World && World->IsNetworkGameActive() && World->IsWaitingForUpgradeChoice())
	{
		uint8_t localPlayerID = World->GetLocalPlayerID();
		if (Key == '1' || Key == VK_NUMPAD1)
		{
			if (!NetworkManager.IsHosting())
				NetworkManager.SendUpgradeSelect(0);
			else
				World->SelectUpgrade(0, localPlayerID);
			return;
		}
		else if (Key == '2' || Key == VK_NUMPAD2)
		{
			if (!NetworkManager.IsHosting())
				NetworkManager.SendUpgradeSelect(1);
			else
				World->SelectUpgrade(1, localPlayerID);
			return;
		}
		else if (Key == '3' || Key == VK_NUMPAD3)
		{
			if (!NetworkManager.IsHosting())
				NetworkManager.SendUpgradeSelect(2);
			else
				World->SelectUpgrade(2, localPlayerID);
			return;
		}
	}

	if (WorldState == EWorldState::Playing && (Key == VK_TAB || Key == 9))
	{
		UIState.ShowStatsPanel = !UIState.ShowStatsPanel;
		Key = 0;
		return;
	}

	if (WorldState == EWorldState::Playing && World && World->IsNetworkGameActive())
	{
		if (!World->IsPlayerAlive())
		{
			if (Key == VK_OEM_4 || Key == VK_LEFT)
			{
				World->CycleSpectateTarget(-1);
				Key = 0;
				return;
			}
			if (Key == VK_OEM_6 || Key == VK_RIGHT)
			{
				World->CycleSpectateTarget(1);
				Key = 0;
				return;
			}
			return;
		}
	}

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

void __fastcall TForm1::FormKeyUp(TObject *Sender, WORD &Key, TShiftState Shift)
{
	if (WorldState == EWorldState::GameOver)
		return;

	if (WorldState == EWorldState::Playing && (Key == VK_TAB || Key == 9))
	{
		UIState.ShowStatsPanel = !UIState.ShowStatsPanel;
		Key = 0;
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
