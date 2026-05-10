#include <vcl.h>
#pragma hdrstop

#include "GameUIRenderer.h"
#include "GameUIRenderer_Utils.h"
#include "systems\\GameUpgrade.h"
#include <algorithm>
#include <cmath>

void TGameUIRenderer::DrawGameOver(TCanvas *canvas,
	const TGameStats &stats,
	const TGameRecords &records,
	TGameUIState &uiState,
	bool onlineCoopMatch) const
{
	if (!canvas)
		return;

	const int width = canvas->ClipRect.Width();
	const int height = canvas->ClipRect.Height();
	const float scale = GetScale(width, height);

	canvas->Font->Name = "Segoe UI";

	const float alpha = std::clamp(uiState.OverlayAlpha, 0.0f, 1.0f);
	const int curtainHeight = static_cast<int>(height * 0.5f * alpha);

	canvas->Brush->Style = bsSolid;
	canvas->Brush->Color = static_cast<TColor>(RGB(0, 0, 0));
	canvas->Pen->Style = psClear;
	if (curtainHeight > 0)
	{
		canvas->Rectangle(0, 0, width, curtainHeight);
		canvas->Rectangle(0, height - curtainHeight, width, height);
	}
	canvas->Pen->Style = psSolid;

	const int centerX = width / 2;
	const int centerY = height / 2;
	const int panelWidth = std::min(ScaleValue(500, width, height), width - ScaleValue(40, width, height));
	const int panelHeight = std::min(ScaleValue(400, width, height), height - ScaleValue(40, width, height));
	const TRect panelRect(
		centerX - panelWidth / 2,
		centerY - panelHeight / 2,
		centerX + panelWidth / 2,
		centerY + panelHeight / 2);

	canvas->Brush->Color = static_cast<TColor>(RGB(10, 10, 20));
	canvas->Pen->Color = static_cast<TColor>(RGB(0, 150, 220));
	canvas->Pen->Width = ScaleValue(2, width, height);
	canvas->Rectangle(panelRect);

	canvas->Font->Name = "Segoe UI";
	canvas->Font->Color = static_cast<TColor>(RGB(255, 120, 160));
	canvas->Font->Size = ScaleFontSize(26, width, height);
	const UnicodeString title = L"Игра окончена";
	const TSize titleSize = canvas->TextExtent(title);
	canvas->TextOut(centerX - titleSize.cx / 2, panelRect.Top + ScaleValue(20, width, height), title);

	canvas->Font->Color = static_cast<TColor>(RGB(200, 200, 200));
	canvas->Font->Size = ScaleFontSize(16, width, height);

	const UnicodeString waveText = L"Волна: " + IntToStr(stats.CurrentWave);
	const UnicodeString killText = L"Убито врагов: " + IntToStr(stats.EnemiesDefeated);
	const int totalSeconds = static_cast<int>(stats.RunTimeSeconds + 0.5f);
	const UnicodeString timeText = L"Время: " + NeonGameUIRendererUtils::FormatMinutesSeconds(totalSeconds);

	int statsY = panelRect.Top + ScaleValue(70, width, height);
	const int statsX = panelRect.Left + ScaleValue(40, width, height);

	canvas->Font->Color = static_cast<TColor>(RGB(220, 220, 240));
	canvas->Font->Size = ScaleFontSize(14, width, height);
	canvas->Font->Style = TFontStyles() << fsBold;
	canvas->TextOut(statsX, statsY, L"Текущий результат:"); statsY += ScaleValue(24, width, height);
	canvas->Font->Style = TFontStyles();
	canvas->Font->Size = ScaleFontSize(16, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(200, 200, 200));

	canvas->TextOut(statsX, statsY, waveText); statsY += ScaleValue(28, width, height);
	canvas->TextOut(statsX, statsY, killText); statsY += ScaleValue(28, width, height);
	canvas->TextOut(statsX, statsY, timeText); statsY += ScaleValue(36, width, height);

	canvas->Pen->Color = static_cast<TColor>(RGB(50, 50, 70));
	canvas->Pen->Width = ScaleValue(1, width, height);
	canvas->MoveTo(panelRect.Left + ScaleValue(30, width, height), statsY);
	canvas->LineTo(panelRect.Right - ScaleValue(30, width, height), statsY);
	statsY += ScaleValue(20, width, height);

	canvas->Font->Color = static_cast<TColor>(RGB(255, 200, 100));
	canvas->Font->Size = ScaleFontSize(14, width, height);
	canvas->Font->Style = TFontStyles() << fsBold;
	canvas->TextOut(statsX, statsY, L"Рекорды:"); statsY += ScaleValue(24, width, height);
	canvas->Font->Style = TFontStyles();
	canvas->Font->Size = ScaleFontSize(16, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(200, 200, 200));

	const UnicodeString bestWaveText = L"Лучшая волна: " + IntToStr(records.BestWave);
	const UnicodeString bestKillText = L"Убито врагов: " + IntToStr(records.BestEnemiesKilled);
	const int bestTotalSeconds = static_cast<int>(records.BestRunTime + 0.5f);
	const UnicodeString bestTimeText = L"Время: " + NeonGameUIRendererUtils::FormatMinutesSeconds(bestTotalSeconds);

	canvas->TextOut(statsX, statsY, bestWaveText); statsY += ScaleValue(28, width, height);
	canvas->TextOut(statsX, statsY, bestKillText); statsY += ScaleValue(28, width, height);
	canvas->TextOut(statsX, statsY, bestTimeText); statsY += ScaleValue(36, width, height);

	const int buttonWidth = ScaleValue(220, width, height);
	const int buttonHeight = ScaleValue(42, width, height);
	const int buttonX = centerX - buttonWidth / 2;
	const int restartY = panelRect.Bottom - buttonHeight - ScaleValue(30, width, height);

	uiState.RestartButtonRect = Rect(buttonX, restartY, buttonX + buttonWidth, restartY + buttonHeight);
	const TColor restartBrush = uiState.RestartButtonHover
		? static_cast<TColor>(RGB(40, 70, 110))
		: static_cast<TColor>(RGB(20, 40, 70));
	const TColor restartPen = uiState.RestartButtonHover
		? static_cast<TColor>(RGB(0, 220, 255))
		: static_cast<TColor>(RGB(0, 180, 255));
	canvas->Brush->Color = restartBrush;
	canvas->Pen->Color = restartPen;
	canvas->Pen->Width = uiState.RestartButtonHover ? ScaleValue(3, width, height) : ScaleValue(2, width, height);
	canvas->Rectangle(uiState.RestartButtonRect);
	canvas->Font->Size = ScaleFontSize(15, width, height);
	canvas->Font->Color = uiState.RestartButtonHover
		? static_cast<TColor>(RGB(255, 255, 255))
		: static_cast<TColor>(RGB(240, 240, 240));
	const UnicodeString restartText = onlineCoopMatch ? L"В лобби" : L"Перезапуск (R)";
	const TSize restartSize = canvas->TextExtent(restartText);
	canvas->TextOut(buttonX + (buttonWidth - restartSize.cx) / 2,
		restartY + (buttonHeight - restartSize.cy) / 2, restartText);

	const int menuY = restartY + buttonHeight + ScaleValue(12, width, height);
	uiState.MenuButtonRect = Rect(buttonX, menuY, buttonX + buttonWidth, menuY + buttonHeight);
	const TColor menuBrush = uiState.MenuButtonHover
		? static_cast<TColor>(RGB(25, 35, 55))
		: static_cast<TColor>(RGB(15, 25, 40));
	const TColor menuPen = uiState.MenuButtonHover
		? static_cast<TColor>(RGB(110, 130, 160))
		: static_cast<TColor>(RGB(70, 90, 120));
	canvas->Brush->Color = menuBrush;
	canvas->Pen->Color = menuPen;
	canvas->Pen->Width = uiState.MenuButtonHover ? ScaleValue(3, width, height) : ScaleValue(2, width, height);
	canvas->Rectangle(uiState.MenuButtonRect);
	canvas->Font->Color = uiState.MenuButtonHover
		? static_cast<TColor>(RGB(200, 200, 220))
		: static_cast<TColor>(RGB(140, 140, 160));
	const UnicodeString menuText = L"Меню";
	const TSize menuSize = canvas->TextExtent(menuText);
	canvas->TextOut(buttonX + (buttonWidth - menuSize.cx) / 2,
		menuY + (buttonHeight - menuSize.cy) / 2, menuText);

	canvas->Font->Size = ScaleFontSize(13, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(180, 180, 200));
	if (!onlineCoopMatch)
	{
		const UnicodeString tip = L"или нажмите клавишу R";
		const TSize tipSize = canvas->TextExtent(tip);
		canvas->TextOut(centerX - tipSize.cx / 2, menuY + buttonHeight + ScaleValue(18, width, height), tip);
	}
	else
	{
		const UnicodeString tip = L"Хост — в лобби для всех; клиент — запрос хосту (R)";
		const TSize tipSize = canvas->TextExtent(tip);
		canvas->TextOut(centerX - tipSize.cx / 2, menuY + buttonHeight + ScaleValue(18, width, height), tip);
	}

	canvas->Pen->Width = ScaleValue(1, width, height);
	canvas->Pen->Style = psSolid;
}

