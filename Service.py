#!/usr/bin/env python
from flask import *
from datetime import datetime, timedelta

from libs.Logger import *
from libs.Connection import *

Connection.DatabaseFile = "bin/data.db"

logging.getLogger().info("")
app = Flask(__name__, template_folder = "./")


@app.route("/")
def index():
	with Connection() as conn:
		dt = datetime.now() - timedelta(days=10)
		values = conn.execute(
			"SELECT SensorId, DateTime, Type, Value FROM data "
			"WHERE datetime(DateTime) > datetime('%s')" % dt).fetchall()
	
	data = {}
	for value in values:
		if value[0] not in data:
			data[value[0]] = {}
		if value[1] not in data[value[0]]:
			data[value[0]][value[1]] = {}
		if value[2] not in data[value[0]][value[1]]:
			data[value[0]][value[1]][value[2]] = {}
		data[value[0]][value[1]][value[2]] = value[3]
	
	return render_template("index.html", data=data)
	
@app.route("/lastSensorId")
def lastSensorId():
	id = 0
	with Connection() as conn:
		row = conn.execute("SELECT MAX(SensorId) FROM data").fetchone()
		if row != None and row[0] != None:
			id = row[0]
	return str(id)

@app.route("/AddData/<sensorId>", methods=["GET", "POST"])
def AddData(sensorId):
	data = request.form if request.method == "POST" else request.args
	if ("type" in data) and ("value" in data):
		dt = datetime.now().replace(second=0, microsecond=0)
		if "deltatime" in data:
			dt -= timedelta(minutes=float(data["deltatime"]))
			
		with Connection() as conn:
			prevValue = conn.execute(
				"SELECT Value FROM data " 
				"WHERE SensorId = %s AND Type LIKE '%s' AND datetime(DateTime) < datetime('%s') "
				"ORDER BY datetime(DateTime) DESC LIMIT(1)" \
				% (int(sensorId), data["type"], dt)).fetchone()
				
			if prevValue == None or float(prevValue[0]) != float(data["value"]):
				conn.execute("INSERT INTO data(SensorId, DateTime, Type, Value) VALUES(?, ?, ?, ?)", \
					int(sensorId), dt, str(data["type"]), float(data["value"]))
				conn.commit()

	return "OK"
	
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