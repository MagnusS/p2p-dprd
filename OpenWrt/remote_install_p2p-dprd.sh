#!/bin/bash

# Copyright (c) 2012-2014, Magnus Skjegstad / Forsvarets Forskningsinstitutt (FFI)
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice, 
# this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright notice, 
# this list of conditions and the following disclaimer in the documentation 
# and/or other materials provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
# POSSIBILITY OF SUCH DAMAGE.
# 
# Authors: Halvdan Grelland, Jostein Aardal

###############################################################################################################
# Dead simple script to cross-compile, upload and install p2p-dprd to remote OpenWrt device (router) in one go
#
# - To run: ./remote_install_p2p-dprd.sh <router root password>
#
# - Configure SSH_PATH to desired remote location. Remote device must run an ssh server with root
# - SSH_PASS is the root password of the remote device (duh)
#
# A small quirk:    You need to make sure there are no object files for different a different arch in the 
#                   p2p-dprd source directory. Any leftover object files will cause compilation failure within
#                   the OpenWrt build system, but might not give a proper error message.
#
# The OpenWrt build system log is written to make_p2p.log
###############################################################################################################

VERSION='1.1'
SSH_PATH='root@10.0.0.1:/root/' # Router path to copy package to after build
SSH_PASS="$1"

# Begin build
make package/p2p-dprd/clean V=99
make package/p2p-dprd/compile V=99 2>&1 | tee make_p2p.log
RETVAL=${PIPESTATUS[0]}

if test $RETVAL -eq 0 # Success, let's upload and install it to remote OpenWrt device
then
	 sshpass -p 'cogp2p2' scp bin/ar71xx/packages/p2p-dprd_${VERSION}_ar71xx.ipk ${SSH_PATH}
     echo 'Built and uploaded p2p-dprd to router'
	 echo "opkg install /root/packages/p2p-dprd_${VERSION}_ar71xx.ipk"
     sshpass -p ${SSH_PASS} ssh root@10.0.0.1 "opkg install /root/p2p-dprd_${VERSION}_ar71xx.ipk"
     echo 'Installed package on remote system'

else
     echo 'Build failed'
fi
