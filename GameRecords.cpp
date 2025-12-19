#include <vcl.h>
#pragma hdrstop
//---------------------------------------------------------------------------

#include "GameRecords.h"
#include <System.IniFiles.hpp>
#include <System.SysUtils.hpp>
#include <System.IOUtils.hpp>
#include <System.Classes.hpp>
#include <algorithm>

//---------------------------------------------------------------------------
TGameRecordsManager::TGameRecordsManager()
{
	// Используем путь к исполняемому файлу или текущую директорию
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
	
	// Создаем файл, если его нет
	CreateFileIfNotExists();
	
	Load();
}
//---------------------------------------------------------------------------
void TGameRecordsManager::CreateFileIfNotExists()
{
	// Определяем имя файла
	UnicodeString fileName = ConfigFileName;
	
	// Пробуем использовать путь к исполняемому файлу, если доступен
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
	
	// Если файл не существует, создаем его с начальными значениями
	if (!FileExists(fileName))
	{
		TStringList *sl = new TStringList();
		sl->Add("[Records]");
		sl->Add("BestWave=0");
		sl->Add("BestEnemiesKilled=0");
		sl->Add("BestRunTime=0");
		sl->SaveToFile(fileName);
		delete sl;
		
		// Обновляем ConfigFileName
		ConfigFileName = fileName;
	}
}
//---------------------------------------------------------------------------
void TGameRecordsManager::Load()
{
	LoadFromFile();
}
//---------------------------------------------------------------------------
void TGameRecordsManager::Save()
{
	SaveToFile();
}
//---------------------------------------------------------------------------
void TGameRecordsManager::LoadFromFile()
{
	// Используем ConfigFileName, который был установлен при создании
	UnicodeString fileName = ConfigFileName;
	
	// Если ConfigFileName пустой, пробуем найти файл
	if (fileName.IsEmpty() || !FileExists(fileName))
	{
		// Пробуем текущую директорию
		fileName = "neon_arena_records.ini";
		if (!FileExists(fileName))
		{
			// Файл не существует, используем значения по умолчанию
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
		// Если не удалось загрузить, используем значения по умолчанию
		Records = TGameRecords();
	}
}
//---------------------------------------------------------------------------
void TGameRecordsManager::SaveToFile()
{
	// Используем ConfigFileName, который был установлен при создании/загрузке
	UnicodeString fileName = ConfigFileName;
	
	// Если ConfigFileName пустой, используем путь к исполняемому файлу или текущую директорию
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
	
	// Формируем содержимое файла в формате INI
	sl->Add("[Records]");
	sl->Add("BestWave=" + IntToStr(Records.BestWave));
	sl->Add("BestEnemiesKilled=" + IntToStr(Records.BestEnemiesKilled));
	sl->Add("BestRunTime=" + FloatToStr(Records.BestRunTime));
	
	// Сохраняем файл
	sl->SaveToFile(fileName);
	
	delete sl;
}
//---------------------------------------------------------------------------
void TGameRecordsManager::UpdateRecords(int currentWave, int enemiesKilled, float runTime)
{
	// Обновляем рекорды, если текущие результаты лучше
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
	
	// Сохраняем рекорды после обновления
	Save();
}
//---------------------------------------------------------------------------


