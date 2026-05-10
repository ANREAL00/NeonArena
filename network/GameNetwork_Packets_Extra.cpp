#include <vcl.h>
#pragma hdrstop

#include "GameNetwork.h"
#include <algorithm>

namespace NeonGame
{

static constexpr uint16_t kMaxExpOrbBulkEntries = 2000;

std::vector<uint8_t> TExpOrbBulkUpdatePacket::Serialize() const
{
	std::vector<uint8_t> buffer;
	const uint16_t count = static_cast<uint16_t>(std::min<size_t>(Orbs.size(), kMaxExpOrbBulkEntries));
	NetworkSerialization::WriteUInt16(buffer, count);
	for (uint16_t i = 0; i < count; ++i)
	{
		const TExpOrbNetPacket &o = Orbs[i];
		NetworkSerialization::WriteUInt32(buffer, o.NetInstanceId);
		NetworkSerialization::WriteFloat(buffer, o.PositionX);
		NetworkSerialization::WriteFloat(buffer, o.PositionY);
		NetworkSerialization::WriteUInt32(buffer, static_cast<uint32_t>(o.ExpValue));
		NetworkSerialization::WriteFloat(buffer, o.Lifetime);
	}
	return buffer;
}

bool TExpOrbBulkUpdatePacket::Deserialize(const std::vector<uint8_t> &data)
{
	size_t offset = 0;
	uint16_t count = 0;
	if (!NetworkSerialization::ReadUInt16(data, offset, count))
		return false;
	Orbs.clear();
	Orbs.reserve(count);
	for (uint16_t i = 0; i < count; ++i)
	{
		TExpOrbNetPacket o;
		if (!NetworkSerialization::ReadUInt32(data, offset, o.NetInstanceId))
			return false;
		if (!NetworkSerialization::ReadFloat(data, offset, o.PositionX))
			return false;
		if (!NetworkSerialization::ReadFloat(data, offset, o.PositionY))
			return false;
		uint32_t ev = 0;
		if (!NetworkSerialization::ReadUInt32(data, offset, ev))
			return false;
		o.ExpValue = static_cast<int32_t>(ev);
		if (!NetworkSerialization::ReadFloat(data, offset, o.Lifetime))
			return false;
		Orbs.push_back(o);
	}
	return true;
}

size_t TExpOrbBulkUpdatePacket::GetDataSize() const
{
	const uint16_t count = static_cast<uint16_t>(std::min<size_t>(Orbs.size(), kMaxExpOrbBulkEntries));
	return sizeof(uint16_t) + static_cast<size_t>(count) *
		(sizeof(uint32_t) + 2 * sizeof(float) + sizeof(uint32_t) + sizeof(float));
}

std::vector<uint8_t> TWaveUpdatePacket::Serialize() const
{
	std::vector<uint8_t> buffer;
	NetworkSerialization::WriteUInt32(buffer, WaveNumber);
	NetworkSerialization::WriteUInt8(buffer, WaveState);
	NetworkSerialization::WriteFloat(buffer, CooldownRemaining);
	NetworkSerialization::WriteFloat(buffer, RunTimeSeconds);
	return buffer;
}

bool TWaveUpdatePacket::Deserialize(const std::vector<uint8_t> &data)
{
	size_t offset = 0;
	IncludesRunTimeSeconds = false;
	RunTimeSeconds = 0.0f;
	if (!NetworkSerialization::ReadUInt32(data, offset, WaveNumber))
		return false;
	if (!NetworkSerialization::ReadUInt8(data, offset, WaveState))
		return false;
	if (!NetworkSerialization::ReadFloat(data, offset, CooldownRemaining))
		return false;
	if (data.size() >= offset + sizeof(float))
	{
		if (!NetworkSerialization::ReadFloat(data, offset, RunTimeSeconds))
			return false;
		IncludesRunTimeSeconds = true;
	}
	return true;
}

size_t TWaveUpdatePacket::GetDataSize() const
{
	return sizeof(uint32_t) + sizeof(uint8_t) + 2 * sizeof(float);
}

std::vector<uint8_t> TUpgradeChoicesPacket::Serialize() const
{
	std::vector<uint8_t> buffer;
	NetworkSerialization::WriteUInt8(buffer, PlayerID);
	NetworkSerialization::WriteBool(buffer, IsWaiting);
	const uint8_t count = static_cast<uint8_t>(std::min<size_t>(Choices.size(), 3));
	NetworkSerialization::WriteUInt8(buffer, count);
	for (uint8_t i = 0; i < count; ++i)
	{
		NetworkSerialization::WriteUInt8(buffer, Choices[i].Type);
		NetworkSerialization::WriteUInt8(buffer, Choices[i].Rarity);
	}
	return buffer;
}

bool TUpgradeChoicesPacket::Deserialize(const std::vector<uint8_t> &data)
{
	size_t offset = 0;
	if (!NetworkSerialization::ReadUInt8(data, offset, PlayerID))
		return false;
	if (!NetworkSerialization::ReadBool(data, offset, IsWaiting))
		return false;
	uint8_t count = 0;
	if (!NetworkSerialization::ReadUInt8(data, offset, count))
		return false;
	Choices.clear();
	Choices.reserve(count);
	for (uint8_t i = 0; i < count; ++i)
	{
		TUpgradeChoiceNet c;
		if (!NetworkSerialization::ReadUInt8(data, offset, c.Type))
			return false;
		if (!NetworkSerialization::ReadUInt8(data, offset, c.Rarity))
			return false;
		Choices.push_back(c);
	}
	return true;
}

size_t TUpgradeChoicesPacket::GetDataSize() const
{
	const size_t count = std::min<size_t>(Choices.size(), 3);
	return sizeof(uint8_t) + sizeof(bool) + sizeof(uint8_t) + count * (sizeof(uint8_t) + sizeof(uint8_t));
}

std::vector<uint8_t> TUpgradeSelectPacket::Serialize() const
{
	std::vector<uint8_t> buffer;
	NetworkSerialization::WriteUInt8(buffer, PlayerID);
	NetworkSerialization::WriteUInt8(buffer, ChoiceIndex);
	return buffer;
}

bool TUpgradeSelectPacket::Deserialize(const std::vector<uint8_t> &data)
{
	size_t offset = 0;
	if (!NetworkSerialization::ReadUInt8(data, offset, PlayerID))
		return false;
	if (!NetworkSerialization::ReadUInt8(data, offset, ChoiceIndex))
		return false;
	return true;
}

size_t TUpgradeSelectPacket::GetDataSize() const
{
	return sizeof(uint8_t) + sizeof(uint8_t);
}

std::vector<uint8_t> TSignalPacket::Serialize() const
{
	return {};
}

bool TSignalPacket::Deserialize(const std::vector<uint8_t> &data)
{
	(void)data;
	return true;
}

size_t TSignalPacket::GetDataSize() const
{
	return 0;
}

std::vector<uint8_t> TBossUpdatePacket::Serialize() const
{
	std::vector<uint8_t> buffer;
	NetworkSerialization::WriteBool(buffer, IsAlive);
	NetworkSerialization::WriteFloat(buffer, PositionX);
	NetworkSerialization::WriteFloat(buffer, PositionY);
	NetworkSerialization::WriteUInt32(buffer, static_cast<uint32_t>(Health));
	NetworkSerialization::WriteUInt32(buffer, static_cast<uint32_t>(MaxHealth));
	NetworkSerialization::WriteUInt8(buffer, Phase);
	return buffer;
}

bool TBossUpdatePacket::Deserialize(const std::vector<uint8_t> &data)
{
	size_t offset = 0;
	if (!NetworkSerialization::ReadBool(data, offset, IsAlive))
		return false;
	if (!NetworkSerialization::ReadFloat(data, offset, PositionX))
		return false;
	if (!NetworkSerialization::ReadFloat(data, offset, PositionY))
		return false;
	uint32_t h = 0, mh = 0;
	if (!NetworkSerialization::ReadUInt32(data, offset, h))
		return false;
	if (!NetworkSerialization::ReadUInt32(data, offset, mh))
		return false;
	Health = static_cast<int32_t>(h);
	MaxHealth = static_cast<int32_t>(mh);
	uint8_t ph = 0;
	if (offset < data.size())
	{
		if (!NetworkSerialization::ReadUInt8(data, offset, ph))
			return false;
	}
	Phase = ph;
	return true;
}

size_t TBossUpdatePacket::GetDataSize() const
{
	return sizeof(bool) + 2 * sizeof(float) + 2 * sizeof(uint32_t) + sizeof(uint8_t);
}

static constexpr uint16_t kMaxBulletBulkEntries = 4000;

std::vector<uint8_t> TBulletBulkUpdatePacket::Serialize() const
{
	std::vector<uint8_t> buffer;
	const uint16_t count = static_cast<uint16_t>(std::min<size_t>(Bullets.size(), kMaxBulletBulkEntries));
	NetworkSerialization::WriteUInt16(buffer, count);
	for (uint16_t i = 0; i < count; ++i)
	{
		const TBulletNetPacket &b = Bullets[i];
		NetworkSerialization::WriteFloat(buffer, b.PositionX);
		NetworkSerialization::WriteFloat(buffer, b.PositionY);
		NetworkSerialization::WriteBool(buffer, b.IsPlayerBullet);
	}
	return buffer;
}

bool TBulletBulkUpdatePacket::Deserialize(const std::vector<uint8_t> &data)
{
	size_t offset = 0;
	uint16_t count = 0;
	if (!NetworkSerialization::ReadUInt16(data, offset, count))
		return false;
	Bullets.clear();
	Bullets.reserve(count);
	for (uint16_t i = 0; i < count; ++i)
	{
		TBulletNetPacket b;
		if (!NetworkSerialization::ReadFloat(data, offset, b.PositionX))
			return false;
		if (!NetworkSerialization::ReadFloat(data, offset, b.PositionY))
			return false;
		if (!NetworkSerialization::ReadBool(data, offset, b.IsPlayerBullet))
			return false;
		Bullets.push_back(b);
	}
	return true;
}

size_t TBulletBulkUpdatePacket::GetDataSize() const
{
	const uint16_t count = static_cast<uint16_t>(std::min<size_t>(Bullets.size(), kMaxBulletBulkEntries));
	return sizeof(uint16_t) + static_cast<size_t>(count) * (2 * sizeof(float) + sizeof(bool));
}
}
