all: 
	mkdir -p bin
	cd src/ && $(MAKE) && cp p2p-dprd ../bin/p2pdprd 

clean:
	rm -f bin/p2p-dprd
	rm -f python/*.pyc
	cd src/ && make clean
