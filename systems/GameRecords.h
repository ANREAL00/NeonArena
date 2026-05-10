

#ifndef GameRecordsH
#define GameRecordsH

struct TGameRecords
{
	int BestWave;
	int BestEnemiesKilled;
	float BestRunTime;

	TGameRecords()
		: BestWave(0), BestEnemiesKilled(0), BestRunTime(0.0f)
	{
	}
};

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

	void Load();

	void Save();

	void UpdateRecords(int currentWave, int enemiesKilled, float runTime);

	const TGameRecords& GetRecords() const { return Records; }

	bool IsNewBestWave(int wave) const { return wave > Records.BestWave; }
	bool IsNewBestKills(int kills) const { return kills > Records.BestEnemiesKilled; }
	bool IsNewBestTime(float time) const { return time > Records.BestRunTime; }
};

#endif
