#
# $Id: makefile,v 1.3 2004-06-05 19:48:09 dds Exp $
#

VERSION=1.22
BINARY=outwit-bin-$(VERSION).zip
BINDIR=outwit-bin-$(VERSION)
SOURCE=outwit-src-$(VERSION).zip
SRCDIR=outwit-src-$(VERSION)
WEB=/dds/pubs/web/home/sw/outwit
ZIP=zip

all:
	cd winclip ; nmake
	cd odbc ;  nmake
	cd docprop ; nmake /f docprop.mak CFG="docprop - Win32 Release"
	cd winreg ; nmake /f winreg.mak CFG="winreg - Win32 Release"
	cd readlink ; nmake
	cd readlog ; nmake

clearweb:
	rm -rf $(WEB)
	mkdir $(WEB)

web: clearweb exezip sourcezip
	sed "s/VERSION/$(VERSION)/g" websrc/index.html >$(WEB)/index.html
	cp websrc/outwit.jpg $(WEB)
	cp docprop/docprop.pdf $(WEB)/docprop.pdf
	cp odbc/odbc.pdf $(WEB)/odbc.pdf
	cp readlink/readlink.pdf $(WEB)/readlink.pdf
	cp readlog/readlog.pdf $(WEB)/readlog.pdf
	cp winclip/winclip.pdf $(WEB)/winclip.pdf
	cp winreg/winreg.pdf $(WEB)/winreg.pdf
	cp docprop/docprop.html $(WEB)
	cp odbc/odbc.html $(WEB)
	cp readlink/readlink.html $(WEB)
	cp readlog/readlog.html $(WEB)
	cp winclip/winclip.html $(WEB)
	cp winreg/winreg.html $(WEB)
	cp ChangeLog.txt $(WEB)
	cp $(BINARY) $(WEB)/$(BINARY)
	cp $(SOURCE) $(WEB)/$(SOURCE)

install:
	cp docprop/release/docprop.exe /usr/bin
	cp odbc/odbc.exe /usr/bin
	cp readlink/readlink.exe /usr/bin
	cp readlog/readlog.exe /usr/bin
	cp winclip/winclip.exe /usr/bin
	cp winreg/release/winreg.exe /usr/bin

exezip:
	rm -f $(BINARY)
	-cmd /c rd /s/q $(BINDIR)
	mkdir $(BINDIR)
	mkdir $(BINDIR)/doc $(BINDIR)/bin
	mkdir $(BINDIR)/doc/txt $(BINDIR)/doc/html $(BINDIR)/doc/pdf
	cp docprop/release/docprop.exe \
	 odbc/odbc.exe readlink/readlink.exe winclip/winclip.exe \
	 winreg/release/winreg.exe readlog/readlog.exe \
	 $(BINDIR)/bin
	cp ChangeLog.txt $(BINDIR)
	for i in docprop odbc readlink winclip winreg readlog ; do \
		cp $$i/$$i.txt $(BINDIR)/doc/txt ; \
		cp $$i/$$i.html $(BINDIR)/doc/html ; \
		cp $$i/$$i.pdf $(BINDIR)/doc/pdf ; \
	done
	zip -r $(BINARY) $(BINDIR)
	-cmd /c rd /s/q $(BINDIR)

sourcezip:
	rm -f $(SOURCE)
	-cmd /c rd /s/q $(SRCDIR)
	mkdir $(SRCDIR)
	sh -c "tar -cf - {docprop,odbc,readlink,winclip,winreg,readlog}/makefile */*.[1ch] */*.mak */*.cpp ChangeLog.txt | tar -C $(SRCDIR) -xf -"
	zip -r $(SOURCE) $(SRCDIR)
	-cmd /c rd /s/q $(SRCDIR)
