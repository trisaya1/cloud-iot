from flask import Flask
from pg import DB
import datetime

# Connect to the local database
print("db: connecting to database...")
db = DB(dbname="cloud-3")

# Initialise the Flask framework
app = Flask(__name__)

@app.route("/")
def sensor_data():
    table_rows = ""

    # Query the database, and obtain the five most recent sensor readings
    query = db.query("SELECT timestamp, temperature, humidity, pressure FROM data ORDER BY timestamp DESC LIMIT 5;")

    for row in query:
        table_rows += "<tr>"
        table_rows += "<td>" + row[0].strftime("%d/%m/%Y %H:%M:%S") + "</td>"
        table_rows += "<td>" + str(row[1]) + "</td>"
        table_rows += "<td>" + str(row[2]) + "</td>"
        table_rows += "<td>" + str(row[3]) + "</td>"
        table_rows += "</tr>"

    return """<!DOCTYPE html>
    <html lang="en">
        <head><title>Sensor Data </title><style>body { font-family: sans-serif; }</style><head>
        <body>
             <hl>Sensor Data</h1>
             <button onclick="window.location.reload()">Refresh</button>
             <table border="1">
                 <thead>
                     <tr>
                          <th>Time</th>
                          <th>Temperature</th>
                          <th>Humidity</th>
                          <th>Pressure</th>
                     </tr>
                 </thead>
                 <tbody>
                 """ + table_rows + """
                 </tbody>
            </table>
        </body>
    </html>"""