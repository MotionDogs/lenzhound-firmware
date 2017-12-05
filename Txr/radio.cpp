#include "settings.h"
#include "serial_api.h"
#include "radio.h"
#include "Arduino.h"
#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>
#include "bsp.h"
#include "config.h"
#include "eeprom_assert.h"

#define HEARTBEAT_INTERVAL_MILLIS 2000

radio_state_t radio_state = {0};

bool _is_radio_available()
{
    return !Mirf.isSending();
}


bool _get_radio_packet(void *buffer)
{
    if (!Mirf.isSending() && Mirf.dataReady()) {
        uint8_t *buf = (uint8_t *)buffer;
        Mirf.getData(buf);
        return true;
    } else {
        return false;
    }
}

void _send_radio_packet(void *buffer)
{
    Mirf.setTADDR((uint8_t *)TRANSMIT_ADDRESS);
    Mirf.send((uint8_t *)buffer);
    while (Mirf.isSending()) {
    }
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
    Mirf.spi = &MirfHardwareSpi;
    Mirf.init();
    Mirf.setRADDR((uint8_t *)RECEIVE_ADDRESS);
    Mirf.payload = sizeof(radio_packet_t);

    int channel = settings_get_channel();
    radio_set_channel(channel, true);
}

void radio_run()
{
    radio_packet_t packet = {0};

    _get_radio_packet(&packet);

    if (_is_radio_available() && radio_state.read_index != radio_state.write_index) {
        radio_packet_t *out_packet = &radio_state.buffer[radio_state.read_index++];
        radio_state.read_index %= RADIO_OUT_BUFFER_SIZE;
        _send_radio_packet(out_packet);
    }
}

void radio_set_channel(int channel, bool force)
{
    radio_packet_t packet = {0};
    while (Mirf.isSending()) {
    }
    while (_get_radio_packet(&packet)) {
    }
    if (force || Mirf.channel != channel) {
        char reg[] = { RF_DEFAULT, 0 };

        if (channel >= 1 && channel <= 82) {
            Mirf.channel = channel;
        }

        Mirf.writeRegister(RF_SETUP, (uint8_t *)reg, 1);
        Mirf.config();   
    }
}

bool radio_is_alive()
{
    uint8_t addr[mirf_ADDR_LEN];

    Mirf.readRegister(TX_ADDR, addr, mirf_ADDR_LEN);
    return memcmp(addr, (uint8_t *)TRANSMIT_ADDRESS, mirf_ADDR_LEN) == 0;
}
