#!/usr/bin/env python

# set import
import sys
import time
import socket

#define
RECVBUF=8*1024

def usage():
    print "python udp_ping_serv.py [recv_prot]"
    sys.exit(1)
#end usage

def main():
    if (len(sys.argv) != 2):
        usage()

    sockfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sockfd.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    port = int(sys.argv[1])
    sockfd.bind(("", port))
    sockfd.listen(5)
    print "recv port number is", port


    print "start"
    conn, address = sockfd.accept()
    while True:	
        mesg = conn.recv(RECVBUF)
        if mesg == "ping":
            size = conn.send("pong")
        print mesg, address
        #del mesg
        #del address

    conn.close()

#end_main

if __name__ == "__main__":
    main()
#end_ if
