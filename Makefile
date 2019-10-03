main: defines.cpp jobs.cpp linenoise/*.cpp main.cpp
	g++ -o shell defines.cpp jobs.cpp linenoise/*.cpp main.cpp 
clean :
	-rm shell