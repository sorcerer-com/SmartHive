#!/usr/bin/env python
from flask import *
from datetime import datetime, timedelta

from libs.Logger import *
from libs.Connection import *

Connection.DatabaseFile = "bin/data.db"

logging.getLogger().info("")
app = Flask(__name__, template_folder = "./")
table = None

# TODO: USB connect to Orangepi -> transfer data
# TODO: USB auto-update
# TODO: win app to read the usb data
# TODO: values percision?
@app.route("/")
def index():
	global table
	if table == None:
		with Connection() as conn:
			dt = datetime.now() - timedelta(days=10)
			values = conn.execute(
				"SELECT SensorId, DateTime, Type, Value FROM data "
				"WHERE datetime(DateTime) > datetime('%s')" % dt).fetchall()
		
		table = {}
		for value in values:
			if value[0] not in table:
				table[value[0]] = {}
			if value[1] not in table[value[0]]:
				table[value[0]][value[1]] = {}
			if value[2] not in table[value[0]][value[1]]:
				table[value[0]][value[1]][value[2]] = {}
			table[value[0]][value[1]][value[2]] = value[3]
	
	return render_template("index.html", data=table)

@app.route("/AddData/<sensorId>", methods=["GET", "POST"])
def AddData(sensorId):
	global table
	data = request.form if request.method == "POST" else request.args
	if ("type" in data) and ("value" in data):
		dt = datetime.now().replace(second=0, microsecond=0)
		if "deltatime" in data:
			dt -= timedelta(minutes=float(data["deltatime"]))
			
		with Connection() as conn:
			prevValue = conn.execute(
				"SELECT Value FROM data " 
				"WHERE SensorId = '%s' AND Type LIKE '%s' AND datetime(DateTime) < datetime('%s') "
				"ORDER BY datetime(DateTime) DESC LIMIT(1)" \
				% (sensorId, data["type"], dt)).fetchone()
				
			if prevValue == None or float(prevValue[0]) != float(data["value"]):
				conn.execute("INSERT INTO data(SensorId, DateTime, Type, Value) VALUES(?, ?, ?, ?)", \
					sensorId, dt, str(data["type"]), float(data["value"]))
				conn.commit()
				table = None # cache data for index page

	return "OK"
	
@app.route("/GetSleepTime")
def GetSleepTime():
	return str(15 * 60) # in seconds
	
@app.route("/restart")
def restart():
	func = request.environ.get('werkzeug.server.shutdown')
	if func is not None:
		func()
	return redirect("/")
	

if __name__ == "__main__":
	Logger.log("info", "Start Smart Hive")
	
	app.config['TEMPLATES_AUTO_RELOAD']=True
	app.run(debug=False, host="0.0.0.0")