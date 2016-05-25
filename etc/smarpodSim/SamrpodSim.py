#!/bin/env dls-python
# a script for simulating a Smaract ECM module with connected smarpod

import datetime, threading
import socket
import sys
import time
from thread import *

HOST = 'localhost'  # Symbolic name, meaning all available interfaces
PORT = 9099  # Arbitrary non-privileged port

num_axes = 6
axes_readbacks = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
axes_demands = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
axes_velocity = 0.0003
axes_moving = [0, 0, 0, 0, 0, 0]
homed = False

poll_rate = 0.1

def pidUpdate():
    global axes_readbacks
    global axes_demands
    global axes_moving
    global axes_velocity
    for i in range(num_axes):
        if axes_moving[i] != 0:
            newpos = axes_readbacks[i] + float(axes_moving[i]) * axes_velocity * poll_rate
            # print "move %d from %f to %f" % (i, axes_readbacks[i], newpos)
            axes_readbacks[i] = newpos
            if axes_moving[i] == -1 and axes_readbacks[i] < axes_demands[i] or \
                                    axes_moving[i] == 1 and axes_readbacks[i] > axes_demands[i]:
                axes_readbacks[i] = axes_demands[i]
                axes_moving[i] = 0
    threading.Timer(poll_rate, pidUpdate).start()

def startMotion():
    global axes_readbacks
    global axes_demands
    global axes_moving
    global axes_velocity
    for i in range(num_axes):
        if not axes_readbacks[i] == axes_demands[i]:
            print 'moving %d from %f to %f' % (i, axes_readbacks[i], axes_demands[i])
            axes_moving[i] = 1 if axes_readbacks[i] < axes_demands[i] else -1


def parseCommand(in_command):
    global axes_moving
    global axes_velocity
    global axes_readbacks
    global homed
    global axes_demands
    try:
        words = in_command.split()
        cmd = words[0]
        output = "!0"  # 0 return = all good
        if cmd == "%info":
            if words[1] == "version":
                output = "Versions:\r\n  System: 1.0.0"
        elif cmd == "pos?":
            output = ' '.join(map(str, axes_readbacks))
        elif cmd == 'pos':
            for i in range(6):
                axes_demands[i] = float(words[i + 1])
            startMotion()
        elif cmd == 'ref':
            homed = True
            time.sleep(3)
            axes_readbacks = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
            axes_demands = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
        elif cmd == 'ref?':
            output = int(homed)
        elif cmd == 'vel':
            axes_velocity = float(words[1])
            if axes_velocity > .001:
                axes_velocity = .001
        elif cmd == 'vel?':
            output = axes_velocity
        elif cmd == 'mst?':
            output = 2 if any(x != 0 for x in axes_moving) else 0
        elif cmd == 'stop':
            axes_moving = [0, 0, 0, 0, 0, 0]
            axes_demands = axes_readbacks
        else:
            output = '!98' # unknown command
    except Exception as e:
        print e.message
        output = '!99'  # unknown error
    return str(output) + '\r\n'

dont_print = ['pos?', 'mst?']
#dont_print = ['xxx']

# Function for handling connections
def clientthread(conn):
    while True:
        data = conn.recv(1024)
        if not data:
            break

        reply = parseCommand(data)
        if not any(x in data for x in dont_print):
            print data, "-->", reply
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
            # wait to accept a connection - blocking call
            conn, addr = s.accept()
            print 'Connected with ' + addr[0] + ':' + str(addr[1])

            start_new_thread(clientthread, (conn,))
    except:
        s.close()


pidUpdate()
startSocket()
