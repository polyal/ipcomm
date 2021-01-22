cc = g++
cv = -std=c++1z
wrn = -Wall -Wextra -pedantic
compile = $(cc) $(cv) $(wrn)


default:
	$(compile) src/main.cpp -o out/a.out

clean:
	rm out/*

