# IoT project

## Overview

In this project, I integrated an embedded system with cloud infrastructure using the MQTT protocol. I set up a virtual server on AWS, configured an MQTT broker, stored sensor data in a PostgreSQL database, and displayed data on a simple webpage using Flask. This project taught me how complete IoT workflows are implemented: from data transmission to cloud storage and visualisation


The **Blinky** project is a simple example that can be used to verify the
basic tool setup.

It is compliant to the Cortex Microcontroller Software Interface Standard (CMSIS)
and uses the CMSIS-RTOS2 API interface for RTOS functionality. The CMSIS-RTOS2 API
is available with various real-time operating systems, for example RTX5 or FreeRTOS.

---

## Part 1: Connecting the embedded system to the Cloud using MQTT

The first part is to program the embedded device to communicate with a service in the cloud. Then, I need to process the data collected by the device, store it in a database, then build a small website for displaying the data.

I learnt about setting up an online host via AWS and the use of databases to store all of the data - Python and PostgreSQL was used to faciliate this process.

I used MQTT as the messenger system, allowing me to send the required messages to and from the cloud. MQTT (Message Queueing Telemetry Transport) is a messaging protocol commonly used in IoT applications. It enavbles efficient message exchange while minimising power usage and reducing network overhead.

### Steps taken

- First, I need to set up the cloud infrastructure. Using AWS as the cloud service provider, I created an instance with an Ubuntu OS and an SSH connection that allows SSH traffic from anywhere. The SSH key pair is ED25519 encrypted with a .pem format.

Connecting to an Ubuntu AWS instance via SSH is useful in IoT workflows because it gives developers remote, secure, and full command-line control over the backend "brain" of the IoT system, just like working on a normal Linux computer - but from anywhere in the world.

- After launching the instance, I need to open some ports that will allow me to communicate with the instance. This is done by configuring the firewall under "Security": I added two "Custom TCP rules" - one for port 1883 (MQTT) and the other for port 5000 (Web API). Both is allowed access from anywhere.

- Next, I used an SSH client to log into my new virtual machine. Since I'm on a Windows OS, I downloaded Windows Subsystem for Linux (WSL) which allows me to use the client through my computer's terminal. Then, I activated the SSH key pair I created earlier with an ubuntu username.

- Once connected, I installed the Mosquitto MQTT broker and the PostgreSQL database server by running various sudo commands. Once the packages have been installed, I had to make a minor configuration change to allow the MQTT server to be accessed externally. I used the sudo nano text editor in the terminal to listen on all IP addresses on the 1883 port (MQTT port). I also set allow_anonymous to true which allows public access to the MQTT server.

I understand that in general this is strongly discouraged, however this is only for testing and not for production therefore the risk is low. In a real-world setting, authentication via usernames and passwords, and encryption to validate and verify access to the MQTT server will be used.

- After configuring the server, I saved the file and restarted the server. The next thing to do is to set up the database to receive sensor data. I turned into the postgres user and created a database user called ubuntu which is the same as my local username. Through this, I created a database that contains a table of data with 5 fields: the primary key, a timestamp, then temperature, humidity, and pressure which are numeric values. Each row will store the time the reading was received and the actual sensor data.

- At this point, the MQTT broker and the postgres database has been configured completely. Now, I need to write a small Python program that will facilitate the process of storing sensor data by receiving messages from the MQTT broker and storing the received data in the database. 

- I created a new directory then used VSCode to connect to the SSH host via the remote window. I then had to install the "paho-mqtt", "pygresql", and "flask" packages. The 'message_processor.py' file connects to the database, then connects to the MQTT broker, then continually loops, waiting for messages. When a message is received, it is decoded, and if it's valid, the contents are stored into the database. This file is run in the VSCode terminal: run 'python3 message_processor.py'

- Next, I need to write a service to query the database and display a webpage with the most recent sensor readings. I used the Flask framework to do this. The 'data_viewer.py' file connects to the database, initialised the Flask framework, and it responds to HTTP requests on the root URL. It starts by querying the databse, selecting the most recent 5 sensor readings, and then for every row that's returned, a new HTML table row is created that contains the data and time, the temperature, humidity, and pressure. Then, these table rows are put into a HTML document that displays it on the screen. This file should be ran on another VSCode terminal: run 'flask --app data_viewer run --host=0.0.0.0'

At this point, I successfully connected an embedded device to AWS via MQTT, logged data into PostgreSQL, and visualised data through a web interface. This demonstrates essential IoT and cloud integration principles.



---