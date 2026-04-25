#ifndef GameConstantsH
#define GameConstantsH

#include <System.Types.hpp>

namespace NeonGame
{
	constexpr float PlayerRadius  = 18.0f;
	constexpr float BulletSpeed   = 520.0f;
	constexpr float BulletRadius  = 4.0f;
	constexpr float BulletMaxRange = 150.0f; 
	constexpr float EnemyRadius   = 20.0f;

	constexpr int   EnemyContactDamage = 15; 
	constexpr float PlayerHitInvulnTime = 0.6f; 
	constexpr float PrimaryFireCooldown = 0.35f; 
	constexpr float AltFireCooldown = 1.0f; 

	
	constexpr float ShotgunRecoilBaseSpeed = 360.0f;
	constexpr float PlayerRecoilDamping = 12.0f;

	constexpr float WorldWidth    = 2667.0f; 
	constexpr float WorldHeight   = 2667.0f; 

	inline TRectF WorldBounds()
	{
		return TRectF(0.0f, 0.0f, WorldWidth, WorldHeight);
	}
}

#endif