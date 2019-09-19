main: defines.cpp jobs.cpp ConvertUTF.cpp linenoise.cpp wcwidth.cpp main.cpp
	g++ -o shell defines.cpp ConvertUTF.cpp linenoise.cpp wcwidth.cpp jobs.cpp main.cpp 
