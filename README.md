## P2P-DPRD - The Peer-to-Peer Discovery Protocol for Radio Devices ##

### What is P2P-DPRD? ###
P2P-DPRD is a client for an Internet-based P2P network designed to discover the IP-address of radio devices operating in the same area. It can for example be used to discover nearby Wi-Fi routers. By discovering the IP-address of radio nodes in an area, nodes can contact each other over the Internet and cooperate to avoid interference.

P2P-DPRD is a working prototype of the protocol described in the paper "Large-Scale Distributed Internet-based Discovery Mechanism for Dynamic Spectrum Allocation" by Skjegstad et al, published in proceedings of IEEE DySPAN 2014. The accepted version of the paper can be downloaded [here](http://arxiv.org/pdf/1210.3552).

P2P-DPRD is a stand-alone proof-of-concept C implementation of the discovery protocol from the paper. 
It is the result of the bachelor's thesis project of Halvdan Grelland and Jostein Aardal which was 
supervised by Magnus Skjegstad at Forsvarets Forskningsinstitutt (FFI). The code has since then been improved and documented by Halvdan Grelland and Magnus Skjegstad to add support for OpenWRT and a Python interface.

The code is currently proof-of-concept and experimental. 

### What is the license of P2P-DPRD? ###
P2P-DPRD is (c) Magnus Skjegstad and Forsvarets Forsvarsinstitutt (FFI) and is licensed under a two-clause BSD-license. See the separate LICENSE-file for details.

A list of contributors is available in CONTRIBUTORS. Each file of the source code also includes the authors in the header.

### What platforms are supported? ###
As of today, P2P-DPRD is highly experimental. It has mainly been evaluated on x86
systems running flavours of Ubuntu and Debian, as well as MIPS-based OpenWRT routers. 
The codebase is however small and intended to be portable. You should try building it for 
any Linux- or Unix-based system!

(Note: The client does currently not work in OS X! Patches are welcome..)

### What are the dependencies of P2P-DPRD? ###
Not many!

P2P-DPRD should only require libconfig to compile and run, as it is used to read the configuration file.

To build for OpenWRT you also need the OpenWRT SDK (see wiki for details).

The Python support library currently requires Twisted. 

### Great! How do I compile it? ###

Using GCC on Linux:
```
$ cd /path/to/source/Release
```
```
$ make clean && make all
```

Details for building for OpenWRT are provided in the wiki.

###	.. and running it? ###
The short answer: ./bin/p2pdprd

Long answer: P2P-DPRD needs a sane configuration to be useful. 
For this purpose it uses a configuration file. Examples are distributed with
the source code in /examples/. 

Remember to set your external IP-address, or other nodes will 
be unable to contact you!

To run with a config file:
$ ./bin/p2pdprd /path/to/config.cfg

examples/localhost1.cfg and examples/localhost2.cfg contain configuration files
for two clients listening on port 2001 and 2002 at localhost in a test-location (100.0, 100.0). They can be used to verify that p2p-dprd is working properly.

In terminal 1, run:
```
$ ./bin/p2pdprd examples/localhost1.cfg
```

In terminal 2, run:
```
$ ./bin/p2pdprd examples/localhost2.cfg
```

After a few seconds the two clients should find each other and the debug output 
should list both nodes in both terminals. The ID is assigned randomly, but
the output should be similar to this:

```
NodeCollection:versionID = 1     type = 2        nodeCount = 2   maxNodeCount = 2
0        - 1108001846    100.100000      100.100000      10      2130706433      2002    2130706433      45452   1396471833
1        - 1978674892    100.100000      100.100000      10      2130706433      2001    2130706433      45452   1396471833
```

### Are there any public seed nodes? ###
Yes! We are running a test-server at 178.79.184.208. 
 


