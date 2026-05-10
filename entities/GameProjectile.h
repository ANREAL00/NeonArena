

#ifndef GameProjectileH
#define GameProjectileH

#include <System.Types.hpp>
#include <Vcl.Graphics.hpp>

class TBullet
{
private:
	TPointF position;
	TPointF startPosition;
	TPointF velocity;
	int damage;
	float lifeTime;
	float maxLifeTime;
	float maxRange;
	float radius;
	bool used;

public:
	TBullet(const TPointF &startPos, const TPointF &dir, int bulletDamage = 15,
		float bulletSpeed = 520.0f, float bulletRange = 200.0f, float bulletSize = 4.0f);
	void Update(float deltaTime);
	void Draw(TCanvas *canvas, const TPointF &camera) const;
	bool IsAlive() const;
	void MarkAsUsed() { used = true; }
	bool IsUsed() const { return used; }
	const TPointF &GetPosition() const;
	int GetDamage() const;
	float GetRadius() const { return radius; }
};

class TThrownProjectile
{
private:
	TPointF position;
	TPointF velocity;
	TPointF startPosition;
	TPointF targetPosition;
	int damage;
	float lifeTime;
	float maxLifeTime;
	float radius;
	bool hasLanded;
	bool hitPlayer;
	bool poolCreated;
	float gravity;

public:
	TThrownProjectile(const TPointF &startPos, const TPointF &targetPos, int projDamage = 20, float throwSpeed = 300.0f);
	void Update(float deltaTime);
	void Draw(TCanvas *canvas, const TPointF &camera) const;
	bool IsAlive() const;
	bool HasLanded() const { return hasLanded; }
	bool HitPlayer() const { return hitPlayer; }
	bool WasPoolCreated() const { return poolCreated; }
	void MarkPoolCreated() { poolCreated = true; }
	void MarkAsHitPlayer() { hitPlayer = true; hasLanded = true; }
	const TPointF &GetPosition() const { return position; }
	int GetDamage() const { return damage; }
	float GetRadius() const { return radius; }
	TPointF GetLandingPosition() const { return position; }
};

class TAcidPool
{
private:
	TPointF position;
	float radius;
	float lifeTime;
	float maxLifeTime;
	int damage;
	float damageCooldown;
	float currentDamageCooldown;

public:
	TAcidPool(const TPointF &pos, float poolRadius = 40.0f, float duration = 5.0f, int poolDamage = 10);
	void Update(float deltaTime);
	void Draw(TCanvas *canvas, const TPointF &camera) const;
	bool IsAlive() const;
	const TPointF &GetPosition() const { return position; }
	float GetRadius() const { return radius; }
	int GetDamage() const { return damage; }
	bool CanDealDamage() const;
	void ResetDamageCooldown();
};

#endif
