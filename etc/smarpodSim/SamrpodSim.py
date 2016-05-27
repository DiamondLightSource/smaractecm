#!/bin/env dls-python
# a script for simulating a Smaract ECM module with connected smarpod

import threading
import socket
import sys
import time
from thread import *

HOST = 'localhost'  # Symbolic name, meaning all available interfaces
PORT = 9099  # Arbitrary non-privileged port
poll_rate = 0.1


class Smarpod:
    def __init__(self, unit):
        self.num_axes = 6
        self.axes_readbacks = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
        self.axes_demands = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
        self.axes_velocity = 0.0003
        self.axes_moving = [0, 0, 0, 0, 0, 0]
        self.homed = False
        self.unit = unit
        self.activeUnit = 0 == int(self.unit)

    def pid_update(self):
        for i in range(self.num_axes):
            if self.axes_moving[i] != 0:
                newpos = self.axes_readbacks[i] + float(self.axes_moving[i]) * self.axes_velocity * poll_rate
                # print "unit %d move %d from %f to %f" % (self.unit, i, self.axes_readbacks[i], newpos)
                self.axes_readbacks[i] = newpos
                if self.axes_moving[i] == -1 and self.axes_readbacks[i] < self.axes_demands[i] or \
                        self.axes_moving[i] == 1 and self.axes_readbacks[i] > self.axes_demands[i]:
                    self.axes_readbacks[i] = self.axes_demands[i]
                    self.axes_moving[i] = 0
        threading.Timer(poll_rate, self.pid_update).start()

    def start_motion(self):
        for i in range(self.num_axes):
            if not self.axes_readbacks[i] == self.axes_demands[i]:
                print 'moving %d from %f to %f' % (i, self.axes_readbacks[i], self.axes_demands[i])
                self.axes_moving[i] = 1 if self.axes_readbacks[i] < self.axes_demands[i] else -1

    def parse_command(self, in_command):
        try:
            words = in_command.split()
            cmd = words[0]
            output = "!0"  # 0 return = all good

            if cmd == '%unit':
                self.activeUnit = int(words[1]) == int(self.unit)
            elif self.activeUnit:
                if not self.homed and cmd in ['pos?', 'mov']:
                        output = '!550'  # not referenced
                elif cmd == "%info":
                    if words[1] == "version":
                        output = "Versions:\r\n  System: 1.0.0"
                elif cmd == "pos?":
                    output = ' '.join(map(str, self.axes_readbacks))
                elif cmd == 'mov':
                    for i in range(6):
                        self.axes_demands[i] = float(words[i + 1])
                    self.start_motion()
                elif cmd == 'ref':
                    self.homed = True
                    time.sleep(3)
                    self.axes_readbacks = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
                    self.axes_demands = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
                elif cmd == 'ref?':
                    output = int(self.homed)
                elif cmd == 'vel':
                    self.axes_velocity = float(words[1])
                    if self.axes_velocity > .001:
                        self.axes_velocity = .001
                elif cmd == 'vel?':
                    output = self.axes_velocity
                elif cmd == 'mst?':
                    output = 2 if any(x != 0 for x in self.axes_moving) else 0
                elif cmd == 'stop':
                    self.axes_moving = [0, 0, 0, 0, 0, 0]
                    self.axes_demands = self.axes_readbacks
                else:
                    output = '!240'  # unknown command
        except Exception as e:
            print e.message
            output = '!99'  # unknown error
        if self.activeUnit:
            return str(output) + '\r\n'
        else:
            return ''


pods = [Smarpod(0), Smarpod(1)]
#dont_print = ['pos?', 'mst?']
dont_print = ['xxx']

# Function for handling connections
def client_thread(conn):
    while True:
        data = conn.recv(1024)
        if not data:
            break

        reply = ''
        for pod in pods:
            reply += pod.parse_command(data)

        if not any(x in data for x in dont_print):
            print data, "-->", reply
        conn.sendall(reply)

    print 'closing connection'
    conn.close()


def start_socket():
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

            start_new_thread(client_thread, (conn,))
    except Exception as e:
        print e.message
        s.close()


def go():
    for pod in pods:
        pod.pid_update()
    start_socket()

go()
