CC = c++

CFLAGS = -Os -march=core2 -pipe -pthread -s -Wl,--as-needed -Wl,--sort-common -Wl,--enable-new-dtags -Wl,--reduce-memory-overheads -flto


all: fusion_icon.cpp
##	$(CC) fusion_icon.cpp $(CFLAGS) `pkg-config gtk+-3.0 gtkmm-3.0 appindicator3-0.1 --cflags --libs`  -o fusion_icon.bin
	$(CC) fusion_icon.cpp $(CFLAGS) `pkg-config gtkmm-3.0 --cflags --libs`  -o fusion_icon.bin
	strip -s fusion_icon.bin
	upx -9 --ultra-brute fusion_icon.bin

install: all
	install -Dm755 fusion_icon.bin $(DESTDIR)/usr/bin/fusion_icon
	install -Dm644 icons/fusion-icon.png $(DESTDIR)/usr/share/fusion-icon2/fusion-icon.png
	install -Dm644 icons/marco.png $(DESTDIR)/usr/share/fusion-icon2/marco.png

clean:
	rm -f *.o fusion_icon.bin
