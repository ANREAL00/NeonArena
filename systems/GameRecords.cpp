#include <vcl.h>
#pragma hdrstop

#include "GameRecords.h"
#include <System.IniFiles.hpp>
#include <System.SysUtils.hpp>
#include <System.IOUtils.hpp>
#include <System.Classes.hpp>
#include <algorithm>

TGameRecordsManager::TGameRecordsManager()
{

	try
	{
		if (Application && !Application->ExeName.IsEmpty())
		{
			UnicodeString exePath = ExtractFilePath(Application->ExeName);
			ConfigFileName = exePath + "neon_arena_records.ini";
		}
		else
		{
			ConfigFileName = "neon_arena_records.ini";
		}
	}
	catch (...)
	{
		ConfigFileName = "neon_arena_records.ini";
	}

	CreateFileIfNotExists();

	Load();
}

void TGameRecordsManager::CreateFileIfNotExists()
{

	UnicodeString fileName = ConfigFileName;

	try
	{
		if (Application && !Application->ExeName.IsEmpty())
		{
			UnicodeString exePath = ExtractFilePath(Application->ExeName);
			if (!exePath.IsEmpty())
			{
				fileName = exePath + "neon_arena_records.ini";
			}
		}
	}
	catch (...) {}

	if (!FileExists(fileName))
	{
		TStringList *sl = new TStringList();
		sl->Add("[Records]");
		sl->Add("BestWave=0");
		sl->Add("BestEnemiesKilled=0");
		sl->Add("BestRunTime=0");
		sl->SaveToFile(fileName);
		delete sl;

		ConfigFileName = fileName;
	}
}

void TGameRecordsManager::Load()
{
	LoadFromFile();
}

void TGameRecordsManager::Save()
{
	SaveToFile();
}

void TGameRecordsManager::LoadFromFile()
{

	UnicodeString fileName = ConfigFileName;

	if (fileName.IsEmpty() || !FileExists(fileName))
	{

		fileName = "neon_arena_records.ini";
		if (!FileExists(fileName))
		{

			Records = TGameRecords();
			return;
		}
		ConfigFileName = fileName;
	}

	std::unique_ptr<TIniFile> iniFile;
	try
	{
		iniFile = std::make_unique<TIniFile>(fileName);

		Records.BestWave = iniFile->ReadInteger("Records", "BestWave", 0);
		Records.BestEnemiesKilled = iniFile->ReadInteger("Records", "BestEnemiesKilled", 0);
		Records.BestRunTime = static_cast<float>(iniFile->ReadFloat("Records", "BestRunTime", 0.0));
	}
	catch (...)
	{

		Records = TGameRecords();
	}
}

void TGameRecordsManager::SaveToFile()
{

	UnicodeString fileName = ConfigFileName;

	if (fileName.IsEmpty())
	{
		fileName = "neon_arena_records.ini";
		try
		{
			if (Application && !Application->ExeName.IsEmpty())
			{
				UnicodeString exePath = ExtractFilePath(Application->ExeName);
				if (!exePath.IsEmpty())
				{
					fileName = exePath + "neon_arena_records.ini";
				}
			}
		}
		catch (...) {}
		ConfigFileName = fileName;
	}

	TStringList *sl = new TStringList();

	sl->Add("[Records]");
	sl->Add("BestWave=" + IntToStr(Records.BestWave));
	sl->Add("BestEnemiesKilled=" + IntToStr(Records.BestEnemiesKilled));
	sl->Add("BestRunTime=" + FloatToStr(Records.BestRunTime));

	sl->SaveToFile(fileName);

	delete sl;
}

void TGameRecordsManager::UpdateRecords(int currentWave, int enemiesKilled, float runTime)
{

	if (currentWave > Records.BestWave)
	{
		Records.BestWave = currentWave;
	}

	if (enemiesKilled > Records.BestEnemiesKilled)
	{
		Records.BestEnemiesKilled = enemiesKilled;
	}

	if (runTime > Records.BestRunTime)
	{
		Records.BestRunTime = runTime;
	}

	Save();
}
