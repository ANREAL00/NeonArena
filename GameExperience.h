//---------------------------------------------------------------------------
// Система опыта (Neon Arena)
//---------------------------------------------------------------------------
#ifndef GameExperienceH
#define GameExperienceH
//---------------------------------------------------------------------------

#include <System.Types.hpp>
#include <Vcl.Graphics.hpp>

//---------------------------------------------------------------------------
// Орб опыта, который падает с врагов
//---------------------------------------------------------------------------
class TExperienceOrb
{
private:
	TPointF position;
	float lifetime; // время жизни орба
	float animationTime; // время для анимации пульсации
	float pickupRadius; // радиус подбора
	int experienceValue; // количество опыта

public:
	TExperienceOrb(const TPointF &spawnPos, int expValue);
	void Update(float deltaTime, const TPointF &playerPos);
	void Draw(TCanvas *canvas, const TPointF &camera) const;
	bool IsAlive() const { return lifetime > 0.0f; }
	const TPointF &GetPosition() const { return position; }
	int GetExperienceValue() const { return experienceValue; }
	bool CheckPickup(const TPointF &playerPos, float playerRadius) const;
};

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

