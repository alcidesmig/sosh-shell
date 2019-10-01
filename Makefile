main: defines.cpp jobs.cpp linenoise/*.cpp main.cpp
	g++ shell defines.cpp jobs.cpp linenoise/*.cpp main.cpp -o 
clean :
	-rm shell