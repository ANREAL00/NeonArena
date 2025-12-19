//---------------------------------------------------------------------------
// Система камеры (Neon Arena)
//---------------------------------------------------------------------------
#ifndef GameCameraH
#define GameCameraH
//---------------------------------------------------------------------------

#include <System.Types.hpp>
#include "GameConstants.h"

//---------------------------------------------------------------------------
// Камера с поддержкой шейка
//---------------------------------------------------------------------------
class TGameCamera
{
private:
	TPointF Position;
	float ShakeTimer;
	float ShakeIntensity;
	TPointF ShakeOffset;
	
	float WorldWidth;
	float WorldHeight;

public:
	TGameCamera();
	
	// Обновление камеры
	void Update(float deltaTime, const TPointF &targetPosition, 
		int canvasWidth, int canvasHeight, float worldWidth, float worldHeight);
	
	// Добавление шейка
	void AddShake(float intensity, float duration);
	
	// Получение позиции камеры (с учётом шейка)
	TPointF GetCameraPosition() const;
	
	// Получение позиции без шейка
	TPointF GetBasePosition() const { return Position; }
	
	// Получение смещения от шейка
	TPointF GetShakeOffset() const { return ShakeOffset; }
	
	// Установка границ мира
	void SetWorldBounds(float width, float height);
	
	// Сброс
	void Reset();
};

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

