#include <vcl.h>
#pragma hdrstop

#include "GameNetwork.h"
#include <algorithm>

namespace NeonGame
{
std::vector<uint8_t> TConnectRequestPacket::Serialize() const
{
	std::vector<uint8_t> buffer;
	NetworkSerialization::WriteUInt32(buffer, ProtocolVersion);
	NetworkSerialization::WriteString(buffer, PlayerName);
	return buffer;
}

bool TConnectRequestPacket::Deserialize(const std::vector<uint8_t> &data)
{
	size_t offset = 0;
	if (!NetworkSerialization::ReadUInt32(data, offset, ProtocolVersion))
		return false;
	if (!NetworkSerialization::ReadString(data, offset, PlayerName))
		return false;
	return true;
}

size_t TConnectRequestPacket::GetDataSize() const
{
	return sizeof(uint32_t) + sizeof(uint16_t) + PlayerName.length();
}

std::vector<uint8_t> TConnectResponsePacket::Serialize() const
{
	std::vector<uint8_t> buffer;
	NetworkSerialization::WriteBool(buffer, Accepted);
	NetworkSerialization::WriteUInt8(buffer, PlayerID);
	NetworkSerialization::WriteUInt32(buffer, ProtocolVersion);
	NetworkSerialization::WriteString(buffer, Message);
	return buffer;
}

bool TConnectResponsePacket::Deserialize(const std::vector<uint8_t> &data)
{
	size_t offset = 0;
	if (!NetworkSerialization::ReadBool(data, offset, Accepted))
		return false;
	uint8_t id;
	if (!NetworkSerialization::ReadUInt8(data, offset, id))
		return false;
	PlayerID = id;
	if (!NetworkSerialization::ReadUInt32(data, offset, ProtocolVersion))
		return false;
	if (!NetworkSerialization::ReadString(data, offset, Message))
		return false;
	return true;
}

size_t TConnectResponsePacket::GetDataSize() const
{
	return sizeof(bool) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint16_t) + Message.length();
}

std::vector<uint8_t> TPlayerInputPacket::Serialize() const
{
	std::vector<uint8_t> buffer;
	NetworkSerialization::WriteUInt8(buffer, PlayerID);
	NetworkSerialization::WriteBool(buffer, InputUp);
	NetworkSerialization::WriteBool(buffer, InputDown);
	NetworkSerialization::WriteBool(buffer, InputLeft);
	NetworkSerialization::WriteBool(buffer, InputRight);
	NetworkSerialization::WriteBool(buffer, IsShooting);
	NetworkSerialization::WriteFloat(buffer, MouseX);
	NetworkSerialization::WriteFloat(buffer, MouseY);
	NetworkSerialization::WriteUInt32(buffer, FrameNumber);
	NetworkSerialization::WriteBool(buffer, IsAltShooting);
	return buffer;
}

bool TPlayerInputPacket::Deserialize(const std::vector<uint8_t> &data)
{
	size_t offset = 0;
	uint8_t id;
	if (!NetworkSerialization::ReadUInt8(data, offset, id))
		return false;
	PlayerID = id;
	if (!NetworkSerialization::ReadBool(data, offset, InputUp))
		return false;
	if (!NetworkSerialization::ReadBool(data, offset, InputDown))
		return false;
	if (!NetworkSerialization::ReadBool(data, offset, InputLeft))
		return false;
	if (!NetworkSerialization::ReadBool(data, offset, InputRight))
		return false;
	if (!NetworkSerialization::ReadBool(data, offset, IsShooting))
		return false;
	if (!NetworkSerialization::ReadFloat(data, offset, MouseX))
		return false;
	if (!NetworkSerialization::ReadFloat(data, offset, MouseY))
		return false;
	if (!NetworkSerialization::ReadUInt32(data, offset, FrameNumber))
		return false;
	if (offset < data.size())
	{
		if (!NetworkSerialization::ReadBool(data, offset, IsAltShooting))
			return false;
	}
	else
		IsAltShooting = false;
	return true;
}

size_t TPlayerInputPacket::GetDataSize() const
{
	return sizeof(uint8_t) + 5 * sizeof(bool) + 2 * sizeof(float) + sizeof(uint32_t) + sizeof(bool);
}

std::vector<uint8_t> TPlayerUpdatePacket::Serialize() const
{
	std::vector<uint8_t> buffer;
	NetworkSerialization::WriteUInt8(buffer, PlayerID);
	NetworkSerialization::WriteFloat(buffer, PositionX);
	NetworkSerialization::WriteFloat(buffer, PositionY);
	NetworkSerialization::WriteFloat(buffer, FacingDirectionX);
	NetworkSerialization::WriteFloat(buffer, FacingDirectionY);
	NetworkSerialization::WriteUInt32(buffer, static_cast<uint32_t>(Health));
	NetworkSerialization::WriteUInt32(buffer, static_cast<uint32_t>(MaxHealth));
	NetworkSerialization::WriteUInt32(buffer, static_cast<uint32_t>(Level));
	NetworkSerialization::WriteUInt32(buffer, static_cast<uint32_t>(Experience));
	NetworkSerialization::WriteUInt32(buffer, FrameNumber);
	NetworkSerialization::WriteFloat(buffer, SpeedMultiplier);
	return buffer;
}

bool TPlayerUpdatePacket::Deserialize(const std::vector<uint8_t> &data)
{
	size_t offset = 0;
	uint8_t id;
	if (!NetworkSerialization::ReadUInt8(data, offset, id))
		return false;
	PlayerID = id;
	if (!NetworkSerialization::ReadFloat(data, offset, PositionX))
		return false;
	if (!NetworkSerialization::ReadFloat(data, offset, PositionY))
		return false;
	if (!NetworkSerialization::ReadFloat(data, offset, FacingDirectionX))
		return false;
	if (!NetworkSerialization::ReadFloat(data, offset, FacingDirectionY))
		return false;
	uint32_t health, maxHealth, level, exp, frameNum;
	if (!NetworkSerialization::ReadUInt32(data, offset, health))
		return false;
	Health = static_cast<int>(health);
	if (!NetworkSerialization::ReadUInt32(data, offset, maxHealth))
		return false;
	MaxHealth = static_cast<int>(maxHealth);
	if (!NetworkSerialization::ReadUInt32(data, offset, level))
		return false;
	Level = static_cast<int>(level);
	if (!NetworkSerialization::ReadUInt32(data, offset, exp))
		return false;
	Experience = static_cast<int>(exp);
	if (!NetworkSerialization::ReadUInt32(data, offset, frameNum))
		return false;
	FrameNumber = frameNum;
	SpeedMultiplier = 1.0f;
	if (offset < data.size())
	{
		float sm = 1.0f;
		if (NetworkSerialization::ReadFloat(data, offset, sm))
			SpeedMultiplier = sm;
	}
	return true;
}

size_t TPlayerUpdatePacket::GetDataSize() const
{
	return sizeof(uint8_t) + 5 * sizeof(float) + 5 * sizeof(uint32_t);
}

static constexpr uint16_t kMaxPlayerBulkEntries = 8;

std::vector<uint8_t> TGameStateUpdatePacket::Serialize() const
{
	std::vector<uint8_t> buffer;
	const uint16_t count = static_cast<uint16_t>(std::min<size_t>(Players.size(), kMaxPlayerBulkEntries));
	NetworkSerialization::WriteUInt16(buffer, count);
	if (count > 0)
		buffer.reserve(sizeof(uint16_t) + count * Players[0].GetDataSize());
	for (uint16_t i = 0; i < count; ++i)
	{
		const auto pData = Players[i].Serialize();
		buffer.insert(buffer.end(), pData.begin(), pData.end());
	}
	return buffer;
}

bool TGameStateUpdatePacket::Deserialize(const std::vector<uint8_t> &data)
{
	size_t offset = 0;
	uint16_t count = 0;
	if (!NetworkSerialization::ReadUInt16(data, offset, count))
		return false;

	Players.clear();
	Players.reserve(count);
	for (uint16_t i = 0; i < count; ++i)
	{
		TPlayerUpdatePacket p;
		uint8_t id = 0;
		if (!NetworkSerialization::ReadUInt8(data, offset, id))
			return false;
		p.PlayerID = id;
		if (!NetworkSerialization::ReadFloat(data, offset, p.PositionX))
			return false;
		if (!NetworkSerialization::ReadFloat(data, offset, p.PositionY))
			return false;
		if (!NetworkSerialization::ReadFloat(data, offset, p.FacingDirectionX))
			return false;
		if (!NetworkSerialization::ReadFloat(data, offset, p.FacingDirectionY))
			return false;

		uint32_t health = 0, maxHealth = 0, level = 0, exp = 0, frameNum = 0;
		if (!NetworkSerialization::ReadUInt32(data, offset, health))
			return false;
		if (!NetworkSerialization::ReadUInt32(data, offset, maxHealth))
			return false;
		if (!NetworkSerialization::ReadUInt32(data, offset, level))
			return false;
		if (!NetworkSerialization::ReadUInt32(data, offset, exp))
			return false;
		if (!NetworkSerialization::ReadUInt32(data, offset, frameNum))
			return false;
		p.Health = static_cast<int>(health);
		p.MaxHealth = static_cast<int>(maxHealth);
		p.Level = static_cast<int>(level);
		p.Experience = static_cast<int>(exp);
		p.FrameNumber = frameNum;

		float sm = 1.0f;
		if (!NetworkSerialization::ReadFloat(data, offset, sm))
			return false;
		p.SpeedMultiplier = sm;

		Players.push_back(p);
	}
	return true;
}

size_t TGameStateUpdatePacket::GetDataSize() const
{
	const uint16_t count = static_cast<uint16_t>(std::min<size_t>(Players.size(), kMaxPlayerBulkEntries));
	return sizeof(uint16_t) + static_cast<size_t>(count) * (TPlayerUpdatePacket().GetDataSize());
}

std::vector<uint8_t> TEnemyUpdatePacket::Serialize() const
{
	std::vector<uint8_t> buffer;
	NetworkSerialization::WriteUInt16(buffer, EnemyID);
	NetworkSerialization::WriteUInt8(buffer, EnemyType);
	NetworkSerialization::WriteFloat(buffer, PositionX);
	NetworkSerialization::WriteFloat(buffer, PositionY);
	NetworkSerialization::WriteUInt32(buffer, static_cast<uint32_t>(Health));
	NetworkSerialization::WriteUInt32(buffer, static_cast<uint32_t>(MaxHealth));
	NetworkSerialization::WriteBool(buffer, IsAlive);
	NetworkSerialization::WriteUInt32(buffer, NetInstanceId);
	return buffer;
}

bool TEnemyUpdatePacket::Deserialize(const std::vector<uint8_t> &data)
{
	size_t offset = 0;
	if (!NetworkSerialization::ReadUInt16(data, offset, EnemyID))
		return false;
	if (!NetworkSerialization::ReadUInt8(data, offset, EnemyType))
		return false;
	if (!NetworkSerialization::ReadFloat(data, offset, PositionX))
		return false;
	if (!NetworkSerialization::ReadFloat(data, offset, PositionY))
		return false;
	uint32_t health;
	if (!NetworkSerialization::ReadUInt32(data, offset, health))
		return false;
	Health = static_cast<int>(health);
	uint32_t maxHealth;
	if (!NetworkSerialization::ReadUInt32(data, offset, maxHealth))
		return false;
	MaxHealth = static_cast<int>(maxHealth);
	if (!NetworkSerialization::ReadBool(data, offset, IsAlive))
		return false;
	if (offset < data.size())
	{
		if (!NetworkSerialization::ReadUInt32(data, offset, NetInstanceId))
			return false;
	}
	else
		NetInstanceId = 0;
	return true;
}

size_t TEnemyUpdatePacket::GetDataSize() const
{
	return sizeof(uint16_t) + sizeof(uint8_t) + 2 * sizeof(float) + 2 * sizeof(uint32_t) + sizeof(bool) + sizeof(uint32_t);
}

static constexpr uint16_t kMaxEnemyBulkEntries = 3000;

std::vector<uint8_t> TEnemyBulkUpdatePacket::Serialize() const
{
	std::vector<uint8_t> buffer;
	const uint16_t count = static_cast<uint16_t>(std::min<size_t>(Enemies.size(), kMaxEnemyBulkEntries));
	NetworkSerialization::WriteUInt16(buffer, count);
	for (uint16_t i = 0; i < count; ++i)
	{
		const TEnemyUpdatePacket &e = Enemies[i];
		NetworkSerialization::WriteUInt16(buffer, e.EnemyID);
		NetworkSerialization::WriteUInt8(buffer, e.EnemyType);
		NetworkSerialization::WriteFloat(buffer, e.PositionX);
		NetworkSerialization::WriteFloat(buffer, e.PositionY);
		NetworkSerialization::WriteUInt32(buffer, static_cast<uint32_t>(e.Health));
		NetworkSerialization::WriteUInt32(buffer, static_cast<uint32_t>(e.MaxHealth));
		NetworkSerialization::WriteBool(buffer, e.IsAlive);
		NetworkSerialization::WriteUInt32(buffer, e.NetInstanceId);
	}
	return buffer;
}

bool TEnemyBulkUpdatePacket::Deserialize(const std::vector<uint8_t> &data)
{
	size_t offset = 0;
	uint16_t count = 0;
	if (!NetworkSerialization::ReadUInt16(data, offset, count))
		return false;
	Enemies.clear();
	Enemies.reserve(count);
	for (uint16_t i = 0; i < count; ++i)
	{
		TEnemyUpdatePacket e;
		if (!NetworkSerialization::ReadUInt16(data, offset, e.EnemyID))
			return false;
		uint8_t t = 0;
		if (!NetworkSerialization::ReadUInt8(data, offset, t))
			return false;
		e.EnemyType = t;
		if (!NetworkSerialization::ReadFloat(data, offset, e.PositionX))
			return false;
		if (!NetworkSerialization::ReadFloat(data, offset, e.PositionY))
			return false;
		uint32_t health = 0;
		if (!NetworkSerialization::ReadUInt32(data, offset, health))
			return false;
		e.Health = static_cast<int>(health);
		uint32_t maxHealth = 0;
		if (!NetworkSerialization::ReadUInt32(data, offset, maxHealth))
			return false;
		e.MaxHealth = static_cast<int>(maxHealth);
		if (!NetworkSerialization::ReadBool(data, offset, e.IsAlive))
			return false;
		if (offset < data.size())
		{
			if (!NetworkSerialization::ReadUInt32(data, offset, e.NetInstanceId))
				return false;
		}
		else
			e.NetInstanceId = 0;
		Enemies.push_back(e);
	}
	return true;
}

size_t TEnemyBulkUpdatePacket::GetDataSize() const
{
	const uint16_t count = static_cast<uint16_t>(std::min<size_t>(Enemies.size(), kMaxEnemyBulkEntries));
	return sizeof(uint16_t) + static_cast<size_t>(count) * TEnemyUpdatePacket().GetDataSize();
}

}
