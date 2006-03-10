NAME=winclip
UXHOST=spiti
SSH=plink

winclip.exe: winclip.c getopt.c lang.h
	cl winclip.c getopt.c user32.lib shell32.lib gdi32.lib

lang.h: lang.pl
	perl $? 'c:/program files/devstudio/vc/include/winnt.h' >$@


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
