import sqlite3

class Connection:
	DatabaseFile = None
	
	def __init__(self):
		self.conn = None
		
	def __enter__(self):
		if Connection.DatabaseFile != None:
			self.conn = sqlite3.connect(Connection.DatabaseFile)
		return self
	
	def __exit__(self, exc_type, exc_value, traceback):
		self.conn.close()
		
	def read(self, sql):
		curs = self.conn.cursor()
		return curs.execute(sql)