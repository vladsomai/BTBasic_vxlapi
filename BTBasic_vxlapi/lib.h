#ifndef LIB
#define LIB

#include <string>
#include <iostream>
#include <memory>
#include <fstream>
#include <Windows.h>
#include <iomanip>
#include <ctype.h>
#include <thread>
#include <vector>
#include <filesystem>

const uint32_t RECEIVE_EVENT_SIZE = 1;   // DO NOT EDIT! Currently 1 is supported only
const uint32_t RX_QUEUE_SIZE = 4096;     // internal driver queue size in CAN events

const int8_t OK = 0;
const int8_t DRIVER_COULD_NOT_BE_OPENED = -1;
const int8_t PORT_COULD_NOT_BE_OPENED = -2;
const int8_t CHANNEL_BITRATE_COULD_NOT_BE_SET = -3;
const int8_t CHANNEL_COULD_NOT_BE_ACTIVATED = -4;
const int8_t CHANNEL_COULD_NOT_BE_DEACTIVATED = -5;
const int8_t PORT_COULD_NOT_BE_CLOSED = -6;
const int8_t DRIVER_COULD_NOT_BE_CLOSED = -7;
const int8_t FILE_COULD_NOT_BE_OPENED = -8;
const int8_t PRG_FILE_DOES_NOT_EXIST = -9;
const int8_t RX_THREAD_COULD_NOT_BE_CREATED = -10;
const int8_t NO_MESSAGE_RECEIVED_FROM_TOOLBOX_UPLOAD = -11;
const int8_t MESSAGE_NOT_SENT = -12;
const int8_t DATA_IS_INVALID = -13;
const int8_t FHOST_FILE_DOES_NOT_EXIST = -14;
const int8_t ERROR_EXECUTING_FHOST_COMMAND = -15;
const int8_t FHOST_FINISHED_WITH_ERROR = -16;
const int8_t RX_TX_ID_IS_INVALID = -17;
const int8_t TOOLBOX_FILE_DOES_NOT_EXIST = -18;

const int8_t GENERAL_ERROR = -126;

#endif // !LIB
