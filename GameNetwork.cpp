#include <vcl.h>
#pragma hdrstop
//---------------------------------------------------------------------------

#include "GameNetwork.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <algorithm>
#include <cstring>
#include <Windows.h>

#pragma comment(lib, "ws2_32.lib")

//---------------------------------------------------------------------------
// Namespace для сетевых типов
//---------------------------------------------------------------------------
namespace NeonGame
{

//---------------------------------------------------------------------------
// Вспомогательные функции сериализации
//---------------------------------------------------------------------------
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
		// Преобразуем float в байты (little-endian)
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

//---------------------------------------------------------------------------
// Реализация пакетов
//---------------------------------------------------------------------------
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

//---------------------------------------------------------------------------
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

//---------------------------------------------------------------------------
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
	return true;
}

size_t TPlayerInputPacket::GetDataSize() const
{
	return sizeof(uint8_t) + 5 * sizeof(bool) + 2 * sizeof(float) + sizeof(uint32_t);
}

//---------------------------------------------------------------------------
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
	return true;
}

size_t TPlayerUpdatePacket::GetDataSize() const
{
	return sizeof(uint8_t) + 4 * sizeof(float) + 5 * sizeof(uint32_t); // +1 для FrameNumber
}

//---------------------------------------------------------------------------
std::vector<uint8_t> TEnemyUpdatePacket::Serialize() const
{
	std::vector<uint8_t> buffer;
	NetworkSerialization::WriteUInt32(buffer, static_cast<uint32_t>(Enemies.size()));
	for (const auto &enemy : Enemies)
	{
		NetworkSerialization::WriteUInt8(buffer, enemy.Type);
		NetworkSerialization::WriteUInt32(buffer, enemy.EnemyID);
		NetworkSerialization::WriteFloat(buffer, enemy.PositionX);
		NetworkSerialization::WriteFloat(buffer, enemy.PositionY);
		NetworkSerialization::WriteUInt32(buffer, static_cast<uint32_t>(enemy.Health));
		NetworkSerialization::WriteBool(buffer, enemy.IsAlive);
	}
	NetworkSerialization::WriteUInt32(buffer, FrameNumber);
	return buffer;
}

bool TEnemyUpdatePacket::Deserialize(const std::vector<uint8_t> &data)
{
	size_t offset = 0;
	uint32_t count;
	if (!NetworkSerialization::ReadUInt32(data, offset, count))
		return false;
	
	Enemies.clear();
	for (uint32_t i = 0; i < count; i++)
	{
		TEnemyData enemy;
		if (!NetworkSerialization::ReadUInt8(data, offset, enemy.Type))
			return false;
		if (!NetworkSerialization::ReadUInt32(data, offset, enemy.EnemyID))
			return false;
		if (!NetworkSerialization::ReadFloat(data, offset, enemy.PositionX))
			return false;
		if (!NetworkSerialization::ReadFloat(data, offset, enemy.PositionY))
			return false;
		uint32_t health;
		if (!NetworkSerialization::ReadUInt32(data, offset, health))
			return false;
		enemy.Health = static_cast<int>(health);
		if (!NetworkSerialization::ReadBool(data, offset, enemy.IsAlive))
			return false;
		Enemies.push_back(enemy);
	}
	
	if (!NetworkSerialization::ReadUInt32(data, offset, FrameNumber))
		return false;
	
	return true;
}

size_t TEnemyUpdatePacket::GetDataSize() const
{
	return sizeof(uint32_t) + Enemies.size() * (sizeof(uint8_t) + sizeof(uint32_t) + 2 * sizeof(float) + sizeof(uint32_t) + sizeof(bool)) + sizeof(uint32_t);
}

//---------------------------------------------------------------------------
std::vector<uint8_t> TBossUpdatePacket::Serialize() const
{
	std::vector<uint8_t> buffer;
	NetworkSerialization::WriteFloat(buffer, PositionX);
	NetworkSerialization::WriteFloat(buffer, PositionY);
	NetworkSerialization::WriteUInt32(buffer, static_cast<uint32_t>(Health));
	NetworkSerialization::WriteUInt32(buffer, static_cast<uint32_t>(MaxHealth));
	NetworkSerialization::WriteUInt8(buffer, Phase);
	NetworkSerialization::WriteBool(buffer, IsAlive);
	NetworkSerialization::WriteUInt32(buffer, FrameNumber);
	return buffer;
}

bool TBossUpdatePacket::Deserialize(const std::vector<uint8_t> &data)
{
	size_t offset = 0;
	if (!NetworkSerialization::ReadFloat(data, offset, PositionX))
		return false;
	if (!NetworkSerialization::ReadFloat(data, offset, PositionY))
		return false;
	uint32_t health, maxHealth;
	if (!NetworkSerialization::ReadUInt32(data, offset, health))
		return false;
	Health = static_cast<int>(health);
	if (!NetworkSerialization::ReadUInt32(data, offset, maxHealth))
		return false;
	MaxHealth = static_cast<int>(maxHealth);
	if (!NetworkSerialization::ReadUInt8(data, offset, Phase))
		return false;
	if (!NetworkSerialization::ReadBool(data, offset, IsAlive))
		return false;
	if (!NetworkSerialization::ReadUInt32(data, offset, FrameNumber))
		return false;
	return true;
}

size_t TBossUpdatePacket::GetDataSize() const
{
	return 2 * sizeof(float) + 3 * sizeof(uint32_t) + sizeof(uint8_t) + sizeof(bool);
}

//---------------------------------------------------------------------------
std::vector<uint8_t> TExperienceOrbUpdatePacket::Serialize() const
{
	std::vector<uint8_t> buffer;
	NetworkSerialization::WriteUInt32(buffer, static_cast<uint32_t>(Orbs.size()));
	for (const auto &orb : Orbs)
	{
		NetworkSerialization::WriteFloat(buffer, orb.PositionX);
		NetworkSerialization::WriteFloat(buffer, orb.PositionY);
		NetworkSerialization::WriteUInt32(buffer, static_cast<uint32_t>(orb.Value));
		NetworkSerialization::WriteBool(buffer, orb.IsActive);
	}
	NetworkSerialization::WriteUInt32(buffer, FrameNumber);
	return buffer;
}

bool TExperienceOrbUpdatePacket::Deserialize(const std::vector<uint8_t> &data)
{
	size_t offset = 0;
	uint32_t count;
	if (!NetworkSerialization::ReadUInt32(data, offset, count))
		return false;
	
	Orbs.clear();
	for (uint32_t i = 0; i < count; i++)
	{
		TExperienceOrbData orb;
		if (!NetworkSerialization::ReadFloat(data, offset, orb.PositionX))
			return false;
		if (!NetworkSerialization::ReadFloat(data, offset, orb.PositionY))
			return false;
		uint32_t value;
		if (!NetworkSerialization::ReadUInt32(data, offset, value))
			return false;
		orb.Value = static_cast<int32_t>(value);
		if (!NetworkSerialization::ReadBool(data, offset, orb.IsActive))
			return false;
		Orbs.push_back(orb);
	}
	
	if (!NetworkSerialization::ReadUInt32(data, offset, FrameNumber))
		return false;
	
	return true;
}

size_t TExperienceOrbUpdatePacket::GetDataSize() const
{
	return sizeof(uint32_t) + Orbs.size() * (2 * sizeof(float) + sizeof(uint32_t) + sizeof(bool)) + sizeof(uint32_t);
}

//---------------------------------------------------------------------------
std::vector<uint8_t> TPlayerUpgradePacket::Serialize() const
{
	std::vector<uint8_t> buffer;
	NetworkSerialization::WriteUInt8(buffer, PlayerID);
	NetworkSerialization::WriteUInt8(buffer, UpgradeType);
	NetworkSerialization::WriteUInt8(buffer, Rarity);
	return buffer;
}

bool TPlayerUpgradePacket::Deserialize(const std::vector<uint8_t> &data)
{
	size_t offset = 0;
	if (!NetworkSerialization::ReadUInt8(data, offset, PlayerID))
		return false;
	if (!NetworkSerialization::ReadUInt8(data, offset, UpgradeType))
		return false;
	if (!NetworkSerialization::ReadUInt8(data, offset, Rarity))
		return false;
	return true;
}

size_t TPlayerUpgradePacket::GetDataSize() const
{
	return 3 * sizeof(uint8_t);
}

//---------------------------------------------------------------------------
std::vector<uint8_t> TGameStateUpdatePacket::Serialize() const
{
	std::vector<uint8_t> buffer;
	NetworkSerialization::WriteUInt32(buffer, WaveNumber);
	NetworkSerialization::WriteUInt32(buffer, EnemiesAlive);
	NetworkSerialization::WriteUInt8(buffer, WorldState);
	NetworkSerialization::WriteUInt32(buffer, FrameNumber);
	return buffer;
}

bool TGameStateUpdatePacket::Deserialize(const std::vector<uint8_t> &data)
{
	size_t offset = 0;
	if (!NetworkSerialization::ReadUInt32(data, offset, WaveNumber))
		return false;
	if (!NetworkSerialization::ReadUInt32(data, offset, EnemiesAlive))
		return false;
	if (!NetworkSerialization::ReadUInt8(data, offset, WorldState))
		return false;
	if (!NetworkSerialization::ReadUInt32(data, offset, FrameNumber))
		return false;
	return true;
}

size_t TGameStateUpdatePacket::GetDataSize() const
{
	return 3 * sizeof(uint32_t) + sizeof(uint8_t);
}

//---------------------------------------------------------------------------
// Реализация TNetworkManager
//---------------------------------------------------------------------------
TNetworkManager::TNetworkManager()
	: State(ENetworkState::Disconnected),
	  IsHost(false),
	  LocalPlayerID(0),
	  ListenSocket(nullptr),
	  ClientSocket(nullptr),
	  NextSequence(0),
	  LastReceivedSequence(0),
	  ConnectionStartTime(0),
	  LastPacketTime(0),
	  ReconnectTimer(0),
	  IsReconnecting(false),
	  ReconnectPort(0),
	  LocalIPAddress(""),
	  PacketsSent(0),
	  PacketsReceived(0),
	  PacketsLost(0),
	  AverageLatency(0.0f)
{
	Clients.resize(4); // максимум 4 игрока
}

//---------------------------------------------------------------------------
TNetworkManager::~TNetworkManager()
{
	Shutdown();
}

//---------------------------------------------------------------------------
bool TNetworkManager::Initialize()
{
	if (!InitializeWinSock())
		return false;
	return true;
}

//---------------------------------------------------------------------------
void TNetworkManager::Shutdown()
{
	Disconnect();
	CleanupWinSock();
}

//---------------------------------------------------------------------------
bool TNetworkManager::InitializeWinSock()
{
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0)
	{
		// Ошибка инициализации WinSock
		return false;
	}
	return true;
}

//---------------------------------------------------------------------------
void TNetworkManager::CleanupWinSock()
{
	WSACleanup();
}

//---------------------------------------------------------------------------
bool TNetworkManager::CreateListenSocket(uint16_t port)
{
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)
		return false;
	
	// Устанавливаем опцию для переиспользования адреса
	int opt = 1;
	setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
	
	// Настраиваем адрес
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);
	
	// Привязываем сокет
	if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
	{
		closesocket(listenSocket);
		return false;
	}
	
	// Начинаем прослушивание
	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		closesocket(listenSocket);
		return false;
	}
	
	// Устанавливаем неблокирующий режим
	u_long mode = 1;
	ioctlsocket(listenSocket, FIONBIO, &mode);
	
	ListenSocket = reinterpret_cast<void*>(listenSocket);

	// Определяем локальный IP-адрес для отображения хосту
	char hostName[256] = {0};
	if (gethostname(hostName, sizeof(hostName)) == 0)
	{
		addrinfo hints = {};
		hints.ai_family = AF_INET;      // только IPv4
		hints.ai_socktype = SOCK_STREAM;
		addrinfo *result = nullptr;
		if (getaddrinfo(hostName, nullptr, &hints, &result) == 0)
		{
			for (addrinfo *ptr = result; ptr != nullptr; ptr = ptr->ai_next)
			{
				sockaddr_in *addr = reinterpret_cast<sockaddr_in*>(ptr->ai_addr);
				if (!addr)
					continue;

				// Пропускаем 127.0.0.1
				if (addr->sin_addr.s_addr == htonl(INADDR_LOOPBACK))
					continue;

				char ipStr[INET_ADDRSTRLEN] = {0};
				if (inet_ntop(AF_INET, &addr->sin_addr, ipStr, sizeof(ipStr)))
				{
					LocalIPAddress = ipStr;
					break;
				}
			}
			freeaddrinfo(result);
		}
	}

	return true;
}

//---------------------------------------------------------------------------
bool TNetworkManager::ConnectToHost(const std::string &hostIP, uint16_t port)
{
	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket == INVALID_SOCKET)
		return false;

	// Сразу переводим сокет в неблокирующий режим, чтобы connect не подвешивал приложение
	u_long mode = 1;
	ioctlsocket(clientSocket, FIONBIO, &mode);

	// Настраиваем адрес сервера
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	
	// Преобразуем IP-адрес
	if (inet_pton(AF_INET, hostIP.c_str(), &serverAddr.sin_addr) <= 0)
	{
		closesocket(clientSocket);
		return false;
	}

	// Подключаемся
	if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		// Для неблокирующего сокета ожидание подключения даёт WSAEWOULDBLOCK / WSAEINPROGRESS — это нормально
		if (error != WSAEWOULDBLOCK && error != WSAEINPROGRESS)
		{
			closesocket(clientSocket);
			return false;
		}
	}

	ClientSocket = reinterpret_cast<void*>(clientSocket);
	return true;
}

//---------------------------------------------------------------------------
void TNetworkManager::CloseSocket(void* socket)
{
	if (socket)
	{
		SOCKET s = reinterpret_cast<SOCKET>(socket);
		closesocket(s);
	}
}

//---------------------------------------------------------------------------
bool TNetworkManager::SendPacket(void* socket, const TPacket &packet)
{
	if (!socket)
		return false;
	
	// Сериализуем пакет
	std::vector<uint8_t> data = packet.Serialize();
	
	// Создаем заголовок
	TPacketHeader header;
	header.Type = packet.Header.Type;
	header.Size = static_cast<uint16_t>(data.size());
	header.Sequence = NextSequence++;
	header.Timestamp = GetTickCount();
	
	// Сериализуем заголовок
	std::vector<uint8_t> headerData;
	NetworkSerialization::WriteUInt16(headerData, static_cast<uint16_t>(header.Type));
	NetworkSerialization::WriteUInt16(headerData, header.Size);
	NetworkSerialization::WriteUInt32(headerData, header.Sequence);
	NetworkSerialization::WriteUInt32(headerData, header.Timestamp);
	
	// Объединяем заголовок и данные
	std::vector<uint8_t> fullPacket;
	fullPacket.insert(fullPacket.end(), headerData.begin(), headerData.end());
	fullPacket.insert(fullPacket.end(), data.begin(), data.end());
	
	// Отправляем
	SOCKET s = reinterpret_cast<SOCKET>(socket);
	int sent = send(s, reinterpret_cast<const char*>(fullPacket.data()), 
	                static_cast<int>(fullPacket.size()), 0);
	
	if (sent == SOCKET_ERROR)
		return false;
	
	PacketsSent++;
	return true;
}

//---------------------------------------------------------------------------
std::unique_ptr<TPacket> TNetworkManager::ReceivePacket(void* socket)
{
	if (!socket)
		return nullptr;
	
	SOCKET s = reinterpret_cast<SOCKET>(socket);
	
	// Сначала читаем заголовок
	char headerBuffer[sizeof(TPacketHeader)];
	int received = recv(s, headerBuffer, sizeof(TPacketHeader), MSG_PEEK);
	
	if (received <= 0 || received < sizeof(TPacketHeader))
		return nullptr;
	
	// Парсим заголовок
	size_t offset = 0;
	TPacketHeader header;
	std::vector<uint8_t> headerData(headerBuffer, headerBuffer + sizeof(TPacketHeader));
	
	uint16_t type, size;
	uint32_t seq, timestamp;
	if (!NetworkSerialization::ReadUInt16(headerData, offset, type))
		return nullptr;
	header.Type = static_cast<EPacketType>(type);
	if (!NetworkSerialization::ReadUInt16(headerData, offset, size))
		return nullptr;
	header.Size = size;
	if (!NetworkSerialization::ReadUInt32(headerData, offset, seq))
		return nullptr;
	header.Sequence = seq;
	if (!NetworkSerialization::ReadUInt32(headerData, offset, timestamp))
		return nullptr;
	header.Timestamp = timestamp;
	
	// Читаем полный пакет
	std::vector<uint8_t> fullPacket(sizeof(TPacketHeader) + header.Size);
	int totalReceived = 0;
	int targetSize = static_cast<int>(fullPacket.size());
	
	while (totalReceived < targetSize)
	{
		int received = recv(s, reinterpret_cast<char*>(fullPacket.data()) + totalReceived, 
		                   targetSize - totalReceived, 0);
		
		if (received > 0)
		{
			totalReceived += received;
		}
		else if (received == 0)
		{
			// Соединение закрыто
			return nullptr;
		}
		else
		{
			int error = WSAGetLastError();
			if (error == WSAEWOULDBLOCK)
			{
				// Данные еще не дошли, но мы уже начали читать пакет.
				// В идеале мы должны сохранить состояние чтения, но для простоты здесь подождем немного.
				// (Для быстрых сетей это сработает, для плохих — может вызвать лаг).
				Sleep(1);
				continue;
			}
			return nullptr;
		}
	}
	
	// Извлекаем данные пакета
	std::vector<uint8_t> packetData(fullPacket.begin() + sizeof(TPacketHeader), fullPacket.end());
	
	// Создаем соответствующий пакет
	std::unique_ptr<TPacket> packet;
	switch (header.Type)
	{
		case EPacketType::ConnectRequest:
			packet = std::make_unique<TConnectRequestPacket>();
			break;
		case EPacketType::ConnectResponse:
			packet = std::make_unique<TConnectResponsePacket>();
			break;
		case EPacketType::PlayerInput:
			packet = std::make_unique<TPlayerInputPacket>();
			break;
		case EPacketType::PlayerUpdate:
			packet = std::make_unique<TPlayerUpdatePacket>();
			break;
		case EPacketType::EnemyUpdate:
			packet = std::make_unique<TEnemyUpdatePacket>();
			break;
		case EPacketType::BossUpdate:
			packet = std::make_unique<TBossUpdatePacket>();
			break;
		case EPacketType::GameStateUpdate:
			packet = std::make_unique<TGameStateUpdatePacket>();
			break;
		case EPacketType::ExperienceOrbUpdate:
			packet = std::make_unique<TExperienceOrbUpdatePacket>();
			break;
		default:
			return nullptr;
	}
	
	packet->Header = header;
	if (!packet->Deserialize(packetData))
		return nullptr;
	
	if (header.Type == EPacketType::EnemyUpdate)
	{
		ReceivedEnemyUpdates.push_back(*static_cast<TEnemyUpdatePacket*>(packet.get()));
	}
	else if (header.Type == EPacketType::BossUpdate)
	{
		ReceivedBossUpdates.push_back(*static_cast<TBossUpdatePacket*>(packet.get()));
	}
	else if (header.Type == EPacketType::GameStateUpdate)
	{
		ReceivedGameStateUpdate = std::make_unique<TGameStateUpdatePacket>(*static_cast<TGameStateUpdatePacket*>(packet.get()));
	}
	else if (header.Type == EPacketType::ExperienceOrbUpdate)
	{
		ReceivedOrbUpdates.push_back(*static_cast<TExperienceOrbUpdatePacket*>(packet.get()));
	}
	
	PacketsReceived++;
	return packet;
}

//---------------------------------------------------------------------------
bool TNetworkManager::StartHosting(const std::string &playerName, uint16_t port)
{
	if (State != ENetworkState::Disconnected)
		return false;
	
	if (!CreateListenSocket(port))
		return false;
	
	IsHost = true;
	State = ENetworkState::Hosting;
	LocalPlayerID = 0;
	
	// Регистрируем хоста как игрока 0
	Clients[0].PlayerID = 0;
	Clients[0].PlayerName = playerName;
	Clients[0].IsConnected = true;
	Clients[0].LastPacketTime = GetTickCount();
	
	return true;
}

//---------------------------------------------------------------------------
bool TNetworkManager::ConnectToGame(const std::string &hostIP, const std::string &playerName, uint16_t port)
{
	if (State != ENetworkState::Disconnected && State != ENetworkState::Connecting)
		return false;
	
	ClearError();
	State = ENetworkState::Connecting;
	ConnectionStartTime = GetTickCount();
	IsReconnecting = false;
	
	// Сохраняем данные для переподключения
	ReconnectHostIP = hostIP;
	ReconnectPort = port;
	ReconnectPlayerName = playerName;
	
	if (!ConnectToHost(hostIP, port))
	{
		SetError("Failed to connect to host");
		State = ENetworkState::Disconnected;
		return false;
	}
	
	IsHost = false;
	LastPacketTime = GetTickCount();
	
	// ConnectRequest будет отправлен в Update() когда сокет станет готов
	return true;
}

//---------------------------------------------------------------------------
void TNetworkManager::Disconnect()
{
	if (ListenSocket)
	{
		CloseSocket(ListenSocket);
		ListenSocket = nullptr;
	}
	
	if (ClientSocket)
	{
		CloseSocket(ClientSocket);
		ClientSocket = nullptr;
	}
	
	for (void* socket : ClientSockets)
	{
		CloseSocket(socket);
	}
	ClientSockets.clear();
	
	State = ENetworkState::Disconnected;
	IsHost = false;
	Clients.clear();
	Clients.resize(4);
	ReceivedInputs.clear();
}

//---------------------------------------------------------------------------
bool TNetworkManager::SendPlayerInput(const TPlayerInputPacket &input)
{
	if (State != ENetworkState::Connected && State != ENetworkState::Hosting)
		return false;
	
	if (IsHost)
	{
		// Хост обрабатывает ввод локально
		ReceivedInputs.push_back(input);
		return true;
	}
	else
	{
		// Клиент отправляет ввод на сервер
		TPlayerInputPacket packet = input;
		packet.PlayerID = LocalPlayerID;
		return SendPacket(ClientSocket, packet);
	}
}

//---------------------------------------------------------------------------
bool TNetworkManager::BroadcastGameState(const std::vector<TPlayerUpdatePacket> &players)
{
	if (!IsHost || State != ENetworkState::Hosting)
		return false;
	
	// Отправляем обновления всем клиентам
	for (void* clientSocket : ClientSockets)
	{
		for (const auto &player : players)
		{
			SendPacket(clientSocket, player);
		}
	}
	
	return true;
}

//---------------------------------------------------------------------------
bool TNetworkManager::BroadcastPacket(const TPacket &packet)
{
	if (!IsHost || State != ENetworkState::Hosting)
		return false;
	
	bool success = true;
	for (void* clientSocket : ClientSockets)
	{
		if (!SendPacket(clientSocket, packet))
			success = false;
	}
	
	return success;
}

//---------------------------------------------------------------------------
void TNetworkManager::Update(float deltaTime)
{
	uint32_t currentTime = GetTickCount();
	
	if (State == ENetworkState::Hosting)
	{
		AcceptNewConnection();
		ProcessClientPackets();
		CheckClientTimeouts();
	}
	else if (State == ENetworkState::Connecting)
	{
		// Проверяем таймаут подключения
		if (currentTime - ConnectionStartTime > ConnectionTimeout)
		{
			SetError("Connection timeout");
			if (IsReconnecting)
			{
				// Пытаемся переподключиться
				if (TryReconnect())
					return;
			}
			State = ENetworkState::Disconnected;
			IsReconnecting = false;
			return;
		}
		
		// Проверяем, готов ли сокет для записи (соединение установлено)
		if (ClientSocket)
		{
			SOCKET s = reinterpret_cast<SOCKET>(ClientSocket);
			fd_set writeSet;
			fd_set errorSet;
			FD_ZERO(&writeSet);
			FD_ZERO(&errorSet);
			FD_SET(s, &writeSet);
			FD_SET(s, &errorSet);
			
			timeval timeout;
			timeout.tv_sec = 0;
			timeout.tv_usec = 0;
			
			int result = select(0, nullptr, &writeSet, &errorSet, &timeout);
			
			if (result > 0)
			{
				if (FD_ISSET(s, &errorSet))
				{
					// Ошибка подключения
					SetError("Connection failed");
					State = ENetworkState::Disconnected;
					CloseSocket(ClientSocket);
					ClientSocket = nullptr;
					return;
				}
				
				if (FD_ISSET(s, &writeSet))
				{
					// Соединение установлено, отправляем ConnectRequest
					TConnectRequestPacket request;
					request.Header.Type = EPacketType::ConnectRequest;
					request.PlayerName = ReconnectPlayerName;
					request.ProtocolVersion = 1;
					
					if (!SendPacket(ClientSocket, request))
					{
						SetError("Failed to send connection request");
						State = ENetworkState::Disconnected;
						CloseSocket(ClientSocket);
						ClientSocket = nullptr;
						return;
					}
					
					LastPacketTime = currentTime;
				}
			}
		}
		
		// Обрабатываем пакеты от сервера
		std::unique_ptr<TPacket> packet = ReceivePacket(ClientSocket);
		if (packet)
		{
			if (!ValidatePacket(*packet))
			{
				PacketsLost++;
				return;
			}
			
			switch (packet->Header.Type)
			{
				case EPacketType::ConnectResponse:
				{
					TConnectResponsePacket *response = static_cast<TConnectResponsePacket*>(packet.get());
					if (response->Accepted)
					{
						LocalPlayerID = response->PlayerID;
						State = ENetworkState::Connected;
						LastPacketTime = currentTime;
						IsReconnecting = false;
						ClearError();

						// Инициализируем локальный список игроков на клиенте:
						//  - слот 0 зарезервирован под хоста
						//  - LocalPlayerID — это наш игрок
						Clients.clear();
						Clients.resize(4);

						// Хост
						Clients[0].PlayerID = 0;
						Clients[0].PlayerName = "Host";
						Clients[0].IsConnected = true;
						Clients[0].LastPacketTime = currentTime;

						// Локальный игрок (клиент)
						if (LocalPlayerID < Clients.size())
						{
							Clients[LocalPlayerID].PlayerID = LocalPlayerID;
							Clients[LocalPlayerID].PlayerName = ReconnectPlayerName;
							Clients[LocalPlayerID].IsConnected = true;
							Clients[LocalPlayerID].LastPacketTime = currentTime;
						}
					}
					else
					{
						SetError(response->Message.empty() ? "Connection rejected" : response->Message);
						State = ENetworkState::Disconnected;
					}
					break;
				}
				default:
					break;
			}
		}
	}
	else if (State == ENetworkState::Connected)
	{
		// Проверяем таймаут пакетов
		if (currentTime - LastPacketTime > PacketTimeout)
		{
			SetError("Connection lost (timeout)");
			StartReconnect();
			return;
		}
		
		// Обрабатываем все доступные пакеты от сервера
		std::unique_ptr<TPacket> packet;
		while ((packet = ReceivePacket(ClientSocket)) != nullptr)
		{
			if (!ValidatePacket(*packet))
			{
				PacketsLost++;
				continue;
			}
			
			LastPacketTime = currentTime;
			
			// Вычисляем задержку
			uint32_t latency = currentTime - packet->Header.Timestamp;
			AverageLatency = AverageLatency * 0.9f + latency * 0.1f;
			
			switch (packet->Header.Type)
			{
				case EPacketType::PlayerInput:
				{
					TPlayerInputPacket *input = static_cast<TPlayerInputPacket*>(packet.get());
					ReceivedInputs.push_back(*input);
					break;
				}
				case EPacketType::PlayerUpdate:
				{
					TPlayerUpdatePacket *update = static_cast<TPlayerUpdatePacket*>(packet.get());
					ReceivedPlayerUpdates.push_back(*update);
					break;
				}
				case EPacketType::EnemyUpdate:
				{
					TEnemyUpdatePacket *update = static_cast<TEnemyUpdatePacket*>(packet.get());
					ReceivedEnemyUpdates.push_back(*update);
					break;
				}
				case EPacketType::BossUpdate:
				{
					TBossUpdatePacket *update = static_cast<TBossUpdatePacket*>(packet.get());
					ReceivedBossUpdates.push_back(*update);
					break;
				}
				case EPacketType::GameStateUpdate:
				{
					TGameStateUpdatePacket *update = static_cast<TGameStateUpdatePacket*>(packet.get());
					ReceivedGameStateUpdate = std::make_unique<TGameStateUpdatePacket>(*update);
					break;
				}
				case EPacketType::ExperienceOrbUpdate:
				{
					TExperienceOrbUpdatePacket *update = static_cast<TExperienceOrbUpdatePacket*>(packet.get());
					ReceivedOrbUpdates.push_back(*update);
					break;
				}
				case EPacketType::Disconnect:
				{
					SetError("Server disconnected");
					State = ENetworkState::Disconnected;
					return; // Выходим из цикла и функции
				}
				default:
					break;
			}
		}
		
		// Проверяем состояние сокета
		if (ClientSocket)
		{
			char testBuffer[1];
			int result = recv(reinterpret_cast<SOCKET>(ClientSocket), testBuffer, 1, MSG_PEEK);
			if (result == 0 || (result == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK))
			{
				// Соединение разорвано
				SetError("Connection closed");
				StartReconnect();
			}
		}
	}
	
	// Обработка переподключения
	if (IsReconnecting)
	{
		ReconnectTimer += static_cast<uint32_t>(deltaTime * 1000.0f);
		if (ReconnectTimer >= ReconnectDelay)
		{
			ReconnectTimer = 0;
			if (!TryReconnect())
			{
				// Не удалось переподключиться
				IsReconnecting = false;
				State = ENetworkState::Disconnected;
			}
		}
	}
}

//---------------------------------------------------------------------------
void TNetworkManager::AcceptNewConnection()
{
	if (!ListenSocket)
		return;
	
	SOCKET s = reinterpret_cast<SOCKET>(ListenSocket);
	sockaddr_in clientAddr;
	int addrLen = sizeof(clientAddr);
	SOCKET clientSocket = accept(s, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
	
	if (clientSocket != INVALID_SOCKET)
	{
		// Проверяем, есть ли свободные слоты
		bool hasSlot = false;
		for (size_t i = 1; i < Clients.size(); i++)
		{
			if (!Clients[i].IsConnected)
			{
				hasSlot = true;
				break;
			}
		}
		
		if (hasSlot)
		{
			// Устанавливаем неблокирующий режим
			u_long mode = 1;
			ioctlsocket(clientSocket, FIONBIO, &mode);
			
			// Добавляем сокет в список (playerID будет назначен при получении ConnectRequest)
			ClientSockets.push_back(reinterpret_cast<void*>(clientSocket));
		}
		else
		{
			// Нет свободных слотов - отправляем отказ и закрываем соединение
			TConnectResponsePacket response;
			response.Header.Type = EPacketType::ConnectResponse;
			response.Accepted = false;
			response.Message = "Server full";
			
			// Отправляем ответ перед закрытием
			void* tempSocket = reinterpret_cast<void*>(clientSocket);
			SendPacket(tempSocket, response);
			closesocket(clientSocket);
		}
	}
}

//---------------------------------------------------------------------------
void TNetworkManager::ProcessClientPackets()
{
	for (size_t i = 0; i < ClientSockets.size(); i++)
	{
		void* clientSocket = ClientSockets[i];
		if (!clientSocket)
			continue;
		
		std::unique_ptr<TPacket> packet;
		while ((packet = ReceivePacket(clientSocket)) != nullptr)
		{
			// Валидация пакета
			if (!ValidatePacket(*packet))
			{
				PacketsLost++;
				continue;
			}
			
			// Обновляем время последнего пакета для клиента
			// Находим playerID по сокету
			uint8_t playerID = 0;
			for (size_t j = 0; j < ClientSockets.size(); j++)
			{
				if (ClientSockets[j] == clientSocket)
				{
					// Ищем соответствующий ClientInfo
					// В упрощенной версии: ClientSockets[i] соответствует Clients[i+1]
					// Но лучше найти по PlayerID из пакета или по сокету
					// Пока используем упрощенную логику
					if (j < Clients.size() - 1)
					{
						playerID = static_cast<uint8_t>(j + 1);
						Clients[playerID].LastPacketTime = GetTickCount();
					}
					break;
				}
			}
			
			// Если не нашли по сокету, пытаемся получить из пакета
			if (playerID == 0 && packet->Header.Type == EPacketType::PlayerInput)
			{
				TPlayerInputPacket *input = static_cast<TPlayerInputPacket*>(packet.get());
				playerID = input->PlayerID;
				if (playerID < Clients.size())
				{
					Clients[playerID].LastPacketTime = GetTickCount();
				}
			}
			
			switch (packet->Header.Type)
			{
				case EPacketType::ConnectRequest:
				{
					TConnectRequestPacket *request = static_cast<TConnectRequestPacket*>(packet.get());
					HandleConnectRequest(*request, clientSocket);
					break;
				}
				case EPacketType::PlayerInput:
				{
					TPlayerInputPacket *input = static_cast<TPlayerInputPacket*>(packet.get());
					HandlePlayerInput(*input);
					break;
				}
				case EPacketType::Disconnect:
				{
					HandleDisconnect(playerID);
					break;
				}
				default:
					break;
			}
		}
		
		{
			// Проверяем, не разорвано ли соединение
			char testBuffer[1];
			SOCKET s = reinterpret_cast<SOCKET>(clientSocket);
			int result = recv(s, testBuffer, 1, MSG_PEEK);
			if (result == 0 || (result == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK))
			{
				// Соединение разорвано
				uint8_t playerID = 0;
				for (size_t j = 0; j < ClientSockets.size(); j++)
				{
					if (ClientSockets[j] == clientSocket)
					{
						if (j < Clients.size() - 1)
						{
							playerID = static_cast<uint8_t>(j + 1);
						}
						break;
					}
				}
				if (playerID > 0)
				{
					HandleDisconnect(playerID);
				}
			}
		}
	}
}

//---------------------------------------------------------------------------
void TNetworkManager::HandleConnectRequest(const TConnectRequestPacket &packet, void* clientSocket)
{
	// Находим индекс сокета в ClientSockets
	int socketIndex = -1;
	for (size_t i = 0; i < ClientSockets.size(); i++)
	{
		if (ClientSockets[i] == clientSocket)
		{
			socketIndex = static_cast<int>(i);
			break;
		}
	}
	
	if (socketIndex == -1)
	{
		// Сокет не найден - это ошибка
		return;
	}
	
	// Находим свободный слот для игрока
	uint8_t playerID = 0;
	for (size_t i = 1; i < Clients.size(); i++)
	{
		if (!Clients[i].IsConnected)
		{
			playerID = static_cast<uint8_t>(i);
			break;
		}
	}
	
	TConnectResponsePacket response;
	response.Header.Type = EPacketType::ConnectResponse;
	
	if (playerID > 0 && packet.ProtocolVersion == 1)
	{
		response.Accepted = true;
		response.PlayerID = playerID;
		response.ProtocolVersion = 1;
		response.Message = "Connected";
		
		// Регистрируем игрока
		Clients[playerID].PlayerID = playerID;
		Clients[playerID].PlayerName = packet.PlayerName;
		Clients[playerID].IsConnected = true;
		Clients[playerID].LastPacketTime = GetTickCount();
	}
	else
	{
		response.Accepted = false;
		response.Message = playerID == 0 ? "Server full" : "Protocol mismatch";
	}
	
	// Отправляем ответ
	if (!SendPacket(clientSocket, response))
	{
		// Если не удалось отправить ответ, закрываем соединение
		CloseSocket(clientSocket);
		ClientSockets.erase(ClientSockets.begin() + socketIndex);
		if (playerID > 0)
		{
			Clients[playerID].IsConnected = false;
		}
	}
}

//---------------------------------------------------------------------------
void TNetworkManager::HandlePlayerInput(const TPlayerInputPacket &packet)
{
	ReceivedInputs.push_back(packet);
}

//---------------------------------------------------------------------------
void TNetworkManager::HandleDisconnect(uint8_t playerID)
{
	if (playerID >= Clients.size())
		return;
	
	Clients[playerID].IsConnected = false;
	Clients[playerID].PlayerName.clear();
	Clients[playerID].LastPacketTime = 0;
	
	// Удаляем сокет
	if (playerID > 0)
	{
		// В упрощенной версии: ClientSockets[i] соответствует Clients[i+1]
		// Но нужно найти правильный индекс
		size_t socketIndex = static_cast<size_t>(playerID - 1);
		if (socketIndex < ClientSockets.size())
		{
			CloseSocket(ClientSockets[socketIndex]);
			ClientSockets.erase(ClientSockets.begin() + socketIndex);
		}
	}
}

//---------------------------------------------------------------------------
void TNetworkManager::CheckClientTimeouts()
{
	uint32_t currentTime = GetTickCount();
	
	for (size_t i = 1; i < Clients.size(); i++)
	{
		if (Clients[i].IsConnected)
		{
			// Проверяем таймаут
			if (currentTime - Clients[i].LastPacketTime > PacketTimeout)
			{
				// Клиент не отвечает, отключаем
				HandleDisconnect(static_cast<uint8_t>(i));
			}
		}
	}
}

//---------------------------------------------------------------------------
bool TNetworkManager::ValidatePacket(const TPacket &packet) const
{
	if (!ValidatePacketHeader(packet.Header))
		return false;
	
	// Дополнительная валидация в зависимости от типа пакета
	switch (packet.Header.Type)
	{
		case EPacketType::PlayerInput:
		{
			const TPlayerInputPacket *input = dynamic_cast<const TPlayerInputPacket*>(&packet);
			if (!input)
				return false;
			// Проверяем, что PlayerID валидный
			if (input->PlayerID >= 4)
				return false;
			break;
		}
		case EPacketType::PlayerUpdate:
		{
			const TPlayerUpdatePacket *update = dynamic_cast<const TPlayerUpdatePacket*>(&packet);
			if (!update)
				return false;
			if (update->PlayerID >= 4)
				return false;
			// Проверяем разумность значений
			if (update->Health < 0 || update->MaxHealth < 0 || update->Level < 1)
				return false;
			break;
		}
		default:
			break;
	}
	
	return true;
}

//---------------------------------------------------------------------------
bool TNetworkManager::ValidatePacketHeader(const TPacketHeader &header) const
{
	// Проверяем тип пакета
	if (static_cast<uint16_t>(header.Type) < 1 || 
	    static_cast<uint16_t>(header.Type) > static_cast<uint16_t>(EPacketType::ChatMessage))
		return false;
	
	// Проверяем размер (максимальный размер пакета - 64KB)
	if (header.Size > 65535)
		return false;
	
	// Проверяем последовательность (должна быть больше последней полученной)
	if (header.Sequence <= LastReceivedSequence && LastReceivedSequence > 0)
	{
		// Возможно, это дубликат или старый пакет
		// Для TCP это не критично, но логируем
	}
	
	return true;
}

//---------------------------------------------------------------------------
void TNetworkManager::StartReconnect()
{
	if (IsReconnecting || IsHost)
		return;
	
	IsReconnecting = true;
	ReconnectTimer = 0;
	
	// Закрываем текущее соединение
	if (ClientSocket)
	{
		CloseSocket(ClientSocket);
		ClientSocket = nullptr;
	}
	
	State = ENetworkState::Disconnected;
}

//---------------------------------------------------------------------------
bool TNetworkManager::TryReconnect()
{
	if (!IsReconnecting)
		return false;
	
	// Пытаемся переподключиться
	if (ConnectToGame(ReconnectHostIP, ReconnectPlayerName, ReconnectPort))
	{
		// Переподключение начато, ждем ответа
		return true;
	}
	
	return false;
}

//---------------------------------------------------------------------------
std::vector<TPlayerInputPacket> TNetworkManager::GetReceivedInputs()
{
	std::vector<TPlayerInputPacket> inputs = ReceivedInputs;
	ReceivedInputs.clear();
	return inputs;
}

//---------------------------------------------------------------------------
std::vector<TPlayerUpdatePacket> TNetworkManager::GetReceivedPlayerUpdates()
{
	std::vector<TPlayerUpdatePacket> updates = ReceivedPlayerUpdates;
	ReceivedPlayerUpdates.clear();
	return updates;
}

//---------------------------------------------------------------------------
std::vector<TEnemyUpdatePacket> TNetworkManager::GetReceivedEnemyUpdates()
{
	std::vector<TEnemyUpdatePacket> updates = ReceivedEnemyUpdates;
	ReceivedEnemyUpdates.clear();
	return updates;
}

//---------------------------------------------------------------------------
std::vector<TBossUpdatePacket> TNetworkManager::GetReceivedBossUpdates()
{
	std::vector<TBossUpdatePacket> updates = std::move(ReceivedBossUpdates);
	ReceivedBossUpdates.clear();
	return updates;
}

//---------------------------------------------------------------------------
std::vector<TExperienceOrbUpdatePacket> TNetworkManager::GetReceivedOrbUpdates()
{
	std::vector<TExperienceOrbUpdatePacket> updates = std::move(ReceivedOrbUpdates);
	ReceivedOrbUpdates.clear();
	return updates;
}

//---------------------------------------------------------------------------
std::unique_ptr<TGameStateUpdatePacket> TNetworkManager::GetReceivedGameStateUpdate()
{
	return std::move(ReceivedGameStateUpdate);
}

} // namespace NeonGame

//---------------------------------------------------------------------------

