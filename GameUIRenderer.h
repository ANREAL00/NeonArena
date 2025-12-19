//---------------------------------------------------------------------------
// HUD и экраны (Neon Arena)
//---------------------------------------------------------------------------
#ifndef GameUIRendererH
#define GameUIRendererH
//---------------------------------------------------------------------------

#include <System.Types.hpp>
#include <Vcl.Graphics.hpp>
#include <vector>
#include <algorithm>
#include "GameRecords.h"

struct TGameStats
{
	int CurrentWave;
	int EnemiesDefeated;
	float RunTimeSeconds;
};

struct TPlayerStats
{
	float DamageMultiplier;
	float FireRateMultiplier;
	float BulletRangeMultiplier;
	float BulletSizeMultiplier;
	float BulletSpeedMultiplier;
	bool HasPierce;
	float AltDamageMultiplier;
	float AltFireRateMultiplier;
	int AltSpreadShotCount;
	float AltBulletRangeMultiplier;
	float AltBulletSizeMultiplier;
	float AltBulletSpeedMultiplier;
	float SpeedMultiplier;
	float ExperienceGainMultiplier;
	// Новые улучшения
	float HealthRegenPerWave;
	float CriticalChancePercent;
	float DamageReductionPercent;
	float LuckPercent;
	float LifestealChancePercent;
};

struct TUpgrade; // forward declaration

struct TGameUIState
{
	bool IsGameOver;
	float OverlayAlpha;
	TRect RestartButtonRect;
	TRect MenuButtonRect;
	bool RestartButtonHover;
	bool MenuButtonHover;

	// состояние меню улучшений
	std::vector<TRect> UpgradeButtonRects; // 3 кнопки
	std::vector<bool> UpgradeButtonHovers; // 3 флага наведения
	std::vector<float> UpgradeButtonHoverTime; // время наведения для анимации
	
	// состояние главного меню
	TRect StartButtonRect;
	TRect ExitButtonRect;
	TRect CoopButtonRect; // кнопка "Кооператив" в главном меню
	bool StartButtonHover;
	bool ExitButtonHover;
	bool CoopButtonHover; // hover для кнопки "Кооператив" в главном меню
	
	// состояние меню паузы
	TRect ResumeButtonRect;
	TRect PauseMenuButtonRect;
	bool ResumeButtonHover;
	bool PauseMenuButtonHover;
	
	// состояние панели характеристик
	bool ShowStatsPanel; // показывать ли панель характеристик (TAB)
	
	// состояние меню кооператива
	TRect CreateGameButtonRect;
	TRect JoinGameButtonRect;
	TRect BackButtonRect;
	TRect StartGameButtonRect;
	TRect IPInputRect;
	bool CreateGameButtonHover;
	bool JoinGameButtonHover;
	bool BackButtonHover;
	bool StartGameButtonHover;
	std::string IPAddress; // введенный IP адрес
	bool IPInputFocused;
	std::vector<std::string> PlayerNames; // имена подключенных игроков
	std::vector<bool> PlayerReady; // готовность игроков
};

class TGameUIRenderer
{
private:
	// базовое разрешение для масштабирования (1920x1080)
	static constexpr int BaseWidth = 1920;
	static constexpr int BaseHeight = 1080;
	
	// вспомогательные функции для масштабирования
	float GetScaleX(int canvasWidth) const { return static_cast<float>(canvasWidth) / static_cast<float>(BaseWidth); }
	float GetScaleY(int canvasHeight) const { return static_cast<float>(canvasHeight) / static_cast<float>(BaseHeight); }
	float GetScale(int canvasWidth, int canvasHeight) const 
	{ 
		// используем минимальный масштаб для сохранения пропорций
		return std::min(GetScaleX(canvasWidth), GetScaleY(canvasHeight));
	}
	int ScaleFontSize(int baseSize, int canvasWidth, int canvasHeight) const
	{
		const float scale = GetScale(canvasWidth, canvasHeight);
		int size = static_cast<int>(baseSize * scale);
		// легкое сглаживание масштабирования + защита от слишком мелкого и слишком крупного текста
		if (scale > 1.5f)
			size = static_cast<int>(baseSize * (1.0f + (scale - 1.0f) * 0.6f));
		else if (scale < 0.75f)
			size = static_cast<int>(baseSize * (0.75f + (scale - 0.75f) * 0.8f));
		// кламп по разумным пределам
		size = std::clamp(size, 10, 48);
		return size;
	}
	int ScaleValue(int baseValue, int canvasWidth, int canvasHeight) const
	{
		return static_cast<int>(baseValue * GetScale(canvasWidth, canvasHeight));
	}
	float ScaleValueF(float baseValue, int canvasWidth, int canvasHeight) const
	{
		return baseValue * GetScale(canvasWidth, canvasHeight);
	}
	
public:
	void DrawHud(TCanvas *canvas,
		float healthRatio, int health, int maxHealth,
		float experienceRatio, int experience, int experienceToNext,
		int playerLevel,
		const TGameStats &stats,
		bool showStatsPanel) const;
	void DrawGameOver(TCanvas *canvas,
		const TGameStats &stats,
		const TGameRecords &records,
		TGameUIState &uiState) const;
	void DrawUpgradeMenu(TCanvas *canvas,
		const std::vector<TUpgrade> &upgrades,
		TGameUIState &uiState) const;
	void DrawLevelUpNotification(TCanvas *canvas,
		float timer, int level) const;
	void DrawPlayerStats(TCanvas *canvas,
		const TPlayerStats &stats,
		const TGameUIState &uiState) const;
	void DrawBossHealthBar(TCanvas *canvas, float healthRatio, int health, int maxHealth) const;
	void DrawMainMenu(TCanvas *canvas, const TGameRecords &records, TGameUIState &uiState) const;
	void DrawPauseMenu(TCanvas *canvas, TGameUIState &uiState) const;
	void DrawCoopMenu(TCanvas *canvas, TGameUIState &uiState, 
	                  const void *networkManager) const; // используем void* для избежания проблем с forward declaration
	void DrawCooldownIndicators(TCanvas *canvas, float primaryCooldown, float altCooldown, float primaryMax, float altMax) const;
	void DrawMinimap(TCanvas *canvas, 
		const TPointF &playerPos,
		const std::vector<TPointF> &enemyPositions,
		const TPointF &bossPos,
		bool hasBoss,
		float worldWidth,
		float worldHeight) const;
	void DrawMinimap(TCanvas *canvas, const std::vector<TPointF> &playerPositions,
	                 const std::vector<TPointF> &enemyPositions,
	                 const TPointF &bossPos, bool hasBoss,
	                 float worldWidth, float worldHeight,
	                 uint8_t localPlayerID) const;
	void DrawPlayerNames(TCanvas *canvas, const std::vector<TPointF> &playerPositions,
	                    const std::vector<std::string> &playerNames,
	                    const TPointF &cameraPos, uint8_t localPlayerID) const;
};

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

