#!/usr/bin/env python
import sqlite3
from flask import *

from libs.Logger import *

databaseFile = "bin/data.db"

logging.getLogger().info("")
app = Flask(__name__, template_folder = "./")


@app.route("/")
def index():
	return render_template("index.html")
	
@app.route("/lastSensorId")
def lastSensorId():
	id = 0
	row = read("SELECT MAX(SensorId) FROM data").fetchone()
	if row != None and row[0] != None:
		id = row[0]
	return str(id)
	
	
@app.route("/restart")
def restart():
	func = request.environ.get('werkzeug.server.shutdown')
	if func is not None:
		func()
	return redirect("/")


def read(sql):
	conn = sqlite3.connect(databaseFile)
	curs = conn.cursor()
	return curs.execute(sql)
	

if __name__ == "__main__":
	Logger.log("info", "Start Smart Hive")
	
	app.config['TEMPLATES_AUTO_RELOAD']=True
	app.run(debug=False, host="0.0.0.0")