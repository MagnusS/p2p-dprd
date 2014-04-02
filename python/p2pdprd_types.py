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

import socket, struct

class IPCMessage(object):
   
    # Message identifiers
    SET_POSITION = 0
    SET_COORDINATION_RANGE = 1
    SET_POS_AND_RANGE = 2 # Deprecated
    SUB_CANDNODES = 3
    UNSUB_CANDNODES = 4

    def __init__(self, message_type, coord_range = None, position = (None, None), sock_path = None):
        self.message_type = message_type
        self.coord_range = coord_range
        self.position = position
        self.sock_path = sock_path

        # TODO Should properly check integrity of input parameters

    def __str__(self):
        sb = ['IPCMessage ' + str(id(self)) + ':']
        for key in self.__dict__:
            sb.append("  {key}='{value}'".format(key=key, value=self.__dict__[key]))
        return '\n'.join(sb)

    def __repr__(self):
        return self.__str__()
    
    # Direct cosntruction of message with type implied:
    # Example:
    #   m = IPCMessage.set_position(11.23, 178.1233)
    
    @classmethod
    def set_position(cls, lat, lon):
        return cls(cls.SET_POSITION, position = (lat,lon))

    @classmethod
    def set_coordination_range(cls, coord_range):
        return cls(cls.SET_COORDINATION_RANGE, coord_range = coord_range)

    @classmethod
    def unsubscribe_candidate_nodes(cls, sock_path):
        return cls(cls.UNSUB_CANDNODES, sock_path = sock_path)

    @classmethod
    def subscribe_candidate_nodes(cls, sock_path):
        return cls(cls.SUB_CANDNODES, sock_path = sock_path)

    def pack(self):
        """
        Returns a packed/serialized representation which can be used to control
        a p2p-dprd instance over a socket connection.

        The byte layout of a p2p-dprd IPC messsage is as follows:
        +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        | message_type | %%%%%%% payload %%%%%%%%%%%%%%%%%%%%%%%%%%%%%|
        +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        | 1 byte       | variable depending on type, see below        |
        +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        
        There are 4 different (used) payload layouts. They are listed below:
        
        SET_POSITION                  SET_COORDINATION_RANGE
        ++++++++++++++++++++++++++    ++++++++++++++++++++++++++
        | latitude | longitude   |    | coord_range            |
        ++++++++++++++++++++++++++    ++++++++++++++++++++++++++
        | 8 bytes  | 8 bytes     |    | 2 bytes                |
        ++++++++++++++++++++++++++    ++++++++++++++++++++++++++
        
        UNSUB_CANDNODES               SUB_CANDNODES
        ++++++++++++++++++++++++++    ++++++++++++++++++++++++++
        | local_socket_path      |    | local_socket_path      | 
        ++++++++++++++++++++++++++    ++++++++++++++++++++++++++
        | Remaining bytes <= 512 |    | Remaining bytes <= 512 |
        ++++++++++++++++++++++++++    ++++++++++++++++++++++++++ 

        The two last message types are identical, and simply contain a string
        buffer containing a system socket path (local socket) of variable length.
        It will be at most 512 bytes and occupies the whole buffer between the
        1 byte header offset and the end of the buffer.

        Message type 2 (SET_POS_AND_RANGE) is deprecated and should not be used.

        All data is in network byte order (i.e. big endian).
        """
        
        bytes = []
         
        if   self.message_type is self.SET_POSITION:
            bytes = struct.pack('!Bdd', self.message_type, self.position[0], self.position[1])  

        elif self.message_type is self.SET_COORDINATION_RANGE:
            bytes = struct.pack('!BH', self.message_type, self.coord_range)

        elif self.message_type is self.SET_POS_AND_RANGE:
            pass # Depr

        elif self.message_type is self.SUB_CANDNODES:
            bytes = struct.pack('!B' + str(len(self.sock_path)) + 's', self.message_type, self.sock_path)

        elif self.message_type is self.UNSUB_CANDNODES:
            bytes = struct.pack('!B' + str(len(self.sock_path)) + 's', self.message_type, self.sock_path)

        else:
            pass
        
        return bytes

    @classmethod
    def from_bytes(cls, b):
        """
        Construct an IPCMessage from a byte buffer. 
        See pack method for format details.

        Note:  This method isn't really useful for anything but testing as 
               these messages are only sent TO the p2p-dprd instance.
        """
       
        # Extract msg-type header field
        msg_type = struct.unpack('!B', b[:1])[0]
       
        # Parse depending on msg type
        if   msg_type is cls.SET_POSITION:
            _position = struct.unpack('!dd', b[1:])
            return cls(msg_type, position = _position)

        elif msg_type is cls.SET_COORDINATION_RANGE:
            _coord_range = struct.unpack('!H', b[1:])
            return cls(msg_type, coord_range = _coord_range)

        elif msg_type is cls.SET_POS_AND_RANGE:
            return None # depr

        elif msg_type is cls.SUB_CANDNODES:
            # ... winner of multiple code beauty contests
            _sock_path = struct.unpack('!' + str(len(b) - 1) + 's', b[1:])[0]
            return cls(msg_type, sock_path = _sock_path)

        elif msg_type is cls.UNSUB_CANDNODES:
            _sock_path = struct.unpack('!' + str(len(b) - 1) + 's', b[1:])[0]
            return cls(msg_type, sock_path = _sock_path)

        else:
            return None

class NodeCollection(object):
    
    def __init__(self, version_id, payload_type, nodes = []):
        self.version_id = version_id
        self.payload_type = payload_type
        self.node_count = len(nodes)
        self.nodes = nodes

    def __str__(self):
        sb = ['NodeCollection:']
        for key in self.__dict__:
            sb.append("  {key}='{value}'".format(key=key, value=self.__dict__[key]))
        return '\n'.join(sb)

    def __repr__(self):
        return self.__str__()

    def pack(self):
        """
        Returns a packed/serialized representation.
        """
        p = struct.pack('!HBH', self.version_id, self.payload_type, len(self.nodes))
        for node in self.nodes:
            p += node.pack()

        return p

    @classmethod
    def from_bytes(cls, b):
        """
        Construct object from packed byte array.
        
        Byte layout of the NodeCollection is as follows:
        +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        | version_id | payload_type | node_count | %%%%%% nodes %%%%% |
        +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        | 2 bytes    | 1 byte       | 2 bytes    | 38 bytes * count   |
        +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        
        We unpack the leading five bytes of the buffer to extract the header.
        Subsequently we unpack each Node object using the Node.from_bytes() method.

        TODO No real error checking is performed. A malformed result will most
        likely return None.
        """
        offset = 5
        header = struct.unpack('!HBH',b[:offset])   # Bytes 0->5
        version_id = header[0]
        payload_type = header[1]
        node_count = header[2]

        nodes = []
        for i in range(node_count):
            nodes.append(Node.from_bytes(b[ offset : offset + Node.PACKED_SIZE ]))
            offset += Node.PACKED_SIZE

        # Integrity check
        if len(nodes) != node_count:
            # Malformed data structure
            return None

        return cls(version_id, payload_type, nodes)

class Node(object):

    def __init__(self, node_id, lat, lon, coord_range, ip, port, radac_ip, radac_port, timestamp):
        self.node_id = node_id
        self.position = (lat, lon)
        self.coord_range = coord_range
        self.address = (ip, port)
        self.radac_address = (radac_ip, radac_port)
        self.timestamp = timestamp

    PACKED_SIZE = 38 #4 + 8 + 8 + 2 + 4 + 2 + 4 + 2 + 4 bytes
    
    def __str__(self):
        sb = ['\n  Node :']
        for key in self.__dict__:
            sb.append("    {key}='{value}'".format(key=key, value=self.__dict__[key]))
        return '\n'.join(sb)

    def __repr__(self):
        return self.__str__()

    def _packable(self):
        """
        Returns a flat list representation of the Node object. 
        """
        return [self.node_id,                       # nodeID - 0
                self.position[0], self.position[1], # lat - 1, lon - 2
                self.coord_range,                   # coord_range - 3
                _ip2int(self.address[0]),           # p2p-dprd ip - 4
                self.address[1],                    # p2p-dprd port - 5
                _ip2int(self.radac_address[0]),     # radac ip - 6
                self.radac_address[1],              # radac port - 7
                self.timestamp]                     # time created - 8
    
    @classmethod
    def from_bytes(cls, byte_arr):
        u = struct.unpack('!IddHIHIHI', byte_arr)

        # Convert ip addresseses from ints
        l = list(u)
        l[4] = _int2ip(l[4])
        l[6] = _int2ip(l[6])

        return cls(*l)

    def pack(self):
        return struct.pack('!IddHIHIHI', *self._packable())

# Helper functions
def _ip2int(addr):
    """Convert ip from repr format to int"""
    return struct.unpack("!I", socket.inet_aton(addr))[0]                       

def _int2ip(addr):
    """Convert ip from int to repr format"""
    return socket.inet_ntoa(struct.pack("!I", addr))   
