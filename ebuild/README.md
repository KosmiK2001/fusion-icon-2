# Fusion Icon 2 - Gentoo Ebuild

## Installation

### Manual

```bash
# Copy files
sudo mkdir -p /usr/local/portage/packages/fusion-icon-2
sudo cp fusion-icon-2.ebuild /usr/local/portage/packages/fusion-icon-2/
sudo cp -r files/ /usr/local/portage/packages/fusion-icon-2/

# Install
sudo emerge --ask --verbose =fusion-icon-2
```

### Via overlay

Add this package to your custom overlay and emerge.

## Dependencies

- x11-libs/gtkmm:3.0
