VER=21
BINARY=owb$(VER).tar.gz
SOURCE=ows$(VER).tar.gz
WEB=/dds/pubs/web/home/sw/outwit
ZIP=winzip

all:
	cd winclip
	nmake
	cd ..
	cd odbc
	nmake
	cd ..
	cd docprop
	nmake /f docprop.mak CFG="docprop - Win32 Release"
	cd ..
	cd winreg
	nmake /f winreg.mak CFG="winreg - Win32 Release"
	cd ..
	cd readlink
	nmake
	cd ..
	cd readlog
	nmake
	cd ..

clearweb:
	rm -rf $(WEB)
	mkdir $(WEB)

web: clearweb exezip sourcezip
	sed "s/VERSION/$(VER)/g" websrc/index.html >$(WEB)/index.html
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
	tar cvf - docprop/release/docprop.exe ChangeLog.txt \
	 odbc/odbc.exe readlink/readlink.exe winclip/winclip.exe \
	 winreg/release/winreg.exe readlog/readlog.exe \
	 docprop/docprop.txt odbc/odbc.txt readlink/readlink.txt \
	 winclip/winclip.txt winreg/winreg.txt readlog/readlog.txt | \
	 gzip -c >$(BINARY)

sourcezip:
#	find . -name "*.[1hc]" -or -name "*.{mak,cpp}" | sed "s,\\,/,g" >list
	sh -c "tar cvf - */makefile */*.[1ch] */*.mak */*.cpp ChangeLog.txt| gzip -c >$(SOURCE)"
#	$(ZIP) -x:rcs -a -r -p $(SOURCE) '*.1' '*.h' '*.c' '*.cpp' '*.mak' makefile 
#	$(ZIP) -d $(SOURCE) WINCLIP/RCS/WINCLIP.C DOCPROP/RCS/DOCPROP.CPP
#	$(ZIP) -d $(SOURCE) WINREG/RCS/WINREG.C ODBC/RCS/ODBC.C
