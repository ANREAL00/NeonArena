#include <vcl.h>
#pragma hdrstop

#include "GameProjectile.h"
#include "GameConstants.h"
#include <cmath>

using namespace NeonGame;

TBullet::TBullet(const TPointF &startPos, const TPointF &dir, int bulletDamage,
	float bulletSpeed, float bulletRange, float bulletSize)
	: position(startPos),
	  startPosition(startPos),
	  velocity(dir),
	  damage(bulletDamage),
	  lifeTime(0.0f),
	  maxLifeTime(10.0f), 
	  maxRange(bulletRange),
	  radius(bulletSize),
	  used(false) 
{
	
	float len = std::sqrt(velocity.X * velocity.X + velocity.Y * velocity.Y);
	if (len <= 0.0001f)
	{
		velocity = PointF(0.0f, -1.0f);
		len = 1.0f;
	}
	velocity.X = velocity.X / len * bulletSpeed;
	velocity.Y = velocity.Y / len * bulletSpeed;
}

void TBullet::Update(float deltaTime)
{
	position.X += velocity.X * deltaTime;
	position.Y += velocity.Y * deltaTime;
	lifeTime += deltaTime;
}

void TBullet::Draw(TCanvas *canvas, const TPointF &camera) const
{
	if (!canvas)
		return;

	const float screenX = position.X - camera.X;
	const float screenY = position.Y - camera.Y;
	const float trailLength = radius * 2.0f;
	const float trailX = screenX - velocity.X * 0.02f; 
	const float trailY = screenY - velocity.Y * 0.02f;
	
	canvas->Pen->Color = static_cast<TColor>(RGB(255, 120, 220));
	canvas->Pen->Width = 1;
	canvas->MoveTo(static_cast<int>(std::round(trailX)), static_cast<int>(std::round(trailY)));
	canvas->LineTo(static_cast<int>(std::round(screenX)), static_cast<int>(std::round(screenY)));

	const int left = static_cast<int>(std::round(screenX - radius));
	const int top = static_cast<int>(std::round(screenY - radius));
	const int right = static_cast<int>(std::round(screenX + radius));
	const int bottom = static_cast<int>(std::round(screenY + radius));

	canvas->Brush->Color = static_cast<TColor>(RGB(255, 80, 200));
	canvas->Pen->Color = static_cast<TColor>(RGB(255, 160, 255));
	canvas->Pen->Width = 1;
	canvas->Ellipse(left, top, right, bottom);
}

bool TBullet::IsAlive() const
{
	if (used)
		return false;

	if (lifeTime >= maxLifeTime)
		return false;

	const float dx = position.X - startPosition.X;
	const float dy = position.Y - startPosition.Y;
	const float distSq = dx * dx + dy * dy;
	return distSq < maxRange * maxRange;
}

const TPointF &TBullet::GetPosition() const
{
	return position;
}

int TBullet::GetDamage() const
{
	return damage;
}

TThrownProjectile::TThrownProjectile(const TPointF &startPos, const TPointF &targetPos, int projDamage, float throwSpeed)
	: position(startPos),
	  startPosition(startPos),
	  targetPosition(targetPos),
	  damage(projDamage),
	  lifeTime(0.0f),
	  maxLifeTime(5.0f),
	  radius(EnemyRadius * 0.7f), 
	  hasLanded(false),
	  hitPlayer(false),
	  poolCreated(false),
	  gravity(600.0f) 
{
	const float dx = targetPos.X - startPos.X;
	const float dy = targetPos.Y - startPos.Y;
	const float distance = std::sqrt(dx * dx + dy * dy);
	const float flightTime = distance / throwSpeed;
	
	velocity.X = (dx / flightTime);
	velocity.Y = (dy / flightTime) - (gravity * flightTime * 0.5f);
}

void TThrownProjectile::Update(float deltaTime)
{
	if (hasLanded)
		return;
	
	velocity.Y += gravity * deltaTime;

	position.X += velocity.X * deltaTime;
	position.Y += velocity.Y * deltaTime;
	
	lifeTime += deltaTime;
	
	const float dx = position.X - targetPosition.X;
	const float dy = position.Y - targetPosition.Y;
	const float distSq = dx * dx + dy * dy;
	
	const float horizontalThreshold = radius * 2.0f; 
	if ((distSq < horizontalThreshold * horizontalThreshold && position.Y >= targetPosition.Y - 50.0f) || 
		lifeTime >= maxLifeTime)
	{
		hasLanded = true;
	}
}

void TThrownProjectile::Draw(TCanvas *canvas, const TPointF &camera) const
{
	if (!canvas)
		return;
	
	const float screenX = position.X - camera.X;
	const float screenY = position.Y - camera.Y;
	
	const int left = static_cast<int>(std::round(screenX - radius));
	const int top = static_cast<int>(std::round(screenY - radius));
	const int right = static_cast<int>(std::round(screenX + radius));
	const int bottom = static_cast<int>(std::round(screenY + radius));
	
	canvas->Brush->Color = static_cast<TColor>(RGB(150, 100, 50)); 
	canvas->Pen->Color = static_cast<TColor>(RGB(200, 150, 100));
	canvas->Pen->Width = 2;
	canvas->Ellipse(left, top, right, bottom);
}

bool TThrownProjectile::IsAlive() const
{
	return !hasLanded && lifeTime < maxLifeTime;
}

TAcidPool::TAcidPool(const TPointF &pos, float poolRadius, float duration, int poolDamage)
	: position(pos),
	  radius(poolRadius),
	  lifeTime(0.0f),
	  maxLifeTime(duration),
	  damage(poolDamage),
	  damageCooldown(0.5f), 
	  currentDamageCooldown(0.0f)
{
}

void TAcidPool::Update(float deltaTime)
{
	lifeTime += deltaTime;
	if (currentDamageCooldown > 0.0f)
		currentDamageCooldown -= deltaTime;
}

void TAcidPool::Draw(TCanvas *canvas, const TPointF &camera) const
{
	if (!canvas)
		return;
	
	const float screenX = position.X - camera.X;
	const float screenY = position.Y - camera.Y;
	
	
	const float alpha = 1.0f - (lifeTime / maxLifeTime);
	const int alphaValue = static_cast<int>(alpha * 200.0f); 
	
	const int left = static_cast<int>(std::round(screenX - radius));
	const int top = static_cast<int>(std::round(screenY - radius));
	const int right = static_cast<int>(std::round(screenX + radius));
	const int bottom = static_cast<int>(std::round(screenY + radius));
	
	
	canvas->Brush->Color = static_cast<TColor>(RGB(0, alphaValue, alphaValue / 2));
	canvas->Pen->Color = static_cast<TColor>(RGB(0, 255, 150));
	canvas->Pen->Width = 2;
	canvas->Ellipse(left, top, right, bottom);
}

bool TAcidPool::IsAlive() const
{
	return lifeTime < maxLifeTime;
}

bool TAcidPool::CanDealDamage() const
{
	return currentDamageCooldown <= 0.0f;
}

void TAcidPool::ResetDamageCooldown()
{
	currentDamageCooldown = damageCooldown;
}