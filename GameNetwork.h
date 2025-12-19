//---------------------------------------------------------------------------
// Сетевая инфраструктура для кооперативного режима (Neon Arena)
//---------------------------------------------------------------------------
#ifndef GameNetworkH
#define GameNetworkH
//---------------------------------------------------------------------------

#include <System.Types.hpp>
#include <vector>
#include <memory>
#include <string>

//---------------------------------------------------------------------------
// Namespace для сетевых типов
//---------------------------------------------------------------------------
namespace NeonGame
{

// Типы пакетов
enum class EPacketType : uint16_t
{
	ConnectRequest = 1,    // запрос на подключение
	ConnectResponse = 2,   // ответ на подключение
	Disconnect = 3,        // отключение
	PlayerInput = 4,       // ввод игрока
	GameStateUpdate = 5,   // обновление состояния игры
	PlayerUpdate = 6,      // обновление конкретного игрока
	EnemyUpdate = 7,       // обновление врагов
	BulletUpdate = 8,      // обновление пуль
	WaveUpdate = 9,        // обновление волны
	ChatMessage = 10,      // сообщение в чат (опционально)
	BossUpdate = 11,       // обновление босса
	ExperienceOrbUpdate = 12, // обновление сфер опыта
	PlayerUpgrade = 13      // выбор улучшения игроком
};

// Заголовок пакета
struct TPacketHeader
{
	EPacketType Type;      // тип пакета
	uint16_t Size;         // размер данных (без заголовка)
	uint32_t Sequence;     // порядковый номер (для надежности)
	uint32_t Timestamp;    // временная метка
	
	TPacketHeader() : Type(EPacketType::ConnectRequest), Size(0), Sequence(0), Timestamp(0) {}
	TPacketHeader(EPacketType type, uint16_t size) 
		: Type(type), Size(size), Sequence(0), Timestamp(0) {}
};

// Базовый класс для пакетов
class TPacket
{
public:
	TPacketHeader Header;
	
	virtual ~TPacket() {}
	virtual std::vector<uint8_t> Serialize() const = 0;
	virtual bool Deserialize(const std::vector<uint8_t> &data) = 0;
	virtual size_t GetDataSize() const = 0;
};

// Пакет запроса подключения
struct TConnectRequestPacket : public TPacket
{
	std::string PlayerName;  // имя игрока
	uint32_t ProtocolVersion; // версия протокола
	
	TConnectRequestPacket() : ProtocolVersion(1) {}
	
	std::vector<uint8_t> Serialize() const override;
	bool Deserialize(const std::vector<uint8_t> &data) override;
	size_t GetDataSize() const override;
};

// Пакет ответа на подключение
struct TConnectResponsePacket : public TPacket
{
	bool Accepted;           // принят ли запрос
	uint8_t PlayerID;       // ID игрока (0-3)
	uint32_t ProtocolVersion; // версия протокола
	std::string Message;     // сообщение (при ошибке)
	
	TConnectResponsePacket() : Accepted(false), PlayerID(0), ProtocolVersion(1) {}
	
	std::vector<uint8_t> Serialize() const override;
	bool Deserialize(const std::vector<uint8_t> &data) override;
	size_t GetDataSize() const override;
};

// Пакет ввода игрока
struct TPlayerInputPacket : public TPacket
{
	uint8_t PlayerID;
	bool InputUp;
	bool InputDown;
	bool InputLeft;
	bool InputRight;
	bool IsShooting;
	float MouseX;           // позиция мыши в мировых координатах
	float MouseY;
	uint32_t FrameNumber;   // номер кадра (для синхронизации)
	
	TPlayerInputPacket() 
		: PlayerID(0), InputUp(false), InputDown(false), 
		  InputLeft(false), InputRight(false), IsShooting(false),
		  MouseX(0.0f), MouseY(0.0f), FrameNumber(0) {}
	
	std::vector<uint8_t> Serialize() const override;
	bool Deserialize(const std::vector<uint8_t> &data) override;
	size_t GetDataSize() const override;
};

// Пакет обновления позиции игрока
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
	uint32_t FrameNumber; // номер кадра для синхронизации
	
	TPlayerUpdatePacket()
		: PlayerID(0), PositionX(0.0f), PositionY(0.0f),
		  FacingDirectionX(0.0f), FacingDirectionY(-1.0f),
		  Health(100), MaxHealth(100), Level(1), Experience(0), FrameNumber(0) {}
	
	std::vector<uint8_t> Serialize() const override;
	bool Deserialize(const std::vector<uint8_t> &data) override;
	size_t GetDataSize() const override;
};

// Пакет обновления врагов
struct TEnemyUpdatePacket : public TPacket
{
	struct TEnemyData
	{
		uint8_t Type;
		uint32_t EnemyID;
		float PositionX;
		float PositionY;
		int Health;
		bool IsAlive;
	};
	
	std::vector<TEnemyData> Enemies;
	uint32_t FrameNumber;
	
	TEnemyUpdatePacket() : FrameNumber(0) {}
	
	std::vector<uint8_t> Serialize() const override;
	bool Deserialize(const std::vector<uint8_t> &data) override;
	size_t GetDataSize() const override;
};

// Пакет обновления босса
struct TBossUpdatePacket : public TPacket
{
	float PositionX;
	float PositionY;
	int Health;
	int MaxHealth;
	uint8_t Phase;
	bool IsAlive;
	uint32_t FrameNumber;
	
	TBossUpdatePacket() : PositionX(0), PositionY(0), Health(0), MaxHealth(0), Phase(0), IsAlive(false), FrameNumber(0) {}
	
	std::vector<uint8_t> Serialize() const override;
	bool Deserialize(const std::vector<uint8_t> &data) override;
	size_t GetDataSize() const override;
};

// Пакет обновления сфер опыта
struct TExperienceOrbUpdatePacket : public TPacket
{
	struct TExperienceOrbData
	{
		float PositionX, PositionY;
		int32_t Value;
		bool IsActive;
	};
	std::vector<TExperienceOrbData> Orbs;
	uint32_t FrameNumber;

	TExperienceOrbUpdatePacket() : FrameNumber(0) {}

	std::vector<uint8_t> Serialize() const override;
	bool Deserialize(const std::vector<uint8_t> &data) override;
	size_t GetDataSize() const override;
};

// Пакет общего состояния игры
struct TGameStateUpdatePacket : public TPacket
{
	uint32_t WaveNumber;
	uint32_t EnemiesAlive;
	uint8_t WorldState;   // текущее состояние мира (EWorldState)
	uint32_t FrameNumber;
	
	TGameStateUpdatePacket() : WaveNumber(1), EnemiesAlive(0), WorldState(0), FrameNumber(0) {}
	
	std::vector<uint8_t> Serialize() const override;
	bool Deserialize(const std::vector<uint8_t> &data) override;
	size_t GetDataSize() const override;
};

// Пакет выбора обновления
struct TPlayerUpgradePacket : public TPacket
{
	uint8_t PlayerID;
	uint8_t UpgradeType;   // EUpgradeType
	uint8_t Rarity;        // EUpgradeRarity
	
	TPlayerUpgradePacket() : PlayerID(0), UpgradeType(0), Rarity(0) {}
	
	std::vector<uint8_t> Serialize() const override;
	bool Deserialize(const std::vector<uint8_t> &data) override;
	size_t GetDataSize() const override;
};

// Информация о подключенном клиенте
struct TClientInfo
{
	uint8_t PlayerID;
	std::string PlayerName;
	bool IsConnected;
	uint32_t LastPacketTime; // время последнего пакета (для таймаута)
	
	TClientInfo() : PlayerID(0), IsConnected(false), LastPacketTime(0) {}
};

// Состояние сетевого соединения
enum class ENetworkState
{
	Disconnected,  // не подключен
	Connecting,    // подключение...
	Connected,     // подключен
	Hosting        // хост (сервер)
};

//---------------------------------------------------------------------------
// Менеджер сетевых соединений
//---------------------------------------------------------------------------
class TNetworkManager
{
private:
	ENetworkState State;
	bool IsHost;
	uint8_t LocalPlayerID; // ID локального игрока (0-3)
	
	// WinSock сокеты (будет использоваться SOCKET из WinSock2)
	void* ListenSocket;      // сокет для прослушивания (сервер)
	void* ClientSocket;       // сокет клиента
	std::vector<void*> ClientSockets; // сокеты подключенных клиентов (сервер)
	
	// Информация о клиентах
	std::vector<TClientInfo> Clients;
	
	// Счетчики для надежности
	uint32_t NextSequence;
	uint32_t LastReceivedSequence;
	
	// Порт для подключения
	static constexpr uint16_t DefaultPort = 7777;
	
	// Таймауты (в миллисекундах)
	static constexpr uint32_t ConnectionTimeout = 10000;  // 10 секунд на подключение
	// Увеличенный таймаут, чтобы игрок в лобби не отбрасывался слишком быстро,
	// пока хост не нажал \"Начать игру\" и клиент не начал отправлять ввод
	static constexpr uint32_t PacketTimeout = 120000;     // 120 секунд без пакетов = разрыв
	static constexpr uint32_t ReconnectDelay = 2000;      // 2 секунды перед переподключением
	
	// Таймеры
	uint32_t ConnectionStartTime;  // время начала подключения
	uint32_t LastPacketTime;       // время последнего полученного пакета
	uint32_t ReconnectTimer;       // таймер для переподключения
	bool IsReconnecting;           // флаг переподключения
	
	// Информация для переподключения
	std::string ReconnectHostIP;
	uint16_t ReconnectPort;
	std::string ReconnectPlayerName;

	// Локальный IP-адрес (для отображения на экране хоста)
	std::string LocalIPAddress;
	
	// Вспомогательные функции
	bool InitializeWinSock();
	void CleanupWinSock();
	bool CreateListenSocket(uint16_t port);
	bool ConnectToHost(const std::string &hostIP, uint16_t port);
	void CloseSocket(void* socket);
	
	// Отправка/прием пакетов
	bool SendPacket(void* socket, const TPacket &packet);
	std::unique_ptr<TPacket> ReceivePacket(void* socket);
	
	// Обработка подключений (сервер)
	void AcceptNewConnection();
	void ProcessClientPackets();
	void CheckClientTimeouts();
	
	// Обработка пакетов
	void HandleConnectRequest(const TConnectRequestPacket &packet, void* clientSocket);
	void HandlePlayerInput(const TPlayerInputPacket &packet);
	void HandleDisconnect(uint8_t playerID);
	
	// Валидация пакетов
	bool ValidatePacket(const TPacket &packet) const;
	bool ValidatePacketHeader(const TPacketHeader &header) const;
	
	// Переподключение
	void StartReconnect();
	bool TryReconnect();
	
public:
	TNetworkManager();
	~TNetworkManager();
	
	// Инициализация/очистка
	bool Initialize();
	void Shutdown();
	
	// Создание/подключение к игре
	bool StartHosting(const std::string &playerName, uint16_t port = DefaultPort);
	bool ConnectToGame(const std::string &hostIP, const std::string &playerName, uint16_t port = DefaultPort);
	void Disconnect();
	
	// Отправка данных
	bool SendPlayerInput(const TPlayerInputPacket &input);
	bool BroadcastGameState(const std::vector<TPlayerUpdatePacket> &players);
	bool BroadcastPacket(const TPacket &packet);
	
	// Получение данных
	void Update(float deltaTime); // обновление сетевого состояния
	std::vector<TPlayerInputPacket> GetReceivedInputs(); // получение ввода от других игроков
	std::vector<TPlayerUpdatePacket> GetReceivedPlayerUpdates(); // получение обновлений игроков от сервера
	std::vector<TEnemyUpdatePacket> GetReceivedEnemyUpdates(); // получение обновлений врагов от сервера
	std::vector<TBossUpdatePacket> GetReceivedBossUpdates(); // получение обновлений босса
	std::vector<TExperienceOrbUpdatePacket> GetReceivedOrbUpdates(); // получение обновлений сфер опыта
	std::vector<TPlayerUpgradePacket> GetReceivedUpgrades(); // получение выборов улучшений
	std::unique_ptr<TGameStateUpdatePacket> GetReceivedGameStateUpdate(); // получение состояния игры
	
	// Состояние
	ENetworkState GetState() const { return State; }
	bool IsHosting() const { return IsHost; }
	uint8_t GetLocalPlayerID() const { return LocalPlayerID; }
	const std::vector<TClientInfo> &GetClients() const { return Clients; }
	const std::string &GetLocalIPAddress() const { return LocalIPAddress; }
	
	// Статистика (для отладки)
	uint32_t GetPacketsSent() const { return PacketsSent; }
	uint32_t GetPacketsReceived() const { return PacketsReceived; }
	uint32_t GetPacketsLost() const { return PacketsLost; }
	float GetAverageLatency() const { return AverageLatency; }
	
	// Получение ошибок
	std::string GetLastError() const { return LastError; }
	
private:
	uint32_t PacketsSent;
	uint32_t PacketsReceived;
	uint32_t PacketsLost;
	float AverageLatency;  // средняя задержка в миллисекундах
	
	// Буферы для полученных пакетов
	std::vector<TPlayerInputPacket> ReceivedInputs;
	std::vector<TPlayerUpdatePacket> ReceivedPlayerUpdates;
	std::vector<TEnemyUpdatePacket> ReceivedEnemyUpdates;
	std::vector<TBossUpdatePacket> ReceivedBossUpdates;
	std::vector<TExperienceOrbUpdatePacket> ReceivedOrbUpdates;
	std::vector<TPlayerUpgradePacket> ReceivedUpgrades;
	std::unique_ptr<TGameStateUpdatePacket> ReceivedGameStateUpdate;
	
	// Обработка ошибок
	std::string LastError;
	void SetError(const std::string &error) { LastError = error; }
	void ClearError() { LastError.clear(); }
};

//---------------------------------------------------------------------------
// Вспомогательные функции сериализации
//---------------------------------------------------------------------------
namespace NetworkSerialization
{
	// Запись данных в буфер
	void WriteUInt8(std::vector<uint8_t> &buffer, uint8_t value);
	void WriteUInt16(std::vector<uint8_t> &buffer, uint16_t value);
	void WriteUInt32(std::vector<uint8_t> &buffer, uint32_t value);
	void WriteFloat(std::vector<uint8_t> &buffer, float value);
	void WriteBool(std::vector<uint8_t> &buffer, bool value);
	void WriteString(std::vector<uint8_t> &buffer, const std::string &str);
	void WritePointF(std::vector<uint8_t> &buffer, const TPointF &point);
	
	// Чтение данных из буфера
	bool ReadUInt8(const std::vector<uint8_t> &buffer, size_t &offset, uint8_t &value);
	bool ReadUInt16(const std::vector<uint8_t> &buffer, size_t &offset, uint16_t &value);
	bool ReadUInt32(const std::vector<uint8_t> &buffer, size_t &offset, uint32_t &value);
	bool ReadFloat(const std::vector<uint8_t> &buffer, size_t &offset, float &value);
	bool ReadBool(const std::vector<uint8_t> &buffer, size_t &offset, bool &value);
	bool ReadString(const std::vector<uint8_t> &buffer, size_t &offset, std::string &value);
	bool ReadPointF(const std::vector<uint8_t> &buffer, size_t &offset, TPointF &point);
}

} // namespace NeonGame

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

