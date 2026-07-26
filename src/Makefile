CC = c++

CFLAGS = -Os -march=core2 -pipe -pthread -s -Wl,--as-needed -Wl,--sort-common -Wl,--enable-new-dtags -Wl,--reduce-memory-overheads -flto

# Путь к иконкам, можно переопределить через CFLAGS или ebuild
ICON_DIR ?= /usr/share/fusion-icon2

all: fusion_icon.cpp
	$(CC) fusion_icon.cpp $(CFLAGS) -DICON_DIR=\"$(ICON_DIR)\" `pkg-config gtkmm-3.0 --cflags --libs`  -o fusion-icon2
	strip -s fusion-icon2
	upx -9 --ultra-brute fusion-icon2

install: all
	install -Dm755 fusion-icon2 $(DESTDIR)/usr/bin/fusion-icon2
	install -Dm644 icons/fusion-icon.png $(DESTDIR)$(ICON_DIR)/fusion-icon.png
	install -Dm644 icons/marco.png $(DESTDIR)$(ICON_DIR)/marco.png
	install -Dm644 fusion-icon2.desktop $(DESTDIR)/usr/share/applications/fusion-icon2.desktop

clean:
	rm -f *.o fusion-icon2
