#include <vcl.h>
#pragma hdrstop

#include "GameNetwork.h"
#include <vector>

namespace NeonGame
{
namespace NetworkSerialization
{
	void WriteUInt8(std::vector<uint8_t> &buffer, uint8_t value)
	{
		buffer.push_back(value);
	}

	void WriteUInt16(std::vector<uint8_t> &buffer, uint16_t value)
	{
		buffer.push_back(static_cast<uint8_t>(value & 0xFF));
		buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
	}

	void WriteUInt32(std::vector<uint8_t> &buffer, uint32_t value)
	{
		buffer.push_back(static_cast<uint8_t>(value & 0xFF));
		buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
		buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
		buffer.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
	}

	void WriteFloat(std::vector<uint8_t> &buffer, float value)
	{
		const uint8_t *bytes = reinterpret_cast<const uint8_t*>(&value);
		buffer.insert(buffer.end(), bytes, bytes + sizeof(float));
	}

	void WriteBool(std::vector<uint8_t> &buffer, bool value)
	{
		buffer.push_back(value ? 1 : 0);
	}

	void WriteString(std::vector<uint8_t> &buffer, const std::string &str)
	{
		WriteUInt16(buffer, static_cast<uint16_t>(str.length()));
		buffer.insert(buffer.end(), str.begin(), str.end());
	}

	void WritePointF(std::vector<uint8_t> &buffer, const TPointF &point)
	{
		WriteFloat(buffer, point.X);
		WriteFloat(buffer, point.Y);
	}

	bool ReadUInt8(const std::vector<uint8_t> &buffer, size_t &offset, uint8_t &value)
	{
		if (offset >= buffer.size())
			return false;
		value = buffer[offset++];
		return true;
	}

	bool ReadUInt16(const std::vector<uint8_t> &buffer, size_t &offset, uint16_t &value)
	{
		if (offset + 1 >= buffer.size())
			return false;
		value = static_cast<uint16_t>(buffer[offset]) |
		        (static_cast<uint16_t>(buffer[offset + 1]) << 8);
		offset += 2;
		return true;
	}

	bool ReadUInt32(const std::vector<uint8_t> &buffer, size_t &offset, uint32_t &value)
	{
		if (offset + 3 >= buffer.size())
			return false;
		value = static_cast<uint32_t>(buffer[offset]) |
		        (static_cast<uint32_t>(buffer[offset + 1]) << 8) |
		        (static_cast<uint32_t>(buffer[offset + 2]) << 16) |
		        (static_cast<uint32_t>(buffer[offset + 3]) << 24);
		offset += 4;
		return true;
	}

	bool ReadFloat(const std::vector<uint8_t> &buffer, size_t &offset, float &value)
	{
		if (offset + sizeof(float) > buffer.size())
			return false;
		value = *reinterpret_cast<const float*>(&buffer[offset]);
		offset += sizeof(float);
		return true;
	}

	bool ReadBool(const std::vector<uint8_t> &buffer, size_t &offset, bool &value)
	{
		uint8_t byte;
		if (!ReadUInt8(buffer, offset, byte))
			return false;
		value = (byte != 0);
		return true;
	}

	bool ReadString(const std::vector<uint8_t> &buffer, size_t &offset, std::string &value)
	{
		uint16_t length;
		if (!ReadUInt16(buffer, offset, length))
			return false;
		if (offset + length > buffer.size())
			return false;
		value.assign(reinterpret_cast<const char*>(&buffer[offset]), length);
		offset += length;
		return true;
	}

	bool ReadPointF(const std::vector<uint8_t> &buffer, size_t &offset, TPointF &point)
	{
		if (!ReadFloat(buffer, offset, point.X))
			return false;
		if (!ReadFloat(buffer, offset, point.Y))
			return false;
		return true;
	}
}
}

