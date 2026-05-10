#include <vcl.h>
#pragma hdrstop

#include "GameNetwork.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <cstdarg>
#include <cstdio>

#pragma comment(lib, "ws2_32.lib")

namespace
{
bool& NetLogEnabled()
{
	static bool enabled = true;
	return enabled;
}

static CRITICAL_SECTION s_NetLogCs;

static BOOL CALLBACK InitNetLogCs(PINIT_ONCE, PVOID, PVOID*)
{
	InitializeCriticalSection(&s_NetLogCs);
	return TRUE;
}

CRITICAL_SECTION& NetLogCs()
{
	static INIT_ONCE once = INIT_ONCE_STATIC_INIT;
	InitOnceExecuteOnce(&once, InitNetLogCs, nullptr, nullptr);
	return s_NetLogCs;
}

void NetLogAppendLine(const char* line)
{
	if (!line)
		return;

	EnterCriticalSection(&NetLogCs());

	HANDLE h = CreateFileA(
		"network_log.txt",
		FILE_APPEND_DATA,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);

	if (h != INVALID_HANDLE_VALUE)
	{
		DWORD written = 0;
		WriteFile(h, line, static_cast<DWORD>(std::strlen(line)), &written, nullptr);
		WriteFile(h, "\r\n", 2, &written, nullptr);
		CloseHandle(h);
	}

	LeaveCriticalSection(&NetLogCs());
}

void NetLogWriteV(const char* fmt, va_list args)
{
	if (!NetLogEnabled() || !fmt)
		return;

	char msg[1024];
	const int n = vsnprintf(msg, sizeof(msg), fmt, args);
	if (n <= 0)
		return;

	const DWORD now = GetTickCount();
	const DWORD tid = GetCurrentThreadId();

	char line[1400];
	_snprintf(line, sizeof(line), "%lu [tid %lu] %s", static_cast<unsigned long>(now), static_cast<unsigned long>(tid), msg);
	line[sizeof(line) - 1] = '\0';
	NetLogAppendLine(line);
}

void NetLogWrite(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	NetLogWriteV(fmt, args);
	va_end(args);
}

bool WaitTcpConnectReady(SOCKET s, int timeoutMs)
{
	fd_set writeFds;
	fd_set exceptFds;
	FD_ZERO(&writeFds);
	FD_ZERO(&exceptFds);
	FD_SET(s, &writeFds);
	FD_SET(s, &exceptFds);

	timeval tv;
	tv.tv_sec = timeoutMs / 1000;
	tv.tv_usec = (timeoutMs % 1000) * 1000;

	const int sel = select(0, nullptr, &writeFds, &exceptFds, &tv);
	if (sel == 0 || sel == SOCKET_ERROR)
		return false;

	int soError = 0;
	int len = sizeof(soError);
	if (getsockopt(s, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&soError), &len) == SOCKET_ERROR)
		return false;
	return soError == 0;
}

constexpr size_t kWireHeaderBytes = 2 + 2 + 4 + 4;
constexpr uint16_t kMaxPacketPayloadBytes = 60 * 1024;

struct TRecvState
{
	std::vector<uint8_t> Buffer;
	size_t ReadPos = 0;
	bool PeerClosed = false;
	int HardError = 0;
};

struct TSendState
{
	std::vector<uint8_t> Buffer;
	size_t SentPos = 0;
	uint64_t TotalQueuedBytes = 0;
	bool LoggedFault = false;
};

std::unordered_map<SOCKET, TRecvState>& RecvStates()
{
	static std::unordered_map<SOCKET, TRecvState> s;
	return s;
}

std::unordered_map<SOCKET, TSendState>& SendStates()
{
	static std::unordered_map<SOCKET, TSendState> s;
	return s;
}

void ForgetSocketState(SOCKET s)
{
	RecvStates().erase(s);
	SendStates().erase(s);
}

bool IsSocketFaulted(SOCKET s)
{
	auto it = RecvStates().find(s);
	if (it == RecvStates().end())
		return false;
	return it->second.PeerClosed || it->second.HardError != 0;
}

bool ConfigureGameSocket(SOCKET s)
{

	int flag = 1;
	setsockopt(s, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&flag), sizeof(flag));

	int bufSize = 256 * 1024;
	setsockopt(s, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&bufSize), sizeof(bufSize));
	setsockopt(s, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&bufSize), sizeof(bufSize));
	return true;
}

inline uint16_t ReadLE16(const uint8_t* p)
{
	return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

inline uint32_t ReadLE32(const uint8_t* p)
{
	return static_cast<uint32_t>(p[0]) |
		(static_cast<uint32_t>(p[1]) << 8) |
		(static_cast<uint32_t>(p[2]) << 16) |
		(static_cast<uint32_t>(p[3]) << 24);
}

void CompactRecvBuffer(TRecvState& st)
{

	if (st.ReadPos == 0)
		return;
	if (st.ReadPos >= st.Buffer.size())
	{
		st.Buffer.clear();
		st.ReadPos = 0;
		return;
	}
	if (st.ReadPos >= 16 * 1024)
	{
		st.Buffer.erase(st.Buffer.begin(), st.Buffer.begin() + static_cast<ptrdiff_t>(st.ReadPos));
		st.ReadPos = 0;
	}
}

void FlushPendingSend(SOCKET s, TSendState& st)
{
	if (IsSocketFaulted(s))
		return;

	while (st.SentPos < st.Buffer.size())
	{
		const char* data = reinterpret_cast<const char*>(st.Buffer.data() + st.SentPos);
		const int remaining = static_cast<int>(st.Buffer.size() - st.SentPos);
		const int sent = send(s, data, remaining, 0);
		if (sent > 0)
		{
			st.SentPos += static_cast<size_t>(sent);
			continue;
		}
		if (sent == SOCKET_ERROR)
		{
			const int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
				return;
			NetLogWrite("send() hard error socket=%llu err=%d queued=%zu sentPos=%zu",
				static_cast<unsigned long long>(s), err, st.Buffer.size(), st.SentPos);
		}

		return;
	}

	st.Buffer.clear();
	st.SentPos = 0;
}
}

namespace NeonGame
{
bool TNetworkManager::InitializeWinSock()
{
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0)
	{
		return false;
	}
	return true;
}

void TNetworkManager::CleanupWinSock()
{
	WSACleanup();
}

bool TNetworkManager::CreateListenSocket(uint16_t port)
{
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)
		return false;

	int opt = 1;
	setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
	{
		closesocket(listenSocket);
		return false;
	}

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		closesocket(listenSocket);
		return false;
	}

	u_long mode = 1;
	ioctlsocket(listenSocket, FIONBIO, &mode);

	ListenSocket = reinterpret_cast<void*>(listenSocket);
	NetLogWrite("listen created socket=%llu port=%u", static_cast<unsigned long long>(listenSocket), static_cast<unsigned>(port));

	char hostName[256] = {0};
	if (gethostname(hostName, sizeof(hostName)) == 0)
	{
		addrinfo hints = {};
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		addrinfo* result = nullptr;
		if (getaddrinfo(hostName, nullptr, &hints, &result) == 0 && result)
		{

			for (addrinfo* it = result; it; it = it->ai_next)
			{
				if (!it->ai_addr || it->ai_addrlen < sizeof(sockaddr_in))
					continue;

				sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(it->ai_addr);
				const uint32_t ipHostOrder = ntohl(addr->sin_addr.s_addr);

				if ((ipHostOrder & 0xFF000000u) == 0x7F000000u)
					continue;

				const bool is10 = (ipHostOrder & 0xFF000000u) == 0x0A000000u;
				const bool is172 = (ipHostOrder & 0xFFF00000u) == 0xAC100000u;
				const bool is192 = (ipHostOrder & 0xFFFF0000u) == 0xC0A80000u;
				if (!is10 && !is172 && !is192)
					continue;

				char ipStr[INET_ADDRSTRLEN] = {0};
				if (inet_ntop(AF_INET, &(addr->sin_addr), ipStr, INET_ADDRSTRLEN))
				{
					LocalIPAddress = ipStr;
					break;
				}
			}

			if (LocalIPAddress.empty())
			{
				sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(result->ai_addr);
				char ipStr[INET_ADDRSTRLEN] = {0};
				if (inet_ntop(AF_INET, &(addr->sin_addr), ipStr, INET_ADDRSTRLEN))
					LocalIPAddress = ipStr;
			}
			freeaddrinfo(result);
		}
	}

	return true;
}

bool TNetworkManager::ConnectToHost(const std::string &hostIP, uint16_t port)
{
	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket == INVALID_SOCKET)
		return false;

	sockaddr_in serverAddr = {};
	serverAddr.sin_family = AF_INET;
	if (inet_pton(AF_INET, hostIP.c_str(), &serverAddr.sin_addr) != 1)
	{
		closesocket(clientSocket);
		return false;
	}
	serverAddr.sin_port = htons(port);

	u_long mode = 1;
	ioctlsocket(clientSocket, FIONBIO, &mode);

	int result = connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
	if (result == SOCKET_ERROR)
	{
		const int error = WSAGetLastError();
		if (error != WSAEWOULDBLOCK && error != WSAEINPROGRESS)
		{
			NetLogWrite("connect failed socket=%llu ip=%s port=%u err=%d",
				static_cast<unsigned long long>(clientSocket), hostIP.c_str(), static_cast<unsigned>(port), error);
			closesocket(clientSocket);
			return false;
		}

		if (!WaitTcpConnectReady(clientSocket, 10000))
		{
			NetLogWrite("connect timeout socket=%llu ip=%s port=%u",
				static_cast<unsigned long long>(clientSocket), hostIP.c_str(), static_cast<unsigned>(port));
			closesocket(clientSocket);
			return false;
		}
	}

	ConfigureGameSocket(clientSocket);
	NetLogWrite("connect ok socket=%llu ip=%s port=%u", static_cast<unsigned long long>(clientSocket), hostIP.c_str(), static_cast<unsigned>(port));
	ClientSocket = reinterpret_cast<void*>(clientSocket);
	return true;
}

void TNetworkManager::CloseSocket(void* socket)
{
	if (!socket)
		return;
	SOCKET s = reinterpret_cast<SOCKET>(socket);
	NetLogWrite("socket close socket=%llu", static_cast<unsigned long long>(s));
	ForgetSocketState(s);
	closesocket(s);
}

bool TNetworkManager::SendPacket(void* socket, const TPacket &packet)
{
	if (!socket)
		return false;

	SOCKET s = reinterpret_cast<SOCKET>(socket);

	auto& sendState = SendStates()[s];
	if (IsSocketFaulted(s))
	{
		if (!sendState.LoggedFault)
		{
			auto it = RecvStates().find(s);
			const int err = (it != RecvStates().end()) ? it->second.HardError : 0;
			NetLogWrite("SendPacket skipped (socket faulted) socket=%llu err=%d", static_cast<unsigned long long>(s), err);
			sendState.LoggedFault = true;
		}
		return false;
	}
	if (!sendState.Buffer.empty())
		FlushPendingSend(s, sendState);

	std::vector<uint8_t> data = packet.Serialize();
	TPacketHeader header(packet.Header.Type, static_cast<uint16_t>(data.size()));
	header.Sequence = NextSequence++;
	header.Timestamp = GetTickCount();

	std::vector<uint8_t> buffer;
	buffer.reserve(kWireHeaderBytes + data.size());
	NetworkSerialization::WriteUInt16(buffer, static_cast<uint16_t>(header.Type));
	NetworkSerialization::WriteUInt16(buffer, header.Size);
	NetworkSerialization::WriteUInt32(buffer, header.Sequence);
	NetworkSerialization::WriteUInt32(buffer, header.Timestamp);
	buffer.insert(buffer.end(), data.begin(), data.end());

	const int sent = send(s, reinterpret_cast<const char*>(buffer.data()), static_cast<int>(buffer.size()), 0);
	if (sent == SOCKET_ERROR)
	{
		const int err = WSAGetLastError();
		if (err != WSAEWOULDBLOCK)
		{
			NetLogWrite("SendPacket send() error socket=%llu type=%u bytes=%zu err=%d",
				static_cast<unsigned long long>(s), static_cast<unsigned>(header.Type), buffer.size(), err);
			return false;
		}

		sendState.Buffer.insert(sendState.Buffer.end(), buffer.begin(), buffer.end());
		sendState.SentPos = 0;
		sendState.TotalQueuedBytes += buffer.size();
	}
	else if (sent < static_cast<int>(buffer.size()))
	{

		sendState.Buffer.insert(sendState.Buffer.end(), buffer.begin() + sent, buffer.end());
		sendState.SentPos = 0;
		sendState.TotalQueuedBytes += (buffer.size() - static_cast<size_t>(sent));
	}

	PacketsSent++;
	return true;
}

std::unique_ptr<TPacket> TNetworkManager::ReceivePacket(void* socket)
{
	if (!socket)
		return nullptr;

	SOCKET s = reinterpret_cast<SOCKET>(socket);

	auto& recvState = RecvStates()[s];
	if (recvState.Buffer.empty())
		recvState.Buffer.reserve(8 * 1024);
	if (recvState.PeerClosed || recvState.HardError != 0)
		return nullptr;

	static DWORD lastStatLogMs = 0;
	static uint64_t recvBytes = 0;
	static uint64_t parsedPackets = 0;

	for (;;)
	{
		uint8_t temp[4096];
		const int received = recv(s, reinterpret_cast<char*>(temp), static_cast<int>(sizeof(temp)), 0);
		if (received > 0)
		{
			recvState.Buffer.insert(recvState.Buffer.end(), temp, temp + received);
			recvBytes += static_cast<uint64_t>(received);
			continue;
		}
		if (received == 0)
		{

			NetLogWrite("recv() closed by peer socket=%llu", static_cast<unsigned long long>(s));
			recvState.PeerClosed = true;
			return nullptr;
		}

		const int err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK)
			break;
		NetLogWrite("recv() hard error socket=%llu err=%d", static_cast<unsigned long long>(s), err);
		recvState.HardError = err;
		return nullptr;
	}

	CompactRecvBuffer(recvState);

	const size_t available = (recvState.Buffer.size() >= recvState.ReadPos)
		? (recvState.Buffer.size() - recvState.ReadPos)
		: 0;
	if (available < kWireHeaderBytes)
		return nullptr;

	const uint8_t* p = recvState.Buffer.data() + recvState.ReadPos;

	TPacketHeader header;
	header.Type = static_cast<EPacketType>(ReadLE16(p + 0));
	header.Size = ReadLE16(p + 2);
	header.Sequence = ReadLE32(p + 4);
	header.Timestamp = ReadLE32(p + 8);

	if (header.Size > kMaxPacketPayloadBytes)
	{
		NetLogWrite("invalid packet size socket=%llu type=%u size=%u", static_cast<unsigned long long>(s),
			static_cast<unsigned>(header.Type), static_cast<unsigned>(header.Size));
		return nullptr;
	}

	const size_t totalBytes = kWireHeaderBytes + static_cast<size_t>(header.Size);
	if (available < totalBytes)
		return nullptr;

	std::vector<uint8_t> packetData;
	packetData.resize(header.Size);
	if (header.Size > 0)
	{
		std::memcpy(packetData.data(), p + kWireHeaderBytes, header.Size);
	}

	recvState.ReadPos += totalBytes;
	parsedPackets++;

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
		case EPacketType::GameStateUpdate:
			packet = std::make_unique<TGameStateUpdatePacket>();
			break;
		case EPacketType::PlayerUpdate:
			packet = std::make_unique<TPlayerUpdatePacket>();
			break;
		case EPacketType::EnemyUpdate:
			packet = std::make_unique<TEnemyUpdatePacket>();
			break;
		case EPacketType::EnemyBulkUpdate:
			packet = std::make_unique<TEnemyBulkUpdatePacket>();
			break;
		case EPacketType::ExpOrbBulkUpdate:
			packet = std::make_unique<TExpOrbBulkUpdatePacket>();
			break;
		case EPacketType::WaveUpdate:
			packet = std::make_unique<TWaveUpdatePacket>();
			break;
		case EPacketType::UpgradeChoices:
			packet = std::make_unique<TUpgradeChoicesPacket>();
			break;
		case EPacketType::UpgradeSelect:
			packet = std::make_unique<TUpgradeSelectPacket>();
			break;
		case EPacketType::BulletUpdate:
			packet = std::make_unique<TBulletBulkUpdatePacket>();
			break;
		case EPacketType::BossUpdate:
			packet = std::make_unique<TBossUpdatePacket>();
			break;
		case EPacketType::ReturnToLobby:
		case EPacketType::RequestReturnToLobby:
			packet = std::make_unique<TSignalPacket>();
			break;
		default:
			NetLogWrite("unknown packet type socket=%llu type=%u size=%u", static_cast<unsigned long long>(s),
				static_cast<unsigned>(header.Type), static_cast<unsigned>(header.Size));
			return nullptr;
	}

	packet->Header = header;
	if (!packet->Deserialize(packetData))
	{
		NetLogWrite("Deserialize failed socket=%llu type=%u size=%u", static_cast<unsigned long long>(s),
			static_cast<unsigned>(header.Type), static_cast<unsigned>(header.Size));
		return nullptr;
	}

	PacketsReceived++;

	const DWORD now = GetTickCount();
	if (now - lastStatLogMs >= 1000)
	{
		auto it = SendStates().find(s);
		const size_t queued = (it != SendStates().end()) ? it->second.Buffer.size() : 0;
		NetLogWrite("netstat socket=%llu recvBytes/s=%llu parsedPkts/s=%llu recvBuf=%zu queuedSend=%zu",
			static_cast<unsigned long long>(s),
			static_cast<unsigned long long>(recvBytes),
			static_cast<unsigned long long>(parsedPackets),
			(recvState.Buffer.size() - recvState.ReadPos),
			queued);
		recvBytes = 0;
		parsedPackets = 0;
		lastStatLogMs = now;
	}

	return packet;
}

bool TNetworkManager::ConsumeSocketFault(void* socket, int &outErr)
{
	outErr = 0;
	if (!socket)
		return false;
	SOCKET s = reinterpret_cast<SOCKET>(socket);
	auto it = RecvStates().find(s);
	if (it == RecvStates().end())
		return false;

	const bool faulted = it->second.PeerClosed || it->second.HardError != 0;
	if (!faulted)
		return false;

	outErr = it->second.HardError;
	it->second.PeerClosed = false;
	it->second.HardError = 0;

	auto sit = SendStates().find(s);
	if (sit != SendStates().end())
		sit->second.LoggedFault = false;
	return true;
}
}
