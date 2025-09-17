# IoT project

## Overview

In this project, I integrated an embedded system with cloud infrastructure using the MQTT protocol. I set up a virtual server on AWS, configured an MQTT broker, stored sensor data in a PostgreSQL database, and displayed data on a simple webpage using Flask. This project taught me how complete IoT workflows are implemented: from data transmission to cloud storage and visualisation

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

## Part 2: Sending sensor data to the Cloud using MQTT and Wi-Fi

The first part was about establishing somewhere to store the data. Now, this second part is about going through the process of collecting and sending the data, which will be stored in the cloud.

The embedded device software will be built which will periodically send sensor readings using the MQTT protocol - WiFi drivers and an MQTT library will need to be incorporated. A Real-Time Operating System (RTOS) needs to be used for the main processing loop, and the sensors need to be manually interfaced with.

Enabling a 'global interrupt' is essential for this part. It stops the system when an event occurs, takes a specific action, and then returns to the current process.

This part also uses the 'ISM433 component'. This allows for interfacing and interacting with the Wi-Fi network, allowing online correspondence between a web server and site.

These need to be implemented so that the microcontroller can respond to new sensor readings in real time and upload them via Wi-Fi.

### Steps taken

- The first step is to create a new CMSIS solution in VSCode (on another VSCode window) and use the Blinky template as it already has the files needed for this project. 

CMSIS solutions are used in IoT workflows because they give developers a standard, optimised, and portable way to program Arm-based microcontrollers - the tiny processors inside many IoT devices. They save time, improve performance, and make IoT software easier to scale across different hardware.

- Once created, the board needs to be configured using STM32CubeMX as the SPI3 global interrupt needs to be enabled. Then, prompt CubeMX to generate the code.

- Now, some additional packs to the solution needs to be added to bring in the required libraries. In the solution file 'Blinky.csolution.yml', the CMSIS Driver Pack and the coreMQTT pack is added, as well as a new definition to use the default configuration for MQTT.

- In the CMSIS Driver component, the WiFi API and ISM43362 components need to be enabled which require the SPI component to be enabled too. Additionally, coreMQTT under FreeRTOS needs to be enabled.

- Now it is time to edit the configuration for some of the components to connect them up to the right place. First, in the configuration header file of the WiFi driver, the SPI driver number needs to be 3 because the WiFi device on the board is connected to SPI bus number 3. Some code needs to be added to tell the driver how to reset and activate the WiFi device. In the hardware source code file, the GPIO_PINS for the Data ready pin, the Reset pin, and the Peripheral select pin needed to be initialised. Then, some more code was added to perform the reset and device selection functionality.

- Now, the main program code is added in the 'Blinky.c' file. The idea of this program is that the device will connect to WiFi then open an MQTT connection to the server made in part 1, then read and transmit data from the sensors every 5 seconds.

The file starts with definitions used to access the sensors. The sensors are connected to I2C bus number 2 and there are two that are of concern.

The HTS221 sensor is for reading temperature and humidity. The HTS221_Init routine turns the device off, pauses for a moment, then turns the device on, configuring it for one-shot mode which means that the device can be triggered to take a reading. The sensor also contains calibration data which is prepared by the factory and used to calculate the actual temperature and humidity values.

The read calibration routine reads out the calibration values from the device, does some basic arithmetic, and stores the values in global variables for use later. The trigger routine requests the sensor to take a reading, it issues the command, and then waits for the data to become available by polling the status register. The read temperature and read humidity routines then read the values out of the sensor, apply the calibration factor and return the results.

The LPS22HB sensor measures pressure and the code is set up the same way as in the HTS221. The initialisation routine resets the device and the read pressure routine reads out the pressure data. This time, the trigger is built into the reading routine.

Following the sensor code comes the code required to enable the MQTT library to speak to the onboard WiFi device. The MQTT library is generic and can work with a wide range of embedded platforms, so an interface needs to be provided to the particular device used here.

The transport, receive, and send routines talk to the WiFi device to receive and send data over the network. The user callback is not used because topics aren't being subscribed to. The GetTime routine asks the RTOS for the current time, returning it in milliseconds since boot. 

Now the main thread: first, the sensors are initialised. Next, the WiFi device is initalised and connected to an access point which is where my own WiFi credentials needs to be added. (hide this in the code later)
After the WiFi connection is established, a connection to the MQTT broker is attempted. Here, the IP address in the code needs to be changed to the IP address of my broker which is found in the public address of my instance in the AWS console (also hide this in the code later).

Next, the MQTT library is initialised by hooking up the transport routines and connecting through to the broker. 

After all of this initialisation comes the main application loop. Here, the sensor data is read and then put into JSON format using the snprintf routine. Once the JSON payload has been constructed, it is published to the MQTT broker. Then there is a 5 second delay and the loop starts again.

The solution can now be built and ran on the board. Opening the serial monitor will allow the relevant output to be seen. The board is now transmitting to the server, and if the two Python scripts are running (they should be on another VSCode Window), the web page created can be accessed and the real sensor data coming in from the device can be read.
