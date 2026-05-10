#include <vcl.h>
#pragma hdrstop

#include "GameUIRenderer.h"
#include <algorithm>

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

	const int minimapSize = ScaleValue(180, width, height);
	const int minimapX = width - minimapSize - ScaleValue(16, width, height);
	const int minimapY = ScaleValue(16, width, height);
	const int padding = ScaleValue(8, width, height);

	canvas->Brush->Color = static_cast<TColor>(RGB(10, 10, 20));
	canvas->Brush->Style = bsSolid;
	canvas->Pen->Color = static_cast<TColor>(RGB(0, 150, 220));
	canvas->Pen->Width = ScaleValue(2, width, height);
	canvas->Rectangle(minimapX - padding, minimapY - padding,
		minimapX + minimapSize + padding, minimapY + minimapSize + padding);

	canvas->Brush->Color = static_cast<TColor>(RGB(5, 5, 15));
	canvas->Pen->Color = static_cast<TColor>(RGB(15, 25, 50));
	canvas->Pen->Width = ScaleValue(1, width, height);
	canvas->Rectangle(minimapX, minimapY, minimapX + minimapSize, minimapY + minimapSize);

	const float scaleX = minimapSize / worldWidth;
	const float scaleY = minimapSize / worldHeight;

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

	canvas->Font->Name = "Segoe UI";
	canvas->Font->Size = ScaleFontSize(10, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(200, 200, 220));
	canvas->Font->Style = TFontStyles() << fsBold;
	const UnicodeString title = L"Карта";
	const TSize titleSize = canvas->TextExtent(title);
	canvas->TextOut(minimapX + (minimapSize - titleSize.cx) / 2, minimapY - padding - titleSize.cy - ScaleValue(2, width, height), title);
	canvas->Font->Style = TFontStyles();
}

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

	const int minimapSize = ScaleValue(150, width, height);
	const int padding = ScaleValue(10, width, height);
	const int minimapX = width - minimapSize - padding;
	const int minimapY = padding;

	canvas->Brush->Color = static_cast<TColor>(RGB(20, 20, 30));
	canvas->Pen->Color = static_cast<TColor>(RGB(0, 200, 255));
	canvas->Pen->Width = ScaleValue(2, width, height);
	canvas->Rectangle(minimapX, minimapY, minimapX + minimapSize, minimapY + minimapSize);

	const float scaleX = static_cast<float>(minimapSize) / worldWidth;
	const float scaleY = static_cast<float>(minimapSize) / worldHeight;

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

	const TColor playerColors[] = {
		static_cast<TColor>(RGB(0, 255, 160)),
		static_cast<TColor>(RGB(100, 150, 255)),
		static_cast<TColor>(RGB(255, 150, 0)),
		static_cast<TColor>(RGB(255, 100, 255))
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

	canvas->Font->Name = "Segoe UI";
	canvas->Font->Size = ScaleFontSize(10, width, height);
	canvas->Font->Color = static_cast<TColor>(RGB(200, 200, 220));
	canvas->Font->Style = TFontStyles() << fsBold;
	const UnicodeString title = L"Карта";
	const TSize titleSize = canvas->TextExtent(title);
	canvas->TextOut(minimapX + (minimapSize - titleSize.cx) / 2, minimapY - padding - titleSize.cy - ScaleValue(2, width, height), title);
	canvas->Font->Style = TFontStyles();
}

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
			continue;

		const TPointF &pos = playerPositions[i];
		const TPointF screenPos(pos.X - cameraPos.X, pos.Y - cameraPos.Y);

		if (screenPos.X < -50 || screenPos.X > width + 50 ||
			screenPos.Y < -50 || screenPos.Y > height + 50)
			continue;

		const UnicodeString name = UnicodeString(playerNames[i].c_str());
		const TSize nameSize = canvas->TextExtent(name);

		const int textX = static_cast<int>(screenPos.X) - nameSize.cx / 2;
		const int textY = static_cast<int>(screenPos.Y) - ScaleValue(30, width, height);
		const TRect nameRect(textX - ScaleValue(5, width, height), textY - ScaleValue(2, width, height),
			textX + nameSize.cx + ScaleValue(5, width, height), textY + nameSize.cy + ScaleValue(2, width, height));

		canvas->Brush->Color = static_cast<TColor>(RGB(0, 0, 0));
		canvas->Brush->Style = bsSolid;
		canvas->FillRect(nameRect);

		canvas->Font->Color = static_cast<TColor>(RGB(255, 255, 255));
		canvas->TextOut(textX, textY, name);
	}
}
