CC=cl
#CFLAGS=-nologo -Zi
CFLAGS=-nologo -Ox
OBJ=choosecolor.obj
LIBS=comdlg32.lib
NAME=choosecolor

.c.obj:
	$(CC) -c $(CFLAGS) $?

choosecolor.exe: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LIBS)

doc: $(NAME).ps $(NAME).txt $(NAME).pdf $(NAME).html

$(NAME).ps: $(NAME).1
	groff -man -Tps $? > $@

$(NAME).txt: $(NAME).1
	groff -man -Tascii $? | col -b > $@

$(NAME).pdf: $(NAME).ps
	ps2pdf $? $@

$(NAME).html: $(NAME).1
	man2html $? | sed '1d;s,<A HREF="http.*</A>,,;s/^,$$/man2html,/' > $@

clean:
	rm -f $(OBJ) choosecolor.exe
