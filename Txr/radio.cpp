#include "settings.h"
#include "serial_api.h"
#include "radio.h"
#include "Arduino.h"
#include <SPI.h>
#include <RF24.h>
#include "bsp.h"
#include "config.h"
#include "eeprom_assert.h"

#define HEARTBEAT_INTERVAL_MILLIS 2000

RF24 radio(8,7);
byte addresses[][6] = {"clie1","serv1"};
radio_state_t radio_state = {0};

bool _is_radio_available()
{
    return true;
}

bool _get_radio_packet(void *buffer)
{
}

void _send_radio_packet(void *buffer, size_t size)
{
    radio.write(buffer, size);
}

#define PRINT_PACKET_STRING(serial_cmd, name) do {\
    char __buffer[PACKET_STRING_LEN + 4];\
    sprintf(__buffer, "%c=%s",\
            serial_cmd,\
            packet.name.val);\
    serial_api_queue_output(__buffer);\
} while(0)

void radio_queue_message(radio_packet_t packet)
{
    // packet.version = RADIO_VERSION;
    radio_state.buffer[radio_state.write_index++] = packet;
    radio_state.write_index %= RADIO_OUT_BUFFER_SIZE;

    // BSP_assert(radio_state.write_index != radio_state.read_index);
}

void radio_init()
{
    radio.begin();

    // Set the PA Level low to prevent power supply related issues since this is a
    // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
    radio.setRetries(5, 15);
    radio.setChannel(1);
    radio.enableDynamicPayloads();
    radio.setPALevel( RF24_PA_HIGH ) ;
    radio.setDataRate( RF24_250KBPS ) ;
  
    // Transmitting node
    radio.openWritingPipe(addresses[1]);
    radio.openReadingPipe(1,addresses[0]);
}

void radio_run()
{
    radio_packet_t packet = {0};

    _get_radio_packet(&packet);

    if (_is_radio_available() && radio_state.read_index != radio_state.write_index) {
        radio_packet_t *out_packet = &radio_state.buffer[radio_state.read_index++];
        radio_state.read_index %= RADIO_OUT_BUFFER_SIZE;
        _send_radio_packet(out_packet, sizeof(radio_packet_t));
    }
}

void radio_set_channel(int channel, bool force)
{
    radio.setChannel(channel);
}

bool radio_is_alive()
{
    return true;
}
