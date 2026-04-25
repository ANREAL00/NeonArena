#include <vcl.h>
#pragma hdrstop


#include "GameCamera.h"
#include "GameConstants.h"
#include <algorithm>
#include <cmath>


TGameCamera::TGameCamera()
	: Position(PointF(0.0f, 0.0f)),
	  ShakeTimer(0.0f),
	  ShakeIntensity(0.0f),
	  ShakeOffset(PointF(0.0f, 0.0f)),
	  WorldWidth(NeonGame::WorldWidth),
	  WorldHeight(NeonGame::WorldHeight)
{
}

void TGameCamera::Update(float deltaTime, const TPointF &targetPosition,
	int canvasWidth, int canvasHeight, float worldWidth, float worldHeight)
{
	WorldWidth = worldWidth;
	WorldHeight = worldHeight;
	
	
	if (ShakeTimer > 0.0f)
	{
		ShakeTimer -= deltaTime;
		if (ShakeTimer < 0.0f)
			ShakeTimer = 0.0f;
		
		
		const float angle = Random(360) * 3.14159265f / 180.0f;
		const float maxDuration = 0.2f;
		const float progress = ShakeTimer / maxDuration;
		const float distance = ShakeIntensity * progress; 
		ShakeOffset.X = std::cos(angle) * distance;
		ShakeOffset.Y = std::sin(angle) * distance;
	}
	else
	{
		ShakeOffset = PointF(0.0f, 0.0f);
		ShakeIntensity = 0.0f;
	}

	
	float camX = targetPosition.X - canvasWidth / 2.0f;
	float camY = targetPosition.Y - canvasHeight / 2.0f;

	
	camX = std::clamp(camX, 0.0f, std::max(0.0f, WorldWidth - static_cast<float>(canvasWidth)));
	camY = std::clamp(camY, 0.0f, std::max(0.0f, WorldHeight - static_cast<float>(canvasHeight)));

	Position = PointF(camX, camY);
}

void TGameCamera::AddShake(float intensity, float duration)
{
	if (ShakeTimer < duration)
		ShakeTimer = duration;
	if (ShakeIntensity < intensity)
		ShakeIntensity = intensity;
}

TPointF TGameCamera::GetCameraPosition() const
{
	TPointF result = Position;
	result.X += ShakeOffset.X;
	result.Y += ShakeOffset.Y;
	return result;
}

void TGameCamera::SetWorldBounds(float width, float height)
{
	WorldWidth = width;
	WorldHeight = height;
}

void TGameCamera::Reset()
{
	Position = PointF(0.0f, 0.0f);
	ShakeTimer = 0.0f;
	ShakeIntensity = 0.0f;
	ShakeOffset = PointF(0.0f, 0.0f);
}




