#include <vcl.h>
#pragma hdrstop

#include "MainForm.h"
#include <algorithm>
#include <cmath>

namespace
{
float SmoothInterpT(float t)
{
	t = std::max(0.0f, std::min(1.0f, t));
	return t * t * (3.0f - 2.0f * t);
}

float MonotonicSnapshotTime(float recvWallSeconds, float lastBufferedTs, bool hasLast)
{
	if (!hasLast || recvWallSeconds > lastBufferedTs)
		return recvWallSeconds;
	return lastBufferedTs + 1.2e-4f;
}

float ShortestAngleDeltaRad(float fromRad, float toRad)
{
	float d = toRad - fromRad;
	const float twoPi = 6.2831853f;
	while (d > 3.14159265f)
		d -= twoPi;
	while (d < -3.14159265f)
		d += twoPi;
	return d;
}
}

void TForm1::ApplyClientSidePrediction(float deltaTime)
{
	if (!IsNetworkGameActive || NetworkManager.IsHosting())
		return;

	TGamePlayer* localPlayer = World->GetLocalPlayer();
	if (!localPlayer)
		return;

	TPredictedState predicted;
	predicted.Position = localPlayer->GetPosition();
	predicted.FacingDirection = localPlayer->GetFacingDirection();
	predicted.FrameNumber = FrameNumber;

	PredictedStates.push_back(predicted);

	if (PredictedStates.size() > 60)
	{
		PredictedStates.erase(PredictedStates.begin());
	}
}

void TForm1::InterpolateRemotePlayers(float deltaTime)
{
	(void)deltaTime;
	if (!IsNetworkGameActive)
		return;

	if (InterpolatedPlayers.size() < World->GetPlayerCount())
	{
		InterpolatedPlayers.resize(World->GetPlayerCount());
	}

	for (uint8_t i = 0; i < World->GetPlayerCount(); i++)
	{
		if (i == World->GetLocalPlayerID())
			continue;

		TGamePlayer* player = World->GetPlayer(i);
		if (!player)
			continue;

		TInterpolatedPlayer &interp = InterpolatedPlayers[i];

		if (!interp.HasValidState || interp.StateBuffer.size() < 2)
		{
			if (interp.StateBuffer.size() == 1)
			{
				const auto &snapshot = interp.StateBuffer.back();
				player->SetPosition(snapshot.Position.X, snapshot.Position.Y);
				player->SetFacingDirection(snapshot.FacingDirection);
			}
			continue;
		}

		float interpolationDelay = 0.085f;
		{
			const size_t sz = interp.StateBuffer.size();
			const float lastPairDt = interp.StateBuffer[sz - 1].Timestamp - interp.StateBuffer[sz - 2].Timestamp;
			const float span = interp.StateBuffer.back().Timestamp - interp.StateBuffer.front().Timestamp;
			float estDt = std::max(lastPairDt, span / static_cast<float>((sz > 1) ? (sz - 1) : 1));
			const uint32_t tickNew = interp.StateBuffer[sz - 1].Tick;
			const uint32_t tickOld = interp.StateBuffer[sz - 2].Tick;
			uint32_t tickDelta = (tickNew > tickOld) ? (tickNew - tickOld) : 1u;
			if (tickDelta == 0)
				tickDelta = 1u;
			const float tickBasedDt = static_cast<float>(tickDelta) * TForm1::NetworkUpdateInterval;
			estDt = std::max(estDt, tickBasedDt * 0.88f);

			if (!NetworkManager.IsHosting())
				interpolationDelay = std::min(0.098f, std::max(0.036f, estDt * 1.16f));
			else
				interpolationDelay = std::min(0.11f, std::max(0.045f, estDt * 1.35f));
		}

		const float nowClock = NetInterpolationClockSeconds();
		float targetTime = nowClock - interpolationDelay;

		size_t olderIdx = 0;
		size_t newerIdx = 0;
		bool foundPair = false;

		for (size_t j = 0; j < interp.StateBuffer.size() - 1; j++)
		{
			if (interp.StateBuffer[j].Timestamp <= targetTime &&
				interp.StateBuffer[j + 1].Timestamp >= targetTime)
			{
				olderIdx = j;
				newerIdx = j + 1;
				foundPair = true;
				break;
			}
		}

		if (!foundPair)
		{
			if (interp.StateBuffer.size() >= 2)
			{
				olderIdx = interp.StateBuffer.size() - 2;
				newerIdx = interp.StateBuffer.size() - 1;
				foundPair = true;
			}
		}

		if (foundPair)
		{
			const auto &olderState = interp.StateBuffer[olderIdx];
			const auto &newerState = interp.StateBuffer[newerIdx];

			float t = 0.0f;
			if (newerState.Timestamp > olderState.Timestamp)
			{
				t = (targetTime - olderState.Timestamp) / (newerState.Timestamp - olderState.Timestamp);
				t = std::max(0.0f, std::min(1.0f, t));
			}

			TPointF interpolatedPos(
				olderState.Position.X + (newerState.Position.X - olderState.Position.X) * t,
				olderState.Position.Y + (newerState.Position.Y - olderState.Position.Y) * t
			);

			const float a0 = std::atan2(olderState.FacingDirection.Y, olderState.FacingDirection.X);
			const float a1 = std::atan2(newerState.FacingDirection.Y, newerState.FacingDirection.X);
			const float aInterp = a0 + t * ShortestAngleDeltaRad(a0, a1);
			TPointF interpolatedDir(std::cos(aInterp), std::sin(aInterp));

			player->SetPosition(interpolatedPos.X, interpolatedPos.Y);
			player->SetFacingDirection(interpolatedDir);
		}
		else
		{
			const auto &snapshot = interp.StateBuffer.back();
			player->SetPosition(snapshot.Position.X, snapshot.Position.Y);
			player->SetFacingDirection(snapshot.FacingDirection);
		}

		const float pruneClock = NetInterpolationClockSeconds();
		interp.StateBuffer.erase(
			std::remove_if(interp.StateBuffer.begin(), interp.StateBuffer.end(),
				[pruneClock](const TPlayerStateSnapshot &s) {
					return (pruneClock - s.Timestamp) > 0.85f;
				}),
			interp.StateBuffer.end()
		);
	}
}

void TForm1::RestoreHostRemoteAuthoritativePositions()
{
	if (!IsNetworkGameActive || !NetworkManager.IsHosting() || !World || !HostRemoteAuthValid)
		return;

	const uint8_t n = World->GetPlayerCount();
	const uint8_t localId = World->GetLocalPlayerID();

	for (uint8_t i = 0; i < n; ++i)
	{
		if (i == localId)
			continue;
		if (i >= HostRemoteAuthPos.size() || i >= HostRemoteAuthFacing.size())
			continue;
		TGamePlayer *p = World->GetPlayer(i);
		if (!p)
			continue;
		const TPointF &pos = HostRemoteAuthPos[i];
		const TPointF &face = HostRemoteAuthFacing[i];
		p->SetPosition(pos.X, pos.Y);
		p->SetFacingDirection(face);
	}
}

void TForm1::HostFeedRemoteSnapshotsAfterSim()
{
	if (!IsNetworkGameActive || !NetworkManager.IsHosting() || !World)
		return;

	const uint8_t n = World->GetPlayerCount();
	const uint8_t localId = World->GetLocalPlayerID();
	const float currentTime = NetInterpolationClockSeconds();

	if (InterpolatedPlayers.size() < n)
		InterpolatedPlayers.resize(n);
	HostRemoteAuthPos.resize(n);
	HostRemoteAuthFacing.resize(n);

	bool hadRemote = false;
	for (uint8_t i = 0; i < n; ++i)
	{
		if (i == localId)
			continue;
		TGamePlayer *p = World->GetPlayer(i);
		if (!p)
			continue;
		hadRemote = true;

		const TPointF pos = p->GetPosition();
		TPointF face = p->GetFacingDirection();
		const float fl = std::sqrt(face.X * face.X + face.Y * face.Y);
		if (fl > 0.001f)
		{
			face.X /= fl;
			face.Y /= fl;
		}
		else
		{
			face = TPointF(0.0f, -1.0f);
		}

		HostRemoteAuthPos[i] = pos;
		HostRemoteAuthFacing[i] = face;

		TInterpolatedPlayer &interp = InterpolatedPlayers[i];
		TPlayerStateSnapshot snap;
		snap.Position = pos;
		snap.FacingDirection = face;
		snap.Health = static_cast<float>(p->GetHealth());
		snap.MaxHealth = static_cast<float>(p->GetMaxHealth());
		snap.Tick = FrameNumber;
		{
			const bool hasTs = !interp.StateBuffer.empty();
			const float lastTs = hasTs ? interp.StateBuffer.back().Timestamp : 0.0f;
			snap.Timestamp = MonotonicSnapshotTime(currentTime, lastTs, hasTs);
		}
		interp.StateBuffer.push_back(snap);
		if (interp.StateBuffer.size() > TInterpolatedPlayer::MaxBufferSize)
			interp.StateBuffer.erase(interp.StateBuffer.begin());
		interp.LastUpdateTime = snap.Timestamp;
		interp.HasValidState = true;
	}

	if (hadRemote)
		HostRemoteAuthValid = true;
}

void TForm1::InterpolateNetworkEnemies(float deltaTime)
{
	(void)deltaTime;

	if (!IsNetworkGameActive || NetworkManager.IsHosting())
		return;

	const size_t slotCount = World->GetEnemySlotCount();
	if (InterpolatedEnemies.size() != slotCount)
		InterpolatedEnemies.resize(slotCount);

	for (size_t i = 0; i < slotCount; ++i)
	{
		TEnemy *enemy = World->GetEnemyBySlot(i);
		if (!enemy)
		{
			if (i < InterpolatedEnemies.size())
			{
				InterpolatedEnemies[i].StateBuffer.clear();
				InterpolatedEnemies[i].HasValidState = false;
			}
			continue;
		}

		if (i >= InterpolatedEnemies.size())
			continue;

		TInterpolatedEnemySlot &interp = InterpolatedEnemies[i];

		if (!interp.HasValidState || interp.StateBuffer.empty())
			continue;

		if (interp.StateBuffer.size() < 2)
		{
			const auto &snapshot = interp.StateBuffer.back();
			enemy->SetPosition(snapshot.Position);
			continue;
		}

		float enemyInterpolationDelay = 0.085f;
		{
			const size_t esz = interp.StateBuffer.size();
			const float lastPairDt = interp.StateBuffer[esz - 1].Timestamp - interp.StateBuffer[esz - 2].Timestamp;
			const float span = interp.StateBuffer.back().Timestamp - interp.StateBuffer.front().Timestamp;
			const float estDt = std::max(lastPairDt, span / static_cast<float>((esz > 1) ? (esz - 1) : 1));
			enemyInterpolationDelay = std::min(0.11f, std::max(0.045f, estDt * 1.35f));
		}

		const float nowEnemyClock = NetInterpolationClockSeconds();
		float targetTime = nowEnemyClock - enemyInterpolationDelay;

		size_t olderIdx = 0;
		size_t newerIdx = 0;
		bool foundPair = false;

		for (size_t j = 0; j < interp.StateBuffer.size() - 1; j++)
		{
			if (interp.StateBuffer[j].Timestamp <= targetTime &&
				interp.StateBuffer[j + 1].Timestamp >= targetTime)
			{
				olderIdx = j;
				newerIdx = j + 1;
				foundPair = true;
				break;
			}
		}

		if (!foundPair)
		{
			if (interp.StateBuffer.size() >= 2)
			{
				olderIdx = interp.StateBuffer.size() - 2;
				newerIdx = interp.StateBuffer.size() - 1;
				foundPair = true;
			}
		}

		if (foundPair)
		{
			const auto &olderState = interp.StateBuffer[olderIdx];
			const auto &newerState = interp.StateBuffer[newerIdx];

			float t = 0.0f;
			if (newerState.Timestamp > olderState.Timestamp)
			{
				t = (targetTime - olderState.Timestamp) / (newerState.Timestamp - olderState.Timestamp);
				t = std::max(0.0f, std::min(1.0f, t));
			}
			const float st = SmoothInterpT(t);

			TPointF interpolatedPos(
				olderState.Position.X + (newerState.Position.X - olderState.Position.X) * st,
				olderState.Position.Y + (newerState.Position.Y - olderState.Position.Y) * st
			);
			enemy->SetPosition(interpolatedPos);
		}
		else
		{
			enemy->SetPosition(interp.StateBuffer.back().Position);
		}

		const float pruneEnemyClock = NetInterpolationClockSeconds();
		interp.StateBuffer.erase(
			std::remove_if(interp.StateBuffer.begin(), interp.StateBuffer.end(),
				[pruneEnemyClock](const TEnemyNetSnapshot &s) {
					return (pruneEnemyClock - s.Timestamp) > 0.85f;
				}),
			interp.StateBuffer.end()
		);
	}
}
