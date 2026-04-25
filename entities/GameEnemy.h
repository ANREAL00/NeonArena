#ifndef GameEnemyH
#define GameEnemyH


#include <System.Types.hpp>
#include <Vcl.Graphics.hpp>
#include <cstdint>
#include <vector>


class TBullet;

class TEnemy
{
protected:
	TPointF position;
	float speed;
	int health;
	float baseSpeed;
	int baseHealth;
	float scaledBaseSpeed;
	uint32_t netInstanceId = 0;

public:
	virtual ~TEnemy() = default;
	virtual void Update(float deltaTime, const TPointF &playerPos) = 0;
	virtual void Draw(TCanvas *canvas, const TPointF &camera) const = 0;
	virtual bool IsAlive() const { return health > 0; }
	virtual void ApplyDamage(int dmg);
	const TPointF &GetPosition() const { return position; }
	int GetHealth() const { return health; }
	int GetMaxHealth() const { return baseHealth; }
	void SetPosition(const TPointF &newPosition) { position = newPosition; }
	void SetHealth(int newHealth) { health = newHealth; }

	uint32_t GetNetInstanceId() const { return netInstanceId; }
	void SetNetInstanceId(uint32_t id) { netInstanceId = id; }

	void ApplyScaling(float healthMultiplier, float speedMultiplier);

	virtual void ApplyTemporarySpeedMultiplier(float multiplier);

	float GetBaseSpeed() const { return baseSpeed; }
};

class TBasicEnemy : public TEnemy
{
public:
	TBasicEnemy(const TPointF &spawnPos);
	void Update(float deltaTime, const TPointF &playerPos) override;
	void Draw(TCanvas *canvas, const TPointF &camera) const override;
};


class TFastEnemy : public TEnemy
{
public:
	TFastEnemy(const TPointF &spawnPos);
	void Update(float deltaTime, const TPointF &playerPos) override;
	void Draw(TCanvas *canvas, const TPointF &camera) const override;
};


class TThrowerEnemy : public TEnemy
{
private:
	float throwTimer;
	float throwCooldown;
public:
	TThrowerEnemy(const TPointF &spawnPos);
	void Update(float deltaTime, const TPointF &playerPos) override;
	void Draw(TCanvas *canvas, const TPointF &camera) const override;
	float GetThrowTimer() const { return throwTimer; }
	float GetThrowCooldown() const { return throwCooldown; }
	void ResetThrowTimer() { throwTimer = throwCooldown; }
};


class TZigzagEnemy : public TEnemy
{
private:
	float teleportTimer;
	float teleportCooldown;
	float blinkTimer; 
	bool isBlinking; 
	TPointF lastPosition; 
public:
	TZigzagEnemy(const TPointF &spawnPos);
	void Update(float deltaTime, const TPointF &playerPos) override;
	void Draw(TCanvas *canvas, const TPointF &camera) const override;
};


class TKamikazeEnemy : public TEnemy
{
public:
	TKamikazeEnemy(const TPointF &spawnPos);
	void Update(float deltaTime, const TPointF &playerPos) override;
	void Draw(TCanvas *canvas, const TPointF &camera) const override;
};


class TShootingEnemy : public TEnemy
{
private:
	float shootTimer;
	float shootCooldown;
public:
	TShootingEnemy(const TPointF &spawnPos);
	void Update(float deltaTime, const TPointF &playerPos) override;
	void Draw(TCanvas *canvas, const TPointF &camera) const override;
	float GetShootTimer() const { return shootTimer; }
	float GetShootCooldown() const { return shootCooldown; }
	void ResetShootTimer() { shootTimer = shootCooldown; }
};


enum class EBossPhase
{
	Phase1,  
	Phase2,  
	Phase3,  
	Phase4,  
	Phase5   
};


class TBossEnemy : public TEnemy
{
private:
	EBossPhase currentPhase;
	float phaseTransitionTimer;
	float shootTimer;
	float shootCooldown;
	float baseShootCooldown; 
	float movementTimer;
	float spiralAngle;
	int maxHealth;
	int appearanceLevel; 
	float stunTimer; 
	
	void UpdatePhase(float deltaTime, const TPointF &playerPos);
	void ShootFanPattern(const TPointF &playerPos, std::vector<TBullet> &bullets, float damageMultiplier = 1.0f);
	void ShootCirclePattern(const TPointF &playerPos, std::vector<TBullet> &bullets, float damageMultiplier = 1.0f);
	void ShootSpiralPattern(const TPointF &playerPos, std::vector<TBullet> &bullets, float damageMultiplier = 1.0f);
	void ShootCombinedPattern(const TPointF &playerPos, std::vector<TBullet> &bullets, float damageMultiplier = 1.0f);
	void ShootAggressivePattern(const TPointF &playerPos, std::vector<TBullet> &bullets, float damageMultiplier = 1.0f);
	
public:
	TBossEnemy(const TPointF &spawnPos, int appearanceLevel = 0);
	void Update(float deltaTime, const TPointF &playerPos) override;
	void Draw(TCanvas *canvas, const TPointF &camera) const override;
	void ApplyDamage(int dmg) override;
	
	
	void CreateBullets(const TPointF &playerPos, std::vector<TBullet> &bullets, float damageMultiplier = 1.0f);
	EBossPhase GetPhase() const { return currentPhase; }
	void SetPhaseFromNetwork(uint8_t phase);
	float GetHealthRatio() const;
	
	
	void ApplyTemporarySpeedMultiplier(float multiplier) override;
	
	
	void StartStun(float duration = 0.25f);
	bool IsStunned() const { return stunTimer > 0.0f; }
};


#endif


