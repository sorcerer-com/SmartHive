import sqlite3

from Logger import *

class Connection:
	DatabaseFile = None
	Debug = False
	
	def __init__(self):
		self.conn = None
		self.curs = None
		
	def __enter__(self):
		if Connection.DatabaseFile != None:
			Connection.debug("Connect to " + Connection.DatabaseFile)
			self.conn = sqlite3.connect(Connection.DatabaseFile, detect_types=sqlite3.PARSE_DECLTYPES|sqlite3.PARSE_COLNAMES)
			self.curs = self.conn.cursor()
		return self
	
	def __exit__(self, exc_type, exc_value, traceback):
		Connection.debug("Close connection")
		self.conn.close()
		
	def execute(self, sql, *args):
		Connection.debug("Execute - " + sql + str(args))
		return self.curs.execute(sql, args)
		
	def commit(self):
		Connection.debug("Commit")
		self.conn.commit()
	
	
	@staticmethod
	def debug(text):
		if Connection.Debug:
			Logger.log("info", "Connection: " + text)
		else:
			Logger.log("debug", "Connection: " + text)