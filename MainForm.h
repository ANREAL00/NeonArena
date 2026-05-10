#ifndef MainFormH
#define MainFormH

#include <System.Classes.hpp>
#include <System.Types.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.Graphics.hpp>
#include <cstdint>
#include <memory>
#include <vector>

#include "ui\\GameUIRenderer.h"
#include "world\\GameWorld.h"
#include "core\\GameInput.h"
#include "core\\GameState.h"
#include "systems\\GameRecords.h"
#include "network\\GameNetwork.h"

inline bool PointInRect(const TRect &r, int x, int y)
{
	return x >= r.Left && x <= r.Right && y >= r.Top && y <= r.Bottom;
}

float NetInterpolationClockSeconds();

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

	NeonGame::TNetworkManager NetworkManager;
	std::vector<TInputState> HostLastSimulatedInputs;
	bool IsNetworkGameActive;
	uint32_t FrameNumber;
	float NetworkUpdateTimer;
	static constexpr float NetworkUpdateInterval = 1.0f / 60.0f;

	struct TPredictedState
	{
		TPointF Position;
		TPointF FacingDirection;
		uint32_t FrameNumber;
	};
	std::vector<TPredictedState> PredictedStates;

	struct TPlayerStateSnapshot
	{
		TPointF Position;
		TPointF FacingDirection;
		float Health;
		float MaxHealth;
		uint32_t Tick;
		float Timestamp;
	};

	struct TInterpolatedPlayer
	{
		std::vector<TPlayerStateSnapshot> StateBuffer;
		static constexpr size_t MaxBufferSize = 24;
		float LastUpdateTime;
		bool HasValidState;

		TInterpolatedPlayer() : LastUpdateTime(0.0f), HasValidState(false) {}
	};
	std::vector<TInterpolatedPlayer> InterpolatedPlayers;

	struct TEnemyNetSnapshot
	{
		TPointF Position;
		float Timestamp;
	};

	struct TInterpolatedEnemySlot
	{
		std::vector<TEnemyNetSnapshot> StateBuffer;
		static constexpr size_t MaxBufferSize = 24;
		float LastUpdateTime;
		bool HasValidState;

		TInterpolatedEnemySlot() : LastUpdateTime(0.0f), HasValidState(false) {}
	};
	std::vector<TInterpolatedEnemySlot> InterpolatedEnemies;
	std::vector<uint32_t> InterpEnemyLastNetInstanceId;

	std::vector<TPointF> HostRemoteAuthPos;
	std::vector<TPointF> HostRemoteAuthFacing;
	bool HostRemoteAuthValid;

	void RestoreHostRemoteAuthoritativePositions();
	void HostFeedRemoteSnapshotsAfterSim();

	void ResetGame();
	void ReturnToCoopLobby();
	void UpdateGame(double deltaSeconds);
	void RenderGame();

	void InitializeNetwork();
	void UpdateNetwork(float deltaTime);
	void SendPlayerInput();
	void ReceiveNetworkUpdates();
	void ApplyClientSidePrediction(float deltaTime);
	void InterpolateRemotePlayers(float deltaTime);
	void InterpolateNetworkEnemies(float deltaTime);
public:
	__fastcall TForm1(TComponent* Owner);
};

extern PACKAGE TForm1 *Form1;

#endif
