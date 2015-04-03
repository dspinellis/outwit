NAME=winclip
UXHOST=spiti
SSH=plink

winclip.exe: winclip.c getopt.c imageload.cpp lang.h
	cl winclip.c getopt.c imageload.cpp user32.lib shell32.lib gdi32.lib WindowsCodecs.lib

lang.h: lang.pl
	/usr/bin/perl $? '/cygdrive/c/Program Files (x86)/Windows Kits/8.1/Include/um/winnt.h' >$@


drop2txt.exe: drop2txt.c
	cl drop2txt.c user32.lib shell32.lib

doc: $(NAME).ps $(NAME).txt $(NAME).pdf $(NAME).html

$(NAME).ps: $(NAME).1
	$(SSH) $(UXHOST) groff -man -Tps <$? > $@

$(NAME).txt: $(NAME).1
	$(SSH) $(UXHOST) groff -man -Tascii <$? | $(SSH) $(UXHOST) col -b > $@

$(NAME).pdf: $(NAME).ps
	ps2pdf $? $@

$(NAME).html: $(NAME).1
	$(SSH) $(UXHOST) groff -mhtml -Thtml -man <$? | sed -e 's/&minus;/-/g;s/&bull;/\&#8226;/g' >$@
#	man2html $? | sed '1d;s,<A HREF="http.*</A>,,;s/^,$$/man2html,/' > $@

zip:
	z -a winclip winclip.c winclip.exe winclip.man winclip.1 makefile
