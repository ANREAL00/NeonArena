

#ifndef GamePlayerH
#define GamePlayerH

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
	float hitInvulnTimer;
	float levelUpInvulnTimer;

	int level;
	int experience;
	int experienceToNextLevel;

	float recoilVelX;
	float recoilVelY;

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

	void TakeDamage(int amount);
	void Heal(int amount);

	void NetworkRespawn(float centerX, float centerY, int resumeHealth);

	void SyncHealthFromNetwork(int newHealth, int newMaxHealth);
	bool IsAlive() const;
	int GetHealth() const { return health; }
	int GetMaxHealth() const { return maxHealth; }
	float GetHealthRatio() const;
	bool IsInvulnerable() const { return hitInvulnTimer > 0.0f || levelUpInvulnTimer > 0.0f; }
	void SetLevelUpInvulnerability(float duration) { levelUpInvulnTimer = duration; }

	void AddExperience(int amount);
	void ApplyAuthorityProgress(int authLevel, int authExperience);
	int GetLevel() const { return level; }
	int GetExperience() const { return experience; }
	int GetExperienceToNextLevel() const { return experienceToNextLevel; }
	float GetExperienceRatio() const;
	bool CheckLevelUp();

	void ApplySpeedMultiplier(float multiplier);
	void ApplyMaxHealthBonus(int bonus);
	float GetBaseSpeed() const { return 260.0f; }
	float GetCurrentSpeed() const { return speed; }

	void ApplyShotgunRecoil(float aimDirX, float aimDirY);
};

#endif
