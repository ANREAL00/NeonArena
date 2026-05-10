#include <vcl.h>
#pragma hdrstop

#include "GameUIRenderer.h"
#include "GameUIRenderer_Utils.h"
#include "network\\GameNetwork.h"
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

