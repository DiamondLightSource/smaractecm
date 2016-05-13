#!/bin/env dls-python
# a script for simulating a Smaract ECM module with connected smarpod

import socket
import sys
from thread import *

HOST = 'localhost'  # Symbolic name, meaning all available interfaces
PORT = 9097         # Arbitrary non-privileged port

# Function for handling connections
def clientthread(conn):
    while True:
        data = conn.recv(1024)
        reply = 'OK...' + data
        if not data:
            break

        conn.sendall(reply)

    print 'closing connection'
    conn.close()


def startSocket():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # Bind socket to local host and port
    try:
        s.bind((HOST, PORT))
    except socket.error as msg:
        print 'Bind failed. Error Code : ' + str(msg[0]) + ' Message ' + msg[1]
        sys.exit()

    # Start listening on socket
    s.listen(10)
    print 'Socket now listening'

    try:
        while 1:
            #wait to accept a connection - blocking call
            conn, addr = s.accept()
            print 'Connected with ' + addr[0] + ':' + str(addr[1])

            start_new_thread(clientthread ,(conn,))
    except:
        s.close()

startSocket()
