//---------------------------------------------------------------------------

#ifndef Unit1H
#define Unit1H
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <System.Types.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.Graphics.hpp>
#include <memory>
#include <vector>

#include "GamePlayer.h"
#include "GameProjectile.h"
#include "GameEnemy.h"
#include "GameConstants.h"
//---------------------------------------------------------------------------
class TForm1 : public TForm
{
__published:
	TPaintBox *GameCanvas;
	TTimer *GameTimer;

	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormKeyDown(TObject *Sender, WORD &Key, TShiftState Shift);
	void __fastcall FormKeyUp(TObject *Sender, WORD &Key, TShiftState Shift);
	void __fastcall GameTimerTimer(TObject *Sender);
	void __fastcall GameCanvasPaint(TObject *Sender);
	void __fastcall GameCanvasMouseMove(TObject *Sender, TShiftState Shift, int X, int Y);
	void __fastcall GameCanvasMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift, int X, int Y);
	void __fastcall GameCanvasMouseUp(TObject *Sender, TMouseButton Button, TShiftState Shift, int X, int Y);
private:
	std::unique_ptr<TGamePlayer> Player;
	TDateTime LastTick;
	bool HasLastTick;

	std::vector<TBullet> Bullets;
	std::vector<std::unique_ptr<TEnemy>> Enemies;
	float EnemySpawnTimer;

	TPoint MousePosScreen;
	bool HasMousePos;

	bool PrimaryMouseDown;
	bool AltMouseDown;

	float PrimaryFireCooldown;
	float AltFireCooldown;
	float PrimaryFireTimer;
	float AltFireTimer;

	void UpdateGame(double deltaSeconds);
	void RenderGame();
public:
	__fastcall TForm1(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
