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
					Logger.log("info", "USBHandler: Flash detected")
					fullPath = os.path.join(baseDir, path)
					exportDB(os.path.join(fullPath, "data.csv"))
					copyfiles(fullPath)
					execute(os.path.join(fullPath, "exec.py"))
					update(os.path.join(fullPath, "update"))
					Logger.log("info", "USBHandler: Done")
						
		elif len(list) == 0:
			checked = False
		time.sleep(delay)
		
def exportDB(filePath):
	try:
		Logger.log("info", "USBHandler: Export database")
		with Connection() as conn:
			values = conn.execute(
				"SELECT DateTime, mac.Id, mac.MAC, Type, Value FROM data JOIN MACs mac ON SensorMAC = MAC "
				"ORDER BY datetime(DateTime)").fetchall()
		
		with open(filePath, "w") as file:
			for value in values:
				line = ",".join([str(v) for v in value])
				file.write(line + "\n")
	except Exception as e:
		Logger.log("error", "USBHandler: Exception handled")
		Logger.log("exception", str(e))
			
def copyfiles(dstPath):
	try:
		srcPath = os.path.dirname(Connection.DatabaseFile)
		files = os.listdir(srcPath)
		if len(files) == 0:
			return
		Logger.log("info", "USBHandler: Backup data")

		for fileName in files:
			filePath = os.path.join(srcPath, fileName)
			if (os.path.isfile(filePath)):
				Logger.log("info", "USBHandler: - Copying " + fileName)
				copyfile(filePath, os.path.join(dstPath, fileName))
	except Exception as e:
		Logger.log("error", "USBHandler: Exception handled")
		Logger.log("exception", str(e))
			
def execute(filePath):
	try:
		if not os.path.isfile(filePath):
			return
		Logger.log("info", "USBHandler: Execute")
		
		execfile(filePath)
	except Exception as e:
		Logger.log("error", "USBHandler: Exception handled")
		Logger.log("exception", str(e))
	
def update(srcPath):
	try:
		if not os.path.isdir(srcPath):
			return

		files = os.listdir(srcPath)
		if len(files) == 0:
			return
		Logger.log("info", "USBHandler: Update")

		for fileName in files:
			filePath = os.path.join(srcPath, fileName)
			if (os.path.isfile(filePath)):
				Logger.log("info", "USBHandler: - Copying " + fileName)
				copy(filePath, "./")
				os.remove(filePath)
		
		thread.interrupt_main()
	except Exception as e:
		Logger.log("error", "USBHandler: Exception handled")
		Logger.log("exception", str(e))