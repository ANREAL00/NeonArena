//---------------------------------------------------------------------------
// Пули / снаряды
//---------------------------------------------------------------------------
#ifndef GameProjectileH
#define GameProjectileH
//---------------------------------------------------------------------------

#include <System.Types.hpp>
#include <Vcl.Graphics.hpp>

class TBullet
{
private:
	TPointF position;
	TPointF startPosition; // стартовая позиция для расчёта дальности
	TPointF velocity;
	int damage;
	float lifeTime;
	float maxLifeTime;
	float maxRange; // максимальная дальность
	float radius; // размер пули
	bool used; // была ли пуля использована (попала во врага)
	uint8_t ownerID; // ID игрока, выпустившего пулю

public:
	TBullet(const TPointF &startPos, const TPointF &dir, int bulletDamage = 15, 
		float bulletSpeed = 520.0f, float bulletRange = 200.0f, float bulletSize = 4.0f, uint8_t owner = 255);
	void Update(float deltaTime);
	void Draw(TCanvas *canvas, const TPointF &camera) const;
	bool IsAlive() const;
	void MarkAsUsed() { used = true; }
	bool IsUsed() const { return used; }
	const TPointF &GetPosition() const;
	int GetDamage() const;
	float GetRadius() const { return radius; }
	uint8_t GetOwnerID() const { return ownerID; }
};

//---------------------------------------------------------------------------
// Снаряд метателя (бросается по дуге)
class TThrownProjectile
{
private:
	TPointF position;
	TPointF velocity;
	TPointF startPosition;
	TPointF targetPosition; // целевая позиция приземления
	int damage;
	float lifeTime;
	float maxLifeTime;
	float radius;
	bool hasLanded; // приземлился ли снаряд
	bool hitPlayer; // попал ли в игрока (не создаем лужу)
	bool poolCreated; // была ли создана лужа (чтобы не создавать несколько раз)
	float gravity; // гравитация для дуги
	
public:
	TThrownProjectile(const TPointF &startPos, const TPointF &targetPos, int projDamage = 20, float throwSpeed = 300.0f);
	void Update(float deltaTime);
	void Draw(TCanvas *canvas, const TPointF &camera) const;
	bool IsAlive() const;
	bool HasLanded() const { return hasLanded; }
	bool HitPlayer() const { return hitPlayer; }
	bool WasPoolCreated() const { return poolCreated; }
	void MarkPoolCreated() { poolCreated = true; }
	void MarkAsHitPlayer() { hitPlayer = true; hasLanded = true; } // помечаем как попавший в игрока
	const TPointF &GetPosition() const { return position; }
	int GetDamage() const { return damage; }
	float GetRadius() const { return radius; }
	TPointF GetLandingPosition() const { return position; } // возвращаем реальную позицию приземления
};

//---------------------------------------------------------------------------
// Лужа кислоты (остается после приземления снаряда)
class TAcidPool
{
private:
	TPointF position;
	float radius;
	float lifeTime;
	float maxLifeTime;
	int damage; // урон при наступлении
	float damageCooldown; // кулдаун между уронами
	float currentDamageCooldown;
	
public:
	TAcidPool(const TPointF &pos, float poolRadius = 40.0f, float duration = 5.0f, int poolDamage = 10);
	void Update(float deltaTime);
	void Draw(TCanvas *canvas, const TPointF &camera) const;
	bool IsAlive() const;
	const TPointF &GetPosition() const { return position; }
	float GetRadius() const { return radius; }
	int GetDamage() const { return damage; }
	bool CanDealDamage() const; // можно ли нанести урон (проверка кулдауна)
	void ResetDamageCooldown(); // сброс кулдауна после нанесения урона
};

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

