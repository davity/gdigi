#!/usr/bin/python

# Simply python program to exercise the gdigi DBus API.

import sys
import dbus

val = 0
op = sys.argv[1]
pos = sys.argv[2]
i = sys.argv[3]

if op == "set":
    val = sys.argv[4]

bus = dbus.SessionBus("gdigi.server")
proxy = bus.get_object('gdigi.server', '/gdigi/parameter/Object')
io_interface = dbus.Interface(proxy, dbus_interface='gdigi.parameter.io')

if op == "get":
    val = io_interface.get(pos, i)
    print "(",pos,",",i,") --> ", val,"\n"
    exit(0)

if op == "set":
    print "calling set\n"
    print "(",pos,",",i,") --> ", val,"\n"
    io_interface.set(pos, i, val)
    exit(0)
