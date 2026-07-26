# Copyright 2024 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=8

inherit eutils

DESCRIPTION="Fusion Icon 2 - system tray for switching Compiz window managers"
HOMEPAGE="https://github.com/user/fusion-icon-2"
SRC_URI=""

LICENSE="GPL-2"
SLOT="0"
KEYWORDS="~amd64 ~x86"
IUSE=""

DEPEND="x11-libs/gtkmm:3.0"
RDEPEND="${DEPEND}"

src_compile() {
	emake
}

src_install() {
	dobin fusion_icon.bin
	dodir /usr/share/icons/hicolor/48x48/apps
	insinto /usr/share/icons/hicolor/48x48/apps
	doins "${FILESDIR}/fusion-icon.png" 2>/dev/null || true
}
