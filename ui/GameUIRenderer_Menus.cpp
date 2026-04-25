#include <vcl.h>
#pragma hdrstop

#include "GameUIRenderer.h"
#include "GameUIRenderer_Utils.h"
#include "GameNetwork.h"
#include <algorithm>

void TGameUIRenderer::DrawMainMenu(TCanvas *canvas, const TGameRecords &records, TGameUIState &uiState) const
{
	if (!canvas)
		return;

	const int width = canvas->ClipRect.Width();
	const int height = canvas->ClipRect.Height();
	const float scale = GetScale(width, height);

	canvas->Font->Name = "Segoe UI";

	canvas->Brush->Color = static_cast<TColor>(RGB(5, 5, 15));
	canvas->FillRect(canvas->ClipRect);

	canvas->Font->Name = "Segoe UI";
	canvas->Font->Size = ScaleFontSize(48, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(0, 200, 255));
	canvas->Font->Style = TFontStyles() << fsBold;
	const UnicodeString title = L"NEON ARENA";
	const TSize titleSize = canvas->TextExtent(title);
	const int titleX = (width - titleSize.cx) / 2;
	const int titleY = height / 4;
	canvas->TextOut(titleX, titleY, title);

	const int buttonWidth = ScaleValue(300, width, height);
	const int buttonHeight = ScaleValue(60, width, height);
	const int buttonX = (width - buttonWidth) / 2;
	const int startY = height / 2;
	const int buttonSpacing = ScaleValue(80, width, height);

	uiState.StartButtonRect = Rect(buttonX, startY, buttonX + buttonWidth, startY + buttonHeight);
	const float startHoverAnim = uiState.StartButtonHover ? 1.0f : 0.0f;
	const int startScale = static_cast<int>(ScaleValue(5, width, height) * startHoverAnim);
	const TRect startRect(uiState.StartButtonRect.Left - startScale, uiState.StartButtonRect.Top - startScale,
		uiState.StartButtonRect.Right + startScale, uiState.StartButtonRect.Bottom + startScale);

	canvas->Brush->Color = uiState.StartButtonHover ?
		static_cast<TColor>(RGB(0, 150, 220)) : static_cast<TColor>(RGB(0, 100, 150));
	canvas->Pen->Color = static_cast<TColor>(RGB(0, 200, 255));
	canvas->Pen->Width = ScaleValue(2, width, height);
	canvas->Rectangle(startRect);

	canvas->Font->Size = ScaleFontSize(24, width, height);
	canvas->Font->Color = clWhite;
	canvas->Font->Style = TFontStyles() << fsBold;
	const UnicodeString startText = L"НАЧАТЬ ИГРУ";
	const TSize startTextSize = canvas->TextExtent(startText);
	const int startTextX = buttonX + (buttonWidth - startTextSize.cx) / 2;
	const int startTextY = startY + (buttonHeight - startTextSize.cy) / 2;
	canvas->TextOut(startTextX, startTextY, startText);

	uiState.ExitButtonRect = Rect(buttonX, startY + buttonSpacing, buttonX + buttonWidth, startY + buttonSpacing + buttonHeight);
	const float exitHoverAnim = uiState.ExitButtonHover ? 1.0f : 0.0f;
	const int exitScale = static_cast<int>(ScaleValue(5, width, height) * exitHoverAnim);
	const TRect exitRect(uiState.ExitButtonRect.Left - exitScale, uiState.ExitButtonRect.Top - exitScale,
		uiState.ExitButtonRect.Right + exitScale, uiState.ExitButtonRect.Bottom + exitScale);

	canvas->Brush->Color = uiState.ExitButtonHover ?
		static_cast<TColor>(RGB(200, 50, 50)) : static_cast<TColor>(RGB(150, 30, 30));
	canvas->Pen->Color = static_cast<TColor>(RGB(255, 100, 100));
	canvas->Pen->Width = ScaleValue(2, width, height);
	canvas->Rectangle(exitRect);

	const UnicodeString exitText = L"ВЫХОД";
	const TSize exitTextSize = canvas->TextExtent(exitText);
	const int exitTextX = buttonX + (buttonWidth - exitTextSize.cx) / 2;
	const int exitTextY = startY + buttonSpacing + (buttonHeight - exitTextSize.cy) / 2;
	canvas->TextOut(exitTextX, exitTextY, exitText);

	uiState.CoopButtonRect = Rect(buttonX, startY + buttonSpacing * 2, buttonX + buttonWidth, startY + buttonSpacing * 2 + buttonHeight);
	const float coopHoverAnim = uiState.CoopButtonHover ? 1.0f : 0.0f;
	const int coopScale = static_cast<int>(ScaleValue(5, width, height) * coopHoverAnim);
	const TRect coopRect(uiState.CoopButtonRect.Left - coopScale, uiState.CoopButtonRect.Top - coopScale,
		uiState.CoopButtonRect.Right + coopScale, uiState.CoopButtonRect.Bottom + coopScale);

	canvas->Brush->Color = uiState.CoopButtonHover ?
		static_cast<TColor>(RGB(100, 0, 200)) : static_cast<TColor>(RGB(70, 0, 150));
	canvas->Pen->Color = static_cast<TColor>(RGB(150, 0, 255));
	canvas->Pen->Width = ScaleValue(2, width, height);
	canvas->Rectangle(coopRect);

	const UnicodeString coopText = L"КООПЕРАТИВ";
	const TSize coopTextSize = canvas->TextExtent(coopText);
	const int coopTextX = buttonX + (buttonWidth - coopTextSize.cx) / 2;
	const int coopTextY = startY + buttonSpacing * 2 + (buttonHeight - coopTextSize.cy) / 2;
	canvas->TextOut(coopTextX, coopTextY, coopText);

	canvas->Brush->Color = static_cast<TColor>(RGB(0, 0, 0));
	canvas->Font->Size = ScaleFontSize(18, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(255, 200, 100));
	canvas->Font->Style = TFontStyles() << fsBold;
	const UnicodeString recordsTitle = L"Рекорды:";
	const TSize recordsTitleSize = canvas->TextExtent(recordsTitle);
	const int recordsX = (width - recordsTitleSize.cx) / 2;
	int recordsY = height - ScaleValue(140, width, height);
	canvas->TextOut(recordsX, recordsY, recordsTitle);
	recordsY += ScaleValue(30, width, height);

	canvas->Font->Size = ScaleFontSize(16, width, height);
	canvas->Font->Style = TFontStyles();
	canvas->Font->Color = static_cast<TColor>(RGB(200, 200, 200));

	const UnicodeString bestWaveText = L"Лучшая волна: " + IntToStr(records.BestWave);
	const UnicodeString bestKillText = L"Убито врагов: " + IntToStr(records.BestEnemiesKilled);
	const int bestTotalSeconds = static_cast<int>(records.BestRunTime + 0.5f);
	const UnicodeString bestTimeText = L"Время: " + NeonGameUIRendererUtils::FormatMinutesSeconds(bestTotalSeconds);

	const TSize bestWaveSize = canvas->TextExtent(bestWaveText);
	const TSize bestKillSize = canvas->TextExtent(bestKillText);
	const TSize bestTimeSize = canvas->TextExtent(bestTimeText);

	const int maxWidth = std::max({bestWaveSize.cx, bestKillSize.cx, bestTimeSize.cx});
	const int recordsTextX = (width - maxWidth) / 2;

	canvas->TextOut(recordsTextX, recordsY, bestWaveText); recordsY += ScaleValue(26, width, height);
	canvas->TextOut(recordsTextX, recordsY, bestKillText); recordsY += ScaleValue(26, width, height);
	canvas->TextOut(recordsTextX, recordsY, bestTimeText);
}

void TGameUIRenderer::DrawPauseMenu(TCanvas *canvas, TGameUIState &uiState) const
{
	if (!canvas)
		return;

	const int width = canvas->ClipRect.Width();
	const int height = canvas->ClipRect.Height();
	const float scale = GetScale(width, height);

	canvas->Font->Name = "Segoe UI";

	canvas->Brush->Style = bsSolid;
	canvas->Brush->Color = static_cast<TColor>(RGB(0, 0, 0));
	canvas->Pen->Style = psClear;
	const int alpha = 180;
	canvas->Brush->Color = static_cast<TColor>(RGB(0, 0, 0));
	canvas->FillRect(canvas->ClipRect);

	const int panelWidth = ScaleValue(400, width, height);
	const int panelHeight = ScaleValue(300, width, height);
	const int panelX = (width - panelWidth) / 2;
	const int panelY = (height - panelHeight) / 2;
	const TRect panelRect(panelX, panelY, panelX + panelWidth, panelY + panelHeight);

	canvas->Brush->Color = static_cast<TColor>(RGB(10, 10, 20));
	canvas->Pen->Color = static_cast<TColor>(RGB(0, 150, 220));
	canvas->Pen->Width = ScaleValue(2, width, height);
	canvas->Pen->Style = psSolid;
	canvas->Rectangle(panelRect);

	canvas->Font->Name = "Segoe UI";
	canvas->Font->Size = ScaleFontSize(36, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(0, 200, 255));
	canvas->Font->Style = TFontStyles() << fsBold;
	const UnicodeString pauseText = L"ПАУЗА";
	const TSize pauseSize = canvas->TextExtent(pauseText);
	const int pauseX = panelX + (panelWidth - pauseSize.cx) / 2;
	const int pauseY = panelY + ScaleValue(30, width, height);
	canvas->TextOut(pauseX, pauseY, pauseText);

	const int buttonWidth = ScaleValue(280, width, height);
	const int buttonHeight = ScaleValue(50, width, height);
	const int buttonX = panelX + (panelWidth - buttonWidth) / 2;
	const int resumeY = panelY + ScaleValue(100, width, height);
	const int menuY = panelY + ScaleValue(170, width, height);

	uiState.ResumeButtonRect = Rect(buttonX, resumeY, buttonX + buttonWidth, resumeY + buttonHeight);
	const float resumeHoverAnim = uiState.ResumeButtonHover ? 1.0f : 0.0f;
	const int resumeScale = static_cast<int>(ScaleValue(3, width, height) * resumeHoverAnim);
	const TRect resumeRect(uiState.ResumeButtonRect.Left - resumeScale, uiState.ResumeButtonRect.Top - resumeScale,
		uiState.ResumeButtonRect.Right + resumeScale, uiState.ResumeButtonRect.Bottom + resumeScale);

	canvas->Brush->Color = uiState.ResumeButtonHover ?
		static_cast<TColor>(RGB(0, 150, 220)) : static_cast<TColor>(RGB(0, 100, 150));
	canvas->Pen->Color = static_cast<TColor>(RGB(0, 200, 255));
	canvas->Pen->Width = ScaleValue(2, width, height);
	canvas->Rectangle(resumeRect);

	canvas->Font->Size = ScaleFontSize(20, width, height);
	canvas->Font->Color = clWhite;
	canvas->Font->Style = TFontStyles() << fsBold;
	const UnicodeString resumeText = L"ПРОДОЛЖИТЬ";
	const TSize resumeTextSize = canvas->TextExtent(resumeText);
	const int resumeTextX = buttonX + (buttonWidth - resumeTextSize.cx) / 2;
	const int resumeTextY = resumeY + (buttonHeight - resumeTextSize.cy) / 2;
	canvas->TextOut(resumeTextX, resumeTextY, resumeText);

	uiState.PauseMenuButtonRect = Rect(buttonX, menuY, buttonX + buttonWidth, menuY + buttonHeight);
	const float menuHoverAnim = uiState.PauseMenuButtonHover ? 1.0f : 0.0f;
	const int menuScale = static_cast<int>(ScaleValue(3, width, height) * menuHoverAnim);
	const TRect menuRect(uiState.PauseMenuButtonRect.Left - menuScale, uiState.PauseMenuButtonRect.Top - menuScale,
		uiState.PauseMenuButtonRect.Right + menuScale, uiState.PauseMenuButtonRect.Bottom + menuScale);

	canvas->Brush->Color = uiState.PauseMenuButtonHover ?
		static_cast<TColor>(RGB(100, 100, 120)) : static_cast<TColor>(RGB(60, 60, 80));
	canvas->Pen->Color = static_cast<TColor>(RGB(150, 150, 170));
	canvas->Pen->Width = ScaleValue(2, width, height);
	canvas->Rectangle(menuRect);

	const UnicodeString menuText = L"В МЕНЮ";
	const TSize menuTextSize = canvas->TextExtent(menuText);
	const int menuTextX = buttonX + (buttonWidth - menuTextSize.cx) / 2;
	const int menuTextY = menuY + (buttonHeight - menuTextSize.cy) / 2;
	canvas->TextOut(menuTextX, menuTextY, menuText);
}

void TGameUIRenderer::DrawCoopMenu(TCanvas *canvas, TGameUIState &uiState,
	const void *networkManagerPtr) const
{
	if (!canvas)
		return;

	const NeonGame::TNetworkManager *networkManager = static_cast<const NeonGame::TNetworkManager*>(networkManagerPtr);
	if (!networkManager)
		return;

	const int width = canvas->ClipRect.Width();
	const int height = canvas->ClipRect.Height();
	const float scale = GetScale(width, height);

	canvas->Font->Name = "Segoe UI";

	canvas->Brush->Color = static_cast<TColor>(RGB(5, 5, 15));
	canvas->FillRect(canvas->ClipRect);

	canvas->Font->Name = "Segoe UI";
	canvas->Font->Size = ScaleFontSize(48, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(0, 200, 255));
	canvas->Font->Style = TFontStyles() << fsBold;
	const UnicodeString title = L"КООПЕРАТИВ";
	const TSize titleSize = canvas->TextExtent(title);
	const int titleX = (width - titleSize.cx) / 2;
	const int titleY = height / 8;
	canvas->TextOut(titleX, titleY, title);

	const int buttonWidth = ScaleValue(300, width, height);
	const int buttonHeight = ScaleValue(60, width, height);
	const int buttonX = (width - buttonWidth) / 2;
	const int startY = height / 3;
	const int buttonSpacing = ScaleValue(80, width, height);

	uiState.CreateGameButtonRect = Rect(buttonX, startY, buttonX + buttonWidth, startY + buttonHeight);
	const float createHoverAnim = uiState.CreateGameButtonHover ? 1.0f : 0.0f;
	const int createScale = static_cast<int>(ScaleValue(5, width, height) * createHoverAnim);
	const TRect createRect(uiState.CreateGameButtonRect.Left - createScale, uiState.CreateGameButtonRect.Top - createScale,
		uiState.CreateGameButtonRect.Right + createScale, uiState.CreateGameButtonRect.Bottom + createScale);

	canvas->Brush->Color = uiState.CreateGameButtonHover ?
		static_cast<TColor>(RGB(0, 150, 220)) : static_cast<TColor>(RGB(0, 100, 150));
	canvas->Pen->Color = static_cast<TColor>(RGB(0, 200, 255));
	canvas->Pen->Width = ScaleValue(2, width, height);
	canvas->Rectangle(createRect);

	canvas->Font->Size = ScaleFontSize(24, width, height);
	canvas->Font->Color = clWhite;
	canvas->Font->Style = TFontStyles() << fsBold;
	const UnicodeString createText = L"СОЗДАТЬ ИГРУ";
	const TSize createTextSize = canvas->TextExtent(createText);
	const int createTextX = buttonX + (buttonWidth - createTextSize.cx) / 2;
	const int createTextY = startY + (buttonHeight - createTextSize.cy) / 2;
	canvas->TextOut(createTextX, createTextY, createText);

	uiState.JoinGameButtonRect = Rect(buttonX, startY + buttonSpacing, buttonX + buttonWidth, startY + buttonSpacing + buttonHeight);
	const float joinHoverAnim = uiState.JoinGameButtonHover ? 1.0f : 0.0f;
	const int joinScale = static_cast<int>(ScaleValue(5, width, height) * joinHoverAnim);
	const TRect joinRect(uiState.JoinGameButtonRect.Left - joinScale, uiState.JoinGameButtonRect.Top - joinScale,
		uiState.JoinGameButtonRect.Right + joinScale, uiState.JoinGameButtonRect.Bottom + joinScale);

	canvas->Brush->Color = uiState.JoinGameButtonHover ?
		static_cast<TColor>(RGB(100, 0, 200)) : static_cast<TColor>(RGB(70, 0, 150));
	canvas->Pen->Color = static_cast<TColor>(RGB(150, 0, 255));
	canvas->Pen->Width = ScaleValue(2, width, height);
	canvas->Rectangle(joinRect);

	const UnicodeString joinText = L"ПРИСОЕДИНИТЬСЯ";
	const TSize joinTextSize = canvas->TextExtent(joinText);
	const int joinTextX = buttonX + (buttonWidth - joinTextSize.cx) / 2;
	const int joinTextY = startY + buttonSpacing + (buttonHeight - joinTextSize.cy) / 2;
	canvas->TextOut(joinTextX, joinTextY, joinText);

	if (!networkManager->IsHosting())
	{
		const int inputWidth = ScaleValue(400, width, height);
		const int inputHeight = ScaleValue(40, width, height);
		const int inputX = (width - inputWidth) / 2;
		const int inputY = startY + buttonSpacing * 2;
		uiState.IPInputRect = Rect(inputX, inputY, inputX + inputWidth, inputY + inputHeight);

		canvas->Brush->Color = uiState.IPInputFocused ?
			static_cast<TColor>(RGB(30, 30, 40)) : static_cast<TColor>(RGB(20, 20, 30));
		canvas->Pen->Color = uiState.IPInputFocused ?
			static_cast<TColor>(RGB(0, 255, 255)) : static_cast<TColor>(RGB(0, 200, 255));
		canvas->Pen->Width = ScaleValue(2, width, height);
		canvas->Rectangle(uiState.IPInputRect);

		canvas->Font->Size = ScaleFontSize(18, width, height);
		canvas->Font->Color = clWhite;
		canvas->Font->Style = TFontStyles();
		UnicodeString ipText = UnicodeString(uiState.IPAddress.c_str());
		if (ipText.IsEmpty())
		{
			ipText = L"Введите IP адрес...";
			canvas->Font->Color = static_cast<TColor>(RGB(150, 150, 150));
		}
		const int ipTextX = inputX + ScaleValue(10, width, height);
		const int ipTextY = inputY + (inputHeight - canvas->TextHeight(ipText)) / 2;
		canvas->TextOut(ipTextX, ipTextY, ipText);

		if (uiState.IPInputFocused)
		{
			const int cursorX = ipTextX + canvas->TextWidth(UnicodeString(uiState.IPAddress.c_str()));
			canvas->Pen->Color = clWhite;
			canvas->Pen->Width = ScaleValue(2, width, height);
			canvas->MoveTo(cursorX, ipTextY);
			canvas->LineTo(cursorX, ipTextY + canvas->TextHeight(ipText));
		}
	}

	if (networkManager->IsHosting() || networkManager->GetState() == NeonGame::ENetworkState::Connected)
	{
		int listY = 0;

		if (networkManager->IsHosting())
		{
			listY = startY + buttonSpacing * 2 + ScaleValue(10, width, height);

			const std::string &ipStr = networkManager->GetLocalIPAddress();
			UnicodeString ipText = L"Ваш IP: ";
			if (!ipStr.empty())
				ipText += UnicodeString(ipStr.c_str());
			else
				ipText += L"не определен";

			canvas->Font->Size = ScaleFontSize(18, width, height);
			canvas->Font->Color = static_cast<TColor>(RGB(0, 220, 180));
			canvas->Font->Style = TFontStyles();
			canvas->TextOut(buttonX, listY, ipText);

			listY += ScaleValue(30, width, height);
		}
		else
		{
			listY = uiState.IPInputRect.Bottom + ScaleValue(16, width, height);
		}

		canvas->Font->Size = ScaleFontSize(22, width, height);
		canvas->Font->Color = static_cast<TColor>(RGB(0, 200, 255));
		canvas->Font->Style = TFontStyles() << fsBold;
		const UnicodeString playersTitle = L"Игроки";
		const TSize playersTitleSize = canvas->TextExtent(playersTitle);
		const int playersTitleX = buttonX;
		canvas->TextOut(playersTitleX, listY, playersTitle);

		canvas->Pen->Color = static_cast<TColor>(RGB(0, 120, 220));
		canvas->Pen->Width = ScaleValue(2, width, height);
		canvas->MoveTo(playersTitleX, listY + playersTitleSize.cy + ScaleValue(2, width, height));
		canvas->LineTo(playersTitleX + playersTitleSize.cx, listY + playersTitleSize.cy + ScaleValue(2, width, height));

		const auto &clients = networkManager->GetClients();
		const uint8_t localID = networkManager->GetLocalPlayerID();

		canvas->Font->Style = TFontStyles();
		int playerY = listY + playersTitleSize.cy + ScaleValue(10, width, height);

		for (const auto &client : clients)
		{
			if (!client.IsConnected)
				continue;

			UnicodeString roleTag;
			if (networkManager->IsHosting() && client.PlayerID == 0)
				roleTag = L"[Хост]";
			else if (client.PlayerID == localID)
				roleTag = L"[Вы]";

			const int cardPaddingX = ScaleValue(10, width, height);
			const int cardPaddingY = ScaleValue(6, width, height);
			const int cardWidth = buttonWidth;

			const int cardHeight = ScaleValue(56, width, height);
			const int cardX = buttonX;
			const int cardY = playerY;

			TRect cardRect(cardX, cardY, cardX + cardWidth, cardY + cardHeight);
			canvas->Brush->Color = static_cast<TColor>(RGB(25, 20, 60));
			canvas->Pen->Color = static_cast<TColor>(RGB(90, 70, 200));
			canvas->Pen->Width = ScaleValue(1, width, height);
			canvas->Rectangle(cardRect);

			canvas->Font->Size = ScaleFontSize(16, width, height);
			canvas->Font->Color = static_cast<TColor>(RGB(255, 255, 255));
			UnicodeString line = UnicodeString(client.PlayerName.c_str());
			if (!line.IsEmpty())
				line += L" ";
			line += L"(ID: " + IntToStr(client.PlayerID) + L")";
			if (!roleTag.IsEmpty())
				line += L" " + roleTag;

			const int lineX = cardX + cardPaddingX;
			const int lineY = cardY + cardPaddingY;
			canvas->TextOut(lineX, lineY, line);

			canvas->Font->Color = static_cast<TColor>(RGB(170, 170, 190));
			UnicodeString status = L"Статус: подключен";
			canvas->Font->Size = ScaleFontSize(12, width, height);

			const TSize lineSize = canvas->TextExtent(line);
			const int statusX = cardX + cardPaddingX + ScaleValue(10, width, height);
			const int statusY = lineY + lineSize.cy + ScaleValue(4, width, height);
			canvas->TextOut(statusX, statusY, status);

			playerY += cardHeight + ScaleValue(6, width, height);
		}

		if (networkManager->IsHosting())
		{
			const int startButtonY = height - ScaleValue(120, width, height);
			uiState.StartGameButtonRect = Rect(buttonX, startButtonY,
				buttonX + buttonWidth, startButtonY + buttonHeight);
			const float startHoverAnim = uiState.StartGameButtonHover ? 1.0f : 0.0f;
			const int startScale = static_cast<int>(ScaleValue(5, width, height) * startHoverAnim);
			const TRect startRect(uiState.StartGameButtonRect.Left - startScale, uiState.StartGameButtonRect.Top - startScale,
				uiState.StartGameButtonRect.Right + startScale, uiState.StartGameButtonRect.Bottom + startScale);

			canvas->Brush->Color = uiState.StartGameButtonHover ?
				static_cast<TColor>(RGB(0, 200, 100)) : static_cast<TColor>(RGB(0, 150, 70));
			canvas->Pen->Color = static_cast<TColor>(RGB(0, 255, 150));
			canvas->Pen->Width = ScaleValue(2, width, height);
			canvas->Rectangle(startRect);

			canvas->Font->Size = ScaleFontSize(24, width, height);
			canvas->Font->Color = clWhite;
			canvas->Font->Style = TFontStyles() << fsBold;
			const UnicodeString startText = L"НАЧАТЬ ИГРУ";
			const TSize startTextSize = canvas->TextExtent(startText);
			const int startTextX = buttonX + (buttonWidth - startTextSize.cx) / 2;
			const int startTextY = startButtonY + (buttonHeight - startTextSize.cy) / 2;
			canvas->TextOut(startTextX, startTextY, startText);
		}
	}

	uiState.BackButtonRect = Rect(ScaleValue(20, width, height), ScaleValue(20, width, height),
		ScaleValue(120, width, height), ScaleValue(60, width, height));
	const float backHoverAnim = uiState.BackButtonHover ? 1.0f : 0.0f;
	const int backScale = static_cast<int>(ScaleValue(3, width, height) * backHoverAnim);
	const TRect backRect(uiState.BackButtonRect.Left - backScale, uiState.BackButtonRect.Top - backScale,
		uiState.BackButtonRect.Right + backScale, uiState.BackButtonRect.Bottom + backScale);

	canvas->Brush->Color = uiState.BackButtonHover ?
		static_cast<TColor>(RGB(100, 100, 100)) : static_cast<TColor>(RGB(60, 60, 60));
	canvas->Pen->Color = static_cast<TColor>(RGB(200, 200, 200));
	canvas->Pen->Width = ScaleValue(2, width, height);
	canvas->Rectangle(backRect);

	canvas->Font->Size = ScaleFontSize(18, width, height);
	canvas->Font->Color = clWhite;
	canvas->Font->Style = TFontStyles() << fsBold;
	const UnicodeString backText = L"НАЗАД";
	const TSize backTextSize = canvas->TextExtent(backText);
	const int backTextX = uiState.BackButtonRect.Left + (uiState.BackButtonRect.Width() - backTextSize.cx) / 2;
	const int backTextY = uiState.BackButtonRect.Top + (uiState.BackButtonRect.Height() - backTextSize.cy) / 2;
	canvas->TextOut(backTextX, backTextY, backText);

	canvas->Font->Size = ScaleFontSize(14, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(200, 200, 200));
	canvas->Font->Style = TFontStyles();
	UnicodeString statusText;
	switch (networkManager->GetState())
	{
		case NeonGame::ENetworkState::Disconnected:
			statusText = L"Отключено";
			canvas->Font->Color = static_cast<TColor>(RGB(200, 50, 50));
			break;
		case NeonGame::ENetworkState::Connecting:
			statusText = L"Подключение...";
			canvas->Font->Color = static_cast<TColor>(RGB(255, 200, 0));
			break;
		case NeonGame::ENetworkState::Connected:
			statusText = L"Подключено";
			canvas->Font->Color = static_cast<TColor>(RGB(0, 255, 100));
			break;
		case NeonGame::ENetworkState::Hosting:
			statusText = L"Ожидание игроков...";
			canvas->Font->Color = static_cast<TColor>(RGB(0, 200, 255));
			break;
	}
	const int statusX = width - ScaleValue(200, width, height);
	const int statusY = ScaleValue(20, width, height);
	canvas->TextOut(statusX, statusY, statusText);
}