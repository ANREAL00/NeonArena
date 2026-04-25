# Neon Arena

Neon Arena — курсовой проект на C++ Builder (Embarcadero RAD Studio): аркадный top-down shooter с прогрессией, волнами врагов и сетевым режимом (host/client).

## Возможности

- Игровой цикл, отрисовка мира и UI (HUD/меню/оверлеи/миникарта)
- Игрок, враги (в т.ч. босс), снаряды, коллизии
- Волны, спавн, опыт/уровни, улучшения
- Рекорды/настройки в ini-файлах
- Сетевой обмен состоянием (WinSock), порт по умолчанию: `7777`

## Требования

### Для сборки

- Embarcadero RAD Studio (C++ Builder) **10.3+**
- Windows 7/8/10/11

### Для запуска собранного `exe`

- Windows 7/8/10/11

## Быстрый старт

### Запуск готовой сборки (если есть в папке)

1. Откройте `Win32\Debug\` или `Win32\Release\`
2. Запустите `Project1.exe`

Примечание: в git-репозитории папки `Win32/Win64` обычно игнорируются через `.gitignore`, поэтому готовый `exe` может отсутствовать — в этом случае соберите проект из исходников.

### Сборка в RAD Studio

1. Откройте `Project1.cbproj`
2. Выберите конфигурацию `Debug` или `Release`
3. Выберите платформу `Win32` или `Win64`
4. Нажмите `F9` (Run) или `Project > Build All`

### Сборка из командной строки (если установлен RAD Studio)

```batch
msbuild Project1.cbproj /p:Config=Debug /p:Platform=Win32
```

Подробные шаги — в `BUILD_INSTRUCTIONS.md`.

## Управление

- **W, A, S, D** — движение
- **ЛКМ** — основная атака
- **ПКМ** — альтернативная атака
- **Мышь** — направление/прицеливание
- **ESC** — пауза/меню

## Структура проекта

```
CourseWork/
├── Project1.cbproj                 # проект RAD Studio
├── Project1.cpp                     # точка входа (VCL)
├── MainForm.cpp/.h/.dfm             # главная форма
├── MainForm_GameLoop.cpp            # игровой цикл/тайминги
├── MainForm_Input.cpp               # обработка ввода
├── MainForm_Network.cpp             # интеграция сетевого режима с формой
├── core/                            # базовые типы/константы/ввод/состояния/коллизии
│   ├── GameConstants.h
│   ├── GameInput.h
│   ├── GameState.h
│   └── GameCollision.h
├── entities/                        # сущности (игрок/враги/снаряды)
│   ├── GamePlayer.cpp/.h
│   ├── GameEnemy*.cpp + GameEnemy.h
│   └── GameProjectile.cpp/.h
├── systems/                         # подсистемы (камера/спавн/волны/апгрейды/опыт/рекорды)
│   ├── GameCamera.cpp/.h
│   ├── GameSpawner.cpp/.h
│   ├── WaveManager.cpp/.h
│   ├── GameUpgrade.cpp/.h
│   ├── GameExperience.cpp/.h
│   └── GameRecords.cpp/.h
├── world/                           # игровой мир: логика/рендер/синхронизация/спавн/стрельба
│   ├── GameWorld.cpp/.h
│   └── GameWorld_*.cpp
├── ui/                              # отрисовка интерфейса
│   ├── GameUIRenderer.h
│   └── GameUIRenderer_*.cpp
├── network/                         # пакеты/сериализация/host/client/менеджер
│   ├── GameNetwork.h
│   └── GameNetwork_*.cpp
└── tools/
    └── strip_cpp_comments.py        # вспомогательный скрипт
```

## Сетевой режим (кратко)

- Реализован менеджер сети `TNetworkManager` (папка `network/`)
- Обмен пакетами: ввод игрока, состояние игроков/врагов/пуль, волны, выбор улучшений, возврат в лобби
- Порт по умолчанию: `7777`

## Возможные проблемы

### Ошибка запуска про `rtl*.bpl`

Если при запуске появляется ошибка вида “rtl290.bpl не найден”:

1. Откройте `Project > Options...`
2. Проверьте, что `Dynamic RTL = false`
3. Проверьте, что `Use runtime packages = false`
4. Пересоберите проект (`Build All`)

### Cannot find include file / Unresolved external

- Проверьте `Project > Options > C++ Compiler > Paths and Defines` (Include Path)
- Убедитесь, что все нужные `.cpp` добавлены в проект

## Лицензия

Проект создан в учебных целях.

