/*
  xsns_18_pms5003.ino - PMS3003, PMS5003, PMS7003 particle concentration sensor support for Tasmota

  Copyright (C) 2019  Theo Arends

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef USE_PMS5003
/*********************************************************************************************\
 * PlanTower PMS3003, PMS5003, PMS7003 particle concentration sensor
 * For background information see http://aqicn.org/sensor/pms5003-7003/ or
 * http://aqicn.org/sensor/pms3003/
 *
 * Hardware Serial will be selected if GPIO3 = [PMS5003]
 * You can either support PMS3003 or PMS5003-7003 at one time. To enable the PMS3003 support
 * you must enable the define PMS_MODEL_PMS3003 on your configuration file.
\*********************************************************************************************/

#define XSNS_18             18

#include <TasmotaSerial.h>

TasmotaSerial *PmsSerial;

uint8_t pms_type = 1;
uint8_t pms_valid = 0;

struct pmsX003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm10_env, pm25_env, pm100_env;
#ifdef PMS_MODEL_PMS3003
  uint16_t reserved1, reserved2, reserved3;
#else
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
#endif  // PMS_MODEL_PMS3003
  uint16_t checksum;
} pms_data;

/*********************************************************************************************/

bool PmsReadData(void)
{
  if (! PmsSerial->available()) {
    return false;
  }
  while ((PmsSerial->peek() != 0x42) && PmsSerial->available()) {
    PmsSerial->read();
  }
#ifdef PMS_MODEL_PMS3003
  if (PmsSerial->available() < 22) {
#else
  if (PmsSerial->available() < 32) {
#endif  // PMS_MODEL_PMS3003
    return false;
  }

#ifdef PMS_MODEL_PMS3003
  uint8_t buffer[22];
  PmsSerial->readBytes(buffer, 22);
#else
  uint8_t buffer[32];
  PmsSerial->readBytes(buffer, 32);
#endif  // PMS_MODEL_PMS3003
  uint16_t sum = 0;
  PmsSerial->flush();  // Make room for another burst

#ifdef PMS_MODEL_PMS3003
  AddLogBuffer(LOG_LEVEL_DEBUG_MORE, buffer, 22);
#else
  AddLogBuffer(LOG_LEVEL_DEBUG_MORE, buffer, 32);
#endif  // PMS_MODEL_PMS3003

  // get checksum ready
#ifdef PMS_MODEL_PMS3003
  for (uint32_t i = 0; i < 20; i++) {
#else
  for (uint32_t i = 0; i < 30; i++) {
#endif  // PMS_MODEL_PMS3003
    sum += buffer[i];
  }
  // The data comes in endian'd, this solves it so it works on all platforms
#ifdef PMS_MODEL_PMS3003
  uint16_t buffer_u16[10];
  for (uint32_t i = 0; i < 10; i++) {
#else
  uint16_t buffer_u16[15];
  for (uint32_t i = 0; i < 15; i++) {
#endif  // PMS_MODEL_PMS3003
    buffer_u16[i] = buffer[2 + i*2 + 1];
    buffer_u16[i] += (buffer[2 + i*2] << 8);
  }
#ifdef PMS_MODEL_PMS3003
  if (sum != buffer_u16[9]) {
#else
  if (sum != buffer_u16[14]) {
#endif  // PMS_MODEL_PMS3003
    AddLog_P(LOG_LEVEL_DEBUG, PSTR("PMS: " D_CHECKSUM_FAILURE));
    return false;
  }

#ifdef PMS_MODEL_PMS3003
  memcpy((void *)&pms_data, (void *)buffer_u16, 20);
#else
  memcpy((void *)&pms_data, (void *)buffer_u16, 30);
#endif  // PMS_MODEL_PMS3003
  pms_valid = 10;

  return true;
}

/*********************************************************************************************/

void PmsSecond(void)                 // Every second
{
  if (PmsReadData()) {
    pms_valid = 10;
  } else {
    if (pms_valid) {
      pms_valid--;
    }
  }
}

/*********************************************************************************************/

void PmsInit(void)
{
  pms_type = 0;
  if (pin[GPIO_PMS5003] < 99) {
    PmsSerial = new TasmotaSerial(pin[GPIO_PMS5003], -1, 1);
    if (PmsSerial->begin(9600)) {
      if (PmsSerial->hardwareSerial()) { ClaimSerial(); }
      pms_type = 1;
    }
  }
}

#ifdef USE_WEBSERVER
#ifdef PMS_MODEL_PMS3003
const char HTTP_PMS3003_SNS[] PROGMEM =
//  "{s}PMS3003 " D_STANDARD_CONCENTRATION " 1 " D_UNIT_MICROMETER "{m}%d " D_UNIT_MICROGRAM_PER_CUBIC_METER "{e}"
//  "{s}PMS3003 " D_STANDARD_CONCENTRATION " 2.5 " D_UNIT_MICROMETER "{m}%d " D_UNIT_MICROGRAM_PER_CUBIC_METER "{e}"
//  "{s}PMS3003 " D_STANDARD_CONCENTRATION " 10 " D_UNIT_MICROMETER "{m}%d " D_UNIT_MICROGRAM_PER_CUBIC_METER "{e}"
  "{s}PMS3003 " D_ENVIRONMENTAL_CONCENTRATION " 1 " D_UNIT_MICROMETER "{m}%d " D_UNIT_MICROGRAM_PER_CUBIC_METER "{e}"
  "{s}PMS3003 " D_ENVIRONMENTAL_CONCENTRATION " 2.5 " D_UNIT_MICROMETER "{m}%d " D_UNIT_MICROGRAM_PER_CUBIC_METER "{e}"
  "{s}PMS3003 " D_ENVIRONMENTAL_CONCENTRATION " 10 " D_UNIT_MICROMETER "{m}%d " D_UNIT_MICROGRAM_PER_CUBIC_METER "{e}";
#else
const char HTTP_PMS5003_SNS[] PROGMEM =
//  "{s}PMS5003 " D_STANDARD_CONCENTRATION " 1 " D_UNIT_MICROMETER "{m}%d " D_UNIT_MICROGRAM_PER_CUBIC_METER "{e}"
//  "{s}PMS5003 " D_STANDARD_CONCENTRATION " 2.5 " D_UNIT_MICROMETER "{m}%d " D_UNIT_MICROGRAM_PER_CUBIC_METER "{e}"
//  "{s}PMS5003 " D_STANDARD_CONCENTRATION " 10 " D_UNIT_MICROMETER "{m}%d " D_UNIT_MICROGRAM_PER_CUBIC_METER "{e}"
  "{s}PMS5003 " D_ENVIRONMENTAL_CONCENTRATION " 1 " D_UNIT_MICROMETER "{m}%d " D_UNIT_MICROGRAM_PER_CUBIC_METER "{e}"
  "{s}PMS5003 " D_ENVIRONMENTAL_CONCENTRATION " 2.5 " D_UNIT_MICROMETER "{m}%d " D_UNIT_MICROGRAM_PER_CUBIC_METER "{e}"
  "{s}PMS5003 " D_ENVIRONMENTAL_CONCENTRATION " 10 " D_UNIT_MICROMETER "{m}%d " D_UNIT_MICROGRAM_PER_CUBIC_METER "{e}"
  "{s}PMS5003 " D_PARTICALS_BEYOND " 0.3 " D_UNIT_MICROMETER "{m}%d " D_UNIT_PARTS_PER_DECILITER "{e}"
  "{s}PMS5003 " D_PARTICALS_BEYOND " 0.5 " D_UNIT_MICROMETER "{m}%d " D_UNIT_PARTS_PER_DECILITER "{e}"
  "{s}PMS5003 " D_PARTICALS_BEYOND " 1 " D_UNIT_MICROMETER "{m}%d " D_UNIT_PARTS_PER_DECILITER "{e}"
  "{s}PMS5003 " D_PARTICALS_BEYOND " 2.5 " D_UNIT_MICROMETER "{m}%d " D_UNIT_PARTS_PER_DECILITER "{e}"
  "{s}PMS5003 " D_PARTICALS_BEYOND " 5 " D_UNIT_MICROMETER "{m}%d " D_UNIT_PARTS_PER_DECILITER "{e}"
  "{s}PMS5003 " D_PARTICALS_BEYOND " 10 " D_UNIT_MICROMETER "{m}%d " D_UNIT_PARTS_PER_DECILITER "{e}";      // {s} = <tr><th>, {m} = </th><td>, {e} = </td></tr>
#endif  // PMS_MODEL_PMS3003
#endif  // USE_WEBSERVER

void PmsShow(bool json)
{
  if (pms_valid) {
    if (json) {
#ifdef PMS_MODEL_PMS3003
      ResponseAppend_P(PSTR(",\"PMS3003\":{\"CF1\":%d,\"CF2.5\":%d,\"CF10\":%d,\"PM1\":%d,\"PM2.5\":%d,\"PM10\":%d}"),
        pms_data.pm10_standard, pms_data.pm25_standard, pms_data.pm100_standard,
        pms_data.pm10_env, pms_data.pm25_env, pms_data.pm100_env);
#else
      ResponseAppend_P(PSTR(",\"PMS5003\":{\"CF1\":%d,\"CF2.5\":%d,\"CF10\":%d,\"PM1\":%d,\"PM2.5\":%d,\"PM10\":%d,\"PB0.3\":%d,\"PB0.5\":%d,\"PB1\":%d,\"PB2.5\":%d,\"PB5\":%d,\"PB10\":%d}"),
        pms_data.pm10_standard, pms_data.pm25_standard, pms_data.pm100_standard,
        pms_data.pm10_env, pms_data.pm25_env, pms_data.pm100_env,
        pms_data.particles_03um, pms_data.particles_05um, pms_data.particles_10um, pms_data.particles_25um, pms_data.particles_50um, pms_data.particles_100um);
#endif  // PMS_MODEL_PMS3003
#ifdef USE_DOMOTICZ
      if (0 == tele_period) {
        DomoticzSensor(DZ_COUNT, pms_data.pm10_env);     // PM1
        DomoticzSensor(DZ_VOLTAGE, pms_data.pm25_env);   // PM2.5
        DomoticzSensor(DZ_CURRENT, pms_data.pm100_env);  // PM10
      }
#endif  // USE_DOMOTICZ
#ifdef USE_WEBSERVER
    } else {

#ifdef PMS_MODEL_PMS3003
        WSContentSend_PD(HTTP_PMS3003_SNS,
//        pms_data.pm10_standard, pms_data.pm25_standard, pms_data.pm100_standard,
        pms_data.pm10_env, pms_data.pm25_env, pms_data.pm100_env);
#else
        WSContentSend_PD(HTTP_PMS5003_SNS,
//        pms_data.pm10_standard, pms_data.pm25_standard, pms_data.pm100_standard,
        pms_data.pm10_env, pms_data.pm25_env, pms_data.pm100_env,
        pms_data.particles_03um, pms_data.particles_05um, pms_data.particles_10um, pms_data.particles_25um, pms_data.particles_50um, pms_data.particles_100um);
#endif  // PMS_MODEL_PMS3003
#endif  // USE_WEBSERVER
    }
  }
}

/*********************************************************************************************\
 * Interface
\*********************************************************************************************/

bool Xsns18(uint8_t function)
{
  bool result = false;

  if (pms_type) {
    switch (function) {
      case FUNC_INIT:
        PmsInit();
        break;
      case FUNC_EVERY_SECOND:
        PmsSecond();
        break;
      case FUNC_JSON_APPEND:
        PmsShow(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_SENSOR:
        PmsShow(0);
        break;
#endif  // USE_WEBSERVER
    }
  }
  return result;
}

#endif  // USE_PMS5003
