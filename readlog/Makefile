CC=cl
#CFLAGS=-nologo -Zi
CFLAGS=-nologo -Ox
OBJ=readlog.obj getopt.obj
LIBS=advapi32.lib
NAME=readlog
SSH=plink

.c.obj:
	$(CC) -c $(CFLAGS) $?

readlog.exe: $(OBJ)
	$(CC) $(CFLAGS) -o readlog.exe $(OBJ) $(LIBS)

doc: $(NAME).ps $(NAME).txt $(NAME).pdf $(NAME).html

$(NAME).ps: $(NAME).1
	groff -man -Tps <$? > $@

$(NAME).txt: $(NAME).1
	groff -man -Tascii <$? | col -b > $@

$(NAME).pdf: $(NAME).ps
	ps2pdf $? $@

$(NAME).html: $(NAME).1
	groff -mhtml -Thtml -man <$? | sed -e 's/&minus;/-/g;s/&bull;/\&#8226;/g' >$@
