#include <vcl.h>
#pragma hdrstop

#include "GameNetwork.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <cstdarg>
#include <cstdio>

namespace NeonGame
{
namespace
{
static CRITICAL_SECTION s_HostLogCs;

static BOOL CALLBACK InitHostLogCs(PINIT_ONCE, PVOID, PVOID*)
{
	InitializeCriticalSection(&s_HostLogCs);
	return TRUE;
}

CRITICAL_SECTION& NetLogCs()
{
	static INIT_ONCE once = INIT_ONCE_STATIC_INIT;
	InitOnceExecuteOnce(&once, InitHostLogCs, nullptr, nullptr);
	return s_HostLogCs;
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
	if (!fmt)
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
}

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
		u_long mode = 1;
		ioctlsocket(clientSocket, FIONBIO, &mode);

		int flag = 1;
		setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&flag), sizeof(flag));

		int bufSize = 256 * 1024;
		setsockopt(clientSocket, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&bufSize), sizeof(bufSize));
		setsockopt(clientSocket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&bufSize), sizeof(bufSize));

		NetLogWrite("accept ok socket=%llu", static_cast<unsigned long long>(clientSocket));
		ClientSockets.push_back(reinterpret_cast<void*>(clientSocket));
	}
}

void TNetworkManager::ProcessClientPackets()
{
	for (size_t i = 0; i < ClientSockets.size(); i++)
	{
		void* clientSocket = ClientSockets[i];
		if (!clientSocket)
			continue;

		std::unique_ptr<TPacket> packet = ReceivePacket(clientSocket);
		bool processedAnyPacket = false;
		while (packet)
		{
			processedAnyPacket = true;
			if (!ValidatePacket(*packet))
			{
				PacketsLost++;
				packet = ReceivePacket(clientSocket);
				continue;
			}

			uint8_t playerID = 0;
			for (size_t j = 0; j < ClientSockets.size(); j++)
			{
				if (ClientSockets[j] == clientSocket)
				{
					if (j < Clients.size() - 1)
					{
						playerID = static_cast<uint8_t>(j + 1);
						Clients[playerID].LastPacketTime = GetTickCount();
					}
					break;
				}
			}

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
				case EPacketType::UpgradeSelect:
				{
					TUpgradeSelectPacket *sel = static_cast<TUpgradeSelectPacket*>(packet.get());
					ReceivedUpgradeSelects.push_back(*sel);
					break;
				}
				case EPacketType::Disconnect:
				{
					HandleDisconnect(playerID);
					break;
				}
				case EPacketType::RequestReturnToLobby:
				{
					BroadcastReturnToLobby();
					PendingReturnToLobby = true;
					break;
				}
				default:
					break;
			}

			packet = ReceivePacket(clientSocket);
		}

		int sockErr = 0;
		if (ConsumeSocketFault(clientSocket, sockErr))
		{
			NetLogWrite("client socket faulted socket=%llu err=%d", static_cast<unsigned long long>(reinterpret_cast<SOCKET>(clientSocket)), sockErr);
			uint8_t playerID = 0;
			for (size_t j = 0; j < ClientSockets.size(); j++)
			{
				if (ClientSockets[j] == clientSocket)
				{
					if (j < Clients.size() - 1)
						playerID = static_cast<uint8_t>(j + 1);
					break;
				}
			}
			if (playerID > 0)
				HandleDisconnect(playerID);
			continue;
		}

		if (!processedAnyPacket)
		{
			char testBuffer[1];
			SOCKET s = reinterpret_cast<SOCKET>(clientSocket);
			int result = recv(s, testBuffer, 1, MSG_PEEK);
			if (result == 0 || (result == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK))
			{
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

void TNetworkManager::HandleConnectRequest(const TConnectRequestPacket &packet, void* clientSocket)
{
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

		Clients[playerID].PlayerID = playerID;
		Clients[playerID].PlayerName = packet.PlayerName;
		Clients[playerID].IsConnected = true;
	}
	else
	{
		response.Accepted = false;
		response.Message = playerID == 0 ? "Server full" : "Protocol mismatch";
	}

	SendPacket(clientSocket, response);

	if (!response.Accepted)
	{
		CloseSocket(clientSocket);
		ClientSockets.erase(
			std::remove(ClientSockets.begin(), ClientSockets.end(), clientSocket),
			ClientSockets.end());
	}
}

void TNetworkManager::HandlePlayerInput(const TPlayerInputPacket &packet)
{
	ReceivedInputs.push_back(packet);
}

void TNetworkManager::HandleDisconnect(uint8_t playerID)
{
	if (playerID >= Clients.size())
		return;

	Clients[playerID].IsConnected = false;
	Clients[playerID].PlayerName.clear();
	Clients[playerID].LastPacketTime = 0;

	if (playerID > 0)
	{
		size_t socketIndex = static_cast<size_t>(playerID - 1);
		if (socketIndex < ClientSockets.size())
		{
			CloseSocket(ClientSockets[socketIndex]);
			ClientSockets.erase(ClientSockets.begin() + socketIndex);
		}
	}
}

void TNetworkManager::CheckClientTimeouts()
{
	uint32_t currentTime = GetTickCount();

	for (size_t i = 1; i < Clients.size(); i++)
	{
		if (Clients[i].IsConnected)
		{
			if (currentTime - Clients[i].LastPacketTime > PacketTimeout)
			{
				HandleDisconnect(static_cast<uint8_t>(i));
			}
		}
	}
}
}
