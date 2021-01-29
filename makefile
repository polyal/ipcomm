cc = g++
cv = -std=c++1z
wrn = -Wall -Wextra -pedantic
compile = $(cc) $(cv) $(wrn)
libs = -pthread


default:
	$(compile) src/main.cpp src/comm.cpp src/channel.cpp $(libs) -o out/a.out

clean:
	rm out/*

