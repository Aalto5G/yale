yaletest: yaletest.o yale.lex.o yale.tab.o yyutils.o
	cc -o yaletest yaletest.o yale.lex.o yale.tab.o yyutils.o
YALE.LEX.INTERMEDIATE: yale.l
	mkdir -p intermediatestore
	flex --outfile=intermediatestore/yale.lex.c --header-file=intermediatestore/yale.lex.h yale.l
	touch YALE.LEX.INTERMEDIATE
YALE.TAB.INTERMEDIATE: yale.y
	mkdir -p intermediatestore
	bison --defines=intermediatestore/yale.tab.h --output=intermediatestore/yale.tab.c yale.y
	touch YALE.TAB.INTERMEDIATE
yale.lex.c: YALE.LEX.INTERMEDIATE
	cp intermediatestore/yale.lex.c .
yale.lex.h: YALE.LEX.INTERMEDIATE
	cp intermediatestore/yale.lex.h .
yale.tab.c: YALE.TAB.INTERMEDIATE
	cp intermediatestore/yale.tab.c .
yale.tab.h: YALE.TAB.INTERMEDIATE
	cp intermediatestore/yale.tab.h .

yale.lex.d: yale.tab.h yale.lex.h
yale.lex.o: yale.tab.h yale.lex.h
yale.tab.d: yale.lex.h yale.tab.h
yale.tab.o: yale.lex.h yale.tab.h
