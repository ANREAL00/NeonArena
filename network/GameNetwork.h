

#ifndef GameNetworkH
#define GameNetworkH

#include <System.Types.hpp>
#include <vector>
#include <memory>
#include <string>

namespace NeonGame
{

enum class EPacketType : uint16_t
{
	ConnectRequest = 1,
	ConnectResponse = 2,
	Disconnect = 3,
	PlayerInput = 4,
	GameStateUpdate = 5,
	PlayerUpdate = 6,
	EnemyUpdate = 7,
	BulletUpdate = 8,
	WaveUpdate = 9,
	ChatMessage = 10,
	EnemyBulkUpdate = 11,
	ExpOrbBulkUpdate = 12,
	UpgradeChoices = 13,
	UpgradeSelect = 14,
	ReturnToLobby = 15,
	RequestReturnToLobby = 16,
	BossUpdate = 17
};

struct TPacketHeader
{
	EPacketType Type;
	uint16_t Size;
	uint32_t Sequence;
	uint32_t Timestamp;

	TPacketHeader() : Type(EPacketType::ConnectRequest), Size(0), Sequence(0), Timestamp(0) {}
	TPacketHeader(EPacketType type, uint16_t size)
		: Type(type), Size(size), Sequence(0), Timestamp(0) {}
};

class TPacket
{
public:
	TPacketHeader Header;

	virtual ~TPacket() {}
	virtual std::vector<uint8_t> Serialize() const = 0;
	virtual bool Deserialize(const std::vector<uint8_t> &data) = 0;
	virtual size_t GetDataSize() const = 0;
};

struct TConnectRequestPacket : public TPacket
{
	std::string PlayerName;
	uint32_t ProtocolVersion;

	TConnectRequestPacket() : ProtocolVersion(1) {}

	std::vector<uint8_t> Serialize() const override;
	bool Deserialize(const std::vector<uint8_t> &data) override;
	size_t GetDataSize() const override;
};

struct TConnectResponsePacket : public TPacket
{
	bool Accepted;
	uint8_t PlayerID;
	uint32_t ProtocolVersion;
	std::string Message;

	TConnectResponsePacket() : Accepted(false), PlayerID(0), ProtocolVersion(1) {}

	std::vector<uint8_t> Serialize() const override;
	bool Deserialize(const std::vector<uint8_t> &data) override;
	size_t GetDataSize() const override;
};

struct TPlayerInputPacket : public TPacket
{
	uint8_t PlayerID;
	bool InputUp;
	bool InputDown;
	bool InputLeft;
	bool InputRight;
	bool IsShooting;
	bool IsAltShooting;
	float MouseX;
	float MouseY;
	uint32_t FrameNumber;

	TPlayerInputPacket()
		: PlayerID(0), InputUp(false), InputDown(false),
		  InputLeft(false), InputRight(false), IsShooting(false), IsAltShooting(false),
		  MouseX(0.0f), MouseY(0.0f), FrameNumber(0) {}

	std::vector<uint8_t> Serialize() const override;
	bool Deserialize(const std::vector<uint8_t> &data) override;
	size_t GetDataSize() const override;
};

struct TPlayerUpdatePacket : public TPacket
{
	uint8_t PlayerID;
	float PositionX;
	float PositionY;
	float FacingDirectionX;
	float FacingDirectionY;
	int Health;
	int MaxHealth;
	int Level;
	int Experience;
	uint32_t FrameNumber;
	float SpeedMultiplier;

	TPlayerUpdatePacket()
		: PlayerID(0), PositionX(0.0f), PositionY(0.0f),
		  FacingDirectionX(0.0f), FacingDirectionY(-1.0f),
		  Health(100), MaxHealth(100), Level(1), Experience(0), FrameNumber(0), SpeedMultiplier(1.0f) {}

	std::vector<uint8_t> Serialize() const override;
	bool Deserialize(const std::vector<uint8_t> &data) override;
	size_t GetDataSize() const override;
};

struct TGameStateUpdatePacket : public TPacket
{
	std::vector<TPlayerUpdatePacket> Players;

	TGameStateUpdatePacket() = default;

	std::vector<uint8_t> Serialize() const override;
	bool Deserialize(const std::vector<uint8_t> &data) override;
	size_t GetDataSize() const override;
};

struct TEnemyUpdatePacket : public TPacket
{
	uint16_t EnemyID;
	uint8_t EnemyType;
	float PositionX;
	float PositionY;
	int Health;
	int MaxHealth;
	bool IsAlive;
	uint32_t NetInstanceId;

	TEnemyUpdatePacket()
		: EnemyID(0), EnemyType(0), PositionX(0.0f), PositionY(0.0f),
		  Health(0), MaxHealth(0), IsAlive(false), NetInstanceId(0) {}

	std::vector<uint8_t> Serialize() const override;
	bool Deserialize(const std::vector<uint8_t> &data) override;
	size_t GetDataSize() const override;
};

struct TEnemyBulkUpdatePacket : public TPacket
{
	std::vector<TEnemyUpdatePacket> Enemies;

	TEnemyBulkUpdatePacket() = default;

	std::vector<uint8_t> Serialize() const override;
	bool Deserialize(const std::vector<uint8_t> &data) override;
	size_t GetDataSize() const override;
};

struct TExpOrbNetPacket
{
	uint32_t NetInstanceId = 0;
	float PositionX = 0.0f;
	float PositionY = 0.0f;
	int32_t ExpValue = 0;
	float Lifetime = 0.0f;
};

struct TExpOrbBulkUpdatePacket : public TPacket
{
	std::vector<TExpOrbNetPacket> Orbs;

	TExpOrbBulkUpdatePacket() = default;

	std::vector<uint8_t> Serialize() const override;
	bool Deserialize(const std::vector<uint8_t> &data) override;
	size_t GetDataSize() const override;
};

struct TWaveUpdatePacket : public TPacket
{
	uint32_t WaveNumber = 0;
	uint8_t WaveState = 0;
	float CooldownRemaining = 0.0f;

	float RunTimeSeconds = 0.0f;
	bool IncludesRunTimeSeconds = false;

	std::vector<uint8_t> Serialize() const override;
	bool Deserialize(const std::vector<uint8_t> &data) override;
	size_t GetDataSize() const override;
};

struct TUpgradeChoiceNet
{
	uint8_t Type = 0;
	uint8_t Rarity = 0;
};

struct TUpgradeChoicesPacket : public TPacket
{
	uint8_t PlayerID = 0;
	bool IsWaiting = false;
	std::vector<TUpgradeChoiceNet> Choices;

	std::vector<uint8_t> Serialize() const override;
	bool Deserialize(const std::vector<uint8_t> &data) override;
	size_t GetDataSize() const override;
};

struct TUpgradeSelectPacket : public TPacket
{
	uint8_t PlayerID = 0;
	uint8_t ChoiceIndex = 0;

	std::vector<uint8_t> Serialize() const override;
	bool Deserialize(const std::vector<uint8_t> &data) override;
	size_t GetDataSize() const override;
};

struct TSignalPacket : public TPacket
{
	std::vector<uint8_t> Serialize() const override;
	bool Deserialize(const std::vector<uint8_t> &data) override;
	size_t GetDataSize() const override;
};

struct TBossUpdatePacket : public TPacket
{
	bool IsAlive = false;
	float PositionX = 0.0f;
	float PositionY = 0.0f;
	int32_t Health = 0;
	int32_t MaxHealth = 0;
	uint8_t Phase = 0;

	std::vector<uint8_t> Serialize() const override;
	bool Deserialize(const std::vector<uint8_t> &data) override;
	size_t GetDataSize() const override;
};

struct TBulletNetPacket
{
	float PositionX = 0.0f;
	float PositionY = 0.0f;
	bool IsPlayerBullet = true;
};

struct TBulletBulkUpdatePacket : public TPacket
{
	std::vector<TBulletNetPacket> Bullets;

	std::vector<uint8_t> Serialize() const override;
	bool Deserialize(const std::vector<uint8_t> &data) override;
	size_t GetDataSize() const override;
};

struct TClientInfo
{
	uint8_t PlayerID;
	std::string PlayerName;
	bool IsConnected;
	uint32_t LastPacketTime;

	TClientInfo() : PlayerID(0), IsConnected(false), LastPacketTime(0) {}
};

enum class ENetworkState
{
	Disconnected,
	Connecting,
	Connected,
	Hosting
};

class TNetworkManager
{
private:
	ENetworkState State;
	bool IsHost;
	uint8_t LocalPlayerID;

	void* ListenSocket;
	void* ClientSocket;
	std::vector<void*> ClientSockets;

	std::vector<TClientInfo> Clients;

	uint32_t NextSequence;
	uint32_t LastReceivedSequence;

	static constexpr uint16_t DefaultPort = 7777;
	static constexpr uint32_t ConnectionTimeout = 10000;
	static constexpr uint32_t PacketTimeout = 120000;
	static constexpr uint32_t ReconnectDelay = 2000;

	uint32_t ConnectionStartTime;
	uint32_t LastPacketTime;
	uint32_t ReconnectTimer;
	bool IsReconnecting;

	std::string ReconnectHostIP;
	uint16_t ReconnectPort;
	std::string ReconnectPlayerName;

	std::string LocalIPAddress;

	bool InitializeWinSock();
	void CleanupWinSock();
	bool CreateListenSocket(uint16_t port);
	bool ConnectToHost(const std::string &hostIP, uint16_t port);
	void CloseSocket(void* socket);

	bool SendPacket(void* socket, const TPacket &packet);
	std::unique_ptr<TPacket> ReceivePacket(void* socket);
	bool ConsumeSocketFault(void* socket, int &outErr);

	void AcceptNewConnection();
	void ProcessClientPackets();
	void CheckClientTimeouts();

	void HandleConnectRequest(const TConnectRequestPacket &packet, void* clientSocket);
	void HandlePlayerInput(const TPlayerInputPacket &packet);
	void HandleDisconnect(uint8_t playerID);

	bool ValidatePacket(const TPacket &packet) const;
	bool ValidatePacketHeader(const TPacketHeader &header) const;

	void StartReconnect();
	bool TryReconnect();

public:
	TNetworkManager();
	~TNetworkManager();

	bool Initialize();
	void Shutdown();

	bool StartHosting(const std::string &playerName, uint16_t port = DefaultPort);
	bool ConnectToGame(const std::string &hostIP, const std::string &playerName, uint16_t port = DefaultPort);
	void Disconnect();

	void EndClientGameSession();

	bool SendPlayerInput(const TPlayerInputPacket &input);
	bool SendUpgradeSelect(uint8_t choiceIndex);
	bool BroadcastReturnToLobby();
	bool SendReturnToLobbyRequest();
	bool ConsumePendingReturnToLobby();
	bool BroadcastGameState(const std::vector<TPlayerUpdatePacket> &players);
	bool BroadcastEnemyState(const std::vector<TEnemyUpdatePacket> &enemies);
	bool BroadcastExpOrbState(const std::vector<TExpOrbNetPacket> &orbs);
	bool BroadcastWaveState(const TWaveUpdatePacket &wave);
	bool BroadcastUpgradeChoices(const std::vector<TUpgradeChoicesPacket> &choices);
	bool BroadcastBossState(const TBossUpdatePacket &boss);
	bool BroadcastBulletState(const std::vector<TBulletNetPacket> &bullets);

	void Update(float deltaTime);
	std::vector<TPlayerInputPacket> GetReceivedInputs();
	std::vector<TPlayerUpdatePacket> GetReceivedPlayerUpdates();
	std::vector<TEnemyUpdatePacket> GetReceivedEnemyUpdates();
	std::vector<TExpOrbNetPacket> GetReceivedExpOrbUpdates();
	std::vector<TWaveUpdatePacket> GetReceivedWaveUpdates();
	std::vector<TUpgradeChoicesPacket> GetReceivedUpgradeChoices();
	std::vector<TUpgradeSelectPacket> GetReceivedUpgradeSelects();
	std::vector<TBossUpdatePacket> GetReceivedBossUpdates();
	std::vector<TBulletNetPacket> GetReceivedBulletUpdates();

	int GetLastEnemyBulkEntryCount() const { return LastEnemyBulkEntryCount; }
	int GetLastExpOrbBulkEntryCount() const { return LastExpOrbBulkEntryCount; }

	ENetworkState GetState() const { return State; }
	bool IsHosting() const { return IsHost; }
	uint8_t GetLocalPlayerID() const { return LocalPlayerID; }
	const std::vector<TClientInfo> &GetClients() const { return Clients; }
	const std::string &GetLocalIPAddress() const { return LocalIPAddress; }
	std::string GetPlayerName(uint8_t playerID) const;

	uint32_t GetPacketsSent() const { return PacketsSent; }
	uint32_t GetPacketsReceived() const { return PacketsReceived; }
	uint32_t GetPacketsLost() const { return PacketsLost; }
	float GetAverageLatency() const { return AverageLatency; }

	std::string GetLastError() const { return LastError; }

private:
	uint32_t PacketsSent;
	uint32_t PacketsReceived;
	uint32_t PacketsLost;
	float AverageLatency;

	std::vector<TPlayerInputPacket> ReceivedInputs;
	std::vector<TPlayerUpdatePacket> ReceivedPlayerUpdates;
	std::vector<TEnemyUpdatePacket> ReceivedEnemyUpdates;
	std::vector<TExpOrbNetPacket> ReceivedExpOrbUpdates;
	std::vector<TWaveUpdatePacket> ReceivedWaveUpdates;
	std::vector<TUpgradeChoicesPacket> ReceivedUpgradeChoices;
	std::vector<TUpgradeSelectPacket> ReceivedUpgradeSelects;
	std::vector<TBossUpdatePacket> ReceivedBossUpdates;
	std::vector<TBulletNetPacket> ReceivedBulletUpdates;
	int LastEnemyBulkEntryCount = -1;
	int LastExpOrbBulkEntryCount = -1;

	bool PendingReturnToLobby = false;

	std::string LastError;
	void SetError(const std::string &error) { LastError = error; }
	void ClearError() { LastError.clear(); }
};

namespace NetworkSerialization
{
	void WriteUInt8(std::vector<uint8_t> &buffer, uint8_t value);
	void WriteUInt16(std::vector<uint8_t> &buffer, uint16_t value);
	void WriteUInt32(std::vector<uint8_t> &buffer, uint32_t value);
	void WriteFloat(std::vector<uint8_t> &buffer, float value);
	void WriteBool(std::vector<uint8_t> &buffer, bool value);
	void WriteString(std::vector<uint8_t> &buffer, const std::string &str);
	void WritePointF(std::vector<uint8_t> &buffer, const TPointF &point);

	bool ReadUInt8(const std::vector<uint8_t> &buffer, size_t &offset, uint8_t &value);
	bool ReadUInt16(const std::vector<uint8_t> &buffer, size_t &offset, uint16_t &value);
	bool ReadUInt32(const std::vector<uint8_t> &buffer, size_t &offset, uint32_t &value);
	bool ReadFloat(const std::vector<uint8_t> &buffer, size_t &offset, float &value);
	bool ReadBool(const std::vector<uint8_t> &buffer, size_t &offset, bool &value);
	bool ReadString(const std::vector<uint8_t> &buffer, size_t &offset, std::string &value);
	bool ReadPointF(const std::vector<uint8_t> &buffer, size_t &offset, TPointF &point);
}
}
#endif
