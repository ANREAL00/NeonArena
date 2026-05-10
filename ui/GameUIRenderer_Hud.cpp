#include <vcl.h>
#pragma hdrstop

#include "GameUIRenderer.h"
#include "GameUIRenderer_Utils.h"
#include <algorithm>
#include <cmath>

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

	canvas->Font->Name = "Segoe UI";

	const int barWidth = ScaleValue(220, width, height);
	const int barHeight = ScaleValue(18, width, height);
	const int barX = ScaleValue(16, width, height);
	const int barY = ScaleValue(16, width, height);
	const int barSpacing = ScaleValue(28, width, height);

	const int topY = ScaleValue(16, width, height);
	canvas->Font->Size = ScaleFontSize(16, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(200, 200, 220));

	const int totalSeconds = static_cast<int>(stats.RunTimeSeconds + 0.5f);
	const UnicodeString timeText = NeonGameUIRendererUtils::FormatMinutesSeconds(totalSeconds);
	const TSize timeSize = canvas->TextExtent(timeText);

	const UnicodeString waveText = L"Волна " + IntToStr(stats.CurrentWave);
	const TSize waveSize = canvas->TextExtent(waveText);

	const int totalTopWidth = timeSize.cx + ScaleValue(20, width, height) + waveSize.cx;
	const int topX = (width - totalTopWidth) / 2;
	const int padding = ScaleValue(8, width, height);

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

	canvas->Font->Size = ScaleFontSize(12, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(255, 255, 255));
	canvas->Font->Style = TFontStyles() << fsBold;
	const UnicodeString healthText = IntToStr(health) + L" / " + IntToStr(maxHealth);
	const TSize healthTextSize = canvas->TextExtent(healthText);
	canvas->TextOut(barX + (barWidth - healthTextSize.cx) / 2, healthBarY + (barHeight - healthTextSize.cy) / 2, healthText);
	canvas->Font->Style = TFontStyles();

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

	canvas->Font->Size = ScaleFontSize(12, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(255, 255, 255));
	canvas->Font->Style = TFontStyles() << fsBold;
	const UnicodeString expText = IntToStr(experience) + L" / " + IntToStr(experienceToNext);
	const TSize expTextSize = canvas->TextExtent(expText);
	canvas->TextOut(barX + (barWidth - expTextSize.cx) / 2, expBarY + (barHeight - expTextSize.cy) / 2, expText);
	canvas->Font->Style = TFontStyles();

	const int hintY = height - ScaleValue(30, width, height);
	canvas->Font->Size = ScaleFontSize(11, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(150, 150, 170));
	canvas->Font->Style = TFontStyles();
	const UnicodeString hintText = showStatsPanel ? L"TAB - скрыть характеристики" : L"TAB - показать характеристики";
	const TSize hintSize = canvas->TextExtent(hintText);
	const int hintX = (width - hintSize.cx) / 2;

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
	const int barY = ScaleValue(30, width, height);

	canvas->Brush->Color = static_cast<TColor>(RGB(20, 20, 20));
	canvas->Pen->Color = static_cast<TColor>(RGB(100, 100, 100));
	canvas->Pen->Width = ScaleValue(2, width, height);
	canvas->Rectangle(barX, barY, barX + barWidth, barY + barHeight);

	const int fillWidth = static_cast<int>(barWidth * healthRatio);
	if (fillWidth > 0)
	{
		canvas->Brush->Color = static_cast<TColor>(RGB(255, 50, 50));
		canvas->Pen->Color = static_cast<TColor>(RGB(255, 150, 150));
		canvas->Rectangle(barX + ScaleValue(2, width, height), barY + ScaleValue(2, width, height),
			barX + fillWidth - ScaleValue(2, width, height), barY + barHeight - ScaleValue(2, width, height));
	}

	canvas->Font->Color = clWhite;
	canvas->Font->Size = ScaleFontSize(14, width, height);
	canvas->Font->Style = TFontStyles() << fsBold;
	const UnicodeString hpText = L"БОСС: " + IntToStr(health) + L" / " + IntToStr(maxHealth);
	const int textWidth = canvas->TextWidth(hpText);
	const int textX = barX + (barWidth - textWidth) / 2;
	const int textY = barY + (barHeight - canvas->TextHeight(hpText)) / 2;
	canvas->TextOut(textX, textY, hpText);
}

void TGameUIRenderer::DrawCooldownIndicators(TCanvas *canvas, float primaryCooldown, float altCooldown, float primaryMax, float altMax) const
{
	if (!canvas)
		return;

	const int width = canvas->ClipRect.Width();
	const int height = canvas->ClipRect.Height();
	const float scale = GetScale(width, height);

	const int indicatorSize = ScaleValue(20, width, height);
	const int indicatorY = height - ScaleValue(50, width, height);
	const int primaryX = width - ScaleValue(60, width, height);
	const int altX = width - ScaleValue(30, width, height);

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
