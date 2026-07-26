# Fusion Icon 2

System tray utility for switching between Compiz window managers on Linux.

## Features

- Switch between Compiz, Marco, and Metacity window managers
- Compiz Options: Indirect Rendering and Loose Binding toggles
- Auto-detect current window manager
- Launch CCSM (Compiz Settings Manager) and Emerald Theme Manager
- Minimal GTK3 system tray icon

## Dependencies

- GTKmm 3.0 (`x11-libs/gtkmm:3.0`)
- Compiz, Marco, or Metacity

## Build

```bash
make
```

## Install

```bash
sudo make install
```

## Usage

Run `fusion_icon` to start the system tray icon. Right-click to access the menu.

## License

GPL-2.0
