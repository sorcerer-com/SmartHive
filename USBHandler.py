#!/usr/bin/env python
from threading import Thread, Timer
import os, subprocess
from shutil import copyfile, copy
import thread

from libs.Logger import *
from libs.Connection import *

t = None
running = False
delay = 1

baseDir = "/media/usb/"
checked = False

# sudo apt-get install usbmount 
# FS_MOUNTOPTIONS="-fstype=vfat,umask=0000"
def init():
	global running, t
	Logger.log("info", "Init USB Handler")
	
	running = True
	t = Thread(target=run)
	t.daemon = True
	t.start()

def deinit():
	global running, t
	Logger.log("info", "Deinit USB Handler")
	
	running = False
	t.join()
	
def run():
	global running, checked
	
	while running:
		list = os.listdir(baseDir)
		if len(list) > 0 and not checked:
			checked = True
			for path in list:
				if os.path.isdir(os.path.join(baseDir, path)) and path == "SmartHive":
					Logger.log("info", "USBHandler - flash detected")
					copyfile(Connection.DatabaseFile, os.path.join(baseDir, path, "data.db"))
					exportDB(os.path.join(baseDir, path, "data.csv"))
					update(os.path.join(baseDir, path, "update"))
		elif len(list) == 0:
			checked = False
		time.sleep(delay)
		
def exportDB(filePath):
	Logger.log("info", "USBHandler - export database")
	with Connection() as conn:
		values = conn.execute(
			"SELECT DateTime, mac.Id, mac.MAC, Type, Value FROM data JOIN MACs mac ON SensorMAC = MAC "
			"ORDER BY datetime(DateTime)").fetchall()
	
	with open(filePath, "w") as file:
		for value in values:
			line = ",".join([str(v) for v in value])
			file.write(line + "\n")
			
def update(path):
	if not os.path.isdir(path):
		return
	Logger.log("info", "USBHandler - update")
	
	files = os.listdir(path)
	for file_name in files:
		full_file_name = os.path.join(path, file_name)
		if (os.path.isfile(full_file_name)):
			copy(full_file_name, "./")
			os.remove(full_file_name)
	
	if len(files) > 0:
		thread.interrupt_main()