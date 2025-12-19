//---------------------------------------------------------------------------
// Игрок (Neon Arena)
//---------------------------------------------------------------------------
#ifndef GamePlayerH
#define GamePlayerH
//---------------------------------------------------------------------------

#include <System.Types.hpp>
#include <Vcl.Graphics.hpp>

class TGamePlayer
{
private:
	float x;
	float y;
	float speed;
	int health;
	int maxHealth;
	TPointF facingDirection;
	float hitInvulnTimer; // время оставшейся неуязвимости
	float levelUpInvulnTimer; // невосприимчивость к урону на 5 секунд при получении уровня
	float primaryFireTimer;
	float altFireTimer;

	// система опыта и уровней
	int level;
	int experience;
	int experienceToNextLevel;
	int pendingUpgradeCount;

public:
	bool inputUp;
	bool inputDown;
	bool inputLeft;
	bool inputRight;
	bool isShooting;

	TGamePlayer();
	void SetPosition(float px, float py);
	void Update(float deltaTime, const TRectF &bounds);
	void Draw(TCanvas *canvas, const TPointF &camera);
	TPointF GetPosition() const;
	void SetFacingDirection(const TPointF &dir);
	TPointF GetFacingDirection() const { return facingDirection; }

	// здоровье
	void TakeDamage(int amount);
	void Heal(int amount);
	bool IsAlive() const;
	int GetHealth() const { return health; }
	int GetMaxHealth() const { return maxHealth; }
	float GetHealthRatio() const;
	bool IsInvulnerable() const { return hitInvulnTimer > 0.0f || levelUpInvulnTimer > 0.0f; }
	void SetLevelUpInvulnerability(float duration) { levelUpInvulnTimer = duration; }
	void SetHealth(int h) { health = h; }
	void SetMaxHealth(int mh) { maxHealth = mh; }

	// опыт и уровни
	void AddExperience(int amount);
	int GetLevel() const { return level; }
	int GetExperience() const { return experience; }
	int GetExperienceToNextLevel() const { return experienceToNextLevel; }
	float GetExperienceRatio() const;
	bool CheckLevelUp(); // возвращает true, если произошёл level up
	void SetLevel(int l) { level = l; }
	void SetExperience(int e) { experience = e; }
	void SetExperienceToNextLevel(int e) { experienceToNextLevel = e; }

	// модификаторы улучшений
	void ApplySpeedMultiplier(float multiplier);
	void ApplyMaxHealthBonus(int bonus);
	float GetBaseSpeed() const { return 260.0f; }
	float GetCurrentSpeed() const { return speed; }

	// таймеры стрельбы
	float GetPrimaryFireTimer() const { return primaryFireTimer; }
	void SetPrimaryFireTimer(float t) { primaryFireTimer = t; }
	float GetAltFireTimer() const { return altFireTimer; }
	void SetAltFireTimer(float t) { altFireTimer = t; }
	
	// Отложенные улучшения (для кооператива)
	int GetPendingUpgradeCount() const { return pendingUpgradeCount; }
	void IncrementPendingUpgrades() { pendingUpgradeCount++; }
	void ClearPendingUpgrades() { pendingUpgradeCount = 0; }
};

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

