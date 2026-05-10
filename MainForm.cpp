#include <vcl.h>
#pragma hdrstop

#include "MainForm.h"
#include <System.SysUtils.hpp>

#pragma package(smart_init)
#pragma resource "*.dfm"
TForm1 *Form1;

__fastcall TForm1::TForm1(TComponent* Owner)
	: TForm(Owner),
	  LastTick(0),
	  HasLastTick(false),
	  WorldState(EWorldState::MainMenu),
	  IsNetworkGameActive(false),
	  FrameNumber(0),
	  NetworkUpdateTimer(0.0f),
	  HostRemoteAuthValid(false)
{
	World = std::make_unique<TGameWorld>();
	InputState = TInputState{};
	UIState = TGameUIState{};
	UIState.ShowStatsPanel = false;

	if (NetworkManager.Initialize())
	{
	}
}

void __fastcall TForm1::FormCreate(TObject *Sender)
{
	KeyPreview = true;
	DoubleBuffered = true;

	WindowState = wsMaximized;
	BorderStyle = bsNone;

	World->Reset();
	WorldState = EWorldState::MainMenu;

	LastTick = Now();

	if (GameTimer)
		GameTimer->Enabled = true;
}
