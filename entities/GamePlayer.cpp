#include <vcl.h>
#pragma hdrstop

#include "GamePlayer.h"
#include "GameConstants.h"
#include <Windows.h>
#include <algorithm>
#include <cmath>

using namespace NeonGame;

TGamePlayer::TGamePlayer()
	: x(0.0f),
	  y(0.0f),
	  speed(260.0f),
	  health(100),
	  maxHealth(100),
	  facingDirection(PointF(0.0f, -1.0f)),
	  hitInvulnTimer(0.0f),
	  levelUpInvulnTimer(0.0f),
	  level(1),
	  experience(0),
	  experienceToNextLevel(100),
	  inputUp(false),
	  inputDown(false),
	  inputLeft(false),
	  inputRight(false),
	  isShooting(false),
	  recoilVelX(0.0f),
	  recoilVelY(0.0f)
{
}

void TGamePlayer::ApplySpeedMultiplier(float multiplier)
{
	speed = GetBaseSpeed() * multiplier;
}


void TGamePlayer::ApplyMaxHealthBonus(int bonus)
{
	const int oldMaxHealth = maxHealth;
	maxHealth = 100 + bonus; 
	
	if (oldMaxHealth > 0)
	{
		const float healthRatio = static_cast<float>(health) / static_cast<float>(oldMaxHealth);
		health = static_cast<int>(std::round(maxHealth * healthRatio));
	}
	else
	{
		health = maxHealth; 
	}
}

void TGamePlayer::SetPosition(float px, float py)
{
	x = px;
	y = py;
}

void TGamePlayer::Update(float deltaTime, const TRectF &bounds)
{
	if (!IsAlive())
		return;

	if (hitInvulnTimer > 0.0f)
	{
		hitInvulnTimer -= deltaTime;
		if (hitInvulnTimer < 0.0f)
			hitInvulnTimer = 0.0f;
	}
	
	if (levelUpInvulnTimer > 0.0f)
	{
		levelUpInvulnTimer -= deltaTime;
		if (levelUpInvulnTimer < 0.0f)
			levelUpInvulnTimer = 0.0f;
	}

	float dx = 0.0f;
	float dy = 0.0f;

	if (inputUp)
		dy -= 1.0f;
	if (inputDown)
		dy += 1.0f;
	if (inputLeft)
		dx -= 1.0f;
	if (inputRight)
		dx += 1.0f;

	if (dx != 0.0f || dy != 0.0f)
	{
		const float length = std::sqrt(dx * dx + dy * dy);
		dx /= length;
		dy /= length;
		facingDirection = PointF(dx, dy);
	}

	x += dx * speed * deltaTime;
	y += dy * speed * deltaTime;

	x += recoilVelX * deltaTime;
	y += recoilVelY * deltaTime;
	const float damp = std::exp(-PlayerRecoilDamping * deltaTime);
	recoilVelX *= damp;
	recoilVelY *= damp;

	const float minX = bounds.Left + PlayerRadius;
	const float maxX = bounds.Right - PlayerRadius;
	const float minY = bounds.Top + PlayerRadius;
	const float maxY = bounds.Bottom - PlayerRadius;

	x = std::clamp(x, minX, maxX);
	y = std::clamp(y, minY, maxY);
}

void TGamePlayer::SetFacingDirection(const TPointF &dir)
{
	float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
	if (len > 0.0001f)
		facingDirection = PointF(dir.X / len, dir.Y / len);
}

void TGamePlayer::Draw(TCanvas *canvas, const TPointF &camera)
{
	if (!canvas)
		return;

	const bool flashing = (hitInvulnTimer > 0.0f) &&
		((static_cast<int>(hitInvulnTimer * 10.0f) % 2) == 0);

	const TColor outer = flashing
		? static_cast<TColor>(RGB(255, 255, 255))
		: static_cast<TColor>(RGB(40, 220, 255));
	const TColor inner = flashing
		? static_cast<TColor>(RGB(255, 120, 120))
		: static_cast<TColor>(RGB(30, 255, 200));
	const TColor glowPen = flashing
		? static_cast<TColor>(RGB(255, 200, 200))
		: static_cast<TColor>(RGB(120, 250, 255));

	const float screenX = x - camera.X;
	const float screenY = y - camera.Y;
	const int centerX = static_cast<int>(std::round(screenX));
	const int centerY = static_cast<int>(std::round(screenY));

	const float t = static_cast<float>(GetTickCount()) * 0.001f;
	const float pulse = 0.5f + 0.5f * std::sin(t * 3.2f + x * 0.02f + y * 0.017f);
	const float r = PlayerRadius * (0.94f + 0.06f * pulse);

	const float fx = facingDirection.X;
	const float fy = facingDirection.Y;
	const float px = -fy;
	const float py = fx;

	canvas->Brush->Style = bsSolid;
	canvas->Pen->Style = psSolid;

	canvas->Brush->Style = bsClear;
	canvas->Pen->Color = glowPen;
	canvas->Pen->Width = 2;
	const int glowR = static_cast<int>(std::round(r + 5.0f + 2.0f * pulse));
	canvas->Ellipse(centerX - glowR, centerY - glowR, centerX + glowR, centerY + glowR);

	canvas->Brush->Style = bsSolid;
	canvas->Pen->Width = 3;

	TPoint hull[4];
	const float noseR = r * 1.08f;
	const float wingR = r * 0.58f;
	const float tailR = r * 0.32f;
	hull[0].X = centerX + static_cast<int>(std::round(fx * noseR));
	hull[0].Y = centerY + static_cast<int>(std::round(fy * noseR));
	hull[1].X = centerX + static_cast<int>(std::round(-fx * wingR + px * wingR));
	hull[1].Y = centerY + static_cast<int>(std::round(-fy * wingR + py * wingR));
	hull[2].X = centerX + static_cast<int>(std::round(-fx * tailR));
	hull[2].Y = centerY + static_cast<int>(std::round(-fy * tailR));
	hull[3].X = centerX + static_cast<int>(std::round(-fx * wingR - px * wingR));
	hull[3].Y = centerY + static_cast<int>(std::round(-fy * wingR - py * wingR));

	canvas->Brush->Color = inner;
	canvas->Pen->Color = outer;
	canvas->Polygon(hull, 3);

	TPoint innerHull[4];
	const float in = 0.48f;
	innerHull[0].X = centerX + static_cast<int>(std::round(fx * noseR * in));
	innerHull[0].Y = centerY + static_cast<int>(std::round(fy * noseR * in));
	innerHull[1].X = centerX + static_cast<int>(std::round((-fx * wingR + px * wingR) * in));
	innerHull[1].Y = centerY + static_cast<int>(std::round((-fy * wingR + py * wingR) * in));
	innerHull[2].X = centerX + static_cast<int>(std::round(-fx * tailR * in * 0.6f));
	innerHull[2].Y = centerY + static_cast<int>(std::round(-fy * tailR * in * 0.6f));
	innerHull[3].X = centerX + static_cast<int>(std::round((-fx * wingR - px * wingR) * in));
	innerHull[3].Y = centerY + static_cast<int>(std::round((-fy * wingR - py * wingR) * in));

	canvas->Pen->Width = 2;
	canvas->Pen->Color = static_cast<TColor>(RGB(
		static_cast<int>((outer >> 16) & 0xFF) * 0.65f,
		static_cast<int>((outer >> 8) & 0xFF) * 0.65f,
		static_cast<int>(outer & 0xFF) * 0.65f));
	canvas->Brush->Color = static_cast<TColor>(RGB(
		static_cast<int>((inner >> 16) & 0xFF) * 0.85f,
		static_cast<int>((inner >> 8) & 0xFF) * 0.85f,
		static_cast<int>(inner & 0xFF) * 0.85f));
	canvas->Polygon(innerHull, 3);

	const float coreR = r * 0.22f * (0.9f + 0.1f * pulse);
	const int c0 = static_cast<int>(std::round(screenX - fx * coreR * 0.35f - coreR));
	const int c1 = static_cast<int>(std::round(screenY - fy * coreR * 0.35f - coreR));
	const int c2 = static_cast<int>(std::round(screenX - fx * coreR * 0.35f + coreR));
	const int c3 = static_cast<int>(std::round(screenY - fy * coreR * 0.35f + coreR));
	canvas->Pen->Width = 1;
	canvas->Pen->Color = outer;
	canvas->Brush->Color = static_cast<TColor>(RGB(240, 255, 255));
	canvas->Ellipse(c0, c1, c2, c3);
}

TPointF TGamePlayer::GetPosition() const
{
	return PointF(x, y);
}

void TGamePlayer::TakeDamage(int amount)
{
	if (amount <= 0)
		return;

	if (hitInvulnTimer > 0.0f)
		return;

	health -= amount;
	if (health < 0)
		health = 0;

	hitInvulnTimer = PlayerHitInvulnTime;
}

void TGamePlayer::Heal(int amount)
{
	if (amount <= 0)
		return;

	health += amount;
	if (health > maxHealth)
		health = maxHealth;
}

void TGamePlayer::ApplyShotgunRecoil(float aimDirX, float aimDirY)
{
	float len = std::sqrt(aimDirX * aimDirX + aimDirY * aimDirY);
	if (len < 1.0e-4f)
		return;
	aimDirX /= len;
	aimDirY /= len;
	const float kick = ShotgunRecoilBaseSpeed;
	recoilVelX -= aimDirX * kick;
	recoilVelY -= aimDirY * kick;
}

void TGamePlayer::NetworkRespawn(float centerX, float centerY, int resumeHealth)
{
	recoilVelX = 0.0f;
	recoilVelY = 0.0f;
	SetPosition(centerX, centerY);
	if (resumeHealth < 1)
		resumeHealth = 1;
	health = std::min(resumeHealth, maxHealth);
	hitInvulnTimer = PlayerHitInvulnTime;
	levelUpInvulnTimer = 2.0f;
}

void TGamePlayer::SyncHealthFromNetwork(int newHealth, int newMaxHealth)
{
	if (newMaxHealth > 0)
		maxHealth = newMaxHealth;
	health = std::clamp(newHealth, 0, std::max(1, maxHealth));
}

bool TGamePlayer::IsAlive() const
{
	return health > 0;
}

float TGamePlayer::GetHealthRatio() const
{
	if (maxHealth <= 0)
		return 0.0f;

	return static_cast<float>(health) / static_cast<float>(maxHealth);
}

void TGamePlayer::AddExperience(int amount)
{
	if (amount <= 0)
		return;

	experience += amount;
}

void TGamePlayer::ApplyAuthorityProgress(int authLevel, int authExperience)
{
	level = std::max(1, authLevel);
	experience = std::max(0, authExperience);
	const int L = level - 1;
	experienceToNextLevel = static_cast<int>(100 + L * 50 + L * L * 10);
}

float TGamePlayer::GetExperienceRatio() const
{
	if (experienceToNextLevel <= 0)
		return 1.0f;

	return std::clamp(static_cast<float>(experience) / static_cast<float>(experienceToNextLevel), 0.0f, 1.0f);
}

bool TGamePlayer::CheckLevelUp()
{
	if (experience >= experienceToNextLevel)
	{
		experience -= experienceToNextLevel;
		level++;

		experienceToNextLevel = static_cast<int>(100 + (level - 1) * 50 + (level - 1) * (level - 1) * 10);

		Heal(20);
		
		levelUpInvulnTimer = 5.0f;

		return true;
	}
	return false;
}