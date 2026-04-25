#include <vcl.h>
#pragma hdrstop

#include "GameEnemy.h"
#include "GameConstants.h"
#include <Windows.h>
#include <cmath>

using namespace NeonGame;

void TBossEnemy::Draw(TCanvas *canvas, const TPointF &camera) const
{
	if (!canvas)
		return;

	const float screenX = position.X - camera.X;
	const float screenY = position.Y - camera.Y;

	const float r = EnemyRadius * 2.5f;
	const int left = static_cast<int>(std::round(screenX - r));
	const int top = static_cast<int>(std::round(screenY - r));
	const int right = static_cast<int>(std::round(screenX + r));
	const int bottom = static_cast<int>(std::round(screenY + r));

	TColor bossColor, bossPenColor;
	switch (currentPhase)
	{
		case EBossPhase::Phase1:
			bossColor = static_cast<TColor>(RGB(255, 100, 0));
			bossPenColor = static_cast<TColor>(RGB(255, 200, 100));
			break;
		case EBossPhase::Phase2:
			bossColor = static_cast<TColor>(RGB(200, 0, 255));
			bossPenColor = static_cast<TColor>(RGB(255, 100, 255));
			break;
		case EBossPhase::Phase3:
			bossColor = static_cast<TColor>(RGB(255, 0, 0));
			bossPenColor = static_cast<TColor>(RGB(255, 150, 150));
			break;
		case EBossPhase::Phase4:
			bossColor = static_cast<TColor>(RGB(255, 50, 150));
			bossPenColor = static_cast<TColor>(RGB(255, 150, 200));
			break;
		case EBossPhase::Phase5:
			bossColor = static_cast<TColor>(RGB(150, 0, 0));
			bossPenColor = static_cast<TColor>(RGB(255, 0, 0));
			break;
		default:
			bossColor = static_cast<TColor>(RGB(255, 100, 0));
			bossPenColor = static_cast<TColor>(RGB(255, 200, 100));
			break;
	}

	const int centerX = static_cast<int>(std::round(screenX));
	const int centerY = static_cast<int>(std::round(screenY));

	const int numPoints = 8;
	TPoint outerOct[8];
	for (int i = 0; i < numPoints; i++)
	{
		const float angle = i * 2.0f * 3.14159265f / numPoints;
		outerOct[i].X = centerX + static_cast<int>(std::round(r * std::cos(angle)));
		outerOct[i].Y = centerY + static_cast<int>(std::round(r * std::sin(angle)));
	}

	canvas->Brush->Color = bossColor;
	canvas->Pen->Color = bossPenColor;
	canvas->Pen->Width = 4;
	canvas->Brush->Style = bsSolid;
	canvas->Polygon(outerOct, numPoints - 1);

	const float innerR = r * 0.6f;
	TPoint innerOct[8];
	for (int i = 0; i < numPoints; i++)
	{
		const float angle = i * 2.0f * 3.14159265f / numPoints;
		innerOct[i].X = centerX + static_cast<int>(std::round(innerR * std::cos(angle)));
		innerOct[i].Y = centerY + static_cast<int>(std::round(innerR * std::sin(angle)));
	}

	canvas->Pen->Width = 2;
	canvas->Pen->Color = static_cast<TColor>(RGB(
		static_cast<int>((bossPenColor >> 16) & 0xFF) * 0.8f,
		static_cast<int>((bossPenColor >> 8) & 0xFF) * 0.8f,
		static_cast<int>(bossPenColor & 0xFF) * 0.8f));
	canvas->Brush->Color = static_cast<TColor>(RGB(
		static_cast<int>((bossColor >> 16) & 0xFF) * 0.7f,
		static_cast<int>((bossColor >> 8) & 0xFF) * 0.7f,
		static_cast<int>(bossColor & 0xFF) * 0.7f));
	canvas->Polygon(innerOct, numPoints - 1);

	const float centerR = r * 0.3f;
	const int centerLeft = static_cast<int>(std::round(screenX - centerR));
	const int centerTop = static_cast<int>(std::round(screenY - centerR));
	const int centerRight = static_cast<int>(std::round(screenX + centerR));
	const int centerBottom = static_cast<int>(std::round(screenY + centerR));
	canvas->Brush->Color = bossPenColor;
	canvas->Pen->Color = static_cast<TColor>(RGB(255, 255, 255));
	canvas->Pen->Width = 1;
	canvas->Ellipse(centerLeft, centerTop, centerRight, centerBottom);

	const float rot = static_cast<float>(GetTickCount()) * 0.0007f;
	TPoint ring[8];
	for (int i = 0; i < numPoints; i++)
	{
		const float angle = rot + i * 2.0f * 3.14159265f / numPoints;
		const float ringR = r * 1.12f;
		ring[i].X = centerX + static_cast<int>(std::round(ringR * std::cos(angle)));
		ring[i].Y = centerY + static_cast<int>(std::round(ringR * std::sin(angle)));
	}
	canvas->Brush->Style = bsClear;
	canvas->Pen->Color = bossPenColor;
	canvas->Pen->Width = 2;
	canvas->Polygon(ring, numPoints - 1);

	const float crossSize = centerR * 0.7f;
	canvas->MoveTo(centerX - static_cast<int>(crossSize), centerY);
	canvas->LineTo(centerX + static_cast<int>(crossSize), centerY);
	canvas->MoveTo(centerX, centerY - static_cast<int>(crossSize));
	canvas->LineTo(centerX, centerY + static_cast<int>(crossSize));

	canvas->Brush->Style = bsSolid;
}

