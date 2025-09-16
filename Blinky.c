/*---------------------------------------------------------------------------
 * Copyright (c) 2024 Arm Limited (or its affiliates).
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *---------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>

#include "Driver_Common.h"
#include "core_mqtt.h"
#include "core_mqtt_serializer.h"
#include "main.h"

#include "Driver_I2C.h"
#include "Driver_WiFi.h"
#include "cmsis_os2.h" // ::CMSIS:RTOS2
#include "cmsis_vio.h"
#include "os_tick.h"


extern I2C_HandleTypeDef hi2c2;

// ----------------------------------------------------- //
// Sensor devices
// ----------------------------------------------------- //

#define HTS221_ADDRESS 0xBE
#define HTS221_TEMP_OUT_L 0x2A  // Temperature output register (low byte)
#define HTS221_TEMP_OUT_H 0x2B  // Temperature output register (high byte)
#define HTS221_HUMID_OUT_L 0x28 // Hummidity output register (low byte)
#define HTS221_HUMID_OUT_H 0x29 // Hummidity output register (high byte)
#define HTS221_CTRL_REG1 0x20   // Control register for power on
#define HTS221_CTRL_REG2 0x21   // Control register for one-shot
#define HTS221_STATUS_REG 0x27  // Status register for data readiness

#define LPS22HB_ADDRESS 0xBA
#define LPS22HB_CTRL_REG1 0x10 // Control register 1
#define LPS22HB_CTRL_REG2 0x11 // Control register 2
#define LPS22HB_PRESS_OUT_XL 0x28
#define LPS22HB_PRESS_OUT_L 0x29
#define LPS22HB_PRESS_OUT_H 0x2A
#define LPS22HB_STATUS_REG 0x2A // Status register for data readiness

float T0_degC, T1_degC;
int16_t T0_OUT, T1_OUT;
float H0_rH, H1_rH;
int16_t H0_T0_OUT, H1_T0_OUT;

// Helper routines to read and write I2C register-based devices.
static uint8_t I2C_ReadRegister(I2C_HandleTypeDef *iface, uint32_t dev_addr,
                           uint8_t mem_addr) {
  uint8_t mem_val;
  HAL_I2C_Mem_Read(iface, dev_addr, mem_addr, 1, &mem_val, 1, HAL_MAX_DELAY);

  return mem_val;
}

static void I2C_WriteRegister(I2C_HandleTypeDef *iface, uint32_t dev_addr,
                         uint8_t mem_addr, uint8_t mem_val) {
  HAL_I2C_Mem_Write(iface, dev_addr, mem_addr, 1, &mem_val, 1, HAL_MAX_DELAY);
}

// ----------------------------------------------------- //
// HTS221 Temperature and Humidity Sensor
// ----------------------------------------------------- //

int32_t HTS221_Init(void) {
  // Power-off the device, to ensure it's reset.
  I2C_WriteRegister(&hi2c2, HTS221_ADDRESS, HTS221_CTRL_REG1, 0x00);
  osDelay(1000);

  // Power-on the device, and set to one-shot mode.
  I2C_WriteRegister(&hi2c2, HTS221_ADDRESS, HTS221_CTRL_REG1, 0x84);
  osDelay(1000);

  return 0;
}

void HTS221_Read_Calibration(void) {
  uint8_t T0_degC_x8, T1_degC_x8;
  uint8_t T0_T1_msb;
  uint8_t T0_OUT_L, T0_OUT_H, T1_OUT_L, T1_OUT_H;
  uint8_t H0_rH_x2, H1_rH_x2;
  uint8_t H0_T0_OUT_L, H0_T0_OUT_H, H1_T0_OUT_L, H1_T0_OUT_H;

  // Retrieve temperature calibration values
  T0_degC_x8 = I2C_ReadRegister(&hi2c2, HTS221_ADDRESS, 0x32);
  T1_degC_x8 = I2C_ReadRegister(&hi2c2, HTS221_ADDRESS, 0x33);
  T0_T1_msb = I2C_ReadRegister(&hi2c2, HTS221_ADDRESS, 0x35);

  T0_degC = ((T0_T1_msb & 0x03) << 8 | T0_degC_x8) / 8.0;
  T1_degC = ((T0_T1_msb & 0x0C) << 6 | T1_degC_x8) / 8.0;

  T0_OUT_L = I2C_ReadRegister(&hi2c2, HTS221_ADDRESS, 0x3C);
  T0_OUT_H = I2C_ReadRegister(&hi2c2, HTS221_ADDRESS, 0x3D);
  T0_OUT = (int16_t)((T0_OUT_H << 8) | T0_OUT_L);

  T1_OUT_L = I2C_ReadRegister(&hi2c2, HTS221_ADDRESS, 0x3E);
  T1_OUT_H = I2C_ReadRegister(&hi2c2, HTS221_ADDRESS, 0x3F);
  T1_OUT = (int16_t)((T1_OUT_H << 8) | T1_OUT_L);

  // Retrieve humidity calibration values
  H0_rH_x2 = I2C_ReadRegister(&hi2c2, HTS221_ADDRESS, 0x30);
  H1_rH_x2 = I2C_ReadRegister(&hi2c2, HTS221_ADDRESS, 0x31);

  H0_rH = H0_rH_x2 / 2.0;
  H1_rH = H1_rH_x2 / 2.0;

  H0_T0_OUT_L = I2C_ReadRegister(&hi2c2, HTS221_ADDRESS, 0x36);
  H0_T0_OUT_H = I2C_ReadRegister(&hi2c2, HTS221_ADDRESS, 0x37);
  H0_T0_OUT = (int16_t)((H0_T0_OUT_H << 8) | H0_T0_OUT_L);

  H1_T0_OUT_L = I2C_ReadRegister(&hi2c2, HTS221_ADDRESS, 0x3A);
  H1_T0_OUT_H = I2C_ReadRegister(&hi2c2, HTS221_ADDRESS, 0x3B);
  H1_T0_OUT = (int16_t)((H1_T0_OUT_H << 8) | H1_T0_OUT_L);

  printf("hts221: calibration: t0=%.2f, t1=%.2f, h0=%.2f, h1=%.2f\n", T0_degC,
         T1_degC, H0_rH, H1_rH);
}

void HTS221_Trigger(void) {
  I2C_WriteRegister(&hi2c2, HTS221_ADDRESS, HTS221_CTRL_REG2, 1);

  while ((I2C_ReadRegister(&hi2c2, HTS221_ADDRESS, HTS221_STATUS_REG) & 3) != 3)
    ;
}

float HTS221_Read_Temperature(void) {
  uint8_t temp_out_l = 0, temp_out_h = 0;
  int16_t temp_raw = 0;

  temp_out_l = I2C_ReadRegister(&hi2c2, HTS221_ADDRESS, HTS221_TEMP_OUT_L);
  temp_out_h = I2C_ReadRegister(&hi2c2, HTS221_ADDRESS, HTS221_TEMP_OUT_H);

  temp_raw = ((int16_t)temp_out_h << 8) | temp_out_l;

  // Apply calibration formula and manual offset
  float temperature_celsius =
      T0_degC +
      ((float)(temp_raw - T0_OUT) / (T1_OUT - T0_OUT)) * (T1_degC - T0_degC);
  temperature_celsius -= 5.0; // Adjust to correct for observed offset

  return temperature_celsius;
}

float HTS221_Read_Humidity(void) {
  uint8_t hum_out_l = 0, hum_out_h = 0;
  int16_t hum_raw = 0;

  hum_out_l = I2C_ReadRegister(&hi2c2, HTS221_ADDRESS, HTS221_HUMID_OUT_L);
  hum_out_h = I2C_ReadRegister(&hi2c2, HTS221_ADDRESS, HTS221_HUMID_OUT_H);

  hum_raw = ((int16_t)hum_out_h << 8) | hum_out_l;

  // Convert raw humidity to percentage
  float humidity_percentage =
      H0_rH + ((float)(hum_raw - H0_T0_OUT) * (H1_rH - H0_rH)) /
                  (H1_T0_OUT - H0_T0_OUT);

  return humidity_percentage;
}

// ----------------------------------------------------- //
// LPS22HB Pressure Sensor
// ----------------------------------------------------- //

int32_t LPS22HB_Init(void) {
  // Power-off the device, to ensure it's reset.
  I2C_WriteRegister(&hi2c2, LPS22HB_ADDRESS, LPS22HB_CTRL_REG1, 0x00);
  osDelay(1000);

  // Power-on the device, and set to one-shot mode.
  I2C_WriteRegister(&hi2c2, LPS22HB_ADDRESS, LPS22HB_CTRL_REG1, 0x02);
  osDelay(1000);

  return 0;
}

float LPS22HB_Read_Pressure(void) {
  // Trigger one-shot data collection
  I2C_WriteRegister(&hi2c2, LPS22HB_ADDRESS, LPS22HB_CTRL_REG2, 0x01);

  // Wait for presure data to be available...
  while ((I2C_ReadRegister(&hi2c2, LPS22HB_ADDRESS, LPS22HB_STATUS_REG) & 1) != 1)
    ;

  uint8_t press_out_xl, press_out_l, press_out_h;
  int32_t press_raw = 0;

  press_out_xl = I2C_ReadRegister(&hi2c2, LPS22HB_ADDRESS, LPS22HB_PRESS_OUT_XL);
  press_out_l = I2C_ReadRegister(&hi2c2, LPS22HB_ADDRESS, LPS22HB_PRESS_OUT_L);
  press_out_h = I2C_ReadRegister(&hi2c2, LPS22HB_ADDRESS, LPS22HB_PRESS_OUT_H);

  // Combine bytes into 24-bit raw pressure value
  press_raw =
      ((int32_t)press_out_h << 16) | ((int32_t)press_out_l << 8) | press_out_xl;

  // Convert raw pressure to hPa
  return press_raw / 4096.0;
}

// ----------------------------------------------------- //
// Wi-Fi and MQTT
// ----------------------------------------------------- //

extern ARM_DRIVER_WIFI Driver_WiFi0;

// Static buffer for MQTT data storage.
static uint8_t mqttBufferStorage[2048];
static MQTTFixedBuffer_t mqttBuffer = {mqttBufferStorage,
                                       sizeof(mqttBufferStorage)};

// Represents a network connection.
struct NetworkContext {
  ARM_DRIVER_WIFI *iface;
  int32_t socket;
};

/*
 * Receives data from the WiFi connection.
 */
static int32_t transportRecv(NetworkContext_t *pNetworkContext, void *pBuffer,
                             size_t bytesToRecv) {
  return pNetworkContext->iface->SocketRecv(pNetworkContext->socket, pBuffer,
                                            bytesToRecv);
}

/*
 * Sends data over the WiFi connection.
 */
static int32_t transportSend(NetworkContext_t *pNetworkContext,
                             const void *pBuffer, size_t bytesToSend) {
  return pNetworkContext->iface->SocketSend(pNetworkContext->socket, pBuffer,
                                            bytesToSend);
}

/*
 * Called for incoming MQTT messages.
 */
static void mqttUserCallback(struct MQTTContext *pContext,
                             struct MQTTPacketInfo *pPacketInfo,
                             struct MQTTDeserializedInfo *pDeserializedInfo) {
  // Not used.
}

/*
 * Returns the current system time in milliseconds.
 */
static uint32_t mqttGetTime() {

  return (1000 * osKernelGetTickCount()) / osKernelGetTickFreq();
}

// A unique MQTT client identifier
#define MQTT_CLIENT_IDENTIFIER "lab4-iot-client"

/*-----------------------------------------------------------------------------
 * Application main thread
 *----------------------------------------------------------------------------*/
__NO_RETURN void app_main_thread(void *argument) {

  printf("LAB4!\n");

  HTS221_Init();
  HTS221_Read_Calibration();
  LPS22HB_Init();

  // Initialise the Wi-Fi device.
  printf("wi-fi: initialising...\n");
  int32_t rc = Driver_WiFi0.Initialize(NULL);
  if (rc) {
    printf("Wi-Fi initialization failed: %d\n", rc);
    for (;;) {
    }
  }

  Driver_WiFi0.PowerControl(ARM_POWER_FULL);

  // Connect to a wireless network

  // ** IMPORTANT! ** //
  // You must change these settings to your own Wi-Fi network configuration
  ARM_WIFI_CONFIG_t wifiConfig = {0};
  wifiConfig.ssid = "Glide_Resident";                // Your network SSID.
  wifiConfig.pass = "SavedStepCork";            // Your network password.
  wifiConfig.security = ARM_WIFI_SECURITY_WPA2; // Your network security mode.
  wifiConfig.ch = 0;
  wifiConfig.wps_method = ARM_WIFI_WPS_METHOD_NONE;

  // Activate the Wi-Fi connection.
  printf("wi-fi: connecting to ssid '%s'...\n", wifiConfig.ssid);
  rc = Driver_WiFi0.Activate(0, &wifiConfig);
  if (rc) {
    printf("Wi-Fi activation failed: %d\n", rc);
    for (;;) {
    }
  }

  // Create a socket to connect to the server
  printf("wi-fi: creating comms socket...\n");
  int32_t brokerConnectionSocket = Driver_WiFi0.SocketCreate(
      ARM_SOCKET_AF_INET, ARM_SOCKET_SOCK_STREAM, ARM_SOCKET_IPPROTO_TCP);
  if (brokerConnectionSocket < 0) {
    printf("Socket creation failed: %d\n", brokerConnectionSocket);
    for (;;) {
    }
  }

  // ** IMPORTANT! ** //
  // You must change this IP address to the IP address of your virtual machine
  // which you can get from the AWS (or otherwise) console.
  uint8_t ip[4] = {13, 60, 231, 205};

  printf("wi-fi: establishing tcp connection to broker...\n");
  int32_t src =
      Driver_WiFi0.SocketConnect(brokerConnectionSocket, ip, sizeof(ip), 1883);

  if (src) {
    printf("Socket connection failed: %d\n", src);
    for (;;) {
    }
  }

  // Create the MQTT context.
  MQTTContext_t mqtt = {0};

  // Network connection details.
  NetworkContext_t mqttNetwork = {&Driver_WiFi0, brokerConnectionSocket};

  // Routines for communicating over the Wi-Fi connection.
  TransportInterface_t mqttTransport = {0};
  mqttTransport.pNetworkContext = &mqttNetwork;
  mqttTransport.recv = transportRecv;
  mqttTransport.send = transportSend;

  // Initialise the MQTT subsystem.
  printf("mqtt: initialising...\n");
  MQTTStatus_t mrc = MQTT_Init(&mqtt, &mqttTransport, mqttGetTime,
                               mqttUserCallback, &mqttBuffer);
  if (mrc) {
    printf("MQTT initialisation failed: %s\n", MQTT_Status_strerror(mrc));
    for (;;) {
    }
  }

  // Establish a connection to the broker.
  MQTTConnectInfo_t connectInfo = {0};
  connectInfo.cleanSession = true;
  connectInfo.pClientIdentifier = MQTT_CLIENT_IDENTIFIER;
  connectInfo.clientIdentifierLength = strlen(connectInfo.pClientIdentifier);
  connectInfo.keepAliveSeconds = 60;

  bool sessionPresent = false;
  printf("mqtt: connecting...\n");
  mrc = MQTT_Connect(&mqtt, &connectInfo, NULL, 1000, &sessionPresent);
  if (mrc) {
    printf("MQTT connection failed: %s\n", MQTT_Status_strerror(mrc));
    for (;;) {
    }
  }

  printf("ok! starting sensor loop...\n");
  do {
    HTS221_Trigger();

    printf("reading sensor data...\n");
    float temperature = HTS221_Read_Temperature();
    float humidity = HTS221_Read_Humidity();
    float pressure = LPS22HB_Read_Pressure();

    printf("sensors: temperature=%f, humidity=%f, pressure=%f\n", temperature,
           humidity, pressure);

    char payload[256];
    snprintf(payload, 256,
             "{ \"temperature\": %f, \"humidity\": %f, \"pressure\": %f }",
             temperature, humidity, pressure);

    // Send a message to the broker.
    uint16_t pid = MQTT_GetPacketId(&mqtt);
    MQTTPublishInfo_t publishInfo = {0};

    publishInfo.pTopicName = "arm/edx/lab4";
    publishInfo.topicNameLength = strlen(publishInfo.pTopicName);
    publishInfo.pPayload = payload;
    publishInfo.payloadLength = strlen(payload);

    printf("mqtt: publishing...\n");
    mrc = MQTT_Publish(&mqtt, &publishInfo, pid);
    if (mrc) {
      printf("MQTT publish failed: %s\n", MQTT_Status_strerror(mrc));
      for (;;) {
      }
    }

    // Delay for five seconds
    osDelay(osKernelGetTickFreq() * 5);

  } while (1);
}

/*-----------------------------------------------------------------------------
 * Application initialization
 *----------------------------------------------------------------------------*/
int app_main(void) {
  osKernelInitialize(); /* Initialize CMSIS-RTOS2 */
  osThreadNew(app_main_thread, NULL, NULL);
  osKernelStart(); /* Start thread execution */
  return 0;
}
