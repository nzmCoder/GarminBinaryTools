// GarminBinary.h : main header file for the GARMINBINARY application
//

#if !defined(AFX_GARMINBINARY_H__6D6FB20D_647B_4CFB_AF64_29BF5015A110__INCLUDED_)
#define AFX_GARMINBINARY_H__6D6FB20D_647B_4CFB_AF64_29BF5015A110__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
#error include 'stdafx.h' before including this file for PCH
#endif

// Main Resource Symbols
#include "resource.h"

// Define size specific types.
typedef unsigned char   t_UINT8;
typedef unsigned short  t_UINT16;
typedef unsigned int    t_UINT32;

// Due to late decoding, need space in payload for received message's
// checksum, DLE, and ETX bytes, therefore 3 extra.
enum { PAYLOAD_BYTES = 0xFF + 3 };
typedef struct
{
    t_UINT8 Start;
    t_UINT8 CmdId;
    t_UINT8 SizeBytes;
    t_UINT8 Payload[PAYLOAD_BYTES];
    t_UINT8 ChkSum;
    t_UINT8 End1;
    t_UINT8 End2;
} t_MSG_FORMAT;

typedef enum
{
    MSG_ACK     = 0x06,
    MSG_ASYNC   = 0x1C,
    MSG_PSEUD   = 0x38,
    MSG_REQUEST = 0x0A,

    MSG_ID_CMD  = 0xFE,  // Cmd Request ID
    MSG_ID_RSP  = 0xFF,  // Rsp ID
    MSG_ID_EXT  = 0xFD,  // Rsp ID Extended

} e_MSG_TYPE;


/////////////////////////////////////////////////////////////////////////////
// CGarminBinaryApp:
// See GarminBinary.cpp for the implementation of this class
//

class CGarminBinaryApp : public CWinApp
{
public:
    CGarminBinaryApp();

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CGarminBinaryApp)
public:
    virtual BOOL InitInstance();
    //}}AFX_VIRTUAL

// Implementation

    //{{AFX_MSG(CGarminBinaryApp)
    // NOTE - the ClassWizard will add and remove member functions here.
    //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GARMINBINARY_H__6D6FB20D_647B_4CFB_AF64_29BF5015A110__INCLUDED_)
