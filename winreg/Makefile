NAME=winreg
SSH=plink

winreg.exe: winreg.c getopt.c
	cl -Ox winreg.c getopt.c advapi32.lib

doc: $(NAME).ps $(NAME).txt $(NAME).pdf $(NAME).html

$(NAME).ps: $(NAME).1
	groff -man -Tps <$? > $@

$(NAME).txt: $(NAME).1
	groff -man -Tascii <$? | col -b > $@

$(NAME).pdf: $(NAME).ps
	ps2pdf $? $@

$(NAME).html: $(NAME).1
	groff -mhtml -Thtml -man <$? | sed -e 's/&minus;/-/g;s/&bull;/\&#8226;/g' >$@
#	man2html $? | sed '1d;s,<A HREF="http.*</A>,,;s/^,$$/man2html,/' > $@
