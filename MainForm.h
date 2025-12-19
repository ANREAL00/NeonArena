//---------------------------------------------------------------------------

#ifndef MainFormH
#define MainFormH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <System.Types.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.Graphics.hpp>
#include <memory>
#include <vector>
#include <map>

#include "GameUIRenderer.h"
#include "GameWorld.h"
#include "GameInput.h"
#include "GameState.h"
#include "GameRecords.h"
#include "GameNetwork.h"

// вспомогательная функция для проверки попадания точки в прямоугольник
inline bool PointInRect(const TRect &r, int x, int y)
{
	return x >= r.Left && x <= r.Right && y >= r.Top && y <= r.Bottom;
}

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
	std::unique_ptr<TGameWorld> World;
	TDateTime LastTick;
	bool HasLastTick;

	TInputState InputState;
	EWorldState WorldState;

	TGameUIState UIState;
	TGameUIRenderer UiRenderer;
	TGameRecordsManager RecordsManager;
	
	// Сетевая система
	NeonGame::TNetworkManager NetworkManager;
	bool IsNetworkGameActive;
	uint32_t FrameNumber; // номер кадра для синхронизации
	float NetworkUpdateTimer; // таймер для обновления сети
	static constexpr float NetworkUpdateInterval = 0.05f; // обновление каждые 50ms (20 FPS)
	
	// Client-side prediction
	struct TPredictedState
	{
		TPointF Position;
		TPointF FacingDirection;
		uint32_t FrameNumber;
	};
	std::vector<TPredictedState> PredictedStates; // история предсказанных состояний
	
	// Интерполяция для других игроков
	struct TPlayerStateSnapshot
	{
		TPointF Position;
		TPointF FacingDirection;
		float Health;
		float MaxHealth;
		uint32_t Tick; // номер кадра с сервера
		float Timestamp; // время получения (локальное)
	};
	
	struct TInterpolatedPlayer
	{
		std::vector<TPlayerStateSnapshot> StateBuffer; // буфер состояний для интерполяции
		static constexpr size_t MaxBufferSize = 10; // максимум состояний в буфере
		float LastUpdateTime; // время последнего обновления
		bool HasValidState; // есть ли валидное состояние
		
		TInterpolatedPlayer() : LastUpdateTime(0.0f), HasValidState(false) {}
	};
	std::vector<TInterpolatedPlayer> InterpolatedPlayers;
	
	// Интерполяция для врагов
	struct TEnemyStateSnapshot
	{
		TPointF Position;
		uint32_t EnemyID;
		uint32_t Tick;
		float Timestamp;
	};
	
	struct TInterpolatedEnemy
	{
		std::vector<TEnemyStateSnapshot> StateBuffer;
		static constexpr size_t MaxBufferSize = 10;
		uint32_t EnemyID;
		bool IsActive;
		
		TInterpolatedEnemy() : EnemyID(0), IsActive(false) {}
	};
	std::map<uint32_t, TInterpolatedEnemy> InterpolatedEnemies; // ID -> данные
	
	float InterpolationClock; // локальное время интерполяции
	std::vector<TInputState> PersistentInputs; // сохраненные вводы для хоста

	void ResetGame();
	void UpdateGame(double deltaSeconds);
	void RenderGame();
	
	// Сетевые методы
	void InitializeNetwork();
	void UpdateNetwork(float deltaTime);
	void SendPlayerInput();
	void ReceiveNetworkUpdates(float deltaTime);
	void ApplyClientSidePrediction(float deltaTime);
	void InterpolateRemotePlayers(float deltaTime);
	void InterpolateEnemies(float deltaTime);
public:
	__fastcall TForm1(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
