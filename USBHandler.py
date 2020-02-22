#!/usr/bin/env python
from threading import Thread, Timer
import os, subprocess, thread, shutil
from datetime import datetime, timedelta

from libs.Logger import *
from libs.Connection import *

t = None
running = False
delay = 1

baseDir = "/media/usb/"
checked = False
cleanupInterval = 365 # days

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
					exportDB(fullPath, "data.csv")
					copyfiles(fullPath)
					execute(os.path.join(fullPath, "exec.py"))
					update(os.path.join(fullPath, "update"))
					cleanupDB()
					Logger.log("info", "USBHandler: Done")
						
		elif len(list) == 0:
			checked = False
		time.sleep(delay)
		
def exportDB(dstPath, fileName):
	try:
		Logger.log("info", "USBHandler: Export database")
		with Connection() as conn:
			values = conn.execute(
				"SELECT DateTime, mac.Id, mac.MAC, Type, Value, mac.LastActivity FROM data "
				"JOIN MACs mac ON SensorMAC = MAC "
				"ORDER BY datetime(DateTime)").fetchall()
		
		srcPath = os.path.dirname(Connection.DatabaseFile)
		filePath = os.path.join(srcPath, fileName)
		with open(filePath, "w") as file:
			for value in values:
				line = ",".join([str(v) for v in value])
				file.write(line + "\n")
		shutil.move(filePath, os.path.join(dstPath, fileName))
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

		if "zip" in shutil._ARCHIVE_FORMATS:
			shutil.make_archive(os.path.join("/tmp", "backup"), "zip", base_dir=srcPath, logger=logging.root)
			shutil.move(os.path.join("/tmp", "backup.zip"), os.path.join(dstPath, "backup.zip"))
		elif "gztar" in shutil._ARCHIVE_FORMATS:
			shutil.make_archive(os.path.join("/tmp", "backup"), "gztar", base_dir=srcPath, logger=logging.root)
			shutil.move(os.path.join("/tmp", "backup.gztar"), os.path.join(dstPath, "backup.gztar"))
		else:
			for fileName in files:
				filePath = os.path.join(srcPath, fileName)
				if os.path.isfile(filePath) and os.path.splitext(fileName)[1] != ".bak":
					Logger.log("info", "USBHandler: - Copying " + fileName)
					shutil.copyfile(filePath, os.path.join(dstPath, fileName))
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
				shutil.copy(filePath, "./")
				os.remove(filePath)
		
		thread.interrupt_main()
	except Exception as e:
		Logger.log("error", "USBHandler: Exception handled")
		Logger.log("exception", str(e))
		
def cleanupDB():
	try:
		Logger.log("info", "USBHandler: Cleanup database")
		dt = datetime.now() - timedelta(days=cleanupInterval)
		with Connection() as conn:
			conn.execute(
				"INSERT OR IGNORE INTO archive "
				"SELECT * FROM data "
				"WHERE datetime(DateTime) < datetime('%s')" % dt).fetchall()
			conn.execute("DELETE FROM data WHERE datetime(DateTime) < datetime('%s')" % dt).fetchall()
			conn.commit()
	except Exception as e:
		Logger.log("error", "USBHandler: Exception handled")
		Logger.log("exception", str(e))
