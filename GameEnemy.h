//---------------------------------------------------------------------------
// Враги
//---------------------------------------------------------------------------
#ifndef GameEnemyH
#define GameEnemyH
//---------------------------------------------------------------------------

#include <System.Types.hpp>
#include <Vcl.Graphics.hpp>
#include <vector>

// forward declaration
class TBullet;

enum class EEnemyType : uint8_t
{
	Basic = 0,
	Fast = 1,
	Thrower = 2,
	Zigzag = 3,
	Kamikaze = 4,
	Shooting = 5,
	Boss = 6
};

class TEnemy
{
protected:
	TPointF position;
	uint32_t EnemyID; // уникальный ID для синхронизации
	float speed;
	int health;
	float baseSpeed; // базовая скорость для масштабирования
	int baseHealth; // базовое здоровье для масштабирования
	float scaledBaseSpeed; // скорость после масштабирования волной (для применения временных множителей)

public:
	virtual ~TEnemy() = default;
	virtual void Update(float deltaTime, const TPointF &playerPos) = 0;
	virtual void Draw(TCanvas *canvas, const TPointF &camera) const = 0;
	virtual bool IsAlive() const { return health > 0; }
	virtual void ApplyDamage(int dmg);
	const TPointF &GetPosition() const { return position; }
	int GetHealth() const { return health; }
	int GetMaxHealth() const { return baseHealth; }
	
	// масштабирование характеристик врага
	void ApplyScaling(float healthMultiplier, float speedMultiplier);
	
	// применение временного множителя скорости (для усилений при долгой волне)
	virtual void ApplyTemporarySpeedMultiplier(float multiplier);
	
	// получение базовой скорости (после масштабирования волной)
	float GetBaseSpeed() const { return baseSpeed; }
	
	// Методы для синхронизации
	void SetPosition(float px, float py) { position.X = px; position.Y = py; }
	void SetPosition(const TPointF &pos) { position = pos; }
	void SetHealth(int h) { health = h; }
	void SetEnemyID(uint32_t id) { EnemyID = id; }
	uint32_t GetEnemyID() const { return EnemyID; }
	virtual EEnemyType GetType() const = 0;
	virtual int GetBaseExperienceValue() const { return 10; } // по умолчанию 10 опыта
};
//---------------------------------------------------------------------------
class TBasicEnemy : public TEnemy
{
public:
	TBasicEnemy(const TPointF &spawnPos);
	void Update(float deltaTime, const TPointF &playerPos) override;
	void Draw(TCanvas *canvas, const TPointF &camera) const override;
	EEnemyType GetType() const override { return EEnemyType::Basic; }
};

//---------------------------------------------------------------------------
class TFastEnemy : public TEnemy
{
public:
	TFastEnemy(const TPointF &spawnPos);
	void Update(float deltaTime, const TPointF &playerPos) override;
	void Draw(TCanvas *canvas, const TPointF &camera) const override;
	EEnemyType GetType() const override { return EEnemyType::Fast; }
};

//---------------------------------------------------------------------------
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
	EEnemyType GetType() const override { return EEnemyType::Thrower; }
};

//---------------------------------------------------------------------------
class TZigzagEnemy : public TEnemy
{
private:
	float teleportTimer;
	float teleportCooldown;
	float blinkTimer; // таймер мерцания перед телепортацией
	bool isBlinking; // мерцает ли враг
	TPointF lastPosition; // последняя позиция для эффекта телепортации
public:
	TZigzagEnemy(const TPointF &spawnPos);
	void Update(float deltaTime, const TPointF &playerPos) override;
	void Draw(TCanvas *canvas, const TPointF &camera) const override;
	EEnemyType GetType() const override { return EEnemyType::Zigzag; }
};

//---------------------------------------------------------------------------
class TKamikazeEnemy : public TEnemy
{
public:
	TKamikazeEnemy(const TPointF &spawnPos);
	void Update(float deltaTime, const TPointF &playerPos) override;
	void Draw(TCanvas *canvas, const TPointF &camera) const override;
	EEnemyType GetType() const override { return EEnemyType::Kamikaze; }
};

//---------------------------------------------------------------------------
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
	EEnemyType GetType() const override { return EEnemyType::Shooting; }
};

//---------------------------------------------------------------------------
enum class EBossPhase
{
	Phase1,  // первая фаза - стрельба веером (улучшенная)
	Phase2,  // вторая фаза - стрельба по кругу (двойной круг)
	Phase3,  // третья фаза - стрельба спиралью + движение (двойная спираль)
	Phase4,  // четвертая фаза - комбинированная атака (веер + круг)
	Phase5   // пятая фаза - агрессивная атака (быстрое движение + множественные спирали)
};

//---------------------------------------------------------------------------
class TBossEnemy : public TEnemy
{
private:
	EBossPhase currentPhase;
	float phaseTransitionTimer;
	float shootTimer;
	float shootCooldown;
	float baseShootCooldown; // базовый кулдаун для масштабирования
	float movementTimer;
	float spiralAngle;
	int maxHealth;
	int appearanceLevel; // уровень появления (для масштабирования)
	float stunTimer; // таймер остановки после нанесения урона игроку
	
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
	
	// для создания пуль босса
	void CreateBullets(const TPointF &playerPos, std::vector<TBullet> &bullets, float damageMultiplier = 1.0f);
	EBossPhase GetPhase() const { return currentPhase; }
	float GetHealthRatio() const;
	
	// переопределяем для учета фазовых модификаций скорости
	void ApplyTemporarySpeedMultiplier(float multiplier) override;
	
	// для остановки после нанесения урона
	void StartStun(float duration = 0.25f);
	bool IsStunned() const { return stunTimer > 0.0f; }
	EEnemyType GetType() const override { return EEnemyType::Boss; }
	
	// для синхронизации конкретно босса
	void SetPhase(EBossPhase phase) { currentPhase = phase; }
	void SetMaxHealth(int mh) { maxHealth = mh; }
};

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

