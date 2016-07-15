#include "version.h"
#include "settings.h"
#include "radio.h"
#include "util.h"
#include "Arduino.h"
#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>

bool _is_radio_available()
{
    return !Mirf.isSending();
}

bool _get_radio_packet(void* buffer)
{
    if (!Mirf.isSending() && Mirf.dataReady()) {
        uint8_t* buf = (uint8_t*)buffer;
        Mirf.getData(buf);
        return true;
    } else {
        return false;
    }
}

void _send_radio_packet(void* buffer)
{
    Mirf.setTADDR((uint8_t *)TRANSMIT_ADDRESS);
    Mirf.send((uint8_t*)buffer);
    while (Mirf.isSending()) {}
}

void _send_value(radio_state_t* state, char type, int value)
{
    radio_packet_t packet;
    packet.typed_val.type = type;
    packet.typed_val.val = value;
    radio_queue_message(state, packet);
}

void _send_uvalue(radio_state_t* state, char type, unsigned int value)
{
    radio_packet_t packet;
    packet.typed_uval.type = type;
    packet.typed_uval.val = value;
    radio_queue_message(state, packet);
}

void _process_packet(radio_state_t* state, radio_packet_t packet)
{
    unsigned int uval = packet.typed_uval.val;
    int val = packet.typed_val.val;

    switch (packet.type) {
        case (PACKET_PRINT_VERSION): {
        } break;
        case (PACKET_PRINT_ROLE): {
        } break;
        case (PACKET_PRINT_MAX_VELOCITY): {
        } break;
        case (PACKET_PRINT_ACCEL): {
        } break;
        case (PACKET_PRINT_CHANNEL): {
        } break;
        case (PACKET_GET_VERSION): {
            _send_uvalue(state, PACKET_PRINT_VERSION, VERSION);
        } break;
        case (PACKET_GET_ROLE): {
            _send_uvalue(state, PACKET_PRINT_ROLE, ROLE);
        } break;
        case (PACKET_GET_MAX_VELOCITY): {
            unsigned int max_velocity = (unsigned int)state->motor_controller->get_velocity();
            _send_uvalue(state, PACKET_PRINT_MAX_VELOCITY, max_velocity);
        } break;
        case (PACKET_GET_ACCEL): {
            unsigned int accel = (unsigned int)state->motor_controller->get_accel();
            _send_uvalue(state, PACKET_PRINT_ACCEL, accel);
        } break;
        case (PACKET_GET_Z_MAX_VELOCITY): {
            unsigned int max_velocity = (unsigned int)state->motor_controller->get_z_velocity();
            _send_uvalue(state, PACKET_PRINT_Z_MAX_VELOCITY, max_velocity);
        } break;
        case (PACKET_GET_Z_ACCEL): {
            unsigned int accel = (unsigned int)state->motor_controller->get_z_accel();
            _send_uvalue(state, PACKET_PRINT_Z_ACCEL, accel);
        } break;
        case (PACKET_GET_CHANNEL): {
            unsigned int channel = (unsigned int)settings_get_channel();;
            _send_uvalue(state, PACKET_PRINT_CHANNEL, channel);
        } break;
        case (PACKET_SET_MAX_VELOCITY): {
            state->motor_controller->set_velocity(uval);
        } break;
        case (PACKET_SET_ACCEL): {
            state->motor_controller->set_accel(uval);
        } break;
        case (PACKET_SET_Z_MAX_VELOCITY): {
            state->motor_controller->set_z_velocity(uval);
        } break;
        case (PACKET_SET_Z_ACCEL): {
            state->motor_controller->set_z_accel(uval);
        } break;
        case (PACKET_SET_CHANNEL): {
            settings_set_channel(uval);
            radio_set_channel(uval);
        } break;
        case (PACKET_SET_VELOCITY_PERCENT): {
            state->motor_controller->set_velocity_percent(uval);
        } break;
        case (PACKET_SET_ACCEL_PERCENT): {
            state->motor_controller->set_accel_percent(uval);
        } break;
        case (PACKET_SET_TARGET_POSITION): {
            long position = i16_to_fixed(packet.typed_val.val);
            state->motor_controller->move_to_position(position);
        } break;
        case (PACKET_SAVE_CONFIG): {
            int max_velocity = state->motor_controller->get_velocity();
            settings_set_max_velocity(max_velocity);
            int max_accel = state->motor_controller->get_accel();
            settings_set_accel(max_accel);
        } break;
    }
}

void radio_queue_message(radio_state_t* state, radio_packet_t packet)
{
    state->buffer[state->write_index++] = packet;
    state->write_index %= RADIO_OUT_BUFFER_SIZE;
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

        // void* buffer = &packet;
        // for (int i = 0; i < sizeof(radio_packet_t); i++) {
        //     char pbuffer[16];
        //     sprintf(pbuffer, "%03d.", ((char*)buffer)[i]);
        //     Serial.print(pbuffer);
        // }
        // Serial.println();

        _process_packet(state, packet);
    }

    if (_is_radio_available() && state->read_index != state->write_index) {
        radio_packet_t* out_packet = &state->buffer[state->read_index++];
        state->read_index %= RADIO_OUT_BUFFER_SIZE;
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
    return memcmp(addr, (uint8_t*)TRANSMIT_ADDRESS, mirf_ADDR_LEN) == 0;
}
