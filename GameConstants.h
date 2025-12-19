//---------------------------------------------------------------------------
// Общие константы и типы игры Neon Arena
//---------------------------------------------------------------------------
#ifndef GameConstantsH
#define GameConstantsH
//---------------------------------------------------------------------------

#include <System.Types.hpp>

namespace NeonGame
{
	constexpr float PlayerRadius  = 18.0f;
	constexpr float BulletSpeed   = 520.0f;
	constexpr float BulletRadius  = 4.0f;
	constexpr float BulletMaxRange = 150.0f; // максимальная дальность пули (уменьшена)
	constexpr float EnemyRadius   = 20.0f;

	constexpr int   EnemyContactDamage = 15; // увеличен урон от контакта
	constexpr float PlayerHitInvulnTime = 0.6f; // уменьшено время неуязвимости
	constexpr float PrimaryFireCooldown = 0.35f; // скорострельность (увеличена для усложнения)
	constexpr float AltFireCooldown = 1.0f; // кулдаун альтернативной атаки (увеличен)

	constexpr float WorldWidth    = 2667.0f; // уменьшено в 3 раза (было 8000)
	constexpr float WorldHeight   = 2667.0f; // уменьшено в 3 раза (было 8000)

	inline TRectF WorldBounds()
	{
		return TRectF(0.0f, 0.0f, WorldWidth, WorldHeight);
	}
}

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

