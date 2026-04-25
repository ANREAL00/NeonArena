#ifndef GameInputH
#define GameInputH

#include <System.Types.hpp>

struct TInputState
{
	bool MoveUp;
	bool MoveDown;
	bool MoveLeft;
	bool MoveRight;
	bool PrimaryFire;
	bool AltFire;
	TPoint MouseClient;
	bool HasMouse;
	float AimWorldX;
	float AimWorldY;

	TInputState()
		: MoveUp(false),
		  MoveDown(false),
		  MoveLeft(false),
		  MoveRight(false),
		  PrimaryFire(false),
		  AltFire(false),
		  MouseClient(0, 0),
		  HasMouse(false),
		  AimWorldX(0.0f),
		  AimWorldY(0.0f)
	{
	}
	
	void Reset()
	{
		MoveUp = false;
		MoveDown = false;
		MoveLeft = false;
		MoveRight = false;
		PrimaryFire = false;
		AltFire = false;
		HasMouse = false;
		AimWorldX = 0.0f;
		AimWorldY = 0.0f;
	}
};

#endif