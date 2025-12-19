//---------------------------------------------------------------------------
// Система рекордов (Neon Arena)
//---------------------------------------------------------------------------
#ifndef GameRecordsH
#define GameRecordsH
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Структура рекордов
//---------------------------------------------------------------------------
struct TGameRecords
{
	int BestWave;           // лучшая достигнутая волна
	int BestEnemiesKilled;  // максимальное количество убитых врагов
	float BestRunTime;      // лучшее время выживания (в секундах)
	
	TGameRecords()
		: BestWave(0), BestEnemiesKilled(0), BestRunTime(0.0f)
	{
	}
};

//---------------------------------------------------------------------------
// Менеджер рекордов
//---------------------------------------------------------------------------
class TGameRecordsManager
{
private:
	TGameRecords Records;
	UnicodeString ConfigFileName;
	
	void LoadFromFile();
	void SaveToFile();
	void CreateFileIfNotExists();

public:
	TGameRecordsManager();
	
	// Загрузка рекордов
	void Load();
	
	// Сохранение рекордов
	void Save();
	
	// Обновление рекордов (если текущие результаты лучше)
	void UpdateRecords(int currentWave, int enemiesKilled, float runTime);
	
	// Получение текущих рекордов
	const TGameRecords& GetRecords() const { return Records; }
	
	// Проверка, является ли результат новым рекордом
	bool IsNewBestWave(int wave) const { return wave > Records.BestWave; }
	bool IsNewBestKills(int kills) const { return kills > Records.BestEnemiesKilled; }
	bool IsNewBestTime(float time) const { return time > Records.BestRunTime; }
};

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

