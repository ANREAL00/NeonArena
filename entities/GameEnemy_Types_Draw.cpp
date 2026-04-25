#include <vcl.h>
#pragma hdrstop

#include "GameEnemy.h"
#include "GameConstants.h"
#include <Windows.h>
#include <cmath>

using namespace NeonGame;

namespace
{
float AnimTimeSeconds()
{
	return static_cast<float>(GetTickCount()) * 0.001f;
}

float PulseAt(float worldX, float worldY, float speedMul = 1.0f)
{
	const float t = AnimTimeSeconds() * speedMul;
	return 0.5f + 0.5f * std::sin(t * 2.8f + worldX * 0.031f + worldY * 0.027f);
}

void DrawGlowRing(TCanvas *canvas, int cx, int cy, int radius, TColor pen, int penW)
{
	if (!canvas || radius < 2)
		return;
	canvas->Pen->Color = pen;
	canvas->Pen->Width = penW;
	canvas->Brush->Style = bsClear;
	canvas->Ellipse(cx - radius, cy - radius, cx + radius, cy + radius);
}

void RotPoly(TPoint *out, int n, int cx, int cy, float baseR, float rotRad)
{
	for (int i = 0; i < n; ++i)
	{
		const float a = rotRad + (i * 2.0f * 3.14159265f / n);
		out[i].X = cx + static_cast<int>(std::round(baseR * std::cos(a)));
		out[i].Y = cy + static_cast<int>(std::round(baseR * std::sin(a)));
	}
}
}

void TBasicEnemy::Draw(TCanvas *canvas, const TPointF &camera) const
{
	if (!canvas)
		return;

	const float screenX = position.X - camera.X;
	const float screenY = position.Y - camera.Y;
	const int cx = static_cast<int>(std::round(screenX));
	const int cy = static_cast<int>(std::round(screenY));

	const float pulse = PulseAt(position.X, position.Y);
	const float r = EnemyRadius * (0.92f + 0.06f * pulse);
	const float rot = AnimTimeSeconds() * 0.9f + position.X * 0.004f;

	TPoint hex[6];
	RotPoly(hex, 6, cx, cy, r * 1.08f, rot);

	canvas->Brush->Color = static_cast<TColor>(RGB(40, 8, 12));
	canvas->Pen->Color = static_cast<TColor>(RGB(180, 30, 50));
	canvas->Pen->Width = 3;
	canvas->Brush->Style = bsSolid;
	canvas->Polygon(hex, 5);

	TPoint innerHex[6];
	RotPoly(innerHex, 6, cx, cy, r * 0.72f, rot + 0.2f);
	canvas->Brush->Color = static_cast<TColor>(RGB(
		static_cast<int>(90 + 80 * pulse),
		static_cast<int>(15 + 25 * pulse),
		static_cast<int>(25 + 30 * pulse)));
	canvas->Pen->Color = static_cast<TColor>(RGB(255, 90, 100));
	canvas->Pen->Width = 2;
	canvas->Polygon(innerHex, 5);

	TPoint dia[4];
	dia[0].X = cx; dia[0].Y = cy - static_cast<int>(r * 0.55f);
	dia[1].X = cx + static_cast<int>(r * 0.55f); dia[1].Y = cy;
	dia[2].X = cx; dia[2].Y = cy + static_cast<int>(r * 0.55f);
	dia[3].X = cx - static_cast<int>(r * 0.55f); dia[3].Y = cy;
	canvas->Brush->Color = static_cast<TColor>(RGB(255, 70, 90));
	canvas->Pen->Color = static_cast<TColor>(RGB(255, 200, 210));
	canvas->Pen->Width = 2;
	canvas->Polygon(dia, 3);

	const int coreR = static_cast<int>(r * (0.22f + 0.06f * pulse));
	canvas->Brush->Color = static_cast<TColor>(RGB(255, 240, 220));
	canvas->Pen->Color = static_cast<TColor>(RGB(255, 180, 160));
	canvas->Pen->Width = 1;
	canvas->Ellipse(cx - coreR, cy - coreR, cx + coreR, cy + coreR);

	const float orbit = AnimTimeSeconds() * 2.4f;
	for (int k = 0; k < 3; ++k)
	{
		const float a = orbit + k * 2.1f;
		const int ox = cx + static_cast<int>(std::cos(a) * r * 1.15f);
		const int oy = cy + static_cast<int>(std::sin(a) * r * 1.15f);
		canvas->Brush->Color = static_cast<TColor>(RGB(255, 120, 140));
		canvas->Pen->Color = static_cast<TColor>(RGB(255, 220, 230));
		canvas->Pen->Width = 1;
		const int pr = static_cast<int>(4 + 2 * pulse);
		canvas->Ellipse(ox - pr, oy - pr, ox + pr, oy + pr);
	}

	DrawGlowRing(canvas, cx, cy, static_cast<int>(r * 1.25f + 4 * pulse),
		static_cast<TColor>(RGB(255, 60, 80)), 1);
}

void TFastEnemy::Draw(TCanvas *canvas, const TPointF &camera) const
{
	if (!canvas)
		return;

	const float screenX = position.X - camera.X;
	const float screenY = position.Y - camera.Y;
	const int cx = static_cast<int>(std::round(screenX));
	const int cy = static_cast<int>(std::round(screenY));

	const float t = AnimTimeSeconds();
	const float wobble = std::sin(t * 5.0f + position.Y * 0.05f) * 0.25f;
	const float r = EnemyRadius * 0.85f;
	const float aim = t * 3.0f + position.X * 0.02f + wobble;

	const float fx = std::cos(aim);
	const float fy = std::sin(aim);

	canvas->Pen->Mode = pmCopy;

	for (int layer = 0; layer < 3; ++layer)
	{
		const float back = static_cast<float>(layer + 1) * 10.0f;
		const int bx = cx - static_cast<int>(fx * back);
		const int by = cy - static_cast<int>(fy * back);
		const int fade = 80 - layer * 22;
		TPoint tri[3];
		tri[0].X = bx + static_cast<int>(std::round(fx * r * 1.1f));
		tri[0].Y = by + static_cast<int>(std::round(fy * r * 1.1f));
		const float px = -fy;
		const float py = fx;
		tri[1].X = bx + static_cast<int>(std::round(px * r * 0.55f));
		tri[1].Y = by + static_cast<int>(std::round(py * r * 0.55f));
		tri[2].X = bx - static_cast<int>(std::round(px * r * 0.55f));
		tri[2].Y = by - static_cast<int>(std::round(py * r * 0.55f));
		canvas->Brush->Color = static_cast<TColor>(RGB(255, 120 + fade / 4, 0));
		canvas->Pen->Style = psClear;
		canvas->Polygon(tri, 2);
	}

	TPoint tri[3];
	tri[0].X = cx + static_cast<int>(std::round(fx * r * 1.15f));
	tri[0].Y = cy + static_cast<int>(std::round(fy * r * 1.15f));
	const float px = -fy;
	const float py = fx;
	tri[1].X = cx + static_cast<int>(std::round(px * r * 0.65f));
	tri[1].Y = cy + static_cast<int>(std::round(py * r * 0.65f));
	tri[2].X = cx - static_cast<int>(std::round(px * r * 0.65f));
	tri[2].Y = cy - static_cast<int>(std::round(py * r * 0.65f));

	canvas->Brush->Color = static_cast<TColor>(RGB(255, 220, 40));
	canvas->Pen->Color = static_cast<TColor>(RGB(255, 255, 200));
	canvas->Pen->Width = 2;
	canvas->Pen->Style = psSolid;
	canvas->Polygon(tri, 2);

	canvas->Pen->Width = 2;
	canvas->Pen->Color = static_cast<TColor>(RGB(255, 255, 255));
	canvas->Pen->Style = psSolid;
	TPoint spine[2];
	spine[0].X = cx;
	spine[0].Y = cy;
	spine[1].X = tri[0].X;
	spine[1].Y = tri[0].Y;
	canvas->Polyline(spine, 1);

	const int tipR = static_cast<int>(5 + 3 * PulseAt(position.X, position.Y, 1.4f));
	canvas->Brush->Color = static_cast<TColor>(RGB(255, 255, 220));
	canvas->Pen->Color = static_cast<TColor>(RGB(255, 180, 0));
	canvas->Pen->Width = 1;
	canvas->Pen->Style = psSolid;
	canvas->Ellipse(tri[0].X - tipR, tri[0].Y - tipR, tri[0].X + tipR, tri[0].Y + tipR);

	for (int s = 0; s < 4; ++s)
	{
		const float sa = aim + 3.14159265f + (s - 1.5f) * 0.35f;
		const int x1 = cx + static_cast<int>(std::cos(sa) * r * 0.3f);
		const int y1 = cy + static_cast<int>(std::sin(sa) * r * 0.3f);
		const int x2 = cx - static_cast<int>(std::cos(sa) * (r + 14.0f));
		const int y2 = cy - static_cast<int>(std::sin(sa) * (r + 14.0f));
		canvas->Pen->Color = static_cast<TColor>(RGB(255, 200, 80));
		canvas->Pen->Width = 2;
		canvas->Pen->Style = psSolid;
		TPoint seg[2];
		seg[0].X = x1;
		seg[0].Y = y1;
		seg[1].X = x2;
		seg[1].Y = y2;
		canvas->Polyline(seg, 1);
	}
}

void TThrowerEnemy::Draw(TCanvas *canvas, const TPointF &camera) const
{
	if (!canvas)
		return;

	const float screenX = position.X - camera.X;
	const float screenY = position.Y - camera.Y;
	const int cx = static_cast<int>(std::round(screenX));
	const int cy = static_cast<int>(std::round(screenY));

	const float t = AnimTimeSeconds();
	const float r = EnemyRadius;
	const int n = 10;
	TPoint blob[10];
	for (int i = 0; i < n; ++i)
	{
		const float a = i * 2.0f * 3.14159265f / n;
		const float w = 1.0f + 0.12f * std::sin(t * 3.0f + i * 0.7f + position.X * 0.02f);
		const float rad = r * w;
		blob[i].X = cx + static_cast<int>(std::round(rad * std::cos(a)));
		blob[i].Y = cy + static_cast<int>(std::round(rad * std::sin(a)));
	}

	canvas->Brush->Color = static_cast<TColor>(RGB(35, 55, 95));
	canvas->Pen->Color = static_cast<TColor>(RGB(80, 200, 255));
	canvas->Pen->Width = 3;
	canvas->Brush->Style = bsSolid;
	canvas->Polygon(blob, n - 1);

	TPoint inner[8];
	RotPoly(inner, 8, cx, cy, r * 0.62f, t * 0.8f);
	canvas->Brush->Color = static_cast<TColor>(RGB(60, 140, 90));
	canvas->Pen->Color = static_cast<TColor>(RGB(180, 255, 120));
	canvas->Pen->Width = 2;
	canvas->Polygon(inner, 7);

	const int tankL = cx - static_cast<int>(r * 0.45f);
	const int tankR = cx + static_cast<int>(r * 0.45f);
	const int tankT = cy - static_cast<int>(r * 0.25f);
	const int tankB = cy + static_cast<int>(r * 0.55f);
	canvas->Brush->Color = static_cast<TColor>(RGB(50, 200, 100));
	canvas->Pen->Color = static_cast<TColor>(RGB(220, 255, 200));
	canvas->Pen->Width = 2;
	canvas->Rectangle(tankL, tankT, tankR, tankB);

	canvas->Pen->Width = 2;
	canvas->Pen->Color = static_cast<TColor>(RGB(255, 255, 150));
	const int nozzleW = static_cast<int>(r * 0.2f);
	const int nozzleTipY = tankT - static_cast<int>(r * 0.35f + 4.0f * std::sin(t * 4.0f));
	TPoint nozzle[3];
	nozzle[0].X = cx - nozzleW; nozzle[0].Y = tankT;
	nozzle[1].X = cx + nozzleW; nozzle[1].Y = tankT;
	nozzle[2].X = cx; nozzle[2].Y = nozzleTipY;
	canvas->Brush->Color = static_cast<TColor>(RGB(120, 255, 160));
	canvas->Polygon(nozzle, 2);

	for (int b = 0; b < 5; ++b)
	{
		const float ba = t * 2.0f + b * 1.2f;
		const float br = r * (0.2f + 0.05f * std::sin(t * 5 + b));
		const int bx = cx + static_cast<int>(std::cos(ba) * r * 0.75f);
		const int by = cy + static_cast<int>(std::sin(ba) * r * 0.75f);
		canvas->Brush->Color = static_cast<TColor>(RGB(200, 255, 220));
		canvas->Pen->Color = static_cast<TColor>(RGB(100, 255, 180));
		canvas->Pen->Width = 1;
		const int pr = static_cast<int>(br);
		canvas->Ellipse(bx - pr, by - pr, bx + pr, by + pr);
	}

	DrawGlowRing(canvas, cx, cy, static_cast<int>(r + 6), static_cast<TColor>(RGB(100, 255, 200)), 1);
}

void TZigzagEnemy::Draw(TCanvas *canvas, const TPointF &camera) const
{
	if (!canvas)
		return;

	const float screenX = position.X - camera.X;
	const float screenY = position.Y - camera.Y;
	const int centerX = static_cast<int>(std::round(screenX));
	const int centerY = static_cast<int>(std::round(screenY));

	const float r = EnemyRadius * 1.1f;

	float alpha = 1.0f;
	if (isBlinking)
	{
		const int blinkPhase = static_cast<int>(blinkTimer * 20.0f) % 2;
		alpha = blinkPhase == 0 ? 0.3f : 1.0f;
	}

	const float t = AnimTimeSeconds();
	const float jitter = std::sin(t * 18.0f) * 2.5f;

	TPointF diamond[4];
	diamond[0] = PointF(centerX, centerY - r);
	diamond[1] = PointF(centerX + r, centerY);
	diamond[2] = PointF(centerX, centerY + r);
	diamond[3] = PointF(centerX - r, centerY);

	TPoint diamondPoints[4];
	for (int i = 0; i < 4; i++)
	{
		diamondPoints[i].X = static_cast<int>(diamond[i].X);
		diamondPoints[i].Y = static_cast<int>(diamond[i].Y);
	}

	const int baseR = static_cast<int>(200 * alpha);
	const int baseG = static_cast<int>(0 * alpha);
	const int baseB = static_cast<int>(255 * alpha);
	canvas->Brush->Color = static_cast<TColor>(RGB(baseR, baseG, baseB));
	canvas->Pen->Color = static_cast<TColor>(RGB(
		static_cast<int>(255 * alpha),
		static_cast<int>(150 * alpha),
		static_cast<int>(255 * alpha)));
	canvas->Pen->Width = 3;
	canvas->Brush->Style = bsSolid;
	canvas->Polygon(diamondPoints, 3);

	canvas->Pen->Width = 1;
	canvas->Pen->Color = static_cast<TColor>(RGB(
		static_cast<int>(220 * alpha),
		static_cast<int>(255 * alpha),
		static_cast<int>(255 * alpha)));
	for (int e = 0; e < 4; ++e)
	{
		const int j1 = static_cast<int>(jitter * std::sin(t * 10 + e));
		const int j2 = static_cast<int>(jitter * std::cos(t * 11 + e));
		canvas->MoveTo(diamondPoints[e].X + j1, diamondPoints[e].Y + j2);
		canvas->LineTo(diamondPoints[(e + 1) % 4].X - j1, diamondPoints[(e + 1) % 4].Y - j2);
	}

	const float innerR = r * 0.6f;
	TPointF innerDiamond[4];
	innerDiamond[0] = PointF(centerX, centerY - innerR);
	innerDiamond[1] = PointF(centerX + innerR, centerY);
	innerDiamond[2] = PointF(centerX, centerY + innerR);
	innerDiamond[3] = PointF(centerX - innerR, centerY);

	TPoint innerPoints[4];
	for (int i = 0; i < 4; i++)
	{
		innerPoints[i].X = static_cast<int>(innerDiamond[i].X);
		innerPoints[i].Y = static_cast<int>(innerDiamond[i].Y);
	}

	canvas->Brush->Color = static_cast<TColor>(RGB(
		static_cast<int>(255 * alpha),
		static_cast<int>(100 * alpha),
		static_cast<int>(255 * alpha)));
	canvas->Pen->Color = static_cast<TColor>(RGB(
		static_cast<int>(255 * alpha),
		static_cast<int>(200 * alpha),
		static_cast<int>(255 * alpha)));
	canvas->Pen->Width = 2;
	canvas->Polygon(innerPoints, 3);

	const float coreR = r * 0.25f;
	const int coreLeft = static_cast<int>(screenX - coreR);
	const int coreTop = static_cast<int>(screenY - coreR);
	const int coreRight = static_cast<int>(screenX + coreR);
	const int coreBottom = static_cast<int>(screenY + coreR);
	canvas->Brush->Color = static_cast<TColor>(RGB(
		static_cast<int>(255 * alpha),
		static_cast<int>(255 * alpha),
		static_cast<int>(255 * alpha)));
	canvas->Pen->Color = static_cast<TColor>(RGB(
		static_cast<int>(200 * alpha),
		static_cast<int>(0 * alpha),
		static_cast<int>(255 * alpha)));
	canvas->Pen->Width = 1;
	canvas->Ellipse(coreLeft, coreTop, coreRight, coreBottom);

	canvas->Pen->Width = 1;
	canvas->Pen->Color = static_cast<TColor>(RGB(
		static_cast<int>(180 * alpha),
		static_cast<int>(0 * alpha),
		static_cast<int>(255 * alpha)));
	for (int i = 0; i < 4; i++)
	{
		canvas->MoveTo(centerX, centerY);
		canvas->LineTo(diamondPoints[i].X, diamondPoints[i].Y);
	}

	if (isBlinking && blinkTimer < 0.3f)
	{
		const float lastScreenX = lastPosition.X - camera.X;
		const float lastScreenY = lastPosition.Y - camera.Y;
		const float fadeAlpha = blinkTimer / 0.3f;

		const int lastCenterX = static_cast<int>(std::round(lastScreenX));
		const int lastCenterY = static_cast<int>(std::round(lastScreenY));

		TPointF lastDiamond[4];
		lastDiamond[0] = PointF(lastCenterX, lastCenterY - r * fadeAlpha);
		lastDiamond[1] = PointF(lastCenterX + r * fadeAlpha, lastCenterY);
		lastDiamond[2] = PointF(lastCenterX, lastCenterY + r * fadeAlpha);
		lastDiamond[3] = PointF(lastCenterX - r * fadeAlpha, lastCenterY);

		TPoint lastPoints[4];
		for (int i = 0; i < 4; i++)
		{
			lastPoints[i].X = static_cast<int>(lastDiamond[i].X);
			lastPoints[i].Y = static_cast<int>(lastDiamond[i].Y);
		}

		canvas->Brush->Color = static_cast<TColor>(RGB(
			static_cast<int>(200 * fadeAlpha * 0.5f),
			static_cast<int>(0 * fadeAlpha * 0.5f),
			static_cast<int>(255 * fadeAlpha * 0.5f)));
		canvas->Pen->Color = static_cast<TColor>(RGB(
			static_cast<int>(255 * fadeAlpha * 0.5f),
			static_cast<int>(150 * fadeAlpha * 0.5f),
			static_cast<int>(255 * fadeAlpha * 0.5f)));
		canvas->Pen->Width = 2;
		canvas->Polygon(lastPoints, 3);
	}
}

void TKamikazeEnemy::Draw(TCanvas *canvas, const TPointF &camera) const
{
	if (!canvas)
		return;

	const float screenX = position.X - camera.X;
	const float screenY = position.Y - camera.Y;
	const int cx = static_cast<int>(std::round(screenX));
	const int cy = static_cast<int>(std::round(screenY));

	const float pulse = PulseAt(position.X, position.Y, 1.8f);
	const float r = EnemyRadius * (0.75f + 0.08f * pulse);
	const float t = AnimTimeSeconds();

	const int warnR = static_cast<int>(r + 10 + 8 * pulse);
	canvas->Brush->Style = bsClear;
	canvas->Pen->Width = 2;
	canvas->Pen->Color = static_cast<TColor>(RGB(255, 80, 0));
	canvas->Ellipse(cx - warnR, cy - warnR, cx + warnR, cy + warnR);
	canvas->Pen->Color = static_cast<TColor>(RGB(255, 40, 40));
	canvas->Ellipse(cx - warnR - 4, cy - warnR - 4, cx + warnR + 4, cy + warnR + 4);

	const int numPoints = 8;
	TPoint star[16];
	for (int i = 0; i < numPoints; i++)
	{
		const float angle = i * 2.0f * 3.14159265f / numPoints + t * 0.4f;

		star[i * 2].X = cx + static_cast<int>(std::round(r * std::cos(angle)));
		star[i * 2].Y = cy + static_cast<int>(std::round(r * std::sin(angle)));

		const float innerAngle = angle + 3.14159265f / numPoints;
		star[i * 2 + 1].X = cx + static_cast<int>(std::round(r * 0.48f * std::cos(innerAngle)));
		star[i * 2 + 1].Y = cy + static_cast<int>(std::round(r * 0.48f * std::sin(innerAngle)));
	}

	canvas->Brush->Color = static_cast<TColor>(RGB(255, 30, 0));
	canvas->Pen->Color = static_cast<TColor>(RGB(255, 200, 80));
	canvas->Pen->Width = 2;
	canvas->Brush->Style = bsSolid;
	canvas->Polygon(star, 15);

	const float innerR = r * 0.38f;
	const int innerLeft = static_cast<int>(std::round(screenX - innerR));
	const int innerTop = static_cast<int>(std::round(screenY - innerR));
	const int innerRight = static_cast<int>(std::round(screenX + innerR));
	const int innerBottom = static_cast<int>(std::round(screenY + innerR));
	canvas->Brush->Color = static_cast<TColor>(RGB(255, 200 + static_cast<int>(40 * pulse), 30));
	canvas->Pen->Color = static_cast<TColor>(RGB(255, 255, 200));
	canvas->Ellipse(innerLeft, innerTop, innerRight, innerBottom);

	const int fuse = static_cast<int>(6 + 5 * pulse);
	const int fx = cx + static_cast<int>(r * 0.9f);
	const int fy = cy - static_cast<int>(r * 0.9f);
	canvas->Brush->Color = static_cast<TColor>(RGB(60, 60, 60));
	canvas->Pen->Color = static_cast<TColor>(RGB(255, 100, 0));
	canvas->Ellipse(fx - fuse / 2, fy - fuse, fx + fuse / 2, fy + fuse / 3);

	canvas->Brush->Color = static_cast<TColor>(RGB(255, 255, 200));
	canvas->Pen->Style = psClear;
	const int sparkR = static_cast<int>(4 + 3 * std::sin(t * 20.0f));
	canvas->Ellipse(fx - sparkR, fy - fuse - sparkR * 2, fx + sparkR, fy - fuse);

	for (int sp = 0; sp < 6; ++sp)
	{
		const float sa = t * 8.0f + sp * 1.0f;
		const int sx = cx + static_cast<int>(std::cos(sa) * (r + 6.0f));
		const int sy = cy + static_cast<int>(std::sin(sa) * (r + 6.0f));
		canvas->Brush->Color = static_cast<TColor>(RGB(255, 180, 40));
		canvas->Ellipse(sx - 2, sy - 2, sx + 2, sy + 2);
	}
}

void TShootingEnemy::Draw(TCanvas *canvas, const TPointF &camera) const
{
	if (!canvas)
		return;

	const float screenX = position.X - camera.X;
	const float screenY = position.Y - camera.Y;
	const int cx = static_cast<int>(std::round(screenX));
	const int cy = static_cast<int>(std::round(screenY));

	const float r = EnemyRadius;
	const float t = AnimTimeSeconds();
	const float led = (static_cast<int>(t * 6.0f) % 2 == 0) ? 1.0f : 0.35f;

	TPoint base[6];
	RotPoly(base, 6, cx, cy + static_cast<int>(r * 0.12f), r * 0.92f, 0.0f);
	canvas->Brush->Color = static_cast<TColor>(RGB(25, 70, 45));
	canvas->Pen->Color = static_cast<TColor>(RGB(80, 255, 140));
	canvas->Pen->Width = 2;
	canvas->Brush->Style = bsSolid;
	canvas->Polygon(base, 5);

	const int turR = static_cast<int>(r * 0.55f);
	const int turTop = cy - static_cast<int>(r * 0.35f);
	canvas->Brush->Color = static_cast<TColor>(RGB(40, 120, 70));
	canvas->Pen->Color = static_cast<TColor>(RGB(150, 255, 190));
	canvas->Ellipse(cx - turR, turTop - turR, cx + turR, turTop + turR);

	const float aim = std::sin(t * 2.5f) * 0.15f;
	const float bx = std::sin(aim);
	const float by = -std::cos(aim);
	const int barrelW = static_cast<int>(r * 0.22f);
	const int barrelLen = static_cast<int>(r * (0.95f + 0.06f * std::sin(t * 4.0f)));
	const int bx0 = cx + static_cast<int>(bx * r * 0.15f);
	const int by0 = turTop + static_cast<int>(by * r * 0.15f);
	const int bTipX = bx0 + static_cast<int>(bx * barrelLen);
	const int bTipY = by0 + static_cast<int>(by * barrelLen);

	const float px = -by;
	const float py = bx;
	TPoint barrel[4];
	barrel[0].X = bx0 + static_cast<int>(px * barrelW);
	barrel[0].Y = by0 + static_cast<int>(py * barrelW);
	barrel[1].X = bx0 - static_cast<int>(px * barrelW);
	barrel[1].Y = by0 - static_cast<int>(py * barrelW);
	barrel[2].X = bTipX - static_cast<int>(px * (barrelW * 0.65f));
	barrel[2].Y = bTipY - static_cast<int>(py * (barrelW * 0.65f));
	barrel[3].X = bTipX + static_cast<int>(px * (barrelW * 0.65f));
	barrel[3].Y = bTipY + static_cast<int>(py * (barrelW * 0.65f));

	canvas->Brush->Color = static_cast<TColor>(RGB(55, 160, 90));
	canvas->Pen->Color = static_cast<TColor>(RGB(200, 255, 220));
	canvas->Pen->Width = 2;
	canvas->Polygon(barrel, 3);

	const int muz = static_cast<int>(6 + 2 * std::sin(t * 12.0f));
	canvas->Brush->Color = static_cast<TColor>(RGB(255, 255, 180));
	canvas->Pen->Color = static_cast<TColor>(RGB(255, 220, 100));
	canvas->Pen->Width = 1;
	canvas->Ellipse(bTipX - muz / 2, bTipY - muz / 2, bTipX + muz / 2, bTipY + muz / 2);

	const int bracket = static_cast<int>(r * 1.05f);
	canvas->Brush->Style = bsClear;
	canvas->Pen->Width = 1;
	canvas->Pen->Color = static_cast<TColor>(RGB(100, 255, 160));
	canvas->MoveTo(cx - bracket, cy + bracket);
	canvas->LineTo(cx - bracket, cy - bracket);
	canvas->LineTo(cx - bracket + static_cast<int>(r * 0.25f), cy - bracket);
	canvas->MoveTo(cx + bracket, cy + bracket);
	canvas->LineTo(cx + bracket, cy - bracket);
	canvas->LineTo(cx + bracket - static_cast<int>(r * 0.25f), cy - bracket);

	const int lx = cx + static_cast<int>(r * 0.65f);
	const int ly = cy + static_cast<int>(r * 0.35f);
	canvas->Brush->Color = static_cast<TColor>(RGB(
		static_cast<int>(255 * led),
		static_cast<int>(40 + 200 * led),
		static_cast<int>(60 * led)));
	canvas->Pen->Style = psSolid;
	canvas->Pen->Color = static_cast<TColor>(RGB(150, 255, 180));
	canvas->Ellipse(lx - 4, ly - 4, lx + 4, ly + 4);

	DrawGlowRing(canvas, cx, cy, static_cast<int>(r + 4), static_cast<TColor>(RGB(60, 255, 120)), 1);
}
