/*
  xsns_85_a01nyub.ino - A01NYUB Ultrasonic sensor support for Tasmota

  Copyright (C) 2021  Marc Andre

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

#ifdef USE_A01NYUB
/*********************************************************************************************\
 * A01NYUB Waterproof Ultrasonic Sensor SKU: SEN0313
 * For background information see 
 * https://wiki.dfrobot.com/A01NYUB%20Waterproof%20Ultrasonic%20Sensor%20SKU:%20SEN0313
 *\*********************************************************************************************/

#define XSNS_85             85

#include <TasmotaSerial.h>

TasmotaSerial *a01nyubSerial;
uint8_t a01nyub_initialized = 0;

struct a01nyubdata {
  uint8_t valid;
  uint16_t distance;
  uint8_t buffer[7];
} a01nyub_data;


/*********************************************************************************************/

void A01NYUBPoll(void)                 // Every 1s
{
  if ( !a01nyub_initialized ) return;

  while (a01nyubSerial->available()) {
    int len;
    len = a01nyubSerial->readBytes(a01nyub_data.buffer, 7);
    if ( len < 4 ) continue;


    for ( int i=0 ; i < (len - 3) ; i++ ) 
    {
        if ( a01nyub_data.buffer[i] != 0xff ) continue; // Invalid packet
        int checksum = a01nyub_data.buffer[i] + a01nyub_data.buffer[i+1] + a01nyub_data.buffer[i+2];
        checksum &= 0xff;
        if ( a01nyub_data.buffer[i+3] != checksum ) continue; // Invalid packet

        a01nyub_data.distance = (((int)a01nyub_data.buffer[i+1]) << 8) + a01nyub_data.buffer[i+2];
        a01nyub_data.valid = 1;
    }
  }
}

/*********************************************************************************************/

void A01NYUBInit(void)
{
  a01nyub_data.valid = 0;
  if (PinUsed(GPIO_A01NYUB_RX)) {
    a01nyubSerial = new TasmotaSerial(Pin(GPIO_A01NYUB_RX), -1, 1);
    if (a01nyubSerial->begin(9600)) {
      if (a01nyubSerial->hardwareSerial()) { ClaimSerial(); }
      a01nyub_initialized = 1;
    }
  }
}

/*********************************************************************************************/


#ifdef USE_WEBSERVER
const char HTTP_A01NYUB_SNS[] PROGMEM =
// {s} = <tr><th>, {m} = </th><td>, {e} = </td></tr>
  "{s}A01NYUB " D_DISTANCE "{m}%d " D_UNIT_MILLIMETER "{e}";
#ifdef A01NYUB_DBG
const char HTTP_A01NYUB_DBG[] PROGMEM =
// {s} = <tr><th>, {m} = </th><td>, {e} = </td></tr>
  "{s}A01NYUB data{m}%02x %02x %02x %02x %02x %02x %02x{e}";
#endif
#endif  // USE_WEBSERVER

void A01NYUBShow(bool json)
{
  if (json) {
    ResponseAppend_P(PSTR(",\"A01NYUB\":{\"D\":%d,\"V\":%d}"), a01nyub_data.distance, a01nyub_data.valid);
#ifdef USE_WEBSERVER
    } else {
        if (a01nyub_data.valid) {
            WSContentSend_PD(HTTP_A01NYUB_SNS, a01nyub_data.distance); 
        }
        else {
            if ( a01nyub_initialized ) {
                WSContentSend_PD("{s}A01NYUB{m}no data{e}");
            }
        }
#ifdef A01NYUB_DBG
        if ( a01nyub_initialized ) {
             WSContentSend_PD(HTTP_A01NYUB_DBG,
                a01nyub_data.buffer[0], a01nyub_data.buffer[1], a01nyub_data.buffer[2], a01nyub_data.buffer[3],
                a01nyub_data.buffer[4], a01nyub_data.buffer[5], a01nyub_data.buffer[6]);
        }
#endif
    }
#endif  // USE_WEBSERVER
}

/*********************************************************************************************\
 * Interface
\*********************************************************************************************/

bool Xsns85(uint8_t function)
{
  bool result = false;

    switch (function) {
        case FUNC_INIT:
        A01NYUBInit();
        break;
        case FUNC_EVERY_SECOND:
        A01NYUBPoll();
        break;
        case FUNC_JSON_APPEND:
        A01NYUBShow(1);
        break;
    #ifdef USE_WEBSERVER
        case FUNC_WEB_SENSOR:
        A01NYUBShow(0);
        break;
    #endif  // USE_WEBSERVER
    }
  return result;
}

#endif  // USE_A01NYUB
