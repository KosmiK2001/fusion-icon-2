# Fusion Icon 2

[EN] System tray utility for switching between Compiz window managers on Linux.
[RU] Системный трей для переключения между оконными менеджерами Compiz на Linux.

---

## Features / Возможности

| Feature | EN | RU |
|---------|-----|-----|
| Window Managers | Compiz, Marco, Metacity | Compiz, Marco, Metacity |
| Window Decorators | Emerald, GTK Window Decorator | Emerald, GTK Window Decorator |
| Compiz Options | Indirect Rendering, Loose Binding | Indirect Rendering, Loose Binding |
| NVIDIA Options | Force Composition Pipeline, Force Full Composition Pipeline | Force Composition Pipeline, Force Full Composition Pipeline |
| Auto-detect | Current WM and decorator | Текущий WM и декоратор |
| Settings | Config file or GSettings | Конфиг-файл или GSettings |
| Debug | GPU/driver info with `-d` | Информация о GPU/драйвере с `-d` |

---

## Quick Start / Быстрый старт

### Build from source / Сборка из исходников

```bash
git clone https://github.com/KosmiK2001/fusion-icon-2.git
cd fusion-icon-2/src
make
sudo make install
```

### Gentoo / Установка в Gentoo

```bash
sudo eselect repository enable kosmik2001 https://github.com/KosmiK2001/fusion-icon-2-overlay.git
sudo eselect repository sync kosmik2001
sudo emerge --ask x11-misc/fusion-icon2
```

---

## Usage / Использование

```bash
fusion-icon2        # Запуск / Launch
fusion-icon2 -d     # Debug режим / Debug mode
fusion-icon2 --help # Справка / Help (TODO)
```

### Menu / Меню

- **Settings Manager** — запуск CCSM / launch CCSM
- **Emerald Theme Manager** — запуск Emerald / launch Emerald
- **Restart Compiz** — перезапуск Compiz / restart Compiz
- **Select Window Manager** — выбор WM / select WM
- **Compiz Options** — настройки Compiz / Compiz settings
- **Select Window Decorator** — выбор декоратора / select decorator
- **NVIDIA Options** — опции NVIDIA / NVIDIA settings
- **Exit** — выход / exit

---

## USE Flags / USE-флаги (Gentoo)

| Flag | Default | Description EN | Описание RU |
|------|---------|----------------|-------------|
| `+file` | Yes | Store settings in config file | Хранение настроек в файле |
| `-gsettings` | No | Store settings in GSettings/dconf | Хранение настроек в GSettings |
| `+upx` | Yes | Compress binary with UPX | Сжатие бинарника UPX |

---

## Project Structure / Структура проекта

```
fusion-icon-2/
├── src/
│   ├── fusion_icon.cpp      # Main source / Основной код
│   ├── Makefile
│   ├── fusion-icon2.desktop  # Desktop entry / Ярлык
│   └── icons/               # App icons / Иконки
│       ├── fusion-icon.png
│       ├── marco.svg
│       └── nvidia.svg
├── README.md
└── RESUME.md               # Development log / Журнал разработки
```

---

## Requirements / Требования

- GTKmm 3.0
- One of: Compiz, Marco, Metacity
- Optional: nvidia-settings (for NVIDIA options)

---

## License / Лицензия

GPL-2.0
