#include "settings.h"
#include "radio.h"
#include "Arduino.h"
#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>
#include "bsp.h"
#include "version.h"

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

void _queue_print_val(radio_state_t *state, char type, long val)
{
    char buffer[16];
    sprintf(buffer, "%c=%ld", type, val);
    serial_api_queue_output(state->serial_state, SERIAL_API_SRC_CONSOLE, buffer);
}

void _send_value(radio_state_t* state, char type, int value)
{
    radio_packet_t packet;
    packet.typed_val.type = type;
    packet.typed_val.val = value;
    radio_queue_message(state, packet);
}

void _process_packet(radio_state_t *state, radio_packet_t packet)
{
    unsigned int uval = packet.typed_uval.val;
    int val = packet.typed_val.val;

    switch (packet.type) {
    case PACKET_PRINT_VERSION: {
        char buffer[16];
        int version = packet.typed_val.val;
        sprintf(buffer, "%c=%d.%d",
                SERIAL_API_CMD_REMOTE_VERSION,
                high_byte(version),
                low_byte(version));
        serial_api_queue_output(state->serial_state, SERIAL_API_SRC_CONSOLE,
                                buffer);
    } break;
    case PACKET_PRINT_ROLE: {
        _queue_print_val(state, SERIAL_API_CMD_REMOTE_ROLE, val);
    } break;
    case PACKET_PRINT_MAX_VELOCITY: {
        _queue_print_val(state, SERIAL_API_CMD_GET_MAX_VELOCITY, uval);
    } break;
    case PACKET_PRINT_ACCEL: {
        _queue_print_val(state, SERIAL_API_CMD_GET_ACCEL, uval);
    } break;
    case PACKET_PRINT_Z_MAX_VELOCITY: {
        _queue_print_val(state, SERIAL_API_CMD_GET_Z_MAX_VELOCITY, uval);
    } break;
    case PACKET_PRINT_Z_ACCEL: {
        _queue_print_val(state, SERIAL_API_CMD_GET_Z_ACCEL, uval);
    } break;
    case PACKET_PRINT_CHANNEL: {
        _queue_print_val(state, SERIAL_API_CMD_GET_REMOTE_CHANNEL, uval);
    } break;
    case PACKET_GET_VERSION: {
        _send_value(state, PACKET_PRINT_VERSION, VERSION);
    } break;
    case PACKET_GET_ROLE: {
        _send_value(state, PACKET_PRINT_ROLE, ROLE);
    } break;
    case PACKET_GET_CHANNEL: {
        int channel = settings_get_channel();
        _send_value(state, PACKET_PRINT_CHANNEL, channel);
    } break;
    case PACKET_SET_CHANNEL: {
        settings_set_channel(uval);
        radio_set_channel(uval);
    } break;
    case PACKET_GET_MAX_VELOCITY: {
    } break;
    case PACKET_GET_ACCEL: {
    } break;
    case PACKET_SET_MAX_VELOCITY: {
    } break;
    case PACKET_SET_ACCEL: {
    } break;
    case PACKET_SET_VELOCITY_PERCENT: {
    } break;
    case PACKET_SET_ACCEL_PERCENT: {
    } break;
    }
}

void radio_queue_message(radio_state_t *state, radio_packet_t packet)
{
    state->buffer[state->write_index++] = packet;
    state->write_index %= RADIO_OUT_BUFFER_SIZE;

    // BSP_assert(state->write_index != state->read_index);
}

void radio_init(radio_state_t *state)
{
    Mirf.spi = &MirfHardwareSpi;
    Mirf.init();
    Mirf.setRADDR((uint8_t *)RECEIVE_ADDRESS);
    Mirf.payload = sizeof(radio_packet_t);

    int channel = settings_get_channel();
    radio_set_channel(channel);
}

void radio_run(radio_state_t *state)
{
    radio_packet_t packet;

    if (_get_radio_packet(&packet)) {
        _process_packet(state, packet);
    }

    if (_is_radio_available() && state->read_index != state->write_index) {
        radio_packet_t *out_packet = &state->buffer[state->read_index++];
        state->read_index %= RADIO_OUT_BUFFER_SIZE;
        if (out_packet->type != PACKET_SET_TARGET_POSITION) {
            char buffer[16];
            sprintf(buffer, "%u %u\n", out_packet->type, out_packet->typed_uval.val);
            Serial.println(buffer);
        }
        _send_radio_packet(out_packet);
    }
}

void radio_set_channel(int channel)
{
    char reg[] = { RF_DEFAULT, 0 };

    if (channel >= 1 && channel <= 82) {
        Mirf.channel = channel;
    }

    Mirf.writeRegister(RF_SETUP, (uint8_t *)reg, 1);
    Mirf.config();
}

bool radio_is_alive()
{
    uint8_t addr[mirf_ADDR_LEN];

    Mirf.readRegister(TX_ADDR, addr, mirf_ADDR_LEN);
    return memcmp(addr, (uint8_t *)TRANSMIT_ADDRESS, mirf_ADDR_LEN) == 0;
}
