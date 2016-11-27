#ifndef radio_h
#define radio_h

#include "serial_api.h"

#define RADIO_OUT_BUFFER_SIZE       64
#define STRING_PACKET_BUFFER_SIZE   60
#define RADIO_VERSION               01

#define RF_DEFAULT                  0b00100011  // 250kbps 0dB
#define TRANSMIT_ADDRESS            "clie1"
#define RECEIVE_ADDRESS             "serv1"


enum {
    PACKET_NONE                     = 0,
    PACKET_VERSION_GET              = 1,
    PACKET_VERSION_PRINT            = 2,
    PACKET_ROLE_GET                 = 3,
    PACKET_ROLE_PRINT               = 4,
    PACKET_MAX_SPEED_GET_NO_PRINT   = 5,
    PACKET_MAX_SPEED_GET            = 6,
    PACKET_MAX_SPEED_SET            = 7,
    PACKET_MAX_SPEED_PRINT          = 8,
    PACKET_ACCEL_GET_NO_PRINT       = 9,
    PACKET_ACCEL_GET                = 10,
    PACKET_ACCEL_SET                = 11,
    PACKET_ACCEL_PRINT              = 12,
    PACKET_CHANNEL_GET              = 13,
    PACKET_CHANNEL_SET              = 14,
    PACKET_CHANNEL_PRINT            = 15,
    PACKET_PROFILE_ID_GET           = 16,
    PACKET_PROFILE_ID_SET           = 17,
    PACKET_PROFILE_ID_PRINT         = 18,
    PACKET_PROFILE_NAME_GET         = 19,
    PACKET_PROFILE_NAME_SET         = 20,
    PACKET_PROFILE_NAME_PRINT       = 21,
    PACKET_TARGET_POSITION_GET      = 22,
    PACKET_TARGET_POSITION_SET      = 23,
    PACKET_TARGET_POSITION_PRINT    = 24,
    PACKET_SPEED_PERCENT_SET        = 25,
    PACKET_ACCEL_PERCENT_SET        = 26,
    PACKET_SAVE_CONFIG              = 27,
    PACKET_RELOAD_CONFIG            = 28,
    PACKET_PRESET_INDEX_GET         = 29,
    PACKET_PRESET_INDEX_SET         = 30,
    PACKET_PRESET_INDEX_PRINT       = 31,
    PACKET_START_STATE_GET          = 32,
    PACKET_START_STATE_SET          = 33,
    PACKET_START_STATE_PRINT        = 34,
    PACKET_RE_INIT_POSITION         = 35,
    PACKET_OK                       = 120,
};

#define PACKET_SIZE         6
#define PACKET_STRING_LEN   (PACKET_SIZE - 1)

struct empty_packet_t {
    char type;
};

struct ok_packet_t {
    char type;
    char key;
};

struct string_set_packet_t {
    char type;
    char val[PACKET_STRING_LEN];
};

struct i16_packet_t {
    char type;
    int val;
};

struct u16_packet_t {
    char type;
    unsigned int val;
};

struct i32_packet_t {
    char type;
    long val;
};

struct u32_packet_t {
    char type;
    long val;
};

struct radio_packet_t {
    // char version;
    union {
        char type;
        empty_packet_t version_get;
        string_set_packet_t version_print;
        empty_packet_t role_get;
        u16_packet_t role_print;
        empty_packet_t max_speed_get;
        u16_packet_t max_speed_set;
        u16_packet_t max_speed_print;
        empty_packet_t accel_get;
        i16_packet_t accel_set;
        i16_packet_t accel_print;
        empty_packet_t channel_get;
        i16_packet_t channel_set;
        i16_packet_t channel_print;
        empty_packet_t profile_id_get;
        u32_packet_t profile_id_set;
        u32_packet_t profile_id_print;
        empty_packet_t profile_name_get;
        string_set_packet_t profile_name_set;
        string_set_packet_t profile_name_print;
        empty_packet_t target_position_get;
        i32_packet_t target_position_set;
        i32_packet_t target_position_print;
        i16_packet_t speed_percent_set;
        i16_packet_t accel_percent_set;
        empty_packet_t save_config;
        empty_packet_t reload_config;
        empty_packet_t preset_index_get;
        i16_packet_t preset_index_set;
        i16_packet_t preset_index_print;
        i16_packet_t start_state_set;
        i16_packet_t start_state_print;
        empty_packet_t re_init_position;
        ok_packet_t ok;
    };
};

struct serial_api_state_t;

struct radio_state_t {
    radio_packet_t buffer[RADIO_OUT_BUFFER_SIZE];
    int write_index;
    int read_index;
    char string_packet_command;
    char string_packet_buffer[STRING_PACKET_BUFFER_SIZE];
    int string_packet_buffer_index;
    int version_match;
    long heartbeat_sent_timestamp;
    long heartbeat_received_timestamp;
};

#define PACKET_SEND_EMPTY(packet_type) do {\
    radio_packet_t __packet = {0};\
    __packet.type = packet_type;\
    radio_queue_message(__packet);\
} while(0)

#define PACKET_SEND(packet_type, name, value) do {\
    radio_packet_t __packet = {0};\
    __packet.name.type = packet_type;\
    __packet.name.val = value;\
    radio_queue_message(__packet);\
} while(0)

#define PACKET_SEND_STRING(packet_type, name, value) do {\
    radio_packet_t __packet = {0};\
    __packet.name.type = packet_type;\
    strncpy(__packet.name.val, value, PACKET_STRING_LEN);\
    radio_queue_message(__packet);\
} while(0)

void radio_init();
void radio_run();
void radio_queue_message(radio_packet_t packet);
void radio_set_channel(int channel);
bool radio_is_alive();

#endif //radio_h
