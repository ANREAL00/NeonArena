#include <vcl.h>
#pragma hdrstop
//---------------------------------------------------------------------------

#include "GameUIRenderer.h"
#include "GameUpgrade.h"
#include "GameNetwork.h"
#include <algorithm>
#include <cmath>
#include <System.SysUtils.hpp>
//---------------------------------------------------------------------------

void TGameUIRenderer::DrawHud(TCanvas *canvas,
	float healthRatio, int health, int maxHealth,
	float experienceRatio, int experience, int experienceToNext,
	int playerLevel,
	const TGameStats &stats,
	bool showStatsPanel) const
{
	if (!canvas)
		return;

	const int width = canvas->ClipRect.Width();
	const int height = canvas->ClipRect.Height();
	const float scale = GetScale(width, height);
	
	// единые настройки шрифта для HUD
	canvas->Font->Name = "Segoe UI";
	
	const int barWidth = ScaleValue(220, width, height);
	const int barHeight = ScaleValue(18, width, height);
	const int barX = ScaleValue(16, width, height);
	const int barY = ScaleValue(16, width, height);
	const int barSpacing = ScaleValue(28, width, height);

	canvas->Font->Name = "Segoe UI";

	// Таймер и волна сверху по центру
	const int topY = ScaleValue(16, width, height);
	canvas->Font->Size = ScaleFontSize(16, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(200, 200, 220));

	const int totalSeconds = static_cast<int>(stats.RunTimeSeconds + 0.5f);
	const int minutes = totalSeconds / 60;
	const int seconds = totalSeconds % 60;
	const UnicodeString timeText = IntToStr(minutes) + L":" + 
		(seconds < 10 ? L"0" : L"") + IntToStr(seconds);
	const TSize timeSize = canvas->TextExtent(timeText);

	const UnicodeString waveText = L"Волна " + IntToStr(stats.CurrentWave);
	const TSize waveSize = canvas->TextExtent(waveText);

	const int totalTopWidth = timeSize.cx + ScaleValue(20, width, height) + waveSize.cx;
	const int topX = (width - totalTopWidth) / 2;
	const int padding = ScaleValue(8, width, height);

	// фон для таймера и волны
	canvas->Brush->Style = bsSolid;
	canvas->Brush->Color = static_cast<TColor>(RGB(10, 10, 20));
	canvas->Pen->Color = static_cast<TColor>(RGB(0, 150, 220));
	canvas->Pen->Width = 1;
	canvas->Rectangle(
		topX - padding,
		topY - padding,
		topX + totalTopWidth + padding,
		topY + timeSize.cy + padding);

	canvas->TextOut(topX, topY, timeText);
	canvas->TextOut(topX + timeSize.cx + 20, topY, waveText);

	// полоска здоровья
	const int healthBarY = barY + ScaleValue(40, width, height);
	canvas->Brush->Color = static_cast<TColor>(RGB(20, 20, 30));
	canvas->Pen->Color = static_cast<TColor>(RGB(0, 180, 255));
	canvas->Pen->Width = ScaleValue(2, width, height);
	canvas->Rectangle(barX, healthBarY, barX + barWidth, healthBarY + barHeight);

	const int healthFillWidth = static_cast<int>(std::round(barWidth * std::clamp(healthRatio, 0.0f, 1.0f)));
	canvas->Brush->Color = static_cast<TColor>(RGB(0, 255, 160));
	canvas->Pen->Style = psClear;
	canvas->Rectangle(barX + 2, healthBarY + 2, barX + 2 + healthFillWidth - 2, healthBarY + barHeight - 2);

	canvas->Pen->Style = psSolid;
	canvas->Pen->Width = 1;

	// цифры HP (без фона)
	canvas->Font->Size = ScaleFontSize(12, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(255, 255, 255));
	canvas->Font->Style = TFontStyles() << fsBold;
	const UnicodeString healthText = IntToStr(health) + L" / " + IntToStr(maxHealth);
	const TSize healthTextSize = canvas->TextExtent(healthText);
	canvas->TextOut(barX + (barWidth - healthTextSize.cx) / 2, healthBarY + (barHeight - healthTextSize.cy) / 2, healthText);
	canvas->Font->Style = TFontStyles();

	// полоска опыта
	const int expBarY = healthBarY + barHeight + barSpacing;
	canvas->Brush->Color = static_cast<TColor>(RGB(20, 20, 30));
	canvas->Pen->Color = static_cast<TColor>(RGB(200, 100, 255));
	canvas->Pen->Width = ScaleValue(2, width, height);
	canvas->Rectangle(barX, expBarY, barX + barWidth, expBarY + barHeight);

	const int expFillWidth = static_cast<int>(std::round(barWidth * std::clamp(experienceRatio, 0.0f, 1.0f)));
	canvas->Brush->Color = static_cast<TColor>(RGB(200, 150, 255));
	canvas->Pen->Style = psClear;
	canvas->Rectangle(barX + 2, expBarY + 2, barX + 2 + expFillWidth - 2, expBarY + barHeight - 2);

	canvas->Pen->Style = psSolid;
	canvas->Pen->Width = 1;

	// цифры EXP (без фона)
	canvas->Font->Size = ScaleFontSize(12, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(255, 255, 255));
	canvas->Font->Style = TFontStyles() << fsBold;
	const UnicodeString expText = IntToStr(experience) + L" / " + IntToStr(experienceToNext);
	const TSize expTextSize = canvas->TextExtent(expText);
	canvas->TextOut(barX + (barWidth - expTextSize.cx) / 2, expBarY + (barHeight - expTextSize.cy) / 2, expText);
	canvas->Font->Style = TFontStyles();
	
	// подсказка про TAB внизу экрана
	const int hintY = height - ScaleValue(30, width, height);
	canvas->Font->Size = ScaleFontSize(11, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(150, 150, 170));
	canvas->Font->Style = TFontStyles();
	const UnicodeString hintText = showStatsPanel ? L"TAB - скрыть характеристики" : L"TAB - показать характеристики";
	const TSize hintSize = canvas->TextExtent(hintText);
	const int hintX = (width - hintSize.cx) / 2;
	
	// фон для подсказки
	canvas->Brush->Style = bsSolid;
	canvas->Brush->Color = static_cast<TColor>(RGB(10, 10, 20));
	canvas->Pen->Color = static_cast<TColor>(RGB(0, 150, 220));
	canvas->Pen->Width = ScaleValue(1, width, height);
	const int hintPadding = ScaleValue(6, width, height);
	canvas->Rectangle(
		hintX - hintPadding,
		hintY - hintPadding,
		hintX + hintSize.cx + hintPadding,
		hintY + hintSize.cy + hintPadding);
	
	canvas->TextOut(hintX, hintY, hintText);
}
//---------------------------------------------------------------------------
void TGameUIRenderer::DrawGameOver(TCanvas *canvas,
	const TGameStats &stats,
	const TGameRecords &records,
	TGameUIState &uiState) const
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

	// Текущие результаты
	const UnicodeString waveText = L"Волна: " + IntToStr(stats.CurrentWave);
	const UnicodeString killText = L"Убито врагов: " + IntToStr(stats.EnemiesDefeated);
	const int totalSeconds = static_cast<int>(stats.RunTimeSeconds + 0.5f);
	const int minutes = totalSeconds / 60;
	const int seconds = totalSeconds % 60;
	const UnicodeString timeText = L"Время: " + IntToStr(minutes) + L":" + 
		(seconds < 10 ? L"0" : L"") + IntToStr(seconds);

	int statsY = panelRect.Top + ScaleValue(70, width, height);
	const int statsX = panelRect.Left + ScaleValue(40, width, height);
	
	// Заголовок текущих результатов
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

	// Разделительная линия
	canvas->Pen->Color = static_cast<TColor>(RGB(50, 50, 70));
	canvas->Pen->Width = ScaleValue(1, width, height);
	canvas->MoveTo(panelRect.Left + ScaleValue(30, width, height), statsY);
	canvas->LineTo(panelRect.Right - ScaleValue(30, width, height), statsY);
	statsY += ScaleValue(20, width, height);

	// Рекорды
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
	const int bestMinutes = bestTotalSeconds / 60;
	const int bestSeconds = bestTotalSeconds % 60;
	const UnicodeString bestTimeText = L"Время: " + IntToStr(bestMinutes) + L":" + 
		(bestSeconds < 10 ? L"0" : L"") + IntToStr(bestSeconds);
	
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
	const UnicodeString restartText = L"Перезапуск (R)";
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
	const UnicodeString tip = L"или нажмите клавишу R";
	const TSize tipSize = canvas->TextExtent(tip);
	canvas->TextOut(centerX - tipSize.cx / 2, menuY + buttonHeight + ScaleValue(18, width, height), tip);

	canvas->Pen->Width = ScaleValue(1, width, height);
	canvas->Pen->Style = psSolid;
}
//---------------------------------------------------------------------------
void TGameUIRenderer::DrawUpgradeMenu(TCanvas *canvas,
	const std::vector<TUpgrade> &upgrades,
	TGameUIState &uiState) const
{
	if (!canvas || upgrades.empty())
		return;

	const int width = canvas->ClipRect.Width();
	const int height = canvas->ClipRect.Height();
	const float scale = GetScale(width, height);

	// затемнение фона
	canvas->Brush->Style = bsSolid;
	canvas->Brush->Color = static_cast<TColor>(RGB(0, 0, 0));
	canvas->Pen->Style = psClear;
	canvas->Rectangle(0, 0, width, height);
	canvas->Pen->Style = psSolid;

	// заголовок
	canvas->Font->Name = "Segoe UI";
	canvas->Font->Size = ScaleFontSize(28, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(0, 255, 200));
	canvas->Font->Style = TFontStyles() << fsBold;
	const UnicodeString title = L"Выберите улучшение";
	const TSize titleSize = canvas->TextExtent(title);
	const int centerX = width / 2;
	canvas->TextOut(centerX - titleSize.cx / 2, ScaleValue(40, width, height), title);
	canvas->Font->Style = TFontStyles();

	// размеры кнопок (увеличены)
	const int buttonWidth = ScaleValue(380, width, height);
	const int buttonHeight = ScaleValue(150, width, height);
	const int buttonSpacing = ScaleValue(20, width, height);
	const int totalWidth = buttonWidth * 3 + buttonSpacing * 2;
	const int startX = (width - totalWidth) / 2;
	const int startY = height / 2 - buttonHeight / 2;

	uiState.UpgradeButtonRects.clear();
	uiState.UpgradeButtonHovers.clear();
	if (uiState.UpgradeButtonHoverTime.size() < 3)
		uiState.UpgradeButtonHoverTime.resize(3, 0.0f);

	// рисуем кнопки улучшений
	for (size_t i = 0; i < upgrades.size() && i < 3; i++)
	{
		const TUpgrade &upgrade = upgrades[i];
		const int buttonX = startX + static_cast<int>(i) * (buttonWidth + buttonSpacing);
		const TRect buttonRect(buttonX, startY, buttonX + buttonWidth, startY + buttonHeight);

		// сохраняем исходный rect для проверки hover (не изменяется)
		if (uiState.UpgradeButtonRects.size() <= i)
			uiState.UpgradeButtonRects.push_back(buttonRect);
		else
			uiState.UpgradeButtonRects[i] = buttonRect; // обновляем позицию на случай изменения размера экрана

		if (uiState.UpgradeButtonHovers.size() <= i)
			uiState.UpgradeButtonHovers.push_back(false);

		// используем текущее значение анимации (обновляется в UpdateGame)
		const float hoverAnim = (i < uiState.UpgradeButtonHoverTime.size()) ? uiState.UpgradeButtonHoverTime[i] : 0.0f;
		const float scale = 1.0f + hoverAnim * 0.05f; // увеличение на 5%
		const int scaledWidth = static_cast<int>(buttonWidth * scale);
		const int scaledHeight = static_cast<int>(buttonHeight * scale);
		const int scaledX = buttonX - (scaledWidth - buttonWidth) / 2;
		const int scaledY = startY - (scaledHeight - buttonHeight) / 2;
		const TRect scaledRect(scaledX, scaledY, scaledX + scaledWidth, scaledY + scaledHeight);
		
		// проверяем hover по исходному rect (для стабильности проверки)
		const bool hover = (i < uiState.UpgradeButtonHovers.size()) ? uiState.UpgradeButtonHovers[i] : false;

		// фон кнопки с анимацией и цветом редкости
		float r1, g1, b1, r2, g2, b2;
		
		switch (upgrade.Rarity)
		{
		case EUpgradeRarity::Common: // синий
			r1 = 15.0f + hoverAnim * 15.0f;
			g1 = 25.0f + hoverAnim * 25.0f;
			b1 = 40.0f + hoverAnim * 30.0f;
			r2 = 0.0f;
			g2 = 120.0f + hoverAnim * 60.0f;
			b2 = 200.0f + hoverAnim * 55.0f;
			break;
		case EUpgradeRarity::Rare: // фиолетовый
			r1 = 25.0f + hoverAnim * 20.0f;
			g1 = 15.0f + hoverAnim * 20.0f;
			b1 = 50.0f + hoverAnim * 30.0f;
			r2 = 120.0f + hoverAnim * 60.0f;
			g2 = 0.0f;
			b2 = 200.0f + hoverAnim * 55.0f;
			break;
		case EUpgradeRarity::Legendary: // золотой
			r1 = 40.0f + hoverAnim * 25.0f;
			g1 = 35.0f + hoverAnim * 25.0f;
			b1 = 15.0f + hoverAnim * 15.0f;
			r2 = 220.0f + hoverAnim * 35.0f;
			g2 = 180.0f + hoverAnim * 50.0f;
			b2 = 0.0f;
			break;
		default:
			r1 = 15.0f; g1 = 25.0f; b1 = 40.0f;
			r2 = 0.0f; g2 = 180.0f; b2 = 255.0f;
			break;
		}
		
		const TColor buttonBrush = static_cast<TColor>(RGB(
			static_cast<int>(r1), static_cast<int>(g1), static_cast<int>(b1)));
		const TColor buttonPen = static_cast<TColor>(RGB(
			static_cast<int>(r2), static_cast<int>(g2), static_cast<int>(b2)));
		canvas->Brush->Color = buttonBrush;
		canvas->Pen->Color = buttonPen;
		canvas->Pen->Width = ScaleValue(2, width, height) + static_cast<int>(hoverAnim * ScaleValue(1, width, height));
		canvas->Rectangle(scaledRect);

		// название улучшения
		canvas->Font->Size = ScaleFontSize(22, width, height) + static_cast<int>(hoverAnim * ScaleValue(2, width, height));
		const float textR = 200.0f + hoverAnim * 55.0f;
		const float textG = 200.0f + hoverAnim * 55.0f;
		const float textB = 220.0f + hoverAnim * 35.0f;
		canvas->Font->Color = static_cast<TColor>(RGB(
			static_cast<int>(textR), static_cast<int>(textG), static_cast<int>(textB)));
		canvas->Font->Style = TFontStyles() << fsBold;
		const TSize nameSize = canvas->TextExtent(upgrade.Name);
		canvas->TextOut(scaledX + (scaledWidth - nameSize.cx) / 2, scaledY + ScaleValue(20, width, height), upgrade.Name);
		canvas->Font->Style = TFontStyles();

		// описание в 2 строки
		canvas->Font->Size = ScaleFontSize(12, width, height);
		canvas->Font->Color = static_cast<TColor>(RGB(180, 180, 200));
		
		UnicodeString desc = upgrade.Description;
		const int maxDescWidth = scaledWidth - ScaleValue(20, width, height); // отступы по 10px с каждой стороны
		
		// разбиваем текст на 2 строки
		UnicodeString line1, line2;
		TSize fullSize = canvas->TextExtent(desc);
		
		if (fullSize.cx <= maxDescWidth)
		{
			// текст помещается в одну строку
			line1 = desc;
		}
		else
		{
			// ищем место для разрыва (примерно посередине)
			int breakPos = desc.Length() / 2;
			
			// ищем пробел ближе к середине
			for (int offset = 0; offset < breakPos && breakPos + offset < desc.Length(); offset++)
			{
				if (desc[breakPos + offset] == L' ')
				{
					breakPos = breakPos + offset;
					break;
				}
				if (breakPos - offset > 0 && desc[breakPos - offset] == L' ')
				{
					breakPos = breakPos - offset;
					break;
				}
			}
			
			line1 = desc.SubString(1, breakPos).Trim();
			line2 = desc.SubString(breakPos + 1, desc.Length() - breakPos).Trim();
			
			// проверяем, что обе строки помещаются
			TSize line1Size = canvas->TextExtent(line1);
			TSize line2Size = canvas->TextExtent(line2);
			
			// если первая строка слишком длинная, обрезаем
			if (line1Size.cx > maxDescWidth)
			{
				int pos = line1.Length();
				while (pos > 0 && canvas->TextExtent(line1.SubString(1, pos) + L"...").cx > maxDescWidth)
					pos--;
				if (pos > 0)
					line1 = line1.SubString(1, pos) + L"...";
				else
					line1 = line1.SubString(1, 15) + L"...";
			}
			
			// если вторая строка слишком длинная, обрезаем
			if (line2Size.cx > maxDescWidth)
			{
				int pos = line2.Length();
				while (pos > 0 && canvas->TextExtent(line2.SubString(1, pos) + L"...").cx > maxDescWidth)
					pos--;
				if (pos > 0)
					line2 = line2.SubString(1, pos) + L"...";
				else
					line2 = line2.SubString(1, 15) + L"...";
			}
		}
		
		// рисуем первую строку описания (с большим отступом от названия)
		if (!line1.IsEmpty())
		{
			TSize line1Size = canvas->TextExtent(line1);
			canvas->TextOut(scaledX + (scaledWidth - line1Size.cx) / 2, scaledY + ScaleValue(58, width, height), line1);
		}
		
		// рисуем вторую строку описания
		if (!line2.IsEmpty())
		{
			TSize line2Size = canvas->TextExtent(line2);
			canvas->TextOut(scaledX + (scaledWidth - line2Size.cx) / 2, scaledY + ScaleValue(76, width, height), line2);
		}

		// номер кнопки (1, 2, 3)
		canvas->Font->Size = ScaleFontSize(16, width, height);
		canvas->Font->Color = static_cast<TColor>(RGB(150, 150, 170));
		const UnicodeString keyText = L"[" + IntToStr(static_cast<int>(i) + 1) + L"]";
		const TSize keySize = canvas->TextExtent(keyText);
		canvas->TextOut(scaledX + (scaledWidth - keySize.cx) / 2, scaledY + scaledHeight - ScaleValue(28, width, height), keyText);
	}

	// подсказка
	canvas->Font->Size = ScaleFontSize(14, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(150, 150, 170));
	const UnicodeString hint = L"Нажмите 1, 2 или 3 для выбора";
	const TSize hintSize = canvas->TextExtent(hint);
	canvas->TextOut(centerX - hintSize.cx / 2, startY + buttonHeight + ScaleValue(40, width, height), hint);
}
//---------------------------------------------------------------------------
void TGameUIRenderer::DrawLevelUpNotification(TCanvas *canvas,
	float timer, int level) const
{
	if (!canvas || timer <= 0.0f)
		return;

	const int width = canvas->ClipRect.Width();
	const int height = canvas->ClipRect.Height();
	const float scale = GetScale(width, height);
	const int baseFontSize = ScaleFontSize(48, width, height);

	// анимация появления/исчезновения
	const float alpha = std::min(1.0f, timer / 0.5f); // появляется за 0.5 секунды
	const float fadeOut = (timer < 0.5f) ? (timer / 0.5f) : 1.0f; // исчезает последние 0.5 секунды
	const float finalAlpha = alpha * fadeOut;

	canvas->Font->Name = "Segoe UI";
	canvas->Font->Size = baseFontSize;
	canvas->Font->Style = TFontStyles() << fsBold;

	// пульсация
	const float pulse = 1.0f + 0.1f * std::sin(timer * 10.0f);
	const int fontSize = static_cast<int>(baseFontSize * pulse * finalAlpha);

	canvas->Font->Size = fontSize;
	canvas->Font->Color = static_cast<TColor>(RGB(
		static_cast<int>(0 * finalAlpha),
		static_cast<int>(255 * finalAlpha),
		static_cast<int>(200 * finalAlpha)));

	const UnicodeString levelText = L"УРОВЕНЬ " + IntToStr(level) + L"!";
	const TSize textSize = canvas->TextExtent(levelText);
	const int centerX = width / 2;
	const int centerY = height / 3; // верхняя треть экрана

	// тень
	canvas->Font->Color = static_cast<TColor>(RGB(0, 0, 0));
	const int shadowOffset = ScaleValue(3, width, height);
	canvas->TextOut(centerX - textSize.cx / 2 + shadowOffset, centerY + shadowOffset, levelText);

	// основной текст
	canvas->Font->Color = static_cast<TColor>(RGB(
		static_cast<int>(0 * finalAlpha),
		static_cast<int>(255 * finalAlpha),
		static_cast<int>(200 * finalAlpha)));
	canvas->TextOut(centerX - textSize.cx / 2, centerY, levelText);

	canvas->Font->Style = TFontStyles();
}
//---------------------------------------------------------------------------
void TGameUIRenderer::DrawPlayerStats(TCanvas *canvas, const TPlayerStats &stats, const TGameUIState &uiState) const
{
	if (!canvas || !uiState.ShowStatsPanel)
		return;

	const int width = canvas->ClipRect.Width();
	const int height = canvas->ClipRect.Height();
	const float scale = GetScale(width, height);
	
	canvas->Font->Name = "Segoe UI";
	
	const int startX = ScaleValue(16, width, height);
	int startY = ScaleValue(150, width, height); // начинаем ниже полосок здоровья/опыта
	const int lineHeight = ScaleValue(22, width, height);
	const int panelWidth = ScaleValue(200, width, height);
	const int padding = ScaleValue(8, width, height);

	canvas->Font->Name = "Segoe UI";
	canvas->Font->Size = ScaleFontSize(11, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(200, 200, 220));

	// фон панели
	canvas->Brush->Style = bsSolid;
	canvas->Brush->Color = static_cast<TColor>(RGB(10, 10, 20));
	canvas->Pen->Color = static_cast<TColor>(RGB(0, 150, 220));
	canvas->Pen->Width = ScaleValue(1, width, height);
	const int panelHeight = 30 * lineHeight + padding * 2; // увеличиваем для новых улучшений
	canvas->Rectangle(startX - padding, startY - padding, startX + panelWidth + padding, startY + panelHeight + padding);

	// заголовок
	canvas->Font->Size = ScaleFontSize(13, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(0, 200, 255));
	canvas->Font->Style = TFontStyles() << fsBold;
	canvas->TextOut(startX, startY, L"Характеристики");
	canvas->Font->Style = TFontStyles();
	startY += lineHeight + ScaleValue(5, width, height);

	canvas->Font->Size = ScaleFontSize(11, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(200, 200, 220));

	// Основная атака
	canvas->Font->Color = static_cast<TColor>(RGB(0, 180, 255));
	canvas->TextOut(startX, startY, L"Основная атака:");
	startY += lineHeight;
	canvas->Font->Color = static_cast<TColor>(RGB(200, 200, 220));

	canvas->TextOut(startX, startY, L"Урон: " + FloatToStrF((stats.DamageMultiplier - 1.0f) * 100.0f, ffFixed, 3, 0) + L"%");
	startY += lineHeight;
	
	const float fireRatePercent = (1.0f - stats.FireRateMultiplier) * 100.0f;
	canvas->TextOut(startX, startY, L"Скорострельность: " + FloatToStrF(fireRatePercent, ffFixed, 3, 0) + L"%");
	startY += lineHeight;
	
	canvas->TextOut(startX, startY, L"Дальность: " + FloatToStrF((stats.BulletRangeMultiplier - 1.0f) * 100.0f, ffFixed, 3, 0) + L"%");
	startY += lineHeight;
	
	canvas->TextOut(startX, startY, L"Размер: " + FloatToStrF((stats.BulletSizeMultiplier - 1.0f) * 100.0f, ffFixed, 3, 0) + L"%");
	startY += lineHeight;
	
	canvas->TextOut(startX, startY, L"Скорость: " + FloatToStrF((stats.BulletSpeedMultiplier - 1.0f) * 100.0f, ffFixed, 3, 0) + L"%");
	startY += lineHeight;
	
	canvas->TextOut(startX, startY, UnicodeString(L"Пробитие: ") + (stats.HasPierce ? L"Да" : L"Нет"));
	startY += lineHeight + ScaleValue(5, width, height);

	// Альтернативная атака
	canvas->Font->Color = static_cast<TColor>(RGB(200, 100, 255));
	canvas->TextOut(startX, startY, L"Альтернативная атака:");
	startY += lineHeight;
	canvas->Font->Color = static_cast<TColor>(RGB(200, 200, 220));

	canvas->TextOut(startX, startY, L"Урон: " + FloatToStrF((stats.AltDamageMultiplier - 1.0f) * 100.0f, ffFixed, 3, 0) + L"%");
	startY += lineHeight;
	
	const float altFireRatePercent = (1.0f - stats.AltFireRateMultiplier) * 100.0f;
	canvas->TextOut(startX, startY, L"Скорострельность: " + FloatToStrF(altFireRatePercent, ffFixed, 3, 0) + L"%");
	startY += lineHeight;
	
	canvas->TextOut(startX, startY, L"Пули в залпе: " + IntToStr(stats.AltSpreadShotCount));
	startY += lineHeight;
	
	canvas->TextOut(startX, startY, L"Дальность: " + FloatToStrF((stats.AltBulletRangeMultiplier - 1.0f) * 100.0f, ffFixed, 3, 0) + L"%");
	startY += lineHeight;
	
	canvas->TextOut(startX, startY, L"Размер: " + FloatToStrF((stats.AltBulletSizeMultiplier - 1.0f) * 100.0f, ffFixed, 3, 0) + L"%");
	startY += lineHeight;
	
	canvas->TextOut(startX, startY, L"Скорость: " + FloatToStrF((stats.AltBulletSpeedMultiplier - 1.0f) * 100.0f, ffFixed, 3, 0) + L"%");
	startY += lineHeight + ScaleValue(5, width, height);

	// Общие
	canvas->Font->Color = static_cast<TColor>(RGB(0, 255, 160));
	canvas->TextOut(startX, startY, L"Общие:");
	startY += lineHeight;
	canvas->Font->Color = static_cast<TColor>(RGB(200, 200, 220));

	canvas->TextOut(startX, startY, L"Скорость: " + FloatToStrF((stats.SpeedMultiplier - 1.0f) * 100.0f, ffFixed, 3, 0) + L"%");
	startY += lineHeight;
	
	canvas->TextOut(startX, startY, L"Опыт: " + FloatToStrF((stats.ExperienceGainMultiplier - 1.0f) * 100.0f, ffFixed, 3, 0) + L"%");
	startY += lineHeight + ScaleValue(5, width, height);

	// Новые улучшения
	canvas->Font->Color = static_cast<TColor>(RGB(255, 150, 0));
	canvas->TextOut(startX, startY, L"Дополнительные:");
	startY += lineHeight;
	canvas->Font->Color = static_cast<TColor>(RGB(200, 200, 220));
	
	// Всегда показываем все улучшения, даже если значения 0
	canvas->TextOut(startX, startY, L"Регенерация: " + FloatToStrF(stats.HealthRegenPerWave, ffFixed, 1, 0) + L" HP/волна");
	startY += lineHeight;
	
	canvas->TextOut(startX, startY, L"Критический удар: " + FloatToStrF(stats.CriticalChancePercent, ffFixed, 1, 0) + L"%");
	startY += lineHeight;
	
	canvas->TextOut(startX, startY, L"Защита: " + FloatToStrF(stats.DamageReductionPercent, ffFixed, 1, 0) + L"%");
	startY += lineHeight;
	
	canvas->TextOut(startX, startY, L"Удача: " + FloatToStrF(stats.LuckPercent, ffFixed, 1, 0) + L"%");
	startY += lineHeight;
	
	canvas->TextOut(startX, startY, L"Вампиризм: " + FloatToStrF(stats.LifestealChancePercent, ffFixed, 1, 0) + L"%");
	startY += lineHeight;
}
//---------------------------------------------------------------------------
void TGameUIRenderer::DrawBossHealthBar(TCanvas *canvas, float healthRatio, int health, int maxHealth) const
{
	if (!canvas)
		return;
	
	const int width = canvas->ClipRect.Width();
	const int height = canvas->ClipRect.Height();
	const float scale = GetScale(width, height);
	
	const int barWidth = ScaleValue(600, width, height);
	const int barHeight = ScaleValue(30, width, height);
	const int barX = (width - barWidth) / 2;
	const int barY = ScaleValue(30, width, height); // сверху экрана
	
	// фон полоски
	canvas->Brush->Color = static_cast<TColor>(RGB(20, 20, 20));
	canvas->Pen->Color = static_cast<TColor>(RGB(100, 100, 100));
	canvas->Pen->Width = ScaleValue(2, width, height);
	canvas->Rectangle(barX, barY, barX + barWidth, barY + barHeight);
	
	// заполнение HP
	const int fillWidth = static_cast<int>(barWidth * healthRatio);
	if (fillWidth > 0)
	{
		canvas->Brush->Color = static_cast<TColor>(RGB(255, 50, 50));
		canvas->Pen->Color = static_cast<TColor>(RGB(255, 150, 150));
		canvas->Rectangle(barX + ScaleValue(2, width, height), barY + ScaleValue(2, width, height), 
			barX + fillWidth - ScaleValue(2, width, height), barY + barHeight - ScaleValue(2, width, height));
	}
	
	// текст HP
	canvas->Font->Color = clWhite;
	canvas->Font->Size = ScaleFontSize(14, width, height);
	canvas->Font->Style = TFontStyles() << fsBold;
	const UnicodeString hpText = L"БОСС: " + IntToStr(health) + L" / " + IntToStr(maxHealth);
	const int textWidth = canvas->TextWidth(hpText);
	const int textX = barX + (barWidth - textWidth) / 2;
	const int textY = barY + (barHeight - canvas->TextHeight(hpText)) / 2;
	canvas->TextOut(textX, textY, hpText);
}
//---------------------------------------------------------------------------
void TGameUIRenderer::DrawMainMenu(TCanvas *canvas, const TGameRecords &records, TGameUIState &uiState) const
{
	if (!canvas)
		return;
	
	const int width = canvas->ClipRect.Width();
	const int height = canvas->ClipRect.Height();
	const float scale = GetScale(width, height);

	canvas->Font->Name = "Segoe UI";
	
	// фон
	canvas->Brush->Color = static_cast<TColor>(RGB(5, 5, 15));
	canvas->FillRect(canvas->ClipRect);
	
	// заголовок
	canvas->Font->Name = "Segoe UI";
	canvas->Font->Size = ScaleFontSize(48, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(0, 200, 255));
	canvas->Font->Style = TFontStyles() << fsBold;
	const UnicodeString title = L"NEON ARENA";
	const TSize titleSize = canvas->TextExtent(title);
	const int titleX = (width - titleSize.cx) / 2;
	const int titleY = height / 4;
	canvas->TextOut(titleX, titleY, title);
	
	// кнопки
	const int buttonWidth = ScaleValue(300, width, height);
	const int buttonHeight = ScaleValue(60, width, height);
	const int buttonX = (width - buttonWidth) / 2;
	const int startY = height / 2;
	const int buttonSpacing = ScaleValue(80, width, height);
	
	// кнопка "Начать игру"
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
	
	// кнопка "Выход"
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
	
	// кнопка "Кооператив"
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

	// Рекорды внизу экрана (всегда отображаем)
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
	const int bestMinutes = bestTotalSeconds / 60;
	const int bestSeconds = bestTotalSeconds % 60;
	const UnicodeString bestTimeText = L"Время: " + IntToStr(bestMinutes) + L":" +
		(bestSeconds < 10 ? L"0" : L"") + IntToStr(bestSeconds);

	const TSize bestWaveSize = canvas->TextExtent(bestWaveText);
	const TSize bestKillSize = canvas->TextExtent(bestKillText);
	const TSize bestTimeSize = canvas->TextExtent(bestTimeText);

	const int maxWidth = std::max({bestWaveSize.cx, bestKillSize.cx, bestTimeSize.cx});
	const int recordsTextX = (width - maxWidth) / 2;

	canvas->TextOut(recordsTextX, recordsY, bestWaveText); recordsY += ScaleValue(26, width, height);
	canvas->TextOut(recordsTextX, recordsY, bestKillText); recordsY += ScaleValue(26, width, height);
	canvas->TextOut(recordsTextX, recordsY, bestTimeText);
}
//---------------------------------------------------------------------------
void TGameUIRenderer::DrawPauseMenu(TCanvas *canvas, TGameUIState &uiState) const
{
	if (!canvas)
		return;
	
	const int width = canvas->ClipRect.Width();
	const int height = canvas->ClipRect.Height();
	const float scale = GetScale(width, height);
	
	canvas->Font->Name = "Segoe UI";
	
	// затемнение фона
	canvas->Brush->Style = bsSolid;
	canvas->Brush->Color = static_cast<TColor>(RGB(0, 0, 0));
	canvas->Pen->Style = psClear;
	const int alpha = 180; // полупрозрачность
	canvas->Brush->Color = static_cast<TColor>(RGB(0, 0, 0));
	canvas->FillRect(canvas->ClipRect);
	
	// панель паузы
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
	
	// заголовок "ПАУЗА"
	canvas->Font->Name = "Segoe UI";
	canvas->Font->Size = ScaleFontSize(36, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(0, 200, 255));
	canvas->Font->Style = TFontStyles() << fsBold;
	const UnicodeString pauseText = L"ПАУЗА";
	const TSize pauseSize = canvas->TextExtent(pauseText);
	const int pauseX = panelX + (panelWidth - pauseSize.cx) / 2;
	const int pauseY = panelY + ScaleValue(30, width, height);
	canvas->TextOut(pauseX, pauseY, pauseText);
	
	// кнопки
	const int buttonWidth = ScaleValue(280, width, height);
	const int buttonHeight = ScaleValue(50, width, height);
	const int buttonX = panelX + (panelWidth - buttonWidth) / 2;
	const int resumeY = panelY + ScaleValue(100, width, height);
	const int menuY = panelY + ScaleValue(170, width, height);
	
	// кнопка "Продолжить"
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
	
	// кнопка "В меню"
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
//---------------------------------------------------------------------------
void TGameUIRenderer::DrawCooldownIndicators(TCanvas *canvas, float primaryCooldown, float altCooldown, float primaryMax, float altMax) const
{
	if (!canvas)
		return;
	
	const int width = canvas->ClipRect.Width();
	const int height = canvas->ClipRect.Height();
	const float scale = GetScale(width, height);
	
	const int indicatorSize = ScaleValue(20, width, height);
	const int indicatorY = height - ScaleValue(50, width, height); // справа внизу
	const int primaryX = width - ScaleValue(60, width, height);
	const int altX = width - ScaleValue(30, width, height);
	
	// индикатор основной атаки
	const float primaryRatio = primaryMax > 0.0f ? std::clamp(primaryCooldown / primaryMax, 0.0f, 1.0f) : 0.0f;
	canvas->Brush->Color = static_cast<TColor>(RGB(20, 20, 30));
	canvas->Pen->Color = static_cast<TColor>(RGB(0, 180, 255));
	canvas->Pen->Width = ScaleValue(2, width, height);
	canvas->Rectangle(primaryX - indicatorSize / 2, indicatorY - indicatorSize / 2,
		primaryX + indicatorSize / 2, indicatorY + indicatorSize / 2);
	
	if (primaryRatio > 0.0f)
	{
		const int fillHeight = static_cast<int>(indicatorSize * primaryRatio);
		canvas->Brush->Color = static_cast<TColor>(RGB(0, 150, 255));
		const int pad = ScaleValue(2, width, height);
		canvas->Rectangle(primaryX - indicatorSize / 2 + pad, indicatorY + indicatorSize / 2 - fillHeight - pad,
			primaryX + indicatorSize / 2 - pad, indicatorY + indicatorSize / 2 - pad);
	}
	
	// индикатор альтернативной атаки
	const float altRatio = altMax > 0.0f ? std::clamp(altCooldown / altMax, 0.0f, 1.0f) : 0.0f;
	canvas->Brush->Color = static_cast<TColor>(RGB(20, 20, 30));
	canvas->Pen->Color = static_cast<TColor>(RGB(200, 100, 255));
	canvas->Pen->Width = ScaleValue(2, width, height);
	canvas->Rectangle(altX - indicatorSize / 2, indicatorY - indicatorSize / 2,
		altX + indicatorSize / 2, indicatorY + indicatorSize / 2);
	
	if (altRatio > 0.0f)
	{
		const int fillHeight = static_cast<int>(indicatorSize * altRatio);
		canvas->Brush->Color = static_cast<TColor>(RGB(200, 80, 255));
		const int pad = ScaleValue(2, width, height);
		canvas->Rectangle(altX - indicatorSize / 2 + pad, indicatorY + indicatorSize / 2 - fillHeight - pad,
			altX + indicatorSize / 2 - pad, indicatorY + indicatorSize / 2 - pad);
	}
}
//---------------------------------------------------------------------------
void TGameUIRenderer::DrawMinimap(TCanvas *canvas,
	const TPointF &playerPos,
	const std::vector<TPointF> &enemyPositions,
	const TPointF &bossPos,
	bool hasBoss,
	float worldWidth,
	float worldHeight) const
{
	if (!canvas)
		return;

	const int width = canvas->ClipRect.Width();
	const int height = canvas->ClipRect.Height();
	const float scale = GetScale(width, height);
	
	// Размер мини-карты
	const int minimapSize = ScaleValue(180, width, height);
	const int minimapX = width - minimapSize - ScaleValue(16, width, height); // справа с отступом
	const int minimapY = ScaleValue(16, width, height); // сверху с отступом
	const int padding = ScaleValue(8, width, height);
	
	// Фон мини-карты
	canvas->Brush->Color = static_cast<TColor>(RGB(10, 10, 20));
	canvas->Brush->Style = bsSolid;
	canvas->Pen->Color = static_cast<TColor>(RGB(0, 150, 220));
	canvas->Pen->Width = ScaleValue(2, width, height);
	canvas->Rectangle(minimapX - padding, minimapY - padding,
		minimapX + minimapSize + padding, minimapY + minimapSize + padding);
	
	// Внутренний фон
	canvas->Brush->Color = static_cast<TColor>(RGB(5, 5, 15));
	canvas->Pen->Color = static_cast<TColor>(RGB(15, 25, 50));
	canvas->Pen->Width = ScaleValue(1, width, height);
	canvas->Rectangle(minimapX, minimapY, minimapX + minimapSize, minimapY + minimapSize);
	
	// Масштаб для преобразования мировых координат в координаты мини-карты
	const float scaleX = minimapSize / worldWidth;
	const float scaleY = minimapSize / worldHeight;
	
	// Рисуем врагов
	canvas->Brush->Color = static_cast<TColor>(RGB(255, 80, 80));
	canvas->Brush->Style = bsSolid;
	canvas->Pen->Style = psClear;
	
	for (const auto &enemyPos : enemyPositions)
	{
		const int x = minimapX + static_cast<int>(enemyPos.X * scaleX);
		const int y = minimapY + static_cast<int>(enemyPos.Y * scaleY);
		
		if (x >= minimapX && x < minimapX + minimapSize && y >= minimapY && y < minimapY + minimapSize)
		{
			const int enemyDotSize = ScaleValue(2, width, height);
			canvas->Ellipse(x - enemyDotSize, y - enemyDotSize, x + enemyDotSize, y + enemyDotSize);
		}
	}
	
	// Рисуем босса
	if (hasBoss)
	{
		const int x = minimapX + static_cast<int>(bossPos.X * scaleX);
		const int y = minimapY + static_cast<int>(bossPos.Y * scaleY);
		
		if (x >= minimapX && x < minimapX + minimapSize && y >= minimapY && y < minimapY + minimapSize)
		{
			canvas->Brush->Color = static_cast<TColor>(RGB(255, 100, 0));
			const int bossDotSize = ScaleValue(3, width, height);
			canvas->Ellipse(x - bossDotSize, y - bossDotSize, x + bossDotSize, y + bossDotSize);
		}
	}
	
	// Рисуем игрока (поверх всего)
	const int playerX = minimapX + static_cast<int>(playerPos.X * scaleX);
	const int playerY = minimapY + static_cast<int>(playerPos.Y * scaleY);
	
	if (playerX >= minimapX && playerX < minimapX + minimapSize && 
		playerY >= minimapY && playerY < minimapY + minimapSize)
	{
		canvas->Brush->Color = static_cast<TColor>(RGB(0, 255, 160));
		canvas->Pen->Color = static_cast<TColor>(RGB(0, 200, 120));
		canvas->Pen->Width = ScaleValue(1, width, height);
		canvas->Pen->Style = psSolid;
		const int playerDotSize = ScaleValue(3, width, height);
		canvas->Ellipse(playerX - playerDotSize, playerY - playerDotSize, playerX + playerDotSize, playerY + playerDotSize);
	}
	
	// Заголовок мини-карты
	canvas->Font->Name = "Segoe UI";
	canvas->Font->Size = ScaleFontSize(10, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(200, 200, 220));
	canvas->Font->Style = TFontStyles() << fsBold;
	const UnicodeString title = L"Карта";
	const TSize titleSize = canvas->TextExtent(title);
	canvas->TextOut(minimapX + (minimapSize - titleSize.cx) / 2, minimapY - padding - titleSize.cy - ScaleValue(2, width, height), title);
	canvas->Font->Style = TFontStyles();
}

//---------------------------------------------------------------------------
void TGameUIRenderer::DrawMinimap(TCanvas *canvas, const std::vector<TPointF> &playerPositions,
                                 const std::vector<TPointF> &enemyPositions,
                                 const TPointF &bossPos, bool hasBoss,
                                 float worldWidth, float worldHeight,
                                 uint8_t localPlayerID) const
{
	if (!canvas || playerPositions.empty())
		return;
	
	const int width = canvas->ClipRect.Width();
	const int height = canvas->ClipRect.Height();
	
	// Параметры мини-карты
	const int minimapSize = ScaleValue(150, width, height);
	const int padding = ScaleValue(10, width, height);
	const int minimapX = width - minimapSize - padding;
	const int minimapY = padding;
	
	// Фон мини-карты
	canvas->Brush->Color = static_cast<TColor>(RGB(20, 20, 30));
	canvas->Pen->Color = static_cast<TColor>(RGB(0, 200, 255));
	canvas->Pen->Width = ScaleValue(2, width, height);
	canvas->Rectangle(minimapX, minimapY, minimapX + minimapSize, minimapY + minimapSize);
	
	// Масштаб
	const float scaleX = static_cast<float>(minimapSize) / worldWidth;
	const float scaleY = static_cast<float>(minimapSize) / worldHeight;
	
	// Рисуем врагов
	canvas->Brush->Color = static_cast<TColor>(RGB(255, 50, 50));
	canvas->Pen->Color = static_cast<TColor>(RGB(200, 30, 30));
	canvas->Pen->Width = ScaleValue(1, width, height);
	for (const auto &enemyPos : enemyPositions)
	{
		const int x = minimapX + static_cast<int>(enemyPos.X * scaleX);
		const int y = minimapY + static_cast<int>(enemyPos.Y * scaleY);
		
		if (x >= minimapX && x < minimapX + minimapSize && y >= minimapY && y < minimapY + minimapSize)
		{
			const int enemyDotSize = ScaleValue(2, width, height);
			canvas->Ellipse(x - enemyDotSize, y - enemyDotSize, x + enemyDotSize, y + enemyDotSize);
		}
	}
	
	// Рисуем босса
	if (hasBoss)
	{
		const int x = minimapX + static_cast<int>(bossPos.X * scaleX);
		const int y = minimapY + static_cast<int>(bossPos.Y * scaleY);
		
		if (x >= minimapX && x < minimapX + minimapSize && y >= minimapY && y < minimapY + minimapSize)
		{
			canvas->Brush->Color = static_cast<TColor>(RGB(255, 100, 0));
			const int bossDotSize = ScaleValue(3, width, height);
			canvas->Ellipse(x - bossDotSize, y - bossDotSize, x + bossDotSize, y + bossDotSize);
		}
	}
	
	// Рисуем всех игроков
	const TColor playerColors[] = {
		static_cast<TColor>(RGB(0, 255, 160)),  // Игрок 0 (зеленый)
		static_cast<TColor>(RGB(100, 150, 255)), // Игрок 1 (синий)
		static_cast<TColor>(RGB(255, 150, 0)),  // Игрок 2 (оранжевый)
		static_cast<TColor>(RGB(255, 100, 255))  // Игрок 3 (розовый)
	};
	
	for (size_t i = 0; i < playerPositions.size(); i++)
	{
		const int playerX = minimapX + static_cast<int>(playerPositions[i].X * scaleX);
		const int playerY = minimapY + static_cast<int>(playerPositions[i].Y * scaleY);
		
		if (playerX >= minimapX && playerX < minimapX + minimapSize && 
		    playerY >= minimapY && playerY < minimapY + minimapSize)
		{
			const TColor color = (i < 4) ? playerColors[i] : static_cast<TColor>(RGB(200, 200, 200));
			canvas->Brush->Color = color;
			canvas->Pen->Color = color;
			canvas->Pen->Width = ScaleValue(1, width, height);
			canvas->Pen->Style = psSolid;
			const int playerDotSize = (i == localPlayerID) ? ScaleValue(4, width, height) : ScaleValue(3, width, height);
			canvas->Ellipse(playerX - playerDotSize, playerY - playerDotSize, playerX + playerDotSize, playerY + playerDotSize);
		}
	}
	
	// Заголовок мини-карты
	canvas->Font->Name = "Segoe UI";
	canvas->Font->Size = ScaleFontSize(10, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(200, 200, 220));
	canvas->Font->Style = TFontStyles() << fsBold;
	const UnicodeString title = L"Карта";
	const TSize titleSize = canvas->TextExtent(title);
	canvas->TextOut(minimapX + (minimapSize - titleSize.cx) / 2, minimapY - padding - titleSize.cy - ScaleValue(2, width, height), title);
	canvas->Font->Style = TFontStyles();
}

//---------------------------------------------------------------------------
void TGameUIRenderer::DrawPlayerNames(TCanvas *canvas, const std::vector<TPointF> &playerPositions,
                                     const std::vector<std::string> &playerNames,
                                     const TPointF &cameraPos, uint8_t localPlayerID) const
{
	if (!canvas || playerPositions.empty() || playerNames.empty())
		return;
	
	const int width = canvas->ClipRect.Width();
	const int height = canvas->ClipRect.Height();
	
	canvas->Font->Name = "Segoe UI";
	canvas->Font->Size = ScaleFontSize(14, width, height);
	canvas->Font->Style = TFontStyles() << fsBold;
	
	for (size_t i = 0; i < playerPositions.size() && i < playerNames.size(); i++)
	{
		if (i == localPlayerID)
			continue; // Не показываем имя локального игрока
		
		const TPointF &pos = playerPositions[i];
		const TPointF screenPos(pos.X - cameraPos.X, pos.Y - cameraPos.Y);
		
		// Проверяем, находится ли игрок на экране
		if (screenPos.X < -50 || screenPos.X > width + 50 || 
		    screenPos.Y < -50 || screenPos.Y > height + 50)
			continue;
		
		const UnicodeString name = UnicodeString(playerNames[i].c_str());
		const TSize nameSize = canvas->TextExtent(name);
		
		// Фон для имени
		const int textX = static_cast<int>(screenPos.X) - nameSize.cx / 2;
		const int textY = static_cast<int>(screenPos.Y) - ScaleValue(30, width, height);
		const TRect nameRect(textX - ScaleValue(5, width, height), textY - ScaleValue(2, width, height),
		                    textX + nameSize.cx + ScaleValue(5, width, height), textY + nameSize.cy + ScaleValue(2, width, height));
		
		canvas->Brush->Color = static_cast<TColor>(RGB(0, 0, 0));
		canvas->Brush->Style = bsSolid;
		canvas->FillRect(nameRect);
		
		// Текст имени
		canvas->Font->Color = static_cast<TColor>(RGB(255, 255, 255));
		canvas->TextOut(textX, textY, name);
	}
}

//---------------------------------------------------------------------------
void TGameUIRenderer::DrawCoopMenu(TCanvas *canvas, TGameUIState &uiState,
                                   const void *networkManagerPtr) const
{
	if (!canvas)
		return;
	
	// Приводим void* обратно к типу
	const NeonGame::TNetworkManager *networkManager = static_cast<const NeonGame::TNetworkManager*>(networkManagerPtr);
	if (!networkManager)
		return;
	
	const int width = canvas->ClipRect.Width();
	const int height = canvas->ClipRect.Height();
	const float scale = GetScale(width, height);
	
	canvas->Font->Name = "Segoe UI";
	
	// Фон
	canvas->Brush->Color = static_cast<TColor>(RGB(5, 5, 15));
	canvas->FillRect(canvas->ClipRect);
	
	// Заголовок
	canvas->Font->Name = "Segoe UI";
	canvas->Font->Size = ScaleFontSize(48, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(0, 200, 255));
	canvas->Font->Style = TFontStyles() << fsBold;
	const UnicodeString title = L"КООПЕРАТИВ";
	const TSize titleSize = canvas->TextExtent(title);
	const int titleX = (width - titleSize.cx) / 2;
	const int titleY = height / 8;
	canvas->TextOut(titleX, titleY, title);
	
	// Кнопки
	const int buttonWidth = ScaleValue(300, width, height);
	const int buttonHeight = ScaleValue(60, width, height);
	const int buttonX = (width - buttonWidth) / 2;
	const int startY = height / 3;
	const int buttonSpacing = ScaleValue(80, width, height);
	
	// Кнопка "Создать игру"
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
	
	// Кнопка "Присоединиться к игре"
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
	
	// Поле ввода IP (показываем всегда, если не хост)
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
		
		// Курсор, если поле в фокусе
		if (uiState.IPInputFocused)
		{
			const int cursorX = ipTextX + canvas->TextWidth(UnicodeString(uiState.IPAddress.c_str()));
			canvas->Pen->Color = clWhite;
			canvas->Pen->Width = ScaleValue(2, width, height);
			canvas->MoveTo(cursorX, ipTextY);
			canvas->LineTo(cursorX, ipTextY + canvas->TextHeight(ipText));
		}
	}
	
	// Список игроков (если хост или подключен)
	if (networkManager->IsHosting() || networkManager->GetState() == NeonGame::ENetworkState::Connected)
	{
		int listY = 0;

		// Для хоста список начинается под блоком с IP,
		// для подключенного клиента — строго под полем ввода IP, чтобы ничего не налезало
		if (networkManager->IsHosting())
		{
			// Поднимаем блок игроков выше, чтобы осталось место для 3-4 карточек
			listY = startY + buttonSpacing * 2 + ScaleValue(10, width, height);

			// Если мы хост — показываем наш IP
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
			// Клиент: поле ввода IP уже нарисовано выше и rect сохранён в uiState.IPInputRect
			// Начинаем список игроков строго под ним, с небольшим отступом,
			// чтобы подписи никогда не налезали на поле
			listY = uiState.IPInputRect.Bottom + ScaleValue(16, width, height);
		}

		// Заголовок секции игроков
		canvas->Font->Size = ScaleFontSize(22, width, height);
		canvas->Font->Color = static_cast<TColor>(RGB(0, 200, 255));
		canvas->Font->Style = TFontStyles() << fsBold;
		const UnicodeString playersTitle = L"Игроки";
		const TSize playersTitleSize = canvas->TextExtent(playersTitle);
		const int playersTitleX = buttonX;
		canvas->TextOut(playersTitleX, listY, playersTitle);

		// Небольшая подчеркивающая линия под заголовком
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

			// Бейдж роли/состояния
			UnicodeString roleTag;
			if (networkManager->IsHosting() && client.PlayerID == 0)
				roleTag = L"[Хост]";
			else if (client.PlayerID == localID)
				roleTag = L"[Вы]";

			// Карточка игрока
			const int cardPaddingX = ScaleValue(10, width, height);
			const int cardPaddingY = ScaleValue(6, width, height);
			const int cardWidth = buttonWidth;
			// увеличиваем высоту карточки, чтобы статус не налезал на имя
			const int cardHeight = ScaleValue(56, width, height);
			const int cardX = buttonX;
			const int cardY = playerY;

			TRect cardRect(cardX, cardY, cardX + cardWidth, cardY + cardHeight);
			canvas->Brush->Color = static_cast<TColor>(RGB(25, 20, 60));
			canvas->Pen->Color = static_cast<TColor>(RGB(90, 70, 200));
			canvas->Pen->Width = ScaleValue(1, width, height);
			canvas->Rectangle(cardRect);

			// Основная строка: Имя, ID, роль
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

			// Подстрока со статусом подключения (серый)
			canvas->Font->Color = static_cast<TColor>(RGB(170, 170, 190));
			UnicodeString status = L"Статус: подключен";
			canvas->Font->Size = ScaleFontSize(12, width, height);
			// рассчитываем позицию статуса по высоте основного текста,
			// чтобы он всегда был чуть ниже и не пересекался
			const TSize lineSize = canvas->TextExtent(line);
			const int statusX = cardX + cardPaddingX + ScaleValue(10, width, height);
			const int statusY = lineY + lineSize.cy + ScaleValue(4, width, height);
			canvas->TextOut(statusX, statusY, status);

			// Отступ между карточками игроков
			playerY += cardHeight + ScaleValue(6, width, height);
		}
		
		// Кнопка "Начать игру" (только для хоста)
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
	
	// Кнопка "Назад"
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
	
	// Статус подключения
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
//---------------------------------------------------------------------------


