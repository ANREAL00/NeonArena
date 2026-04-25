


#ifndef GameCameraH
#define GameCameraH


#include <System.Types.hpp>
#include "GameConstants.h"




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
	
	
	void Update(float deltaTime, const TPointF &targetPosition, 
		int canvasWidth, int canvasHeight, float worldWidth, float worldHeight);
	
	
	void AddShake(float intensity, float duration);
	
	
	TPointF GetCameraPosition() const;
	
	
	TPointF GetBasePosition() const { return Position; }
	
	
	TPointF GetShakeOffset() const { return ShakeOffset; }
	
	
	void SetWorldBounds(float width, float height);
	
	
	void Reset();
};


#endif


