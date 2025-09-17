import paho.mqtt.client as mqtt
import json
from pg import DB

# Connect to the local database
print("db: connecting to database...")
db = DB(dbname="cloud-3")

# Prepare to connect to the MQTT broker
broker = "localhost" # The hostname of the MQTT broker
port = 1883 # The port to connect on
topic = "arm/edx/lab4" # The name of the topic to use for messages
client_id = "edx-mqtt" # A unique client-id to identify ourselves to the broker

# Callback for when connection to MQTT broker is successful
def on_mqtt_connect(client, userdata, flags, rc):
    if rc == 0:
        # Connection succeeded
        print("mqtt: connected")
    else:
        # Connection failed
        print("mqtt: connection failed rc=%d\n", rc)

# Callback for when a message is received on a subscribed topic
def on_mqtt_message(client, userdata, msg):
    try:
        # Try to parse the message data from JSON into a Python object
        data = json.loads(msg.payload.decode())

        # Check that the data object contains the necessary keys
        if 'temperature' in data and 'humidity' in data and 'pressure' in data:
            # Indicate we received some data
            print("received sensor data: ", data)

            # Insert the data into the 'data' table, in the database
            db.insert("data",
                temperature =data['temperature'],
                humidity=data['humidity'],
                pressure=data['pressure'],
                timestamp="now()")
    
    except:
        # Catch any exceptions that might occur, probably due
        # to invalid data
        print("invalid data received")

# Create the MQTT client, and register the callbacks
mqtt_client = mqtt.Client(client_id)
mqtt_client.on_connect = on_mqtt_connect
mqtt_client.on_message = on_mqtt_message

# Attempt to connect to the MQTT broker
print("mqtt: connecting...")
mqtt_client.connect(broker, port)

# Subscribe to the MQTT topic we're interested in
mqtt_client.subscribe(topic)

# Run the main loop
mqtt_client.loop_forever()