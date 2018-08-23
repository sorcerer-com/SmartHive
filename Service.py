#!/usr/bin/env python
from flask import *
from datetime import datetime, timedelta
from shutil import copyfile

from libs.Logger import *
from libs.Connection import *
import USBHandler

Connection.DatabaseFile = "bin/data.db"

logging.getLogger().info("")
app = Flask(__name__, template_folder = "./")

table = None
dataThresholds = { "Temperature": 0.5, "Humidity": 5.0, "Weight": 100 }
sleepTime = 30 # minutes
testMode = False

@app.route("/")
def index():
	global table
	if table == None:
		with Connection() as conn:
			dt = datetime.now() - timedelta(days=1)
			values = conn.execute(
				"SELECT DateTime, mac.Id, Type, Value FROM data "
				"JOIN MACs mac ON SensorMAC = MAC "
				"WHERE datetime(DateTime) > datetime('%s')" % dt).fetchall()
		
		table = {}
		for value in values:
			if value[0] not in table:
				table[value[0]] = {}
			if value[1] not in table[value[0]]:
				table[value[0]][value[1]] = {}
			if value[2] not in table[value[0]][value[1]]:
				table[value[0]][value[1]][value[2]] = {}
			table[value[0]][value[1]][value[2]] = value[3] # time / (id / (type / value))
	
	return render_template("index.html", data=table, datetime=datetime.now().strftime("%Y-%m-%d %H:%M"), 
		testMode=testMode)

@app.route("/AddData/<sensorMAC>", methods=["GET", "POST"])
def AddData(sensorMAC):
	global table
	data = request.form if request.method == "POST" else request.args
	if ("type" in data) and ("value" in data):
		dt = datetime.now().replace(second=0, microsecond=0)
		if "index" in data:
			dt += timedelta(minutes=int(data["index"])*sleepTime)
		
		Logger.log("info", "Receiving data %s: %s for %s" % (data["type"], data["value"], dt))
		if testMode: # in test mode don't write data
			return "OK"
			
		# backup data
		copyfile(Connection.DatabaseFile, Connection.DatabaseFile + ".bak")
		
		with Connection() as conn:
			prevValue = conn.execute(
				"SELECT Value FROM data " 
				"WHERE SensorMAC = '%s' AND Type LIKE '%s' AND datetime(DateTime) < datetime('%s') "
				"ORDER BY datetime(DateTime) DESC LIMIT(1)" \
				% (sensorMAC, data["type"], dt)).fetchone()
				
			conn.execute("UPDATE MACs SET LastActivity = ? WHERE MAC = ?", \
				datetime.now().replace(second=0, microsecond=0), sensorMAC)
			conn.commit()
					
			threshold = dataThresholds[data["type"]] if data["type"] in dataThresholds else 0.01
			if prevValue == None or prevValue[0] == None or \
				abs(float(prevValue[0]) - float(data["value"])) > threshold:	
				# insert mac if isn't already there
				conn.execute("INSERT INTO MACs(MAC) "
					"SELECT ? WHERE NOT EXISTS (SELECT MAC FROM MACs WHERE MAC = ?)", sensorMAC, sensorMAC)
				# insert data
				conn.execute("INSERT INTO data(SensorMAC, DateTime, Type, Value) VALUES(?, ?, ?, ?)", \
					sensorMAC, dt, str(data["type"]), float(data["value"]))
				conn.commit()
				table = None # clear cached data for index page

	return "OK"
	
@app.route("/GetSleepTime")
def GetSleepTime():
	return str(sleepTime * 60) # to seconds
	
@app.route("/GetSoftwareVersion")
def GetSoftwareVersion():
	return str(1)
	
@app.route("/GetSoftware")
def GetSoftware():
	fileName = "bin/versions/sensor_v" + GetSoftwareVersion() + ".bin"
	Logger.log("info", "Get Software: " + fileName)
	return send_file(fileName, "application/octet-stream", True, cache_timeout=60)
	
@app.route("/restart")
def restart():
	USBHandler.deinit()
	func = request.environ.get('werkzeug.server.shutdown')
	if func is not None:
		func()
	return redirect("/")
	
@app.route("/setTime")
def setTime():
	data = request.form if request.method == "POST" else request.args
	if "time" in data:
		Logger.log("info", "Set time received: %s" % data["time"])
		os.system('sudo date -s @%s' % data["time"])
	return redirect("/")
	
@app.route("/setTestMode")
def setTestMode():
	global testMode, sleepTime
	testMode = not testMode
	sleepTime = 2 if testMode else 30
	Logger.log("info", "Set test mode: %s (%s s)" % (testMode, sleepTime))
	return redirect("/")
	

if __name__ == "__main__":
	Logger.log("info", "Start Smart Hive")
	
	USBHandler.init()
	app.config['TEMPLATES_AUTO_RELOAD'] = True
	app.run(debug=False, host="0.0.0.0", threaded=True)
	USBHandler.deinit()
