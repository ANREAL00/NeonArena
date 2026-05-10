#include <vcl.h>
#pragma hdrstop

#include "GameUIRenderer.h"
#include "GameUIRenderer_Utils.h"
#include "systems\\GameUpgrade.h"
#include <algorithm>
#include <cmath>

void TGameUIRenderer::DrawUpgradeMenu(TCanvas *canvas,
	const std::vector<TUpgrade> &upgrades,
	TGameUIState &uiState) const
{
	if (!canvas || upgrades.empty())
		return;

	const int width = canvas->ClipRect.Width();
	const int height = canvas->ClipRect.Height();
	const float scale = GetScale(width, height);

	canvas->Brush->Style = bsSolid;
	canvas->Brush->Color = static_cast<TColor>(RGB(0, 0, 0));
	canvas->Pen->Style = psClear;
	canvas->Rectangle(0, 0, width, height);
	canvas->Pen->Style = psSolid;

	canvas->Font->Name = "Segoe UI";
	canvas->Font->Size = ScaleFontSize(28, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(0, 255, 200));
	canvas->Font->Style = TFontStyles() << fsBold;
	const UnicodeString title = L"Выберите улучшение";
	const TSize titleSize = canvas->TextExtent(title);
	const int centerX = width / 2;
	canvas->TextOut(centerX - titleSize.cx / 2, ScaleValue(40, width, height), title);
	canvas->Font->Style = TFontStyles();

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

	for (size_t i = 0; i < upgrades.size() && i < 3; i++)
	{
		const TUpgrade &upgrade = upgrades[i];
		const int buttonX = startX + static_cast<int>(i) * (buttonWidth + buttonSpacing);
		const TRect buttonRect(buttonX, startY, buttonX + buttonWidth, startY + buttonHeight);

		if (uiState.UpgradeButtonRects.size() <= i)
			uiState.UpgradeButtonRects.push_back(buttonRect);
		else
			uiState.UpgradeButtonRects[i] = buttonRect;

		if (uiState.UpgradeButtonHovers.size() <= i)
			uiState.UpgradeButtonHovers.push_back(false);

		const float hoverAnim = (i < uiState.UpgradeButtonHoverTime.size()) ? uiState.UpgradeButtonHoverTime[i] : 0.0f;
		const float scale = 1.0f + hoverAnim * 0.05f;
		const int scaledWidth = static_cast<int>(buttonWidth * scale);
		const int scaledHeight = static_cast<int>(buttonHeight * scale);
		const int scaledX = buttonX - (scaledWidth - buttonWidth) / 2;
		const int scaledY = startY - (scaledHeight - buttonHeight) / 2;
		const TRect scaledRect(scaledX, scaledY, scaledX + scaledWidth, scaledY + scaledHeight);

		const bool hover = (i < uiState.UpgradeButtonHovers.size()) ? uiState.UpgradeButtonHovers[i] : false;

		float r1, g1, b1, r2, g2, b2;

		switch (upgrade.Rarity)
		{
		case EUpgradeRarity::Common:
			r1 = 15.0f + hoverAnim * 15.0f;
			g1 = 25.0f + hoverAnim * 25.0f;
			b1 = 40.0f + hoverAnim * 30.0f;
			r2 = 0.0f;
			g2 = 120.0f + hoverAnim * 60.0f;
			b2 = 200.0f + hoverAnim * 55.0f;
			break;
		case EUpgradeRarity::Rare:
			r1 = 25.0f + hoverAnim * 20.0f;
			g1 = 15.0f + hoverAnim * 20.0f;
			b1 = 50.0f + hoverAnim * 30.0f;
			r2 = 120.0f + hoverAnim * 60.0f;
			g2 = 0.0f;
			b2 = 200.0f + hoverAnim * 55.0f;
			break;
		case EUpgradeRarity::Legendary:
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

		canvas->Font->Size = ScaleFontSize(12, width, height);
		canvas->Font->Color = static_cast<TColor>(RGB(180, 180, 200));

		UnicodeString desc = upgrade.Description;
		const int maxDescWidth = scaledWidth - ScaleValue(20, width, height);

		UnicodeString line1, line2;
		TSize fullSize = canvas->TextExtent(desc);

		if (fullSize.cx <= maxDescWidth)
		{
			line1 = desc;
		}
		else
		{
			int breakPos = desc.Length() / 2;

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

			TSize line1Size = canvas->TextExtent(line1);
			TSize line2Size = canvas->TextExtent(line2);

			if (line1Size.cx > maxDescWidth)
			{
				int pos = line1.Length();
				while (pos > 0 && canvas->TextExtent(line1.SubString(1, pos) + L"...").cx > maxDescWidth)
					pos--;
				if (pos > 0)
					line1 = line1.SubString(1, pos) + L"...";
			}

			if (line2Size.cx > maxDescWidth)
			{
				int pos = line2.Length();
				while (pos > 0 && canvas->TextExtent(line2.SubString(1, pos) + L"...").cx > maxDescWidth)
					pos--;
				if (pos > 0)
					line2 = line2.SubString(1, pos) + L"...";
			}
		}

		if (!line1.IsEmpty())
		{
			TSize line1Size = canvas->TextExtent(line1);
			canvas->TextOut(scaledX + (scaledWidth - line1Size.cx) / 2, scaledY + ScaleValue(58, width, height), line1);
		}

		if (!line2.IsEmpty())
		{
			TSize line2Size = canvas->TextExtent(line2);
			canvas->TextOut(scaledX + (scaledWidth - line2Size.cx) / 2, scaledY + ScaleValue(76, width, height), line2);
		}

		canvas->Font->Size = ScaleFontSize(16, width, height);
		canvas->Font->Color = static_cast<TColor>(RGB(150, 150, 170));
		const UnicodeString keyText = L"[" + IntToStr(static_cast<int>(i) + 1) + L"]";
		const TSize keySize = canvas->TextExtent(keyText);
		canvas->TextOut(scaledX + (scaledWidth - keySize.cx) / 2, scaledY + scaledHeight - ScaleValue(28, width, height), keyText);
	}

	canvas->Font->Size = ScaleFontSize(14, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(150, 150, 170));
	const UnicodeString hint = L"Нажмите 1, 2 или 3 для выбора";
	const TSize hintSize = canvas->TextExtent(hint);
	canvas->TextOut(centerX - hintSize.cx / 2, startY + buttonHeight + ScaleValue(40, width, height), hint);
}

void TGameUIRenderer::DrawLevelUpNotification(TCanvas *canvas,
	float timer, int level) const
{
	if (!canvas || timer <= 0.0f)
		return;

	const int width = canvas->ClipRect.Width();
	const int height = canvas->ClipRect.Height();
	const float scale = GetScale(width, height);
	const int baseFontSize = ScaleFontSize(48, width, height);

	const float alpha = std::min(1.0f, timer / 0.5f);
	const float fadeOut = (timer < 0.5f) ? (timer / 0.5f) : 1.0f;
	const float finalAlpha = alpha * fadeOut;

	canvas->Font->Name = "Segoe UI";
	canvas->Font->Size = baseFontSize;
	canvas->Font->Style = TFontStyles() << fsBold;

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
	const int centerY = height / 3;

	canvas->Font->Color = static_cast<TColor>(RGB(0, 0, 0));
	const int shadowOffset = ScaleValue(3, width, height);
	canvas->TextOut(centerX - textSize.cx / 2 + shadowOffset, centerY + shadowOffset, levelText);

	canvas->Font->Color = static_cast<TColor>(RGB(
		static_cast<int>(0 * finalAlpha),
		static_cast<int>(255 * finalAlpha),
		static_cast<int>(200 * finalAlpha)));
	canvas->TextOut(centerX - textSize.cx / 2, centerY, levelText);

	canvas->Font->Style = TFontStyles();
}

void TGameUIRenderer::DrawPlayerStats(TCanvas *canvas, const TPlayerStats &stats, const TGameUIState &uiState) const
{
	if (!canvas || !uiState.ShowStatsPanel)
		return;

	const int width = canvas->ClipRect.Width();
	const int height = canvas->ClipRect.Height();
	const float scale = GetScale(width, height);

	canvas->Font->Name = "Segoe UI";

	const int startX = ScaleValue(16, width, height);
	int startY = ScaleValue(150, width, height);
	const int lineHeight = ScaleValue(22, width, height);
	const int panelWidth = ScaleValue(200, width, height);
	const int padding = ScaleValue(8, width, height);

	canvas->Font->Name = "Segoe UI";
	canvas->Font->Size = ScaleFontSize(11, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(200, 200, 220));

	canvas->Brush->Style = bsSolid;
	canvas->Brush->Color = static_cast<TColor>(RGB(10, 10, 20));
	canvas->Pen->Color = static_cast<TColor>(RGB(0, 150, 220));
	canvas->Pen->Width = ScaleValue(1, width, height);
	const int panelHeight = 30 * lineHeight + padding * 2;
	canvas->Rectangle(startX - padding, startY - padding, startX + panelWidth + padding, startY + panelHeight + padding);

	canvas->Font->Size = ScaleFontSize(13, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(0, 200, 255));
	canvas->Font->Style = TFontStyles() << fsBold;
	canvas->TextOut(startX, startY, L"Характеристики");
	canvas->Font->Style = TFontStyles();
	startY += lineHeight + ScaleValue(5, width, height);

	canvas->Font->Size = ScaleFontSize(11, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(200, 200, 220));

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

	canvas->Font->Color = static_cast<TColor>(RGB(0, 255, 160));
	canvas->TextOut(startX, startY, L"Общие:");
	startY += lineHeight;
	canvas->Font->Color = static_cast<TColor>(RGB(200, 200, 220));

	canvas->TextOut(startX, startY, L"Скорость: " + FloatToStrF((stats.SpeedMultiplier - 1.0f) * 100.0f, ffFixed, 3, 0) + L"%");
	startY += lineHeight;

	canvas->TextOut(startX, startY, L"Опыт: " + FloatToStrF((stats.ExperienceGainMultiplier - 1.0f) * 100.0f, ffFixed, 3, 0) + L"%");
	startY += lineHeight + ScaleValue(5, width, height);

	canvas->Font->Color = static_cast<TColor>(RGB(255, 150, 0));
	canvas->TextOut(startX, startY, L"Дополнительные:");
	startY += lineHeight;
	canvas->Font->Color = static_cast<TColor>(RGB(200, 200, 220));

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
