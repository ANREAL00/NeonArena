//---------------------------------------------------------------------------
// Система ввода (Neon Arena)
//---------------------------------------------------------------------------
#ifndef GameInputH
#define GameInputH
//---------------------------------------------------------------------------

#include <System.Types.hpp>

//---------------------------------------------------------------------------
// Состояние ввода игрока
//---------------------------------------------------------------------------
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
	
	// Инициализация
	TInputState()
		: MoveUp(false),
		  MoveDown(false),
		  MoveLeft(false),
		  MoveRight(false),
		  PrimaryFire(false),
		  AltFire(false),
		  MouseClient(0, 0),
		  HasMouse(false)
	{
	}
	
	// Сброс состояния
	void Reset()
	{
		MoveUp = false;
		MoveDown = false;
		MoveLeft = false;
		MoveRight = false;
		PrimaryFire = false;
		AltFire = false;
		HasMouse = false;
	}
};

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------



