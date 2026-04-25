#include <vcl.h>
#pragma hdrstop

#include "GameWorld.h"
#include <algorithm>
#include <cmath>

using namespace NeonGame;

namespace
{
	void DrawWorldBackground(TCanvas *canvas, const TPointF &camera, int canvasWidth, int canvasHeight, float worldWidth, float worldHeight)
	{
		canvas->Brush->Color = static_cast<TColor>(RGB(5, 5, 15));
		canvas->Brush->Style = bsSolid;
		canvas->FillRect(TRect(0, 0, canvasWidth, canvasHeight));

		const int platformSpacing = 320;
		canvas->Pen->Color = static_cast<TColor>(RGB(15, 25, 50));
		canvas->Pen->Width = 2;
		canvas->Brush->Color = static_cast<TColor>(RGB(10, 15, 30));
		canvas->Brush->Style = bsSolid;

		for (float px = 0.0f; px <= worldWidth; px += platformSpacing)
		{
			for (float py = 0.0f; py <= worldHeight; py += platformSpacing)
			{
				const float offsetX = (static_cast<int>(px / platformSpacing) % 3) * 40.0f;
				const float offsetY = (static_cast<int>(py / platformSpacing) % 3) * 40.0f;

				const float sx = px + offsetX - camera.X;
				const float sy = py + offsetY - camera.Y;

				if (sx > -100.0f && sx < canvasWidth + 100.0f && sy > -100.0f && sy < canvasHeight + 100.0f)
				{
					const float size = 60.0f;
					const int left = static_cast<int>(std::round(sx - size * 0.5f));
					const int top = static_cast<int>(std::round(sy - size * 0.5f));
					const int right = static_cast<int>(std::round(sx + size * 0.5f));
					const int bottom = static_cast<int>(std::round(sy + size * 0.5f));

					canvas->Rectangle(left, top, right, bottom);

					canvas->Pen->Color = static_cast<TColor>(RGB(20, 40, 80));
					canvas->Rectangle(left - 1, top - 1, right + 1, bottom + 1);
					canvas->Pen->Color = static_cast<TColor>(RGB(15, 25, 50));
				}
			}
		}

		const int gridStep = 160;
		canvas->Pen->Color = static_cast<TColor>(RGB(8, 12, 25));
		canvas->Pen->Width = 1;

		for (float wx = 0.0f; wx <= worldWidth; wx += gridStep)
		{
			const float sx = wx - camera.X;
			if (sx < 0.0f || sx > canvasWidth)
				continue;
			canvas->MoveTo(static_cast<int>(std::round(sx)), 0);
			canvas->LineTo(static_cast<int>(std::round(sx)), canvasHeight);
		}

		for (float wy = 0.0f; wy <= worldHeight; wy += gridStep)
		{
			const float sy = wy - camera.Y;
			if (sy < 0.0f || sy > canvasHeight)
				continue;
			canvas->MoveTo(0, static_cast<int>(std::round(sy)));
			canvas->LineTo(canvasWidth, static_cast<int>(std::round(sy)));
		}

		canvas->Pen->Color = static_cast<TColor>(RGB(5, 8, 15));
		canvas->Pen->Width = 1;

		for (float wx = gridStep * 0.5f; wx <= worldWidth; wx += gridStep)
		{
			const float sx = wx - camera.X;
			if (sx < 0.0f || sx > canvasWidth)
				continue;
			canvas->MoveTo(static_cast<int>(std::round(sx)), 0);
			canvas->LineTo(static_cast<int>(std::round(sx)), canvasHeight);
		}

		for (float wy = gridStep * 0.5f; wy <= worldHeight; wy += gridStep)
		{
			const float sy = wy - camera.Y;
			if (sy < 0.0f || sy > canvasHeight)
				continue;
			canvas->MoveTo(0, static_cast<int>(std::round(sy)));
			canvas->LineTo(canvasWidth, static_cast<int>(std::round(sy)));
		}

		canvas->Pen->Color = static_cast<TColor>(RGB(15, 30, 60));
		canvas->Pen->Width = 1;

		const int lineSpacing = 480;
		for (float lx = 0.0f; lx <= worldWidth; lx += lineSpacing)
		{
			const float sx = lx - camera.X;
			if (sx >= -10.0f && sx <= canvasWidth + 10.0f)
			{
				for (int y = 0; y < canvasHeight; y += 20)
				{
					canvas->MoveTo(static_cast<int>(std::round(sx)), y);
					canvas->LineTo(static_cast<int>(std::round(sx)), y + 10);
				}
			}
		}

		for (float ly = 0.0f; ly <= worldHeight; ly += lineSpacing)
		{
			const float sy = ly - camera.Y;
			if (sy >= -10.0f && sy <= canvasHeight + 10.0f)
			{
				for (int x = 0; x < canvasWidth; x += 20)
				{
					canvas->MoveTo(x, static_cast<int>(std::round(sy)));
					canvas->LineTo(x + 10, static_cast<int>(std::round(sy)));
				}
			}
		}

		canvas->Brush->Color = static_cast<TColor>(RGB(12, 20, 40));
		canvas->Brush->Style = bsSolid;
		canvas->Pen->Color = static_cast<TColor>(RGB(18, 30, 55));
		canvas->Pen->Width = 1;

		for (float wx = 0.0f; wx <= worldWidth; wx += gridStep)
		{
			for (float wy = 0.0f; wy <= worldHeight; wy += gridStep)
			{
				const float sx = wx - camera.X;
				const float sy = wy - camera.Y;

				if (sx >= -5.0f && sx <= canvasWidth + 5.0f && sy >= -5.0f && sy <= canvasHeight + 5.0f)
				{
					const int pointSize = 2;
					const int left = static_cast<int>(std::round(sx - pointSize));
					const int top = static_cast<int>(std::round(sy - pointSize));
					const int right = static_cast<int>(std::round(sx + pointSize));
					const int bottom = static_cast<int>(std::round(sy + pointSize));
					canvas->Ellipse(left, top, right, bottom);
				}
			}
		}
	}

	void DrawAcidPools(TCanvas *canvas, const TPointF &camera, const std::vector<TAcidPool> &pools)
	{
		for (const auto &pool : pools)
		{
			if (pool.IsAlive())
			{
				pool.Draw(canvas, camera);
			}
		}
	}

	void DrawEnemies(TCanvas *canvas, const TPointF &camera, const std::vector<std::unique_ptr<TEnemy>> &enemies)
	{
		for (const auto &enemy : enemies)
		{
			if (enemy)
			{
				enemy->Draw(canvas, camera);
			}
		}
	}

	void DrawExperienceOrbs(TCanvas *canvas, const TPointF &camera, const std::vector<TExperienceOrb> &orbs)
	{
		for (const auto &orb : orbs)
		{
			orb.Draw(canvas, camera);
		}
	}

	void DrawPlayerBullets(TCanvas *canvas, const TPointF &camera, const std::vector<TBullet> &bullets)
	{
		for (const auto &b : bullets)
		{
			b.Draw(canvas, camera);
		}
	}

	void DrawEnemyBullets(TCanvas *canvas, const TPointF &camera, const std::vector<TBullet> &bullets)
	{
		for (const auto &b : bullets)
		{
			if (!b.IsAlive() || b.IsUsed())
				continue;

			const float screenX = b.GetPosition().X - camera.X;
			const float screenY = b.GetPosition().Y - camera.Y;
			const float r = b.GetRadius();
			const int left = static_cast<int>(std::round(screenX - r));
			const int top = static_cast<int>(std::round(screenY - r));
			const int right = static_cast<int>(std::round(screenX + r));
			const int bottom = static_cast<int>(std::round(screenY + r));

			canvas->Brush->Color = static_cast<TColor>(RGB(255, 100, 100));
			canvas->Pen->Color = static_cast<TColor>(RGB(255, 150, 150));
			canvas->Pen->Width = 1;
			canvas->Ellipse(left, top, right, bottom);
		}
	}

	void DrawReplicatedBullets(TCanvas *canvas, const TPointF &camera, const std::vector<NeonGame::TBulletNetPacket> &bullets)
	{
		if (!canvas)
			return;

		for (const auto &b : bullets)
		{
			const float screenX = b.PositionX - camera.X;
			const float screenY = b.PositionY - camera.Y;
			const float r = 4.0f;
			const int left = static_cast<int>(std::round(screenX - r));
			const int top = static_cast<int>(std::round(screenY - r));
			const int right = static_cast<int>(std::round(screenX + r));
			const int bottom = static_cast<int>(std::round(screenY + r));

			if (b.IsPlayerBullet)
			{
				
				const float trailX = screenX - 10.0f;
				const float trailY = screenY;
				canvas->Pen->Color = static_cast<TColor>(RGB(255, 120, 220));
				canvas->Pen->Width = 1;
				canvas->MoveTo(static_cast<int>(std::round(trailX)), static_cast<int>(std::round(trailY)));
				canvas->LineTo(static_cast<int>(std::round(screenX)), static_cast<int>(std::round(screenY)));

				canvas->Brush->Color = static_cast<TColor>(RGB(255, 80, 200));
				canvas->Pen->Color = static_cast<TColor>(RGB(255, 160, 255));
			}
			else
			{
				canvas->Brush->Color = static_cast<TColor>(RGB(255, 100, 100));
				canvas->Pen->Color = static_cast<TColor>(RGB(255, 150, 150));
			}
			canvas->Pen->Width = 1;
			canvas->Ellipse(left, top, right, bottom);
		}
	}

	void DrawThrownProjectiles(TCanvas *canvas, const TPointF &camera, const std::vector<TThrownProjectile> &projectiles)
	{
		for (const auto &proj : projectiles)
		{
			if (proj.IsAlive() && !proj.HasLanded())
			{
				proj.Draw(canvas, camera);
			}
		}
	}

	void DrawBoss(TCanvas *canvas, const TPointF &camera, const std::unique_ptr<TBossEnemy> &boss)
	{
		if (boss && boss->IsAlive())
		{
			boss->Draw(canvas, camera);
		}
	}

	void DrawPlayers(TCanvas *canvas, const TPointF &camera, const std::vector<std::unique_ptr<TGamePlayer>> &players)
	{
		for (const auto &player : players)
		{
			if (player)
			{
				player->Draw(canvas, camera);
			}
		}
	}
}

void TGameWorld::RenderScene(TCanvas *canvas)
{
	if (!canvas)
		return;

	TPointF camera = Camera.GetBasePosition();
	const TPointF shakeOffset = Camera.GetShakeOffset();
	camera.X += shakeOffset.X;
	camera.Y += shakeOffset.Y;

	const int canvasWidth = canvas->ClipRect.Width();
	const int canvasHeight = canvas->ClipRect.Height();

	DrawWorldBackground(canvas, camera, canvasWidth, canvasHeight, WorldWidth, WorldHeight);

	DrawAcidPools(canvas, camera, AcidPools);
	DrawEnemies(canvas, camera, Enemies);
	DrawExperienceOrbs(canvas, camera, ExperienceOrbs);
	DrawPlayerBullets(canvas, camera, Bullets);
	DrawEnemyBullets(canvas, camera, EnemyBullets);
	if (IsNetworkGame && !IsServer && !ReplicatedBullets.empty())
		DrawReplicatedBullets(canvas, camera, ReplicatedBullets);
	DrawThrownProjectiles(canvas, camera, ThrownProjectiles);
	DrawBoss(canvas, camera, Boss);
	DrawPlayers(canvas, camera, Players);
}

