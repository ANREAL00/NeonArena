


#ifndef GameExperienceH
#define GameExperienceH


#include <System.Types.hpp>
#include <Vcl.Graphics.hpp>
#include <cstdint>

class TExperienceOrb
{
private:
	TPointF position;
	float lifetime;
	float animationTime;
	float pickupRadius;
	int experienceValue;
	uint32_t netInstanceId = 0;

public:
	TExperienceOrb(const TPointF &spawnPos, int expValue, uint32_t netId = 0, float initialLifetime = 30.0f);
	void Update(float deltaTime, const TPointF &playerPos);
	void Draw(TCanvas *canvas, const TPointF &camera) const;
	bool IsAlive() const { return lifetime > 0.0f; }
	const TPointF &GetPosition() const { return position; }
	int GetExperienceValue() const { return experienceValue; }
	uint32_t GetNetInstanceId() const { return netInstanceId; }
	float GetLifetime() const { return lifetime; }
	void SetLifetime(float t) { lifetime = t; }
	bool CheckPickup(const TPointF &playerPos, float playerRadius) const;
};


#endif


