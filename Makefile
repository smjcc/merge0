all: merge0

install: all
	mkdir -p $(DESTDIR)/usr/bin
	mv merge0 $(DESTDIR)/usr/bin/
#	chown 0.0 $(DESTDIR)/usr/bin/merge0
#	chmod 555 $(DESTDIR)/usr/bin/merge0
	mkdir -p $(DESTDIR)/usr/share/man/man1
	cp merge0.1 $(DESTDIR)/usr/share/man/man1/
#	chown man.man $(DESTDIR)/usr/share/man/merge0.1
#	chmod 644 $(DESTDIR)/usr/share/man/merge0.1
#	mkdir -p $(DESTDIR)/usr/share/info
#	ls -laF $(DESTDIR)/usr/bin/merge0

clean:
	rm -fv merge0 *~
