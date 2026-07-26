# Fusion Icon 2

[EN] System tray utility for switching between Compiz window managers on Linux.
[RU] Системный трей для переключения между оконными менеджерами Compiz на Linux.

## Features / Возможности

[EN]
- Switch between Compiz, Marco, and Metacity window managers
- Window decorator selection: Emerald, GTK Window Decorator
- Compiz Options: Indirect Rendering and Loose Binding toggles
- NVIDIA Options: Force Composition Pipeline, Force Full Composition Pipeline
- Auto-detect current window manager and decorator
- Launch CCSM (Compiz Settings Manager) and Emerald Theme Manager
- Persistent settings (config file or GSettings)
- Debug mode with GPU/driver info
- Minimal GTK3 system tray icon with DPI-aware scaling

[RU]
- Переключение между оконными менеджерами Compiz, Marco и Metacity
- Выбор декоратора окон: Emerald, GTK Window Decorator
- Compiz Options: Indirect Rendering и Loose Binding
- NVIDIA Options: Force Composition Pipeline, Force Full Composition Pipeline
- Авто-определение текущего WM и декоратора
- Запуск CCSM (Compiz Settings Manager) и Emerald Theme Manager
- Сохранение настроек (конфиг-файл или GSettings)
- Debug режим с информацией о GPU/драйвере
- МинимальныйGTK3 системный трей с масштабированием под DPI

## Dependencies / Зависимости

- GTKmm 3.0 (`dev-cpp/gtkmm:3.0`)
- Compiz, Marco, or Metacity / Compiz, Marco или Metacity
- Optional / Опционально: `media-gfx/librsvg`, `media-gfx/imagemagick` (для SVG иконок)

## Build / Сборка

```bash
make
```

## Install / Установка

```bash
sudo make install
```

## Gentoo / Установка в Gentoo

```bash
sudo emerge --sync kosmik2001
sudo emerge --ask x11-misc/fusion-icon2
```

## Usage / Использование

[EN] Run `fusion-icon2` to start the system tray icon. Right-click to access the menu.
[RU] Запустите `fusion-icon2` для отображения значка в системном трее. Правый клик — меню.

## USE flags / USE-флаги (Gentoo)

| Flag | Description (EN) | Описание (RU) |
|------|------------------|---------------|
| `+file` | Store settings in plain text config file | Хранение настроек в текстовом файле |
| `-gsettings` | Store settings using GSettings/dconf | Хранение настроек через GSettings/dconf |
| `+upx` | Compress binary with UPX | Сжатие бинарника UPX |

## License / Лицензия

GPL-2.0
