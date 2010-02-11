#!/usr/bin/python
# -*- encoding: utf-8 -*-


""" Tester for a bus
"""



import dbus
import dbus.service
import logging
import gobject
from time import sleep
from threading import Thread, Timer

from dbus.mainloop.glib import DBusGMainLoop

DBusGMainLoop(set_as_default=True)
logging.getLogger().setLevel(logging.DEBUG)
logging.getLogger().addHandler(logging.StreamHandler())

log = logging.getLogger('main')

class TestSO(Thread, dbus.service.Object):
    AVC_IF = 'com.foobar.Test'

    def __init__(self, bus, bus_name, path):
        dbus.service.Object.__init__(self, bus, path, bus_name=bus_name)
        Thread.__init__(self)
        self._last_input = None
        self.loop = gobject.MainLoop()
        self.does_run = False

    def run(self):
        self.does_run = True
        self.loop.run()
        
    @dbus.service.method(dbus_interface=AVC_IF)
    def quit(self):
        log.info("called quit!")
        self.loop.quit()
        self.does_run = False

    @dbus.service.method(dbus_interface=AVC_IF)
    def test1(self):
        log.info("Test 1 called")

    @dbus.service.method(dbus_interface=AVC_IF)
    def test2(self):
        log.info("Test 2 called")

    @dbus.service.signal(dbus_interface=AVC_IF)
    def signalTest(self):
        pass


log.info("Starting")
session_bus = dbus.SessionBus()
our_name = dbus.service.BusName("com.foobar.Test",bus=session_bus, do_not_queue=True)
log.info("Registered dbus name")

dvc = TestSO(session_bus, our_name, '/com/foobar')

gobject.threads_init()
dvc.start()
sleep(1)

while dvc.does_run:
   try:
       s = raw_input("tso> ")
   except EOFError:
       dvc.quit()
       break
   if not s:
       continue

   if s == 'q':
        print "Quitting"
        dvc.quit()
        break
   elif s == 's':
        dvc.signalTest()
   else:
        print "Unknown command:", s[:10]


log.info("Exiting test")

#eof
