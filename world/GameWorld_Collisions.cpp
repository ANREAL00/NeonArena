#include <vcl.h>
#pragma hdrstop

#include "GameWorld.h"
#include "GameWorld_Utils.h"
#include <algorithm>

static bool RollChance(float chance)
{
	return chance > 0.0f &&
		(static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) < chance;
}

using namespace NeonGame;

void TGameWorld::UpdateCollisions()
{
	const bool serverMulti = IsNetworkGame && IsServer;
	TGamePlayer* localPlayer = GetLocalPlayer();
	if (!serverMulti && !localPlayer)
		return;

	auto onPlayerKilled = [this]()
	{
		if (IsNetworkGame)
		{
			if (NeonGameWorldUtils::AreAllPlayersDead(Players))
				WorldState = EWorldState::GameOver;
		}
		else
		{
			WorldState = EWorldState::GameOver;
		}
	};

	TGamePlayer* lifestealRecipient = localPlayer;
	if (serverMulti)
	{
		lifestealRecipient = nullptr;
		for (auto &p : Players)
		{
			if (p && p->IsAlive())
			{
				lifestealRecipient = p.get();
				break;
			}
		}
	}

	auto forEachDamageablePlayer = [&](auto &&fn)
	{
		if (serverMulti)
		{
			for (size_t i = 0; i < Players.size(); ++i)
			{
				if (!Players[i] || !Players[i]->IsAlive())
					continue;
				fn(Players[i].get(), static_cast<uint8_t>(i));
			}
		}
		else
		{
			if (localPlayer && localPlayer->IsAlive())
				fn(localPlayer, LocalPlayerID);
		}
	};

	for (auto &b : EnemyBullets)
	{
		if (!b.IsAlive() || b.IsUsed())
			continue;

		const TPointF &bp = b.GetPosition();
		const float bulletRadius = b.GetRadius();
		const float hitRadius = PlayerRadius + bulletRadius;
		const float hitRadiusSq = hitRadius * hitRadius;

		bool consumed = false;
		forEachDamageablePlayer([&](TGamePlayer *pl, uint8_t pid)
		{
			if (consumed || pl->IsInvulnerable())
				return;

			const TPointF playerPos = pl->GetPosition();
			const float dx = bp.X - playerPos.X;
			const float dy = bp.Y - playerPos.Y;
			if (dx * dx + dy * dy > hitRadiusSq)
				return;

			const float dr = NeonGameWorldUtils::GetLocalDamageReductionPercent(
				IsNetworkGame, pid, PlayerUpgradeManagers, UpgradeManager);
			const int damage = NeonGameWorldUtils::ApplyDamageReduction(b.GetDamage(), dr);
			pl->TakeDamage(damage);
			b.MarkAsUsed();
			consumed = true;
			if (!serverMulti || pid == LocalPlayerID)
				Camera.AddShake(4.5f, 0.12f);
			if (!pl->IsAlive())
			{
				onPlayerKilled();
			}
		});

		if (WorldState == EWorldState::GameOver)
			return;
	}

	for (auto &pool : AcidPools)
	{
		if (!pool.IsAlive() || !pool.CanDealDamage())
			continue;

		const TPointF &poolPos = pool.GetPosition();
		const float poolRadius = pool.GetRadius();
		const float hitRadius = PlayerRadius + poolRadius;
		const float hitRadiusSq = hitRadius * hitRadius;

		bool hitAnyone = false;
		forEachDamageablePlayer([&](TGamePlayer *pl, uint8_t pid)
		{
			if (pl->IsInvulnerable())
				return;

			const TPointF playerPos = pl->GetPosition();
			const float dx = poolPos.X - playerPos.X;
			const float dy = poolPos.Y - playerPos.Y;
			if (dx * dx + dy * dy > hitRadiusSq)
				return;

			const float dr = NeonGameWorldUtils::GetLocalDamageReductionPercent(
				IsNetworkGame, pid, PlayerUpgradeManagers, UpgradeManager);
			const int damage = NeonGameWorldUtils::ApplyDamageReduction(pool.GetDamage(), dr);
			pl->TakeDamage(damage);
			hitAnyone = true;
			if (!serverMulti || pid == LocalPlayerID)
				Camera.AddShake(3.5f, 0.1f);
			if (!pl->IsAlive())
			{
				onPlayerKilled();
			}
		});

		if (hitAnyone)
			pool.ResetDamageCooldown();

		if (WorldState == EWorldState::GameOver)
			return;
	}

	if (Boss && Boss->IsAlive())
	{
		const TPointF &bossPos = Boss->GetPosition();
		const float bossRadius = EnemyRadius * 2.5f;

		for (auto &b : Bullets)
		{
			if (!b.IsAlive() || b.IsUsed())
				continue;

			const TPointF &bp = b.GetPosition();
			const float bulletRadius = b.GetRadius();
			const float hitRadius = bossRadius + bulletRadius;
			const float hitRadiusSq = hitRadius * hitRadius;
			const float dx = bp.X - bossPos.X;
			const float dy = bp.Y - bossPos.Y;
			if (dx * dx + dy * dy <= hitRadiusSq)
			{
				int damage = b.GetDamage();
				if (RollChance(UpgradeManager.GetCriticalChancePercent() / 100.0f))
					damage *= 2;
				Boss->ApplyDamage(damage);
				b.MarkAsUsed();

				Camera.AddShake(3.0f, 0.1f);
				if (!Boss->IsAlive())
				{
					if (lifestealRecipient && lifestealRecipient->IsAlive() &&
						RollChance(UpgradeManager.GetLifestealChancePercent() / 100.0f))
					{
						lifestealRecipient->Heal(5);
					}

					const int expValue = 100;
					SpawnExperienceOrb(bossPos, expValue);
					Stats.EnemiesDefeated++;

					Camera.AddShake(8.0f, 0.3f);
				}
				break;
			}
		}
	}

	const float contactRadius = PlayerRadius + EnemyRadius;
	const float contactRadiusSq = contactRadius * contactRadius;
	for (auto &e : Enemies)
	{
		if (!e || !e->IsAlive())
			continue;

		const TPointF &ep = e->GetPosition();
		forEachDamageablePlayer([&](TGamePlayer *pl, uint8_t pid)
		{
			if (pl->IsInvulnerable())
				return;

			const TPointF playerPos = pl->GetPosition();
			const float dx = ep.X - playerPos.X;
			const float dy = ep.Y - playerPos.Y;
			if (dx * dx + dy * dy > contactRadiusSq)
				return;

			int damage = EnemyContactDamage;
			const float dr = NeonGameWorldUtils::GetLocalDamageReductionPercent(
				IsNetworkGame, pid, PlayerUpgradeManagers, UpgradeManager);
			damage = NeonGameWorldUtils::ApplyDamageReduction(damage, dr);
			pl->TakeDamage(damage);

			if (!serverMulti || pid == LocalPlayerID)
				Camera.AddShake(5.0f, 0.15f);
			if (!pl->IsAlive())
			{
				onPlayerKilled();
			}
		});

		if (WorldState == EWorldState::GameOver)
			return;
	}

	if (Boss && Boss->IsAlive())
	{
		const TPointF &bossPos = Boss->GetPosition();
		const float bossRadius = EnemyRadius * 2.5f;
		const float bossContactRadius = PlayerRadius + bossRadius;
		const float bossContactRadiusSq = bossContactRadius * bossContactRadius;

		forEachDamageablePlayer([&](TGamePlayer *pl, uint8_t pid)
		{
			if (pl->IsInvulnerable())
				return;

			const TPointF playerPos = pl->GetPosition();
			const float dx = bossPos.X - playerPos.X;
			const float dy = bossPos.Y - playerPos.Y;
			if (dx * dx + dy * dy > bossContactRadiusSq)
				return;

			int bossDamage = EnemyContactDamage * (2 + Spawner.GetBossAppearanceCount());
			const float dr = NeonGameWorldUtils::GetLocalDamageReductionPercent(
				IsNetworkGame, pid, PlayerUpgradeManagers, UpgradeManager);
			bossDamage = NeonGameWorldUtils::ApplyDamageReduction(bossDamage, dr);
			pl->TakeDamage(bossDamage);

			if (!Boss->IsStunned())
			{
				Boss->StartStun(0.25f);
			}

			if (!serverMulti || pid == LocalPlayerID)
				Camera.AddShake(6.0f, 0.2f);
			if (!pl->IsAlive())
			{
				onPlayerKilled();
			}
		});

		if (WorldState == EWorldState::GameOver)
			return;
	}

	const bool hasPierce = UpgradeManager.GetHasPierce();
	for (auto &e : Enemies)
	{
		if (!e || !e->IsAlive())
			continue;

		for (auto &b : Bullets)
		{
			if (!b.IsAlive() || b.IsUsed())
				continue;

			const TPointF &ep = e->GetPosition();
			const TPointF &bp = b.GetPosition();
			const float bulletRadius = b.GetRadius();
			const float hitRadius = EnemyRadius + bulletRadius;
			const float hitRadiusSq = hitRadius * hitRadius;
			const float dx = ep.X - bp.X;
			const float dy = ep.Y - bp.Y;
			if (dx * dx + dy * dy <= hitRadiusSq)
			{
				int damage = b.GetDamage();
				if (RollChance(UpgradeManager.GetCriticalChancePercent() / 100.0f))
					damage *= 2;
				e->ApplyDamage(damage);
				if (!hasPierce)
				{
					b.MarkAsUsed();
					break;
				}
			}
		}
	}

	Enemies.erase(
		std::remove_if(Enemies.begin(), Enemies.end(),
			[this, lifestealRecipient](const std::unique_ptr<TEnemy> &enemy)
			{
				if (!enemy)
					return true;
				if (!enemy->IsAlive())
				{
					Stats.EnemiesDefeated++;

					if (lifestealRecipient && lifestealRecipient->IsAlive() &&
						RollChance(UpgradeManager.GetLifestealChancePercent() / 100.0f))
					{
						lifestealRecipient->Heal(5);
					}

					const int expValue = 10;
					SpawnExperienceOrb(enemy->GetPosition(), expValue);
					return true;
				}
				return false;
			}),
		Enemies.end());
}
