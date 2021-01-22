cc = g++
cv = -std=c++1z
wrn = -Wall -Wextra -pedantic
compile = $(cc) $(cv) $(wrn)

default:
	$(compile) src/main.cpp -o out/a.out

#device: channel
#	$(compile) -c src/btdevice.cpp src/deviceDescriptor.cpp
#	mv btdevice.o deviceDescriptor.o -t out/

#channel:
#	$(compile) -c src/btchannel.cpp
#	mv btchannel.o out/btchannel.o


clean:
	rm out/*

