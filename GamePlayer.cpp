#include <vcl.h>
#pragma hdrstop
//---------------------------------------------------------------------------

#include "GamePlayer.h"
#include "GameConstants.h"
#include <algorithm>
#include <cmath>
//---------------------------------------------------------------------------

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
	  pendingUpgradeCount(0),
	  primaryFireTimer(0.0f),
	  altFireTimer(0.0f),
	  inputUp(false),
	  inputDown(false),
	  inputLeft(false),
	  inputRight(false),
	  isShooting(false)
{
}

//---------------------------------------------------------------------------
void TGamePlayer::ApplySpeedMultiplier(float multiplier)
{
	speed = GetBaseSpeed() * multiplier;
}

//---------------------------------------------------------------------------
void TGamePlayer::ApplyMaxHealthBonus(int bonus)
{
	const int oldMaxHealth = maxHealth;
	maxHealth = 100 + bonus; // базовое здоровье + бонус
	// увеличиваем текущее здоровье пропорционально
	if (oldMaxHealth > 0)
	{
		const float healthRatio = static_cast<float>(health) / static_cast<float>(oldMaxHealth);
		health = static_cast<int>(std::round(maxHealth * healthRatio));
	}
	else
	{
		health = maxHealth; // если старый максимум был 0, просто устанавливаем новый
	}
}
//---------------------------------------------------------------------------
void TGamePlayer::SetPosition(float px, float py)
{
	x = px;
	y = py;
}
//---------------------------------------------------------------------------
void TGamePlayer::Update(float deltaTime, const TRectF &bounds)
{
	// уменьшаем таймер неуязвимости
	if (hitInvulnTimer > 0.0f)
	{
		hitInvulnTimer -= deltaTime;
		if (hitInvulnTimer < 0.0f)
			hitInvulnTimer = 0.0f;
	}
	
	// уменьшаем таймер невосприимчивости при level up
	if (levelUpInvulnTimer > 0.0f)
	{
		levelUpInvulnTimer -= deltaTime;
		if (levelUpInvulnTimer < 0.0f)
			levelUpInvulnTimer = 0.0f;
	}

	primaryFireTimer = std::max(0.0f, primaryFireTimer - deltaTime);
	altFireTimer = std::max(0.0f, altFireTimer - deltaTime);

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

	const float minX = bounds.Left + PlayerRadius;
	const float maxX = bounds.Right - PlayerRadius;
	const float minY = bounds.Top + PlayerRadius;
	const float maxY = bounds.Bottom - PlayerRadius;

	x = std::clamp(x, minX, maxX);
	y = std::clamp(y, minY, maxY);
}
//---------------------------------------------------------------------------
void TGamePlayer::SetFacingDirection(const TPointF &dir)
{
	float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
	if (len > 0.0001f)
		facingDirection = PointF(dir.X / len, dir.Y / len);
}
//---------------------------------------------------------------------------
void TGamePlayer::Draw(TCanvas *canvas, const TPointF &camera)
{
	if (!canvas)
		return;

	// мигание, если игрок под эффектом неуязвимости
	const bool flashing = (hitInvulnTimer > 0.0f) &&
		((static_cast<int>(hitInvulnTimer * 10.0f) % 2) == 0);

	const TColor outer = flashing
		? static_cast<TColor>(RGB(255, 255, 255))
		: static_cast<TColor>(RGB(0, 200, 255));
	const TColor inner = flashing
		? static_cast<TColor>(RGB(255, 120, 120))
		: static_cast<TColor>(RGB(0, 255, 160));

	const float screenX = x - camera.X;
	const float screenY = y - camera.Y;
	const int centerX = static_cast<int>(std::round(screenX));
	const int centerY = static_cast<int>(std::round(screenY));

	canvas->Brush->Style = bsSolid;
	canvas->Pen->Style = psSolid;
	canvas->Pen->Width = 3;

	// Рисуем игрока в виде шестиугольника (гексагона)
	const float r = PlayerRadius;
	const int numPoints = 6;
	TPoint points[6];
	
	for (int i = 0; i < numPoints; i++)
	{
		const float angle = (i * 2.0f * 3.14159265f / numPoints) - (3.14159265f / 2.0f); // поворачиваем на 90 градусов
		points[i].X = centerX + static_cast<int>(std::round(r * std::cos(angle)));
		points[i].Y = centerY + static_cast<int>(std::round(r * std::sin(angle)));
	}

	// Внешний контур
	canvas->Brush->Color = inner;
	canvas->Pen->Color = outer;
	canvas->Polygon(points, numPoints - 1);

	// Внутренний круг для глубины
	const float innerR = r * 0.6f;
	const int innerLeft = static_cast<int>(std::round(screenX - innerR));
	const int innerTop = static_cast<int>(std::round(screenY - innerR));
	const int innerRight = static_cast<int>(std::round(screenX + innerR));
	const int innerBottom = static_cast<int>(std::round(screenY + innerR));
	canvas->Pen->Width = 2;
	canvas->Pen->Color = static_cast<TColor>(RGB(
		static_cast<int>((outer >> 16) & 0xFF) * 0.7f,
		static_cast<int>((outer >> 8) & 0xFF) * 0.7f,
		static_cast<int>(outer & 0xFF) * 0.7f));
	canvas->Ellipse(innerLeft, innerTop, innerRight, innerBottom);

	// Направление движения - стрелка
	canvas->Pen->Width = 2;
	canvas->Pen->Color = outer;
	const int arrowLength = static_cast<int>(r * 0.8f);
	const int arrowX = centerX + static_cast<int>(std::round(facingDirection.X * arrowLength));
	const int arrowY = centerY + static_cast<int>(std::round(facingDirection.Y * arrowLength));
	
	// Линия направления
	canvas->MoveTo(centerX, centerY);
	canvas->LineTo(arrowX, arrowY);
	
	// Наконечник стрелки
	const float arrowAngle = std::atan2(facingDirection.Y, facingDirection.X);
	const float arrowTipSize = 6.0f;
	TPoint arrowTip[3];
	arrowTip[0].X = arrowX;
	arrowTip[0].Y = arrowY;
	arrowTip[1].X = arrowX - static_cast<int>(std::round(arrowTipSize * std::cos(arrowAngle - 2.5f)));
	arrowTip[1].Y = arrowY - static_cast<int>(std::round(arrowTipSize * std::sin(arrowAngle - 2.5f)));
	arrowTip[2].X = arrowX - static_cast<int>(std::round(arrowTipSize * std::cos(arrowAngle + 2.5f)));
	arrowTip[2].Y = arrowY - static_cast<int>(std::round(arrowTipSize * std::sin(arrowAngle + 2.5f)));
	canvas->Brush->Color = outer;
	canvas->Polygon(arrowTip, 2);
}
//---------------------------------------------------------------------------
TPointF TGamePlayer::GetPosition() const
{
	return PointF(x, y);
}
//---------------------------------------------------------------------------
void TGamePlayer::TakeDamage(int amount)
{
	if (amount <= 0)
		return;

	// если неуязвимость активна — урон игнорируется
	if (hitInvulnTimer > 0.0f)
		return;

	health -= amount;
	if (health < 0)
		health = 0;

	// запускаем неуязвимость после получения урона
	hitInvulnTimer = PlayerHitInvulnTime;
}
//---------------------------------------------------------------------------
void TGamePlayer::Heal(int amount)
{
	if (amount <= 0)
		return;

	health += amount;
	if (health > maxHealth)
		health = maxHealth;
}
//---------------------------------------------------------------------------
bool TGamePlayer::IsAlive() const
{
	return health > 0;
}
//---------------------------------------------------------------------------
float TGamePlayer::GetHealthRatio() const
{
	if (maxHealth <= 0)
		return 0.0f;

	return static_cast<float>(health) / static_cast<float>(maxHealth);
}
//---------------------------------------------------------------------------
void TGamePlayer::AddExperience(int amount)
{
	if (amount <= 0)
		return;

	experience += amount;
}
//---------------------------------------------------------------------------
float TGamePlayer::GetExperienceRatio() const
{
	if (experienceToNextLevel <= 0)
		return 1.0f;

	return std::clamp(static_cast<float>(experience) / static_cast<float>(experienceToNextLevel), 0.0f, 1.0f);
}
//---------------------------------------------------------------------------
bool TGamePlayer::CheckLevelUp()
{
	if (experience >= experienceToNextLevel)
	{
		// прокачка!
		experience -= experienceToNextLevel;
		level++;

		// увеличиваем требуемый опыт для следующего уровня (прогрессия)
		experienceToNextLevel = static_cast<int>(100 + (level - 1) * 50 + (level - 1) * (level - 1) * 10);

		// при прокачке восстанавливаем немного здоровья
		Heal(20);
		
		// невосприимчивость к урону на 5 секунд при получении уровня
		levelUpInvulnTimer = 5.0f;

		return true;
	}
	return false;
}
//---------------------------------------------------------------------------


