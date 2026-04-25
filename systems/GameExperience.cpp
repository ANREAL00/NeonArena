#include <vcl.h>
#pragma hdrstop


#include "GameExperience.h"
#include "GameConstants.h"
#include <cmath>
#include <algorithm>


TExperienceOrb::TExperienceOrb(const TPointF &spawnPos, int expValue, uint32_t netId, float initialLifetime)
	: position(spawnPos),
	  lifetime(initialLifetime),
	  animationTime(0.0f),
	  pickupRadius(30.0f),
	  experienceValue(expValue),
	  netInstanceId(netId)
{
}

void TExperienceOrb::Update(float deltaTime, const TPointF &playerPos)
{
	lifetime -= deltaTime;
	animationTime += deltaTime;

	
	const float dx = playerPos.X - position.X;
	const float dy = playerPos.Y - position.Y;
	const float distSq = dx * dx + dy * dy;
	const float attractDist = 80.0f; 
	const float attractDistSq = attractDist * attractDist;

	if (distSq < attractDistSq && distSq > 0.0001f)
	{
		const float dist = std::sqrt(distSq);
		const float speed = 200.0f; 
		const float moveDist = speed * deltaTime;
		if (moveDist < dist)
		{
			position.X += (dx / dist) * moveDist;
			position.Y += (dy / dist) * moveDist;
		}
		else
		{
			
			position = playerPos;
		}
	}
}

void TExperienceOrb::Draw(TCanvas *canvas, const TPointF &camera) const
{
	if (!canvas)
		return;

	const float screenX = position.X - camera.X;
	const float screenY = position.Y - camera.Y;

	
	const float pulse = 0.8f + 0.2f * std::sin(animationTime * 5.0f);
	const float radius = 8.0f * pulse;

	canvas->Brush->Style = bsSolid;
	canvas->Pen->Style = psClear;

	
	canvas->Brush->Color = static_cast<TColor>(RGB(0, 200, 255));
	const int glowRadius = static_cast<int>(radius * 1.5f);
	canvas->Ellipse(
		static_cast<int>(screenX - glowRadius),
		static_cast<int>(screenY - glowRadius),
		static_cast<int>(screenX + glowRadius),
		static_cast<int>(screenY + glowRadius));

	
	canvas->Brush->Color = static_cast<TColor>(RGB(0, 255, 200));
	const int coreRadius = static_cast<int>(radius);
	canvas->Ellipse(
		static_cast<int>(screenX - coreRadius),
		static_cast<int>(screenY - coreRadius),
		static_cast<int>(screenX + coreRadius),
		static_cast<int>(screenY + coreRadius));

	canvas->Pen->Style = psSolid;
}

bool TExperienceOrb::CheckPickup(const TPointF &playerPos, float playerRadius) const
{
	const float dx = playerPos.X - position.X;
	const float dy = playerPos.Y - position.Y;
	const float distSq = dx * dx + dy * dy;
	const float pickupDist = playerRadius + pickupRadius;
	return distSq <= pickupDist * pickupDist;
}


