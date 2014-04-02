#!/usr/bin/python
'''
Copyright (c) 2013-2014, Magnus Skjegstad / Forsvarets Forskningsinstitutt (FFI)
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, 
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation 
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.

Authors: Halvdan Hoem Grelland, Magnus Skjegstad
'''

import time
from p2pdprd_types import Node, NodeCollection, IPCMessage
# P2P-dprd IPC protocol
from twisted.internet.protocol import DatagramProtocol
from twisted.internet import reactor

class IPCProtocol(DatagramProtocol):

    """
    Twisted-implementation of the p2pdprd IPC protocol
    """

    def __init__(self, p2pdprd_listening_sock, my_listening_sock, update_callback=None):
        """
        Construct the Protocol object.
         - p2pdprd_listening_sock is the ipc socket path of the p2pdprd instance
         - my_listening_sock is the path to the socket used to receive messages from p2pdprd
         - update_callback is called with an array of Nodes whenever an update is received from p2pdprd. Set to None to disable (check self.candidate_nodes instead)
        """
        self.candidate_nodes = None
        self.p2pdprd_listening_sock = p2pdprd_listening_sock
        self.update_callback = update_callback
        self.my_listening_sock = my_listening_sock
        self.updated = None

    def startProtocol(self):
        """Called when a transport is connected to this protocol"""
        print "P2P-DPRD IPC-Protocol listening on", self.transport.port
        self.sendSubscribe()

    def sendDatagram(self, message):
        """Send a datagram over p2p-dprd IPC"""
        self.transport.write(message, self.p2pdprd_listening_sock)

    def sendSubscribe(self):
        # Subscribe to candidate nodes
        m = IPCMessage.subscribe_candidate_nodes(self.my_listening_sock)
        self.sendDatagram(m.pack())

    def sendUnsubscribe(self):
        # Unsubscribe to candidate nodes
        m = IPCMessage.unsubscribe_candidate_nodes(self.my_listening_sock)
        self.sendDatagram(m.pack())

    def setLocation(self, lat, lon):
        # Set position in lat/long
        m = IPCMessage.set_position(lat, lon)
        self.sendDatagram(m.pack())

    def setCoordinationRange(self, coord_range):
        # Set coordination range
        m = IPCMessage.set_coordination_range(coord_range)
        self.sendDatagram(m.pack())

    def datagramReceived(self, datagram, host):
        """Called upon receiving a datagram from p2pdprd IPC"""
        # Write to servers' candidate nodes buffer
        self.updated = time.time()
        self.candidate_nodes = NodeCollection.from_bytes(datagram)
        print self.candidate_nodes
        if (self.update_callback != None):
            self.update_callback(self.candidate_nodes)


###############################################################################
# TESTS                                                                       #
###############################################################################

def test_protocol():
    import os
    
    p2pdprd_listening_sock = "/tmp/p2p-dprd.sock"
    my_listening_sock = "/tmp/test.sock"

    protocol = IPCProtocol(p2pdprd_listening_sock, my_listening_sock)

    # Delete the unix socket path if it already exists
    import os
    if os.path.exists(my_listening_sock): 
        os.remove(my_listening_sock)

    reactor.listenUNIXDatagram(my_listening_sock, protocol)

    reactor.run()

def test_stuff():
    # Run tests for dev purposes

    # Create a NodeCollection.
    nc = NodeCollection(1,2,
            [Node(123, 45.45, 23.34, 12, "127.0.0.1", 12345, "128.0.0.1", 54321, 456789),
            Node(321, 54.45, 11.98, 10, "128.0.0.1", 12345, "127.0.0.1", 36412, 985446),
            Node(123, 45.45, 23.34, 12, "127.0.0.1", 12345, "128.0.0.1", 54321, 456789),
            Node(321, 54.45, 11.98, 10, "128.0.0.1", 12345, "127.0.0.1", 36412, 985446),
            Node(123, 45.45, 23.34, 12, "127.0.0.1", 12345, "128.0.0.1", 54321, 456789),
            Node(321, 54.45, 11.98, 10, "128.0.0.1", 12345, "127.0.0.1", 36412, 985446),
            Node(123, 45.45, 23.34, 12, "127.0.0.1", 12345, "128.0.0.1", 54321, 456789),
            Node(321, 54.45, 11.98, 10, "128.0.0.1", 12345, "127.0.0.1", 36412, 985446)])

    assert(nc.node_count == 8)
    print nc, '\n'

    # Pack it
    binary_data = nc.pack()
    assert(binary_data is not None)

    # Unpack it
    nc_unpacked = NodeCollection.from_bytes(binary_data)

    assert(str(nc) == str(nc_unpacked))

def test_stuff_ipc():
    sub_msg = IPCMessage.subscribe_candidate_nodes('/tmp/pypypy.py').pack()
    print sub_msg
    pos_msg = IPCMessage.set_position(11.43, 67.65).pack()
    print pos_msg
    coord_msg = IPCMessage.set_coordination_range(23).pack()
    print coord_msg
    unsub_msg = IPCMessage.unsubscribe_candidate_nodes('/tmp/pypypy.py').pack()
    print unsub_msg

    import socket
    out_sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    out_sock.connect('/tmp/p2p-dprd.sock')

    out_sock.send(sub_msg)
    out_sock.send(pos_msg)
    out_sock.send(coord_msg)
    out_sock.send(unsub_msg)
###############################################################################
# Uncomment to run tests                                                      #
###############################################################################

#test_stuff()
#test_stuff_ipc()
#test_protocol()
