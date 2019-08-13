aoool: scanner.cpp parser.cpp *.cpp *.h *.l *.y
	g++ -fno-rtti -O3 -o aoool aoool.cpp scanner.cpp parser.cpp handler.cpp http.cpp utils.cpp -lboost_system -lboost_filesystem -lboost_regex
	strip -s aoool

aoool-debug: scanner.cpp parser.cpp *.cpp *.h *.l *.y
	g++ -g -O3 -DENABLE_DLOG -o aoool aoool.cpp scanner.cpp parser.cpp handler.cpp http.cpp utils.cpp -lboost_system -lboost_filesystem -lboost_regex

scanner.cpp: scanner.l parser.cpp
	flex -o scanner.cpp scanner.l

parser.cpp: parser.y osl.h
	bison -o parser.cpp parser.y

oslexec: oslexec.cpp scanner.cpp parser.cpp *.cpp *.h *.l *.y
	g++ -g -O3 -o oslexec oslexec.cpp scanner.cpp parser.cpp utils.cpp http.cpp handler.cpp -lboost_system -lboost_filesystem -lboost_regex

deploy:
	mkdir -p ../deployment/files
	cp aoool ../deployment/aoool
	cp -R files/* ../deployment/files/

serve: aoool
	socat -s tcp-l:8080,reuseaddr,fork,bind=127.0.0.1 exec:"./aoool"

clean:
	rm -f scanner.cpp
	rm -f parser.cpp parser.hpp parser.tab.c parser.tab.h
	rm -f location.hh position.hh stack.hh
	rm -f lex.aoool*_.cc
	rm -f *.o
	rm -f aoool
	rm -f oslexec
