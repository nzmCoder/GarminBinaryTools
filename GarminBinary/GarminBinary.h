/****************************************************************************
GARMIN BINARY EXPLORER for Garmin GPS Receivers that support serial I/O.

Copyright (C) 2016-2017 Norm Moulton

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

****************************************************************************/
// GarminBinary.h : main header file for the GARMINBINARY application
//

#pragma once

#ifndef __AFXWIN_H__
#error include 'stdafx.h' before including this file for PCH
#endif

// Main Resource Symbols
#include "resource.h"
#include <cstdint>

// Due to late decoding, need space in payload for received message's
// checksum, DLE, and ETX bytes, therefore 3 extra.
enum { PAYLOAD_BYTES = 0xFF + 3 };
typedef struct
{
    uint8_t Start;
    uint8_t CmdId;
    uint8_t SizeBytes;
    uint8_t Payload[PAYLOAD_BYTES];
    uint8_t ChkSum;
    uint8_t End1;
    uint8_t End2;
} t_MSG_FORMAT;

typedef enum
{
    MSG_ACK         = 0x06,
    MSG_QUERY       = 0x0A,
    MSG_ASYNC_CMD   = 0x1C,
    MSG_PSEUD_RSP   = 0x38,
    MSG_BAUD_CMD    = 0x30,
    MSG_BAUD_RSP    = 0x31,
    MSG_PVT_RSP     = 0x33,
    MSG_LATLON_RSP  = 0x11,
    MSG_UTC_RSP     = 0x0E,
    MSG_ID_CMD      = 0xFE,  // Cmd Request ID
    MSG_ID_RSP      = 0xFF,  // Rsp ID
    MSG_ID_EXT      = 0xFD,  // Rsp ID Extended
} e_MSG_TYPE;

typedef enum
{
    QUERY_LATLON    = 0x02,
    QUERY_UTC       = 0x05,
    QUERY_PWR_OFF   = 0x08,
    QUERY_PVT_ON    = 0x31,
    QUERY_PVT_OFF   = 0x32,
    QUERY_CQCQ      = 0x3A,
} e_QUERY_TYPE;

typedef enum
{
    STATE_IDLE      = 0x00,
    STATE_START     = 0x03,
    STATE_GET_ID    = 0x05,
    STATE_GET_UTC   = 0x06,
    STATE_GET_POS   = 0x09,
    STATE_PVT_ON    = 0x0A,
    STATE_ASYNC_ON  = 0x0C,
    STATE_ASYNC_TIC = 0x0F,
    STATE_STOP_REC  = 0x11,
    STATE_GPS_OFF   = 0x12,
    STATE_FINISH    = 0x14,
    STATE_NEXT      = 0xFF,
} e_STATE_TYPE;

static const char MONTH[13][4] =
{
    "---", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const double PI = 3.1415926535897932;

/////////////////////////////////////////////////////////////////////////////
// CGarminBinaryApp:
// See GarminBinary.cpp for the implementation of this class
//

class CGarminBinaryApp : public CWinApp
{
public:
    CGarminBinaryApp();

public:
    virtual BOOL InitInstance();

    DECLARE_MESSAGE_MAP()
};
