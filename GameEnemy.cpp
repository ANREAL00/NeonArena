#include <vcl.h>
#pragma hdrstop
//---------------------------------------------------------------------------

#include "GameEnemy.h"
#include "GameConstants.h"
#include "GameProjectile.h"
#include <cmath>
#include <vector>
#include <algorithm>
//---------------------------------------------------------------------------

using namespace NeonGame;

void TEnemy::ApplyDamage(int dmg)
{
	health -= dmg;
}
//---------------------------------------------------------------------------
void TEnemy::ApplyScaling(float healthMultiplier, float speedMultiplier)
{
	health = static_cast<int>(baseHealth * healthMultiplier);
	scaledBaseSpeed = baseSpeed * speedMultiplier;
	speed = scaledBaseSpeed;
}
//---------------------------------------------------------------------------
void TEnemy::ApplyTemporarySpeedMultiplier(float multiplier)
{
	// применяем временный множитель к базовой скорости (после масштабирования волной)
	speed = scaledBaseSpeed * multiplier;
}
//---------------------------------------------------------------------------
TBasicEnemy::TBasicEnemy(const TPointF &spawnPos)
{
	position = spawnPos;
	baseSpeed = 95.0f;
	baseHealth = 100;
	speed = baseSpeed;
	scaledBaseSpeed = baseSpeed;
	health = baseHealth;
}
//---------------------------------------------------------------------------
void TBasicEnemy::Update(float deltaTime, const TPointF &playerPos)
{
	TPointF dir(playerPos.X - position.X, playerPos.Y - position.Y);
	const float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
	if (len > 0.001f)
	{
		dir.X /= len;
		dir.Y /= len;
		position.X += dir.X * speed * deltaTime;
		position.Y += dir.Y * speed * deltaTime;
	}
}
//---------------------------------------------------------------------------
void TBasicEnemy::Draw(TCanvas *canvas, const TPointF &camera) const
{
	if (!canvas)
		return;

	const float screenX = position.X - camera.X;
	const float screenY = position.Y - camera.Y;
	const int centerX = static_cast<int>(std::round(screenX));
	const int centerY = static_cast<int>(std::round(screenY));

	const float r = EnemyRadius;

	// Рисуем ромб (квадрат, повернутый на 45 градусов)
	TPoint diamond[4];
	diamond[0].X = centerX; diamond[0].Y = centerY - static_cast<int>(r);
	diamond[1].X = centerX + static_cast<int>(r); diamond[1].Y = centerY;
	diamond[2].X = centerX; diamond[2].Y = centerY + static_cast<int>(r);
	diamond[3].X = centerX - static_cast<int>(r); diamond[3].Y = centerY;

	canvas->Brush->Color = static_cast<TColor>(RGB(255, 60, 60));
	canvas->Pen->Color = static_cast<TColor>(RGB(255, 150, 150));
	canvas->Pen->Width = 2;
	canvas->Brush->Style = bsSolid;
	canvas->Polygon(diamond, 3);

	// Внутренний крест для детализации
	canvas->Pen->Width = 1;
	canvas->Pen->Color = static_cast<TColor>(RGB(255, 200, 200));
	canvas->MoveTo(centerX - static_cast<int>(r * 0.5f), centerY);
	canvas->LineTo(centerX + static_cast<int>(r * 0.5f), centerY);
	canvas->MoveTo(centerX, centerY - static_cast<int>(r * 0.5f));
	canvas->LineTo(centerX, centerY + static_cast<int>(r * 0.5f));
}
//---------------------------------------------------------------------------
TFastEnemy::TFastEnemy(const TPointF &spawnPos)
{
	position = spawnPos;
	baseSpeed = 180.0f;
	baseHealth = 50;
	speed = baseSpeed;
	scaledBaseSpeed = baseSpeed;
	health = baseHealth;
}
//---------------------------------------------------------------------------
void TFastEnemy::Update(float deltaTime, const TPointF &playerPos)
{
	TPointF dir(playerPos.X - position.X, playerPos.Y - position.Y);
	const float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
	if (len > 0.001f)
	{
		dir.X /= len;
		dir.Y /= len;
		position.X += dir.X * speed * deltaTime;
		position.Y += dir.Y * speed * deltaTime;
	}
}
//---------------------------------------------------------------------------
void TFastEnemy::Draw(TCanvas *canvas, const TPointF &camera) const
{
	if (!canvas)
		return;

	const float screenX = position.X - camera.X;
	const float screenY = position.Y - camera.Y;
	const int centerX = static_cast<int>(std::round(screenX));
	const int centerY = static_cast<int>(std::round(screenY));

	const float r = EnemyRadius * 0.8f;

	// Рисуем треугольник, указывающий направление движения
	// Направление к игроку (используем последнее известное направление или просто вверх)
	TPointF dir(0.0f, -1.0f); // по умолчанию вверх
	TPoint triangle[3];
	
	// Вершина треугольника (направление движения)
	triangle[0].X = centerX + static_cast<int>(std::round(dir.X * r));
	triangle[0].Y = centerY + static_cast<int>(std::round(dir.Y * r));
	
	// Основание треугольника
	const float perpX = -dir.Y;
	const float perpY = dir.X;
	triangle[1].X = centerX + static_cast<int>(std::round(perpX * r * 0.7f));
	triangle[1].Y = centerY + static_cast<int>(std::round(perpY * r * 0.7f));
	triangle[2].X = centerX - static_cast<int>(std::round(perpX * r * 0.7f));
	triangle[2].Y = centerY - static_cast<int>(std::round(perpY * r * 0.7f));

	canvas->Brush->Color = static_cast<TColor>(RGB(255, 200, 0)); // желтый
	canvas->Pen->Color = static_cast<TColor>(RGB(255, 255, 100));
	canvas->Pen->Width = 2;
	canvas->Brush->Style = bsSolid;
	canvas->Polygon(triangle, 2);

	// Внутренняя линия для скорости
	canvas->Pen->Width = 1;
	canvas->Pen->Color = static_cast<TColor>(RGB(255, 240, 150));
	canvas->MoveTo(centerX, centerY);
	canvas->LineTo(triangle[0].X, triangle[0].Y);
}
//---------------------------------------------------------------------------
TThrowerEnemy::TThrowerEnemy(const TPointF &spawnPos)
{
	position = spawnPos;
	baseSpeed = 70.0f;
	baseHealth = 120;
	speed = baseSpeed;
	scaledBaseSpeed = baseSpeed;
	health = baseHealth;
	throwTimer = 0.0f;
	throwCooldown = 2.5f; // кулдаун между бросками
}
//---------------------------------------------------------------------------
void TThrowerEnemy::Update(float deltaTime, const TPointF &playerPos)
{
	throwTimer -= deltaTime;
	
	// метатель движется медленнее, чем обычные враги
	TPointF dir(playerPos.X - position.X, playerPos.Y - position.Y);
	const float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
	if (len > 0.001f)
	{
		dir.X /= len;
		dir.Y /= len;
		position.X += dir.X * speed * deltaTime;
		position.Y += dir.Y * speed * deltaTime;
	}
}
//---------------------------------------------------------------------------
void TThrowerEnemy::Draw(TCanvas *canvas, const TPointF &camera) const
{
	if (!canvas)
		return;

	const float screenX = position.X - camera.X;
	const float screenY = position.Y - camera.Y;
	const int centerX = static_cast<int>(std::round(screenX));
	const int centerY = static_cast<int>(std::round(screenY));

	const float r = EnemyRadius;

	// Рисуем восьмиугольник (октагон)
	const int numPoints = 8;
	TPoint octagon[8];
	for (int i = 0; i < numPoints; i++)
	{
		const float angle = i * 2.0f * 3.14159265f / numPoints;
		octagon[i].X = centerX + static_cast<int>(std::round(r * std::cos(angle)));
		octagon[i].Y = centerY + static_cast<int>(std::round(r * std::sin(angle)));
	}

	canvas->Brush->Color = static_cast<TColor>(RGB(100, 150, 255)); // светло-синий
	canvas->Pen->Color = static_cast<TColor>(RGB(150, 200, 255));
	canvas->Pen->Width = 2;
	canvas->Brush->Style = bsSolid;
	canvas->Polygon(octagon, numPoints - 1);

	// Внутренний крест для обозначения метателя
	canvas->Pen->Width = 2;
	canvas->Pen->Color = static_cast<TColor>(RGB(200, 220, 255));
	const float crossSize = r * 0.6f;
	canvas->MoveTo(centerX - static_cast<int>(crossSize), centerY);
	canvas->LineTo(centerX + static_cast<int>(crossSize), centerY);
	canvas->MoveTo(centerX, centerY - static_cast<int>(crossSize));
	canvas->LineTo(centerX, centerY + static_cast<int>(crossSize));
}
//---------------------------------------------------------------------------
TZigzagEnemy::TZigzagEnemy(const TPointF &spawnPos)
{
	position = spawnPos;
	lastPosition = spawnPos;
	baseSpeed = 140.0f; // увеличена скорость
	baseHealth = 150; // увеличено здоровье (было 80)
	speed = baseSpeed;
	scaledBaseSpeed = baseSpeed;
	health = baseHealth;
	teleportTimer = 0.0f;
	teleportCooldown = 3.5f; // телепортируется каждые 3.5 секунды
	blinkTimer = 0.0f;
	isBlinking = false;
}
//---------------------------------------------------------------------------
void TZigzagEnemy::Update(float deltaTime, const TPointF &playerPos)
{
	teleportTimer -= deltaTime;
	
	// проверяем, нужно ли начать мерцание перед телепортацией
	if (teleportTimer <= 0.5f && teleportTimer > 0.0f && !isBlinking)
	{
		// начинаем мерцать за 0.5 секунды до телепортации
		isBlinking = true;
		blinkTimer = 0.5f;
	}
	
	// если таймер телепортации истек, телепортируемся
	if (teleportTimer <= 0.0f)
	{
		TPointF dir(playerPos.X - position.X, playerPos.Y - position.Y);
		const float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
		
		if (len > 0.001f)
		{
			// телепортируемся на 70% расстояния к игроку, но не ближе чем на 150 единиц
			const float teleportDistance = std::max(150.0f, len * 0.7f);
			dir.X /= len;
			dir.Y /= len;
			
			lastPosition = position; // сохраняем позицию для эффекта
			position.X += dir.X * teleportDistance;
			position.Y += dir.Y * teleportDistance;
			
			// ограничиваем границами мира
			position.X = std::clamp(position.X, 50.0f, WorldWidth - 50.0f);
			position.Y = std::clamp(position.Y, 50.0f, WorldHeight - 50.0f);
		}
		
		teleportTimer = teleportCooldown; // сбрасываем таймер
		isBlinking = false;
		blinkTimer = 0.0f;
	}
	
	// обновляем таймер мерцания
	if (isBlinking)
	{
		blinkTimer -= deltaTime;
		if (blinkTimer <= 0.0f)
		{
			blinkTimer = 0.0f;
		}
	}
	
	// обычное движение к игроку (всегда, кроме момента телепортации)
	// но замедляем движение во время мерцания
	TPointF dir(playerPos.X - position.X, playerPos.Y - position.Y);
	const float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
	if (len > 0.001f)
	{
		dir.X /= len;
		dir.Y /= len;
		
		// если мерцаем, движемся медленнее
		const float moveSpeed = isBlinking ? speed * 0.3f : speed;
		position.X += dir.X * moveSpeed * deltaTime;
		position.Y += dir.Y * moveSpeed * deltaTime;
	}
}
//---------------------------------------------------------------------------
void TZigzagEnemy::Draw(TCanvas *canvas, const TPointF &camera) const
{
	if (!canvas)
		return;

	const float screenX = position.X - camera.X;
	const float screenY = position.Y - camera.Y;
	const int centerX = static_cast<int>(std::round(screenX));
	const int centerY = static_cast<int>(std::round(screenY));

	const float r = EnemyRadius * 1.1f; // немного больше

	// Эффект мерцания при подготовке к телепортации
	float alpha = 1.0f;
	if (isBlinking)
	{
		// мерцание с частотой 10 Гц
		const int blinkPhase = static_cast<int>(blinkTimer * 20.0f) % 2;
		alpha = blinkPhase == 0 ? 0.3f : 1.0f;
	}

	// Рисуем энергетический кристалл (ромб с эффектом)
	const int numPoints = 4;
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

	// Внешний слой - яркий фиолетовый
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

	// Внутренний слой - более светлый
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

	// Центральная точка - энергетическое ядро
	const float coreR = r * 0.25f;
	const int coreLeft = static_cast<int>(screenX - coreR);
	const int coreTop = static_cast<int>(screenY - coreR);
	const int coreRight = static_cast<int>(screenX + coreR);
	const int coreBottom = static_cast<int>(screenY + coreR);
	canvas->Brush->Color = static_cast<TColor>(RGB(
		static_cast<int>(255 * alpha),
		static_cast<int>(255 * alpha),
		static_cast<int>(255 * alpha))); // белый центр
	canvas->Pen->Color = static_cast<TColor>(RGB(
		static_cast<int>(200 * alpha),
		static_cast<int>(0 * alpha),
		static_cast<int>(255 * alpha)));
	canvas->Pen->Width = 1;
	canvas->Ellipse(coreLeft, coreTop, coreRight, coreBottom);

	// Энергетические линии от центра к углам
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

	// Эффект телепортации - след от предыдущей позиции
	if (isBlinking && blinkTimer < 0.3f)
	{
		const float lastScreenX = lastPosition.X - camera.X;
		const float lastScreenY = lastPosition.Y - camera.Y;
		const float fadeAlpha = blinkTimer / 0.3f;
		
		const int lastCenterX = static_cast<int>(std::round(lastScreenX));
		const int lastCenterY = static_cast<int>(std::round(lastScreenY));
		
		// Рисуем полупрозрачный след
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
//---------------------------------------------------------------------------
TKamikazeEnemy::TKamikazeEnemy(const TPointF &spawnPos)
{
	position = spawnPos;
	baseSpeed = 240.0f;
	baseHealth = 30;
	speed = baseSpeed;
	scaledBaseSpeed = baseSpeed;
	health = baseHealth;
}
//---------------------------------------------------------------------------
void TKamikazeEnemy::Update(float deltaTime, const TPointF &playerPos)
{
	TPointF dir(playerPos.X - position.X, playerPos.Y - position.Y);
	const float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
	if (len > 0.001f)
	{
		dir.X /= len;
		dir.Y /= len;
		position.X += dir.X * speed * deltaTime;
		position.Y += dir.Y * speed * deltaTime;
	}
}
//---------------------------------------------------------------------------
void TKamikazeEnemy::Draw(TCanvas *canvas, const TPointF &camera) const
{
	if (!canvas)
		return;

	const float screenX = position.X - camera.X;
	const float screenY = position.Y - camera.Y;
	const int centerX = static_cast<int>(std::round(screenX));
	const int centerY = static_cast<int>(std::round(screenY));

	const float r = EnemyRadius * 0.7f;

	// Рисуем звезду (как символ взрыва)
	const int numPoints = 8;
	TPoint star[16]; // внешние и внутренние точки
	for (int i = 0; i < numPoints; i++)
	{
		const float angle = i * 2.0f * 3.14159265f / numPoints;
		// Внешние точки
		star[i * 2].X = centerX + static_cast<int>(std::round(r * std::cos(angle)));
		star[i * 2].Y = centerY + static_cast<int>(std::round(r * std::sin(angle)));
		// Внутренние точки
		const float innerAngle = angle + 3.14159265f / numPoints;
		star[i * 2 + 1].X = centerX + static_cast<int>(std::round(r * 0.5f * std::cos(innerAngle)));
		star[i * 2 + 1].Y = centerY + static_cast<int>(std::round(r * 0.5f * std::sin(innerAngle)));
	}

	canvas->Brush->Color = static_cast<TColor>(RGB(255, 0, 0)); // красный
	canvas->Pen->Color = static_cast<TColor>(RGB(255, 100, 100));
	canvas->Pen->Width = 2;
	canvas->Brush->Style = bsSolid;
	canvas->Polygon(star, 15);

	// Внутренний круг
	const float innerR = r * 0.4f;
	const int innerLeft = static_cast<int>(std::round(screenX - innerR));
	const int innerTop = static_cast<int>(std::round(screenY - innerR));
	const int innerRight = static_cast<int>(std::round(screenX + innerR));
	const int innerBottom = static_cast<int>(std::round(screenY + innerR));
	canvas->Brush->Color = static_cast<TColor>(RGB(255, 150, 0)); // оранжевый
	canvas->Ellipse(innerLeft, innerTop, innerRight, innerBottom);
}
//---------------------------------------------------------------------------
TShootingEnemy::TShootingEnemy(const TPointF &spawnPos)
{
	position = spawnPos;
	baseSpeed = 60.0f;
	baseHealth = 120;
	speed = baseSpeed;
	scaledBaseSpeed = baseSpeed;
	health = baseHealth;
	shootTimer = 0.0f;
	shootCooldown = 2.0f;
}
//---------------------------------------------------------------------------
void TShootingEnemy::Update(float deltaTime, const TPointF &playerPos)
{
	shootTimer -= deltaTime;
	
	TPointF dir(playerPos.X - position.X, playerPos.Y - position.Y);
	const float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
	if (len > 0.001f)
	{
		dir.X /= len;
		dir.Y /= len;
		position.X += dir.X * speed * deltaTime;
		position.Y += dir.Y * speed * deltaTime;
	}
}
//---------------------------------------------------------------------------
void TShootingEnemy::Draw(TCanvas *canvas, const TPointF &camera) const
{
	if (!canvas)
		return;

	const float screenX = position.X - camera.X;
	const float screenY = position.Y - camera.Y;
	const int centerX = static_cast<int>(std::round(screenX));
	const int centerY = static_cast<int>(std::round(screenY));

	const float r = EnemyRadius;

	// Рисуем квадрат с пушкой
	const int left = static_cast<int>(std::round(screenX - r));
	const int top = static_cast<int>(std::round(screenY - r));
	const int right = static_cast<int>(std::round(screenX + r));
	const int bottom = static_cast<int>(std::round(screenY + r));

	canvas->Brush->Color = static_cast<TColor>(RGB(0, 255, 100)); // зеленый
	canvas->Pen->Color = static_cast<TColor>(RGB(100, 255, 150));
	canvas->Pen->Width = 2;
	canvas->Brush->Style = bsSolid;
	canvas->Rectangle(left, top, right, bottom);

	// Пушка - прямоугольник, выходящий из центра
	canvas->Brush->Color = static_cast<TColor>(RGB(50, 200, 80));
	canvas->Pen->Color = static_cast<TColor>(RGB(150, 255, 200));
	const float gunLength = r * 0.8f;
	const float gunWidth = r * 0.3f;
	const int gunLeft = centerX - static_cast<int>(gunWidth);
	const int gunRight = centerX + static_cast<int>(gunWidth);
	const int gunTop = centerY - static_cast<int>(gunLength);
	const int gunBottom = centerY;
	canvas->Rectangle(gunLeft, gunTop, gunRight, gunBottom);

	// Дуло пушки
	canvas->Brush->Color = static_cast<TColor>(RGB(30, 150, 60));
	const int barrelSize = static_cast<int>(gunWidth * 0.6f);
	const int barrelLeft = centerX - barrelSize;
	const int barrelRight = centerX + barrelSize;
	const int barrelTop = gunTop - static_cast<int>(gunWidth * 0.5f);
	canvas->Rectangle(barrelLeft, barrelTop, barrelRight, gunTop);
}
//---------------------------------------------------------------------------
TBossEnemy::TBossEnemy(const TPointF &spawnPos, int appearanceLevel)
{
	position = spawnPos;
	this->appearanceLevel = appearanceLevel;
	
	// базовые характеристики
	baseSpeed = 50.0f;
	baseHealth = 1000;
	baseShootCooldown = 1.5f;
	
	// масштабирование с каждым появлением
	// HP: +30% за каждое появление
	// Скорость: +10% за каждое появление
	// Скорость стрельбы: -15% кулдауна за каждое появление (стреляет быстрее)
	// Урон пуль: +20% за каждое появление
	const float healthMultiplier = 1.0f + (appearanceLevel * 0.30f);
	const float speedMultiplier = 1.0f + (appearanceLevel * 0.10f);
	const float shootSpeedMultiplier = 1.0f - (appearanceLevel * 0.15f); // уменьшаем кулдаун
	const float shootSpeedFinal = std::max(0.1f, baseShootCooldown * shootSpeedMultiplier); // минимум 0.3 сек
	
	baseHealth = static_cast<int>(baseHealth * healthMultiplier);
	baseSpeed = baseSpeed * speedMultiplier;
	
	speed = baseSpeed;
	scaledBaseSpeed = baseSpeed;
	health = baseHealth;
	maxHealth = baseHealth;
	currentPhase = EBossPhase::Phase1;
	phaseTransitionTimer = 0.0f;
	shootTimer = 0.0f;
	shootCooldown = shootSpeedFinal;
	movementTimer = 0.0f;
	spiralAngle = 0.0f;
	stunTimer = 0.0f;
}
//---------------------------------------------------------------------------
void TBossEnemy::UpdatePhase(float deltaTime, const TPointF &playerPos)
{
	// переход между фазами в зависимости от HP
	const float healthRatio = static_cast<float>(health) / static_cast<float>(maxHealth);
	
	if (healthRatio > 0.80f && currentPhase != EBossPhase::Phase1)
	{
		currentPhase = EBossPhase::Phase1;
		phaseTransitionTimer = 1.0f; // пауза при смене фазы
		shootCooldown = baseShootCooldown * 0.9f; // немного быстрее
	}
	else if (healthRatio > 0.60f && healthRatio <= 0.80f && currentPhase != EBossPhase::Phase2)
	{
		currentPhase = EBossPhase::Phase2;
		phaseTransitionTimer = 1.0f;
		shootCooldown = baseShootCooldown * 0.75f; // быстрее стреляет во второй фазе
	}
	else if (healthRatio > 0.40f && healthRatio <= 0.60f && currentPhase != EBossPhase::Phase3)
	{
		currentPhase = EBossPhase::Phase3;
		phaseTransitionTimer = 1.0f;
		shootCooldown = baseShootCooldown * 0.6f; // еще быстрее в третьей фазе
	}
	else if (healthRatio > 0.20f && healthRatio <= 0.40f && currentPhase != EBossPhase::Phase4)
	{
		currentPhase = EBossPhase::Phase4;
		phaseTransitionTimer = 1.0f;
		shootCooldown = baseShootCooldown * 0.5f; // очень быстро в четвертой фазе
	}
	else if (healthRatio <= 0.20f && currentPhase != EBossPhase::Phase5)
	{
		currentPhase = EBossPhase::Phase5;
		phaseTransitionTimer = 1.0f;
		shootCooldown = baseShootCooldown * 0.35f; // максимально быстро в пятой фазе
	}
	
	if (phaseTransitionTimer > 0.0f)
		phaseTransitionTimer -= deltaTime;
}
//---------------------------------------------------------------------------
void TBossEnemy::ShootFanPattern(const TPointF &playerPos, std::vector<TBullet> &bullets, float damageMultiplier)
{
	// стрельба веером из 7 пуль (увеличено с 5)
	TPointF dir(playerPos.X - position.X, playerPos.Y - position.Y);
	const float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
	if (len < 0.001f)
		return;
	
	const float baseAngle = std::atan2(dir.Y, dir.X);
	const float spread = 0.8f; // угол веера увеличен
	const int bulletCount = 7; // увеличено с 5
	
	// урон пуль увеличивается с каждым появлением
	const int baseBulletDamage = 12;
	int bulletDamage = static_cast<int>(baseBulletDamage * (1.0f + appearanceLevel * 0.20f));
	// применяем временный множитель урона (если волна длится слишком долго)
	bulletDamage = static_cast<int>(bulletDamage * damageMultiplier);
	
	for (int i = 0; i < bulletCount; i++)
	{
		const float t = static_cast<float>(i) / (bulletCount - 1) - 0.5f;
		const float angle = baseAngle + t * spread;
		TPointF bulletDir(std::cos(angle), std::sin(angle));
		
		const float bulletSpeed = 380.0f; // увеличена скорость
		const float bulletRange = 900.0f; // увеличена дальность
		const float bulletSize = 5.0f;
		
		bullets.emplace_back(position, bulletDir, bulletDamage, bulletSpeed, bulletRange, bulletSize);
	}
}
//---------------------------------------------------------------------------
void TBossEnemy::ShootCirclePattern(const TPointF &playerPos, std::vector<TBullet> &bullets, float damageMultiplier)
{
	// стрельба по кругу - двойной круг (16 + 16 = 32 пули)
	const int bulletCount = 16;
	const float angleStep = 2.0f * 3.14159265f / bulletCount;
	
	// урон пуль увеличивается с каждым появлением
	const int baseBulletDamage = 10;
	int bulletDamage = static_cast<int>(baseBulletDamage * (1.0f + appearanceLevel * 0.20f));
	// применяем временный множитель урона (если волна длится слишком долго)
	bulletDamage = static_cast<int>(bulletDamage * damageMultiplier);
	
	// Первый круг
	for (int i = 0; i < bulletCount; i++)
	{
		const float angle = i * angleStep;
		TPointF bulletDir(std::cos(angle), std::sin(angle));
		
		const float bulletSpeed = 340.0f; // увеличена скорость
		const float bulletRange = 900.0f; // увеличена дальность
		const float bulletSize = 4.5f;
		
		bullets.emplace_back(position, bulletDir, bulletDamage, bulletSpeed, bulletRange, bulletSize);
	}
	
	// Второй круг со смещением
	for (int i = 0; i < bulletCount; i++)
	{
		const float angle = i * angleStep + angleStep * 0.5f; // смещение на полшага
		TPointF bulletDir(std::cos(angle), std::sin(angle));
		
		const float bulletSpeed = 300.0f; // немного медленнее для разнообразия
		const float bulletRange = 900.0f;
		const float bulletSize = 4.0f; // немного меньше
		
		bullets.emplace_back(position, bulletDir, bulletDamage, bulletSpeed, bulletRange, bulletSize);
	}
}
//---------------------------------------------------------------------------
void TBossEnemy::ShootSpiralPattern(const TPointF &playerPos, std::vector<TBullet> &bullets, float damageMultiplier)
{
	// стрельба спиралью - двойная спираль (8 пуль)
	spiralAngle += 0.4f; // увеличена скорость вращения спирали
	const int bulletCount = 8; // увеличено с 5
	const float angleStep = 2.0f * 3.14159265f / bulletCount;
	
	// урон пуль увеличивается с каждым появлением
	const int baseBulletDamage = 15;
	int bulletDamage = static_cast<int>(baseBulletDamage * (1.0f + appearanceLevel * 0.20f));
	// применяем временный множитель урона (если волна длится слишком долго)
	bulletDamage = static_cast<int>(bulletDamage * damageMultiplier);
	
	// Первая спираль
	for (int i = 0; i < bulletCount; i++)
	{
		const float angle = spiralAngle + i * angleStep;
		TPointF bulletDir(std::cos(angle), std::sin(angle));
		
		const float bulletSpeed = 400.0f; // увеличена скорость
		const float bulletRange = 900.0f; // увеличена дальность
		const float bulletSize = 5.5f;
		
		bullets.emplace_back(position, bulletDir, bulletDamage, bulletSpeed, bulletRange, bulletSize);
	}
	
	// Вторая спираль (вращается в обратную сторону)
	for (int i = 0; i < bulletCount; i++)
	{
		const float angle = -spiralAngle + i * angleStep;
		TPointF bulletDir(std::cos(angle), std::sin(angle));
		
		const float bulletSpeed = 360.0f; // немного медленнее
		const float bulletRange = 900.0f;
		const float bulletSize = 5.0f;
		
		bullets.emplace_back(position, bulletDir, bulletDamage, bulletSpeed, bulletRange, bulletSize);
	}
}
//---------------------------------------------------------------------------
void TBossEnemy::ShootCombinedPattern(const TPointF &playerPos, std::vector<TBullet> &bullets, float damageMultiplier)
{
	// комбинированная атака: веер + круг одновременно
	// Сначала веер
	TPointF dir(playerPos.X - position.X, playerPos.Y - position.Y);
	const float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
	if (len > 0.001f)
	{
		const float baseAngle = std::atan2(dir.Y, dir.X);
		const float spread = 0.7f;
		const int fanBulletCount = 6;
		
		const int baseBulletDamage = 13;
		int bulletDamage = static_cast<int>(baseBulletDamage * (1.0f + appearanceLevel * 0.20f));
		bulletDamage = static_cast<int>(bulletDamage * damageMultiplier);
		
		for (int i = 0; i < fanBulletCount; i++)
		{
			const float t = static_cast<float>(i) / (fanBulletCount - 1) - 0.5f;
			const float angle = baseAngle + t * spread;
			TPointF bulletDir(std::cos(angle), std::sin(angle));
			
			bullets.emplace_back(position, bulletDir, bulletDamage, 370.0f, 900.0f, 5.0f);
		}
	}
	
	// Затем круг (12 пуль)
	const int circleBulletCount = 12;
	const float angleStep = 2.0f * 3.14159265f / circleBulletCount;
	
	const int baseBulletDamage = 11;
	int bulletDamage = static_cast<int>(baseBulletDamage * (1.0f + appearanceLevel * 0.20f));
	bulletDamage = static_cast<int>(bulletDamage * damageMultiplier);
	
	for (int i = 0; i < circleBulletCount; i++)
	{
		const float angle = i * angleStep;
		TPointF bulletDir(std::cos(angle), std::sin(angle));
		
		bullets.emplace_back(position, bulletDir, bulletDamage, 330.0f, 900.0f, 4.5f);
	}
}
//---------------------------------------------------------------------------
void TBossEnemy::ShootAggressivePattern(const TPointF &playerPos, std::vector<TBullet> &bullets, float damageMultiplier)
{
	// агрессивная атака: множественные спирали + веер
	spiralAngle += 0.5f; // очень быстрое вращение
	
	// Три спирали одновременно
	for (int spiral = 0; spiral < 3; spiral++)
	{
		const float spiralOffset = spiral * (2.0f * 3.14159265f / 3.0f); // смещение между спиралями
		const int bulletCount = 6;
		const float angleStep = 2.0f * 3.14159265f / bulletCount;
		
		const int baseBulletDamage = 16;
		int bulletDamage = static_cast<int>(baseBulletDamage * (1.0f + appearanceLevel * 0.20f));
		bulletDamage = static_cast<int>(bulletDamage * damageMultiplier);
		
		for (int i = 0; i < bulletCount; i++)
		{
			const float angle = spiralAngle + spiralOffset + i * angleStep;
			TPointF bulletDir(std::cos(angle), std::sin(angle));
			
			const float speed = 420.0f + spiral * 20.0f; // разная скорость для каждой спирали
			bullets.emplace_back(position, bulletDir, bulletDamage, speed, 900.0f, 5.5f);
		}
	}
	
	// Дополнительный веер в сторону игрока
	TPointF dir(playerPos.X - position.X, playerPos.Y - position.Y);
	const float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
	if (len > 0.001f)
	{
		const float baseAngle = std::atan2(dir.Y, dir.X);
		const int fanBulletCount = 5;
		const float spread = 0.5f;
		
		const int baseBulletDamage = 14;
		int bulletDamage = static_cast<int>(baseBulletDamage * (1.0f + appearanceLevel * 0.20f));
		bulletDamage = static_cast<int>(bulletDamage * damageMultiplier);
		
		for (int i = 0; i < fanBulletCount; i++)
		{
			const float t = static_cast<float>(i) / (fanBulletCount - 1) - 0.5f;
			const float angle = baseAngle + t * spread;
			TPointF bulletDir(std::cos(angle), std::sin(angle));
			
			bullets.emplace_back(position, bulletDir, bulletDamage, 400.0f, 900.0f, 5.0f);
		}
	}
}
//---------------------------------------------------------------------------
void TBossEnemy::CreateBullets(const TPointF &playerPos, std::vector<TBullet> &bullets, float damageMultiplier)
{
	if (phaseTransitionTimer > 0.0f)
		return; // не стреляем во время смены фазы
	
	if (shootTimer > 0.0f)
		return; // еще не готов стрелять
		
	switch (currentPhase)
	{
		case EBossPhase::Phase1:
			ShootFanPattern(playerPos, bullets, damageMultiplier);
			break;
		case EBossPhase::Phase2:
			ShootCirclePattern(playerPos, bullets, damageMultiplier);
			break;
		case EBossPhase::Phase3:
			ShootSpiralPattern(playerPos, bullets, damageMultiplier);
			break;
		case EBossPhase::Phase4:
			ShootCombinedPattern(playerPos, bullets, damageMultiplier);
			break;
		case EBossPhase::Phase5:
			ShootAggressivePattern(playerPos, bullets, damageMultiplier);
			break;
	}
	
	shootTimer = shootCooldown; // сбрасываем таймер
}
//---------------------------------------------------------------------------
void TBossEnemy::Update(float deltaTime, const TPointF &playerPos)
{
	UpdatePhase(deltaTime, playerPos);
	
	// обновляем таймер остановки
	if (stunTimer > 0.0f)
	{
		stunTimer -= deltaTime;
		if (stunTimer < 0.0f)
			stunTimer = 0.0f;
	}
	
	shootTimer -= deltaTime;
	movementTimer += deltaTime;
	
	// движение зависит от фазы (не двигаемся, если оглушены)
	if (stunTimer > 0.0f)
	{
		// босс остановлен после нанесения урона
		return;
	}
	
	if (currentPhase == EBossPhase::Phase3)
	{
		// в третьей фазе движется по кругу вокруг игрока
		const float circleRadius = 200.0f;
		const float circleSpeed = 0.5f;
		const float angle = movementTimer * circleSpeed;
		
		TPointF offset(std::cos(angle) * circleRadius, std::sin(angle) * circleRadius);
		TPointF targetPos = playerPos;
		targetPos.X += offset.X;
		targetPos.Y += offset.Y;
		
		TPointF dir(targetPos.X - position.X, targetPos.Y - position.Y);
		const float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
		if (len > 0.001f)
		{
			dir.X /= len;
			dir.Y /= len;
			position.X += dir.X * speed * deltaTime;
			position.Y += dir.Y * speed * deltaTime;
		}
	}
	else if (currentPhase == EBossPhase::Phase4)
	{
		// в четвертой фазе движется зигзагом к игроку
		const float zigzagAmplitude = 100.0f;
		const float zigzagSpeed = 2.0f;
		const float zigzagOffset = std::sin(movementTimer * zigzagSpeed) * zigzagAmplitude;
		
		TPointF dir(playerPos.X - position.X, playerPos.Y - position.Y);
		const float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
		if (len > 0.001f)
		{
			// Перпендикулярный вектор для зигзага
			TPointF perp(-dir.Y, dir.X);
			const float perpLen = std::sqrt(perp.X * perp.X + perp.Y * perp.Y);
			if (perpLen > 0.001f)
			{
				perp.X /= perpLen;
				perp.Y /= perpLen;
			}
			
			dir.X /= len;
			dir.Y /= len;
			
			TPointF moveDir(dir.X, dir.Y);
			// Зигзаг только если не слишком близко
			if (len > 50.0f)
			{
				moveDir.X += perp.X * (zigzagOffset / len);
				moveDir.Y += perp.Y * (zigzagOffset / len);
			}
			
			const float moveLen = std::sqrt(moveDir.X * moveDir.X + moveDir.Y * moveDir.Y);
			if (moveLen > 0.001f)
			{
				moveDir.X /= moveLen;
				moveDir.Y /= moveLen;
			}
			
			position.X += moveDir.X * speed * 1.2f * deltaTime; // быстрее
			position.Y += moveDir.Y * speed * 1.2f * deltaTime;
		}
	}
	else if (currentPhase == EBossPhase::Phase5)
	{
		// в пятой фазе агрессивно преследует игрока с быстрыми рывками
		TPointF dir(playerPos.X - position.X, playerPos.Y - position.Y);
		const float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
		if (len > 0.001f)
		{
			dir.X /= len;
			dir.Y /= len;
			position.X += dir.X * speed * 1.8f * deltaTime; // очень быстро
			position.Y += dir.Y * speed * 1.8f * deltaTime;
		}
	}
	else
	{
		// в первых двух фазах медленно приближается к игроку
		TPointF dir(playerPos.X - position.X, playerPos.Y - position.Y);
		const float len = std::sqrt(dir.X * dir.X + dir.Y * dir.Y);
		if (len > 0.001f)
		{
			dir.X /= len;
			dir.Y /= len;
			position.X += dir.X * speed * deltaTime;
			position.Y += dir.Y * speed * deltaTime;
		}
	}
}
//---------------------------------------------------------------------------
void TBossEnemy::Draw(TCanvas *canvas, const TPointF &camera) const
{
	if (!canvas)
		return;

	const float screenX = position.X - camera.X;
	const float screenY = position.Y - camera.Y;

	// босс больше обычных врагов
	const float r = EnemyRadius * 2.5f;
	const int left = static_cast<int>(std::round(screenX - r));
	const int top = static_cast<int>(std::round(screenY - r));
	const int right = static_cast<int>(std::round(screenX + r));
	const int bottom = static_cast<int>(std::round(screenY + r));

	// цвет зависит от фазы
	TColor bossColor, bossPenColor;
	switch (currentPhase)
	{
		case EBossPhase::Phase1:
			bossColor = static_cast<TColor>(RGB(255, 100, 0)); // оранжевый
			bossPenColor = static_cast<TColor>(RGB(255, 200, 100));
			break;
		case EBossPhase::Phase2:
			bossColor = static_cast<TColor>(RGB(200, 0, 255)); // фиолетовый
			bossPenColor = static_cast<TColor>(RGB(255, 100, 255));
			break;
		case EBossPhase::Phase3:
			bossColor = static_cast<TColor>(RGB(255, 0, 0)); // красный
			bossPenColor = static_cast<TColor>(RGB(255, 150, 150));
			break;
		case EBossPhase::Phase4:
			bossColor = static_cast<TColor>(RGB(255, 50, 150)); // розовый
			bossPenColor = static_cast<TColor>(RGB(255, 150, 200));
			break;
		case EBossPhase::Phase5:
			bossColor = static_cast<TColor>(RGB(150, 0, 0)); // темно-красный
			bossPenColor = static_cast<TColor>(RGB(255, 0, 0)); // ярко-красный контур
			break;
		default:
			bossColor = static_cast<TColor>(RGB(255, 100, 0));
			bossPenColor = static_cast<TColor>(RGB(255, 200, 100));
			break;
	}

	const int centerX = static_cast<int>(std::round(screenX));
	const int centerY = static_cast<int>(std::round(screenY));

	// Внешний восьмиугольник
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
	
	// Внутренний восьмиугольник для эффекта глубины
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

	// Центральный круг с крестом
	const float centerR = r * 0.3f;
	const int centerLeft = static_cast<int>(std::round(screenX - centerR));
	const int centerTop = static_cast<int>(std::round(screenY - centerR));
	const int centerRight = static_cast<int>(std::round(screenX + centerR));
	const int centerBottom = static_cast<int>(std::round(screenY + centerR));
	canvas->Brush->Color = bossPenColor;
	canvas->Pen->Color = static_cast<TColor>(RGB(255, 255, 255));
	canvas->Pen->Width = 1;
	canvas->Ellipse(centerLeft, centerTop, centerRight, centerBottom);
	
	// Крест в центре
	const float crossSize = centerR * 0.7f;
	canvas->MoveTo(centerX - static_cast<int>(crossSize), centerY);
	canvas->LineTo(centerX + static_cast<int>(crossSize), centerY);
	canvas->MoveTo(centerX, centerY - static_cast<int>(crossSize));
	canvas->LineTo(centerX, centerY + static_cast<int>(crossSize));
}
//---------------------------------------------------------------------------
void TBossEnemy::ApplyDamage(int dmg)
{
	health -= dmg;
	if (health < 0)
		health = 0;
}
//---------------------------------------------------------------------------
float TBossEnemy::GetHealthRatio() const
{
	if (maxHealth <= 0)
		return 0.0f;
	return static_cast<float>(health) / static_cast<float>(maxHealth);
}
//---------------------------------------------------------------------------
void TBossEnemy::ApplyTemporarySpeedMultiplier(float multiplier)
{
	// учитываем фазовые модификации скорости
	float phaseMultiplier = 1.0f;
	if (currentPhase == EBossPhase::Phase3)
	{
		phaseMultiplier = 1.5f;
	}
	else if (currentPhase == EBossPhase::Phase4)
	{
		phaseMultiplier = 1.2f; // быстрее в четвертой фазе
	}
	else if (currentPhase == EBossPhase::Phase5)
	{
		phaseMultiplier = 1.8f; // очень быстро в пятой фазе
	}
	
	// применяем временный множитель поверх фазового
	speed = scaledBaseSpeed * phaseMultiplier * multiplier;
}
//---------------------------------------------------------------------------
void TBossEnemy::StartStun(float duration)
{
	stunTimer = duration;
}
//---------------------------------------------------------------------------


