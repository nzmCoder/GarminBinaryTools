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

/****************************************************************************

1.12   17 Jun 2017, General cleanup and conversion to VS2017.

1.11   27 Jun 2016, Saves Garmin binary observation file in G12 format.
       Controls GAR2RNX to create Rinex file.

1.10   19 Jun 2016, Serial communications works well.  No framing errors
       when tested on eTrex Vista, GPS V, and 60CSx.

1.00   30 May 2016, First version

****************************************************************************/

// GarminBinaryDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GarminBinary.h"
#include "GarminBinaryDlg.h"
#include "Serial.h"
#include "Profile.h"
#include "Windows.h"
#include "Mmsystem.h"
#include <map>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// This turns on only async messages 36, 37, 38
#define _ASYNC_MASK 0x20;

// Millisecond timer period for fetching serial bytes
#define _RECV_SERIAL_TIMER_MSECS 1

// Define an ID for the serial port timer
#define _RECV_SERIAL_TIMER 1

// Define an ID for the state machine timer
#define _STATE_TIMER 2

// Millisecond timer period for commands and responses
#define _STATE_TIMER_CMDS_MSECS 1000

// Define the hi/lo baud rates supported
static const CString s_StrLoBaud = "9600";
static const CString s_StrHiBaud = "57600";

// Serial driver
CSerial m_Serial;

// XML-like storage of persistent data
CProfile m_Profile("GarminBinary.XML");

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CAboutDlg is the About Box.
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CAboutDlg : public CDialog
{
public:
    CAboutDlg();

// Dialog Data
    enum { IDD = IDD_ABOUTBOX };
    CString m_strAboutText;
    CString m_strBuildDate;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
    m_strAboutText = _T("");
    m_strBuildDate = _T("");
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_STAT_TEXT, m_strAboutText);
    DDX_Text(pDX, IDC_STAT_BUILD_DATE, m_strBuildDate);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    m_strBuildDate = _T("Version Build:  ");
    m_strBuildDate += _T(__DATE__);
    m_strBuildDate += _T(", ");
    m_strBuildDate += _T(__TIME__);

    m_strAboutText =
        "GARMIN BINARY EXPLORER for Garmin GPS Receivers that support serial I/O.\r\n"\
        "Please let me know your experience. Contact normmoulton at yahoo dot com.\r\n"\
        "Copyright (C) 2016-2017 Norm Moulton\r\n"\
        "\r\n"\
        "This program is free software: you can redistribute it and/or modify "\
        "it under the terms of the GNU General Public License as published by "\
        "the Free Software Foundation, either version 3 of the License, or "\
        "(at your option) any later version.\r\n"\
        "\r\n"\
        "This program is distributed in the hope that it will be useful, "\
        "but WITHOUT ANY WARRANTY; without even the implied warranty of "\
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "\
        "GNU General Public License for more details.\r\n"\
        "\r\n"\
        "You should have received a copy of the GNU General Public License "\
        "along with this program.  If not, see <http://www.gnu.org/licenses/>.";

    UpdateData(FALSE);

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CGarminBinaryDlg is the main window for the dialog app.
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CGarminBinaryDlg::CGarminBinaryDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CGarminBinaryDlg::IDD, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CGarminBinaryDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_STAT_BW, m_statBandwidth);
    DDX_Control(pDX, IDC_EDIT_REC_TIME, m_editRecTime);
    DDX_Control(pDX, IDC_STAT_TICK, m_statTick);
    DDX_Control(pDX, IDC_STAT_EPE, m_statEPE);
    DDX_Control(pDX, IDC_CHK_PWR_OFF, m_chkPwrOff);
    DDX_Control(pDX, IDC_STAT_WTRLINE, m_statWaterLn);
    DDX_Control(pDX, IDC_BTN_BAUD_LOCAL, m_btnBaudLocal);
    DDX_Control(pDX, IDC_STAT_GPSFIX, m_statGpsFix);
    DDX_Control(pDX, IDC_STAT_LATLON, m_statLatLon);
    DDX_Control(pDX, IDC_STAT_UTC, m_statUTC);
    DDX_Control(pDX, IDC_STAT_ERR_FRMS, m_statErrFrames);
    DDX_Control(pDX, IDC_STAT_BAUD, m_statBaud);
    DDX_Control(pDX, IDC_STAT_MSG_ID_SEEN, m_statMsgIdSeen);
    DDX_Control(pDX, IDC_STAT_GPS_ID_RSP, m_statGpsId);
    DDX_Control(pDX, IDC_EDIT_MSGS, m_editMsgs);
    DDX_Control(pDX, IDC_CMBO_PORT, m_cmboPort);
    DDX_Control(pDX, IDC_BTN_BAUD_UP, m_btnBaudUp);
    DDX_Control(pDX, IDC_BTN_GET_ID, m_btnGetId);
    DDX_Control(pDX, IDC_BTN_REC_ON, m_btnRecOn);
    DDX_Control(pDX, IDC_BTN_REC_OFF, m_btnRecOff);
    DDX_Control(pDX, IDC_BTN_PVT_ON, m_btnPvtOn);
    DDX_Control(pDX, IDC_BTN_PVT_OFF, m_btnPvtOff);
    DDX_Control(pDX, IDC_BTN_GET_UTC, m_btnGetUtc);
    DDX_Control(pDX, IDC_BTN_GET_LATLON, m_btnGetLatLon);
    DDX_Control(pDX, IDC_BTN_PWR_OFF, m_btnPwrOff);
    DDX_Control(pDX, IDC_BTN_BAUD_DN, m_btnBaudDn);
    DDX_Control(pDX, IDC_BTN_ASYNC_ON, m_btnAsyncOn);
    DDX_Control(pDX, IDC_BTN_ASYNC_OFF, m_btnAsyncOff);
    DDX_Control(pDX, IDC_STAT_TICK_LBL, m_statTickLabel);
    DDX_Control(pDX, IDC_BTN_CLEAR, m_btnClearStats);
    DDX_Control(pDX, IDC_STATIC_EPE_LBL, m_statEpeLabel);
    DDX_Control(pDX, IDC_STATIC_FRMERRS_LBL, m_statFrameErrsLabel);
    DDX_Control(pDX, IDC_STATIC_MSGS_LBL, m_statMsgsSeenLabel);
    DDX_Control(pDX, IDC_STATIC_REC_LBL, m_statRecTimeLabel);
    DDX_Control(pDX, IDC_STATIC_WL_LBL, m_statWaterLineLabel);
    DDX_Control(pDX, IDC_STATIC_FIX_LBL, m_statFixQuality);
}

BEGIN_MESSAGE_MAP(CGarminBinaryDlg, CDialog)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_TIMER()
    ON_CBN_DROPDOWN(IDC_CMBO_PORT, OnDropdownCmboPort)
    ON_CBN_CLOSEUP(IDC_CMBO_PORT, OnCloseupCmboPort)
    ON_BN_CLICKED(IDC_BTN_GET_ID, OnBtnGetId)
    ON_BN_CLICKED(IDC_BTN_GET_UTC, OnBtnGetUTC)
    ON_BN_CLICKED(IDC_BTN_START_ASYNC, OnBtnAsyncOn)
    ON_BN_CLICKED(IDC_BTN_STOP_ASYNC, OnBtnAsyncOff)
    ON_BN_CLICKED(IDC_BTN_POS_ON, OnBtnPvtOn)
    ON_BN_CLICKED(IDC_BTN_POS_OFF, OnBtnPvtOff)
    ON_BN_CLICKED(IDC_BTN_LOG_ON, OnBtnRecOn)
    ON_BN_CLICKED(IDC_BTN_LOG_OFF, OnBtnRecOff)
    ON_BN_CLICKED(IDC_BTN_GET_LATLON, OnBtnGetLatLon)
    ON_BN_CLICKED(IDC_BTN_BAUD_UP, OnBtnBaudUp)
    ON_BN_CLICKED(IDC_BTN_BAUD_DN, OnBtnBaudDn)
    ON_BN_CLICKED(IDC_BTN_BAUD_LOCAL, OnBtnBaudLocal)
    ON_BN_CLICKED(IDC_BTN_PWR_OFF, OnBtnPwrOff)
    ON_BN_CLICKED(IDC_BTN_CLEAR, OnBtnClear)
    ON_BN_CLICKED(IDC_BTN_ABOUT, OnBtnAbout)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGarminBinaryDlg Windows boilerplate functions.
/////////////////////////////////////////////////////////////////////////////

void CGarminBinaryDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialog::OnSysCommand(nID, lParam);
    }
}

void CGarminBinaryDlg::OnPaint()
{
    if(IsIconic())
    {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialog::OnPaint();
    }
}

HCURSOR CGarminBinaryDlg::OnQueryDragIcon()
{
    return (HCURSOR) m_hIcon;
}

/////////////////////////////////////////////////////////////////////////////
///<summary>From the book: "Programming Windows MFC", Jeff Prosise, 1995</summary>
BOOL CGarminBinaryDlg::PeekAndPump()
{
    MSG msg;
    while(::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
    {
        if(!AfxGetApp()->PumpMessage())
        {
            ::PostQuitMessage(0);
            return FALSE;
        }
    }
    LONG Idle = 0;
    while(AfxGetApp()->OnIdle(Idle++));

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CGarminBinaryDlg Windows message handler functions.
/////////////////////////////////////////////////////////////////////////////

///<summary>GUI button message handler.</summary>
void CGarminBinaryDlg::OnOK()
{
    // Don't let the OK button exit the dialog application.
    // CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
///<summary>GUI button message handler.</summary>
void CGarminBinaryDlg::OnCancel()
{
    // Before we go, undo the increased Windows timer resolution.
    timeEndPeriod(1);

    CDialog::OnCancel();
}

/////////////////////////////////////////////////////////////////////////////
///<summary>GUI button message handler.</summary>
void CGarminBinaryDlg::OnBtnAbout()
{
    CAboutDlg dlg;
    dlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Event handler.</summary>
BOOL CGarminBinaryDlg::OnInitDialog()
{
    // Begin boilerplate Windows initialization.
    CDialog::OnInitDialog();

    // Add "About..." menu item to system menu.

    // IDM_ABOUTBOX must be in the system command range.
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if(pSysMenu != NULL)
    {
        CString strAboutMenu;
        (void)strAboutMenu.LoadString(IDS_ABOUTBOX);
        if(!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // Set the big icon. This auto scales to small also.
    SetIcon(m_hIcon, TRUE);

    // End of boilerplate Windows initialization.

    // Try to create the ToolTip control.
    if(m_ToolTip.Create(this))
    {
        // Add tool tips to certain controls.
        m_ToolTip.AddTool(&m_btnBaudUp, "Go to high baud rate, both GPS and PC.");
        m_ToolTip.AddTool(&m_btnBaudDn, "Go to low baud rate, both GPS and PC.");
        m_ToolTip.AddTool(&m_btnBaudLocal, "Change only PC baud rate to resynch with GPS rate.");

        m_ToolTip.AddTool(&m_btnGetId, "Query GPS to report name and firmware version.");
        m_ToolTip.AddTool(&m_btnGetUtc, "Query GPS to report current time in UTC.");
        m_ToolTip.AddTool(&m_btnGetLatLon, "Query GPS to report lattitude and longitude.");

        m_ToolTip.AddTool(&m_btnRecOn, "Start data recording and RINEX file creation for specified minutes.");
        m_ToolTip.AddTool(&m_btnRecOff, "Stop data recording now. RINEX file will be valid but shorter.");
        m_ToolTip.AddTool(&m_chkPwrOff, "Power off GPS automatically when recording is done.");
        m_ToolTip.AddTool(&m_btnPwrOff, "Power off GPS right now.");
        m_ToolTip.AddTool(&m_statRecTimeLabel, "Minutes to record for RINEX file. Value can be decimal, e.g. \"20.1\".");
        m_ToolTip.AddTool(&m_statTickLabel, "While recording, counts down seconds remaining.");

        m_ToolTip.AddTool(&m_btnPvtOn, "Turn on once-per-second GPS reporting of Position, Velocity and Time (PVT).");
        m_ToolTip.AddTool(&m_btnPvtOff, "Turn off GPS reporting of Position, Velocity and Time (PVT).");

        m_ToolTip.AddTool(&m_btnAsyncOn, "Turn on all asynchronous messages that GPS can report. Sent only to console.");
        m_ToolTip.AddTool(&m_btnAsyncOff, "Turn off all asynchronous messages from GPS.");

        m_ToolTip.AddTool(&m_statEpeLabel, "GPS calculated Estimated Position Error, (EPE).");
        m_ToolTip.AddTool(&m_statFixQuality, "GPS solution quality. 1 = poor, 3 and 5 = best.");

        m_ToolTip.AddTool(&m_btnClearStats, "Reset Msgs Seen, Frame Errors, and Water Line fields.");
        m_ToolTip.AddTool(&m_statFrameErrsLabel, "Number of messages from GPS rejected due to bad checksum. (Should be zero.)");
        m_ToolTip.AddTool(&m_statMsgsSeenLabel, "Accumulates the set of binary messages (command IDs) received from GPS.");
        m_ToolTip.AddTool(&m_statWaterLineLabel, "Maximum number of chars ever waiting in the RS-232 receive buffer. (Smaller is good).");

        m_ToolTip.Activate(TRUE);
    }

    // Required initialization of the profile object before use.
    m_Profile.Init();

    // Get and set the path to the GAR2RNX component, so this tag gets put in XML
    CString strRinexPath = m_Profile.GetProfileStr("MainConfig", "Gar2RnxPath" , "C:\\Projects\\GarminTools\\Bin\\");
    if(strRinexPath.Right(1) != "\\") strRinexPath += "\\";  // enforce path ends w/ backslash
    m_Profile.WriteProfileStr("MainConfig", "Gar2RnxPath" , strRinexPath);

    // Get and set the options to GAR2RNX component, so this tag gets put in XML
    CString strRinexOptions = m_Profile.GetProfileStr("MainConfig", "Gar2RnxOptions" , "-etrex");
    m_Profile.WriteProfileStr("MainConfig", "Gar2RnxOptions" , strRinexOptions);

    // Use data from profile to set sticky fields.
    m_strSerialPort = m_Profile.GetProfileStr("MainConfig", "ComPort", "None");
    m_cmboPort.SelectString(-1, m_strSerialPort);

    // Initialize the serial port.
    SetupPort();
    m_pRcv = (char*)&m_RecvMsg;
    m_lastRecv = 0;

    // Set default recording time.
    CString str;
    str = m_Profile.GetProfileStr("MainConfig", "RecTime", "30.0");
    m_editRecTime.SetWindowText(str);

    // Format the protocol message window with a fixed width font.
    m_Font.CreateFont(12, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                      DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS,
                      DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Fixedsys");
    m_editMsgs.SetFont(&m_Font, TRUE);

    // Attempt to increase the systemwide Windows timer resolution.
    if(timeBeginPeriod(_RECV_SERIAL_TIMER_MSECS) != TIMERR_NOERROR)
    {
        AfxMessageBox("Failed to set a faster Windows timer.\r\n"\
                      "You may see more framing errors.");
    }

    // Start the serial receive polling timer.
    SetTimer(_RECV_SERIAL_TIMER, _RECV_SERIAL_TIMER_MSECS, NULL);

    memset(mMsgsSeen, 0, sizeof(mMsgsSeen));
    mErrFrames = 0;
    mHighWater = 0;

    m_bIsLogging = false;
    G12State(STATE_IDLE);

    // Set a custom icon for the Baud Sync button
    m_hIconBtn = AfxGetApp()->LoadIcon(IDI_ICON_BAUD);
    m_btnBaudLocal.ModifyStyle(0, BS_ICON);
    m_btnBaudLocal.SetIcon(m_hIconBtn);

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Event handler.</summary>
void CGarminBinaryDlg::OnTimer(UINT nIDEvent)
{
    if(nIDEvent == _RECV_SERIAL_TIMER)
    {
        RecvMsg();
    }
    else if(nIDEvent == _STATE_TIMER)
    {
        UpdateBandwidth();
        G12State(STATE_NEXT);
    }

    CDialog::OnTimer(nIDEvent);
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Event handler.</summary>
BOOL CGarminBinaryDlg::PreTranslateMessage(MSG* pMsg)
{
    m_ToolTip.RelayEvent(pMsg);

    return CDialog::PreTranslateMessage(pMsg);
}

/////////////////////////////////////////////////////////////////////////////
///<summary>GUI event handler.</summary>
void CGarminBinaryDlg::OnDropdownCmboPort()
{
    // Populate the drop-down with only available com ports.

    // Remember last selected port.
    CString strCurrSelection;
    m_cmboPort.GetWindowTextA(strCurrSelection);

    // Erase all existing port choices.
    m_cmboPort.ResetContent();

    // Use QueryDosDevice to return all the com ports,
    // then sort them into correct order using a map.
    static const int BUFF_SIZE = 0xFFFF;
    char* szDevices = new char[BUFF_SIZE];
    unsigned long dwChars = QueryDosDevice(0, szDevices, BUFF_SIZE);
    char* ptr = szDevices;
    std::map<int, std::string> comPortMap;

    while(dwChars)
    {
        std::string strTest(ptr);

        int findPos = strTest.find("COM");

        if(findPos != std::string::npos)
        {
            // Isolate the number after "COM".
            strTest.erase(findPos, 3);

            int port = atoi(strTest.c_str());
            if(port != 0)
            {
                // Found a COM port, add it to map.
                comPortMap[port] = strTest;
            }
        }

        char* temp_ptr = strchr(ptr, 0);
        dwChars -= (DWORD)((temp_ptr - ptr) / sizeof(char) + 1);
        ptr = temp_ptr + 1;
    }

    // If we found at least one COM port . . .
    if(!comPortMap.empty())
    {
        for(std::map<int, std::string>::iterator comItr = comPortMap.begin();
                comItr != comPortMap.end(); ++comItr)
        {
            std::string str = comItr->second;
            str = "COM" + str;
            m_cmboPort.AddString(str.c_str());
        }
    }

    delete [] szDevices;

    // Try to restore the current selection choice.
    m_cmboPort.SetCurSel(m_cmboPort.FindStringExact(0, strCurrSelection));
}

/////////////////////////////////////////////////////////////////////////////
///<summary>GUI event handler.</summary>
void CGarminBinaryDlg::OnCloseupCmboPort()
{
    SetupPort();
    m_Profile.WriteProfileStr("MainConfig", "ComPort", m_strSerialPort);
}

/////////////////////////////////////////////////////////////////////////////
///<summary>GUI button message handler.</summary>
void CGarminBinaryDlg::OnBtnClear()
{
    // clear the window of received messages
    m_editMsgs.SetWindowText("");

    // clear the list of seen messages
    memset(mMsgsSeen, 0, sizeof(mMsgsSeen));
    m_statMsgIdSeen.SetWindowText("");

    mErrFrames = 0;
    UpdateErrSeen();

    mHighWater = 0;
    UpdateHighWater();
}

/////////////////////////////////////////////////////////////////////////////
///<summary>GUI button message handler.</summary>
void CGarminBinaryDlg::OnBtnGetId()
{
    ClearMsgBuff(&m_SendMsg);

    m_SendMsg.Start     = 0x10;
    m_SendMsg.CmdId     = MSG_ID_CMD;
    m_SendMsg.SizeBytes = 0x00;
    // No payload
    m_SendMsg.ChkSum    = CalcChksum(&m_SendMsg);
    m_SendMsg.End1      = 0x10;
    m_SendMsg.End2      = 0x03;

    SendMsg();
}

/////////////////////////////////////////////////////////////////////////////
///<summary>GUI button message handler.</summary>
void CGarminBinaryDlg::OnBtnAsyncOn()
{
    ClearMsgBuff(&m_SendMsg);

    m_SendMsg.Start      = 0x10;
    m_SendMsg.CmdId      = MSG_ASYNC_CMD;
    m_SendMsg.SizeBytes  = 0x02;
    m_SendMsg.Payload[0] = 0xFF;
    m_SendMsg.Payload[1] = 0xFF;
    m_SendMsg.ChkSum     = CalcChksum(&m_SendMsg);
    m_SendMsg.End1       = 0x10;
    m_SendMsg.End2       = 0x03;

    SendMsg();
}

/////////////////////////////////////////////////////////////////////////////
///<summary>GUI button message handler.</summary>
void CGarminBinaryDlg::AsyncMaskOn()
{
    ClearMsgBuff(&m_SendMsg);

    m_SendMsg.Start      = 0x10;
    m_SendMsg.CmdId      = MSG_ASYNC_CMD;
    m_SendMsg.SizeBytes  = 0x02;
    m_SendMsg.Payload[0] = _ASYNC_MASK;
    m_SendMsg.Payload[1] = 0;
    m_SendMsg.ChkSum     = CalcChksum(&m_SendMsg);
    m_SendMsg.End1       = 0x10;
    m_SendMsg.End2       = 0x03;

    SendMsg();
}

/////////////////////////////////////////////////////////////////////////////
///<summary>GUI button message handler.</summary>
void CGarminBinaryDlg::OnBtnAsyncOff()
{
    ClearMsgBuff(&m_SendMsg);

    m_SendMsg.Start      = 0x10;
    m_SendMsg.CmdId      = MSG_ASYNC_CMD;
    m_SendMsg.SizeBytes  = 0x02;
    m_SendMsg.Payload[0] = 0x00;
    m_SendMsg.Payload[1] = 0x00;
    m_SendMsg.ChkSum     = CalcChksum(&m_SendMsg);
    m_SendMsg.End1       = 0x10;
    m_SendMsg.End2       = 0x03;

    SendMsg();
}

/////////////////////////////////////////////////////////////////////////////
///<summary>GUI button message handler.</summary>
void CGarminBinaryDlg::OnBtnPvtOn()
{
    ClearMsgBuff(&m_SendMsg);

    m_SendMsg.Start      = 0x10;
    m_SendMsg.CmdId      = MSG_QUERY;
    m_SendMsg.SizeBytes  = 0x02;
    m_SendMsg.Payload[0] = QUERY_PVT_ON;
    m_SendMsg.Payload[1] = 0x00;
    m_SendMsg.ChkSum     = CalcChksum(&m_SendMsg);
    m_SendMsg.End1       = 0x10;
    m_SendMsg.End2       = 0x03;

    SendMsg();
}

/////////////////////////////////////////////////////////////////////////////
///<summary>GUI button message handler.</summary>
void CGarminBinaryDlg::OnBtnPvtOff()
{
    ClearMsgBuff(&m_SendMsg);

    m_SendMsg.Start      = 0x10;
    m_SendMsg.CmdId      = MSG_QUERY;
    m_SendMsg.SizeBytes  = 0x02;
    m_SendMsg.Payload[0] = QUERY_PVT_OFF;
    m_SendMsg.Payload[1] = 0x00;
    m_SendMsg.ChkSum     = CalcChksum(&m_SendMsg);
    m_SendMsg.End1       = 0x10;
    m_SendMsg.End2       = 0x03;

    SendMsg();
}

/////////////////////////////////////////////////////////////////////////////
///<summary>GUI button message handler.</summary>
void CGarminBinaryDlg::OnBtnPwrOff()
{
    ClearMsgBuff(&m_SendMsg);

    m_SendMsg.Start      = 0x10;
    m_SendMsg.CmdId      = MSG_QUERY;
    m_SendMsg.SizeBytes  = 0x02;
    m_SendMsg.Payload[0] = QUERY_PWR_OFF;
    m_SendMsg.Payload[1] = 0x00;
    m_SendMsg.ChkSum     = CalcChksum(&m_SendMsg);
    m_SendMsg.End1       = 0x10;
    m_SendMsg.End2       = 0x03;

    SendMsg();
}

/////////////////////////////////////////////////////////////////////////////
///<summary>GUI button message handler.</summary>
void CGarminBinaryDlg::OnBtnRecOn()
{
    if(IsAtLoBaud())
    {
        CString str;
        str =
            "For best recording results it is strongly\n"\
            "recommended to step up to high baud rate.\n"\
            "OK to record at low baud or Cancel?";

        if(IDOK != AfxMessageBox(str, MB_ICONEXCLAMATION | MB_OKCANCEL))
        {
            // Cancel the process.
            return;
        }
    }

    // Get value from GUI
    CString strValue;
    m_editRecTime.GetWindowText(strValue);

    // Update sticky setting for this.
    m_Profile.WriteProfileStr("MainConfig", "RecTime", strValue);

    // Parse recording time, add a few seconds for state machine overhead.
    mTickDown = (unsigned int)((atof(strValue) * 60) + 10); // min to sec
    if(mTickDown == 0 || mTickDown > 86400)
    {
        mTickDown = 86400;
    }

    CString str;
    str.Format(
        "This will start recording a %d second duration Garmin\r\n"\
        "binary format observation file. At the end of recording,\r\n"\
        "a Rinex format observation file will be created if the\r\n"\
        "location of the GAR2RNX tool is properly configured.", mTickDown);

    if(IDOK != AfxMessageBox(str, MB_ICONINFORMATION | MB_OKCANCEL))
    {
        // Cancel the process.
        return;
    }

    // Display "Save As" Dialog
    CFileDialog dlg(FALSE, NULL, "",
                    OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
                    "All Files (*.*)|*.*||");
    dlg.m_ofn.lpstrTitle = "Save Binary Observation file";
    CString strFile = GetGarminBinaryFilename();
    strcpy(dlg.m_ofn.lpstrFile, strFile.GetBuffer(0));

    if(dlg.DoModal() == IDOK)
    {
        m_strFileNameG12 = dlg.GetPathName();

        if(!m_OutFile.Open(m_strFileNameG12, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))
        {
            CString str;
            str.Format("Cannot write file: %s", m_strFileNameG12.GetString());
            AfxMessageBox(str);

            // Cancel the process.
            return;
        }

        // Set the state machine to start the process.
        G12State(STATE_START);
        m_bIsLogging = true;

        CString strValue;
        strValue.Format("%d", mTickDown);
        m_statTick.SetWindowText(strValue);
    }
}

/////////////////////////////////////////////////////////////////////////////
///<summary>GUI button message handler.</summary>
void CGarminBinaryDlg::OnBtnRecOff()
{

    // Doesn't make sense unless recording.
    if(mG12State != STATE_IDLE)
    {
        // If user manually terminates, don't power off GPS.
        m_chkPwrOff.SetCheck(0);

        // Skip any remaining countdown time.
        G12State(STATE_STOP_REC);
    }
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Called by timer, update GUI tick countdown,
///and if detect reaching zero, stop the state machine.</summary>
void CGarminBinaryDlg::TickDown()
{
    // Subtract the number of seconds represented by each tick.
    mTickDown -= (_STATE_TIMER_CMDS_MSECS / 1000);

    CString strValue;
    strValue.Format("%d", mTickDown);
    m_statTick.SetWindowText(strValue);

    if(mTickDown == 0)
    {
        G12State(STATE_STOP_REC);
    }
}

/////////////////////////////////////////////////////////////////////////////
///<summary>GUI button message handler.</summary>
void CGarminBinaryDlg::OnBtnGetLatLon()
{
    ClearMsgBuff(&m_SendMsg);

    m_SendMsg.Start      = 0x10;
    m_SendMsg.CmdId      = MSG_QUERY;
    m_SendMsg.SizeBytes  = 0x02;
    m_SendMsg.Payload[0] = QUERY_LATLON;
    m_SendMsg.Payload[1] = 0x00;
    m_SendMsg.ChkSum     = CalcChksum(&m_SendMsg);
    m_SendMsg.End1       = 0x10;
    m_SendMsg.End2       = 0x03;

    SendMsg();
}

/////////////////////////////////////////////////////////////////////////////
///<summary>GUI button message handler.</summary>
void CGarminBinaryDlg::OnBtnGetUTC()
{
    ClearMsgBuff(&m_SendMsg);

    m_SendMsg.Start      = 0x10;
    m_SendMsg.CmdId      = MSG_QUERY;
    m_SendMsg.SizeBytes  = 0x02;
    m_SendMsg.Payload[0] = QUERY_UTC;
    m_SendMsg.Payload[1] = 0x00;
    m_SendMsg.ChkSum     = CalcChksum(&m_SendMsg);
    m_SendMsg.End1       = 0x10;
    m_SendMsg.End2       = 0x03;

    SendMsg();
}

/////////////////////////////////////////////////////////////////////////////
// CGarminBinaryDlg functions.
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
///<summary>Send an ACK message to GPS.</summary>
void CGarminBinaryDlg::SendAck()
{
    ClearMsgBuff(&m_SendMsg);

    m_SendMsg.Start      = 0x10;
    m_SendMsg.CmdId      = MSG_ACK;
    m_SendMsg.SizeBytes  = 0x02;
    m_SendMsg.Payload[0] = m_lastRecv;  // ACK the last received msg
    m_SendMsg.Payload[1] = 0x00;
    m_SendMsg.ChkSum     = CalcChksum(&m_SendMsg);
    m_SendMsg.End1       = 0x10;
    m_SendMsg.End2       = 0x03;

    SendMsg();
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Opens RS-232 serial port.</summary>
void CGarminBinaryDlg::SetupPort()
{
    // First make sure the port starts off as closed.
    // This does no harm if it was already closed.
    m_Serial.Close();

    // Get user selected port string value. E.g. "COM1"
    m_cmboPort.GetWindowText(m_strSerialPort);

    // The default is "None" before selecting anything.
    if(m_strSerialPort != "None")
    {
        // Change e.g. "COM1" to -> "\\\\.\\COM1"
        // This format is "fully qualified" and always works.
        CString str;
        str.Format("\\\\.\\%s", m_strSerialPort.GetString());

        // Open the port and suggest to the driver to use large buffers.
        // But the actual buffer size is constrained by the hardware.
        if(0 != m_Serial.Open(str, 4096, 4096))
        {
            // Report the error to the user.
            str = "Unable to open port " + m_strSerialPort;
            AfxMessageBox(str, MB_ICONSTOP);
        }
        else
        {
            // Port opened correctly. Now set attributes.
            m_Serial.Setup(CSerial::EBaud9600, CSerial::EData8, CSerial::EParNone, CSerial::EStop1);
            m_Serial.SetupHandshaking(CSerial::EHandshakeOff);
            m_Serial.SetupReadTimeouts(CSerial::EReadTimeoutNonblocking);

            // Clear out any garbage that might be in the buffer.
            m_Serial.Purge();
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Sends an outgoing message to GPS.</summary>
void CGarminBinaryDlg::SendMsg()
{
    AddToDisplay("TX: ", &m_SendMsg);

    // This does not do DLE stuffing because none of the commands
    // we send need doubled DLEs.

    // Send header bytes.
    m_Serial.Write(&m_SendMsg.Start, 1);
    m_Serial.Write(&m_SendMsg.CmdId, 1);
    m_Serial.Write(&m_SendMsg.SizeBytes, 1);

    // Send variable length payload bytes.
    for(int i = 0; i < m_SendMsg.SizeBytes; ++i)
    {
        m_Serial.Write(&m_SendMsg.Payload[i], 1);
    }

    // Send trailer bytes.
    m_Serial.Write(&m_SendMsg.ChkSum, 1);
    m_Serial.Write(&m_SendMsg.End1, 1);
    m_Serial.Write(&m_SendMsg.End2, 1);
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Reads all available input bytes from GPS,
/// generally just a partial message.</summary>
void CGarminBinaryDlg::RecvMsg()
{
    DWORD bytesRead = 0;
    uint8_t byte = 0;
    int nTime = 0;
    static uint8_t sLastbyte = 0;
    unsigned int nNum = 0;

    // See if there is a byte to read.
    m_Serial.Read(&byte, 1, &bytesRead);

    // If we read any bytes, keep reading until buffer is empty.
    while(bytesRead)
    {
        // incr number bytes seen in this grouping
        ++nNum;

        // This part finds DLEs and end of frames.
        if(sLastbyte == 0x10 && byte == 0x10)
        {
            // A second DLE was found.

            // Set sLastbyte to some neutral value.
            sLastbyte = 0;
        }
        else if(sLastbyte == 0x10 && byte == 0x03)
        {
            // It is the end of frame.

            // Call helper function.
            ProcessFrame();

            // Remember sLastbyte.
            sLastbyte = byte;
        }
        else
        {
            // It is a normal mid-frame byte.

            // Add byte, then advance pointer.
            *m_pRcv++ = byte;

            // Do not allow the pointer to overrun buffer size
            if(m_pRcv >= ((char*)&m_RecvMsg) + sizeof(m_RecvMsg))
            {
                // This message has an error; no end of frame was seen.
                // Reset the pointer to the start of the buffer.
                // This packet won't be valid, but the error will be
                // handled as usual in the ProcessFrame function.
                m_pRcv = (char*)&m_RecvMsg;
            }

            // Remember sLastbyte.
            sLastbyte = byte;
        }

        m_Serial.Read(&byte, 1, &bytesRead);
    }

    // Check if we reached a new high water mark.
    if(nNum > mHighWater)
    {
        mHighWater = nNum;
        UpdateHighWater();
    }
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Calculate and return the Garmin checksum
/// for the current message.</summary>
uint8_t CGarminBinaryDlg::CalcChksum(t_MSG_FORMAT *pMsg)
{
    // Chksum calculated over CmdId, Size, and payload only.

    // Sum up the byte values.
    uint8_t val = pMsg->CmdId;
    val += pMsg->SizeBytes;

    // Add sum of payload bytes, if any.
    for(int i = 0; i < pMsg->SizeBytes; ++i)
    {
        val += pMsg->Payload[i];
    }

    // The chksum is the arithmetic negative.
    return -(val);
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Zero out the message buffer.</summary>
void CGarminBinaryDlg::ClearMsgBuff(t_MSG_FORMAT* pMsg)
{
    memset(pMsg, 0, sizeof(t_MSG_FORMAT));
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Display a message box of the message buffer.</summary>
void CGarminBinaryDlg::DisplayMsg(t_MSG_FORMAT *pMsg)
{
    CString str = DecodeMsgBuff(pMsg);

    AfxMessageBox(str);
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Convert message buffer to human readable
/// hexadecimal string.</summary>
CString CGarminBinaryDlg::DecodeMsgBuff(t_MSG_FORMAT *pMsg)
{
    CString str;
    CString strFull;

    str.Format("%02X ", pMsg->Start);
    strFull += str;
    str.Format("%02X ", pMsg->CmdId);
    strFull += str;
    str.Format("%02X ", pMsg->SizeBytes);
    strFull += str;

    for(int i = 0; i < pMsg->SizeBytes; ++i)
    {
        str.Format("%02X ", pMsg->Payload[i]);
        strFull += str;
    }

    str.Format("%02X ", pMsg->ChkSum);
    strFull += str;
    str.Format("%02X ", pMsg->End1);
    strFull += str;
    str.Format("%02X ", pMsg->End2);
    strFull += str;

    return strFull;
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Convert message buffer containing UTC time to human readable
/// string.</summary>
CString CGarminBinaryDlg::DecodeUTC()
{
    CString strValue;

#pragma pack(1)
    typedef struct
    {
        uint8_t  month;
        uint8_t  day;
        uint16_t year;
        uint8_t  hour;
        uint8_t  unused;
        uint8_t  minute;
        uint8_t  second;
    } t_GPS_UTC;

    // Recast the payload to the structure format.
    t_GPS_UTC* pUTC = (t_GPS_UTC*)m_RecvMsg.Payload;

    // Construct a CTime object for current time.
    CTime time = CTime(
                     pUTC->year,
                     pUTC->month,
                     pUTC->day,
                     pUTC->hour,
                     pUTC->minute,
                     pUTC->second);

    // Format like: Sun 19-Jun-2016 07:05:24
    strValue = time.Format("%a %d-%b-%Y %H:%M:%S");

    m_statUTC.SetWindowText(strValue);

    return strValue;
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Convert latitude from numeric representation to human readable
/// string.</summary>
CString CGarminBinaryDlg::Latitude2Str(double lat)
{
    CString strValue;

    double latitude = lat * 180.0 / PI;

    // Determine the letter.
    char latLetter = 'N';

    if(latitude < 0)
    {
        latLetter = 'S';
        latitude = -latitude;
    }

    int latInt = (int)latitude;
    double latMinutes = (latitude - latInt) * 60;

    // Format a string value of the latitude letter and value.
    // Uses format: H DD MM.MMMM
    strValue.Format("%c %d*%02.4f'", latLetter, latInt, latMinutes);

    return strValue;
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Convert longitude from numeric representation to human readable
/// string.</summary>
CString CGarminBinaryDlg::Longitude2Str(double lon)
{
    CString strValue;

    double longitude = lon * 180.0 / PI;

    // Determine the letter.
    char lonLetter = 'E';

    if(longitude < 0)
    {
        lonLetter = 'W';
        longitude = -longitude;
    }

    int lonInt = (int)longitude;
    double lonMinutes = (longitude - lonInt) * 60;

    // Format a string value of the longitude letter and value.
    // Uses format: H DD MM.MMMM
    strValue.Format("%c%03d*%2.4f'", lonLetter, lonInt, lonMinutes);

    return strValue;
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Convert message buffer containig latitude and logitude to
/// human readable string.</summary>
void CGarminBinaryDlg::DecodeLatLon()
{
#pragma pack(1)
    typedef struct
    {
        double latitude;
        double longitude;
    } t_GPS_LATLON;

    // Recast the payload to the structure format.
    t_GPS_LATLON* pLatLon = (t_GPS_LATLON*)m_RecvMsg.Payload;

    // Format a string of the full latitude and longitude values.
    CString strValue =
        Latitude2Str(pLatLon->latitude) + " " +
        Longitude2Str(pLatLon->longitude);

    m_statLatLon.SetWindowText(strValue);

}

/////////////////////////////////////////////////////////////////////////////
///<summary>Convert message buffer containing Position, Velocity, Time to
/// human readable string.</summary>
void CGarminBinaryDlg::DecodePVT()
{
#pragma pack(1)
    typedef struct
    {
        float    alt;       // altitude above WGS 84 (meters)
        float    epe;       // estimated position error, 2 sigma (meters)
        float    eph;       // epe, but horizontal only (meters)
        float    epv;       // epe but vertical only (meters )
        uint16_t fix;       // 1- None, 2- 2D, 3- 3D, 4- 2D WAAS, 5- 3D WAAS
        double   gps_tow;   // gps time of week (seconds)
        double   lat;       // latitude (radians)
        double   lon;       // longitude (radians)
        float    lon_vel;   // velocity east (meters/second)
        float    lat_vel;   // velocity north (meters/second)
        float    alt_vel;   // velocity up (meters/sec)
        float    msl_hght;  // height of WGS 84 above MSL (meters)
        uint16_t leap_sec;  // diff between GPS and UTC (seconds)
        uint16_t grmn_days; // days from UTC 31-December-1989 to last Sunday
    } t_GPS_PVT_DATA;

    // Recast the payload to the structure format.
    t_GPS_PVT_DATA* pPVT = (t_GPS_PVT_DATA*)m_RecvMsg.Payload;

    // Format a string of the fix value.
    CString strValue;
    strValue.Format("%d", pPVT->fix);
    m_statGpsFix.SetWindowText(strValue);

    // The estimated error is invalid until there is a fix.
    if(pPVT->fix > 1)
    {
        // Format a string of the estimate position error.
        strValue.Format("%2.2f m", pPVT->eph);
    }
    else
    {
        strValue = "None";
    }

    m_statEPE.SetWindowText(strValue);

    // Format a string of the latitude and longitude.
    strValue = Latitude2Str(pPVT->lat) + " " +
               Longitude2Str(pPVT->lon);
    m_statLatLon.SetWindowText(strValue);

    // Time is total seconds since midnight UTC 1-Jan-1990
    CTime time = CTime(1989, 12, 31, 0, 0, 0, 1);
    time += (unsigned int)pPVT->grmn_days * 86400
            + (unsigned int)pPVT->gps_tow
            - (unsigned int)pPVT->leap_sec;

    // Format like: Sun 19-Jun-2016 07:05:24
    strValue = time.Format("%a %d-%b-%Y %H:%M:%S");
    m_statUTC.SetWindowText(strValue);
}

/////////////////////////////////////////////////////////////////////////////
///<summary>A completed message frame has arrived, so process it.</summary>
void CGarminBinaryDlg::ProcessFrame()
{
    CString str;

    // Verify received checksum.
    uint8_t size = m_RecvMsg.SizeBytes;

    // Copy checksum byte into checksum field.
    m_RecvMsg.ChkSum = m_RecvMsg.Payload[size];

    // Set end flags: DLE and ETX.
    m_RecvMsg.End1 = 0x10;
    m_RecvMsg.End2 = 0x03;

    // Verify start flag and checksum in the received message.
    if((m_RecvMsg.Start == 0x10) && (CalcChksum(&m_RecvMsg) - m_RecvMsg.ChkSum == 0))
    {
        // A valid message has been parsed. Prepare to display it.

        // Save last command for possible ACK.
        m_lastRecv = m_RecvMsg.CmdId;

        UpdateMsgSeen();

        // Detect GPS ID response.
        if(m_RecvMsg.CmdId == MSG_ID_RSP)
        {
            // Display decoded ID in dedicated field.
            str.Format("%s", &m_RecvMsg.Payload[4]);
            m_statGpsId.SetWindowText(str);

            // Sending ACK for some receivers causes additional data to be received.
            SendAck();
        }

        // Detect Baud rate change response.
        if(m_RecvMsg.CmdId == MSG_BAUD_RSP)
        {
            // Special processing for baud rate change.
            ConfirmNewBaud();
        }

        // Detect receiving UTC response.
        if(m_RecvMsg.CmdId == MSG_UTC_RSP)
        {
            DecodeUTC();
        }

        // Detect receiving Lat/Lon position response.
        if(m_RecvMsg.CmdId == MSG_LATLON_RSP)
        {
            DecodeLatLon();
        }

        // Detect receiving PVT response.
        if(m_RecvMsg.CmdId == MSG_PVT_RSP)
        {
            DecodePVT();
        }

        // Display the received hex string.
        AddToDisplay("", &m_RecvMsg);

        // See if msg should be logged.
        if(m_bIsLogging && (
                    m_RecvMsg.CmdId == 0xFF ||
                    m_RecvMsg.CmdId == 0x11 ||
                    m_RecvMsg.CmdId == 0x0E ||
                    m_RecvMsg.CmdId == 0x33 ||
                    m_RecvMsg.CmdId == 0x36 ||
                    m_RecvMsg.CmdId == 0x37 ||
                    m_RecvMsg.CmdId == 0x38))
        {
            AddToLogFile();
        }
    }
    else
    {
        CString str;
        str.Format("BAD FRAME: %02X %02X %02X . . .",
                   m_RecvMsg.Start, m_RecvMsg.CmdId, m_RecvMsg.SizeBytes);

        AddToDisplay(str, 0);

        ++mErrFrames;
        UpdateErrSeen();
    }

    // Reset message buffer.
    ClearMsgBuff(&m_RecvMsg);
    m_pRcv = (char*)&m_RecvMsg;
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Send received message bytes to G12 formatted file.</summary>
void CGarminBinaryDlg::AddToLogFile()
{
    if(m_bIsLogging)
    {
        m_OutFile.Write(&m_RecvMsg.CmdId, 1);
        m_OutFile.Write(&m_RecvMsg.SizeBytes, 1);

        for(int i = 0; i < m_RecvMsg.SizeBytes; ++i)
        {
            m_OutFile.Write(&m_RecvMsg.Payload[i], 1);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Add a new line to the output console window.</summary>
void CGarminBinaryDlg::AddToDisplay(CString strHdr, t_MSG_FORMAT* pMsg = 0)
{
    // Format the new line: Optional Hdr + Decoded ASCII hex + CRLF.
    CString strLine;
    strLine = strHdr;
    if(pMsg) strLine += DecodeMsgBuff(pMsg);
    strLine += "\r\n";

    // Get the initial text length already in.
    int nLength = m_editMsgs.GetWindowTextLength();

    // Somewhat arbitrary limit on output window.
    static const int nLimit = 5000;
    static const int nDelCount = 1000;

    if(nLength >= nLimit)
    {
        m_editMsgs.SetSel(0, nDelCount);
        m_editMsgs.ReplaceSel("... Text above this point deleted ...\r\n");
    }

    // Set selection at the end of existing text.
    m_editMsgs.SetSel(0x7FFF, 0x7FFF);

    // Replace the selection, this adds new text at the end.
    m_editMsgs.ReplaceSel(strLine);
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Check if the current message has been seen before and if not,
/// update the array that keeps track and update the display.</summary>
void CGarminBinaryDlg::UpdateMsgSeen()
{
    // See if a new valid msg ID was seen.
    if(m_RecvMsg.CmdId && mMsgsSeen[m_RecvMsg.CmdId] == 0)
    {
        // Set a flag indicating which ID was seen.
        mMsgsSeen[m_RecvMsg.CmdId] = 1;

        CString strFinal, str;

        // Regenerate the string of all seen message IDs.
        for(int i = 1; i < 0x100; ++i)
        {
            if(mMsgsSeen[i])
            {
                // Add this ID to the list.
                str.Format("%02X, ", i);
                strFinal += str;
            }
        }

        m_statMsgIdSeen.SetWindowText(strFinal);
    }
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Update the GUI field that displays count of errors.</summary>
void CGarminBinaryDlg::UpdateErrSeen()
{
    CString str;
    str.Format("%d", mErrFrames);
    m_statErrFrames.SetWindowText(str);
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Update the GUI field that displays high water mark.</summary>
void CGarminBinaryDlg::UpdateHighWater()
{
    CString str;
    str.Format("%d", mHighWater);
    m_statWaterLn.SetWindowText(str);
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Update the GUI field that displays the current bandwidth.</summary>
void CGarminBinaryDlg::UpdateBandwidth()
{
    CString strValue;

    // This function is called once every second, so period argument is 1.0
    strValue.Format("%5.1f bps", m_Serial.CalcBandwidth(1.0));
    m_statBandwidth.SetWindowText(strValue);
}

// Newer Garmin GPS receivers seem to support these rates:
// 9600, 19200, 38400, 57600, 115200
// Older receivers are reported to support only up to 38400.

/////////////////////////////////////////////////////////////////////////////
///<summary>GUI button message handler.</summary>
void CGarminBinaryDlg::OnBtnBaudUp()
{
    ClearMsgBuff(&m_SendMsg);

    m_SendMsg.Start      = 0x10;
    m_SendMsg.CmdId      = MSG_BAUD_CMD;
    m_SendMsg.SizeBytes  = 0x04;
    unsigned int baud    = 57600;

    // Little endien
    m_SendMsg.Payload[0] = baud;
    m_SendMsg.Payload[1] = baud >> 8;
    m_SendMsg.Payload[2] = baud >> 16;
    m_SendMsg.Payload[3] = baud >> 24;

    m_SendMsg.ChkSum     = CalcChksum(&m_SendMsg);
    m_SendMsg.End1       = 0x10;
    m_SendMsg.End2       = 0x03;

    SendMsg();
}

/////////////////////////////////////////////////////////////////////////////
///<summary>GUI button message handler.</summary>
void CGarminBinaryDlg::OnBtnBaudDn()
{
    ClearMsgBuff(&m_SendMsg);

    m_SendMsg.Start      = 0x10;
    m_SendMsg.CmdId      = MSG_BAUD_CMD;
    m_SendMsg.SizeBytes  = 0x04;
    unsigned int baud    = 9600;

    // Little endien
    m_SendMsg.Payload[0] = baud;
    m_SendMsg.Payload[1] = baud >> 8;
    m_SendMsg.Payload[2] = baud >> 16;
    m_SendMsg.Payload[3] = baud >> 24;

    m_SendMsg.ChkSum     = CalcChksum(&m_SendMsg);
    m_SendMsg.End1       = 0x10;
    m_SendMsg.End2       = 0x03;

    SendMsg();
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Sends message to GPS to confirm baud rate change.</summary>
///<remarks>
/// This method confirms a baud rate change in accordance with the procedure
/// documented in Garmin GPS18x Technical Specifications, Appendix C.
/// I assume this may work for most Garmin GPS receivers in serial mode.
/// Confirmed to work with eTrex Vista, eTrex Mariner, GPS V and GPS 60CSx.
///</remarks>
void CGarminBinaryDlg::ConfirmNewBaud()
{
    CString strBaud;
    unsigned int baud = 0;
    baud = m_RecvMsg.Payload[0] << 0  |
           m_RecvMsg.Payload[1] << 8  |
           m_RecvMsg.Payload[2] << 16 |
           m_RecvMsg.Payload[3] << 24;

    strBaud.Format("%d", baud);
    AddToDisplay("Baud confirmed as: " + strBaud, 0);

    // According to Garmin, the confirmed baud should be within
    // plus or minus 5% of the requested value.  Check that here.
    // e.g. GPS60CSx confirms 38400 at 38461, GPS V confirms at 37889.
    // e.g. eTrex Vista confirms 115200 at 113668
    // 9600, 19200, 38400, 57600, 115200

    // Look for target baud +/- 5%
    if(baud > 9600 - 480 && baud < 9600 + 480)
    {
        baud = 9600;
    }
    else if(baud > 19200 - 960 && baud < 19200 + 960)
    {
        baud = 19200;
    }
    else if(baud > 38400 - 1920 && baud < 38400 + 1920)
    {
        baud = 38400;
    }
    else if(baud > 57600 - 2880 && baud < 57600 + 2880)
    {
        baud = 57600;
    }
    else if(baud > 115200 - 5760 && baud < 115200 + 5760)
    {
        baud = 115200;
    }
    else
    {
        AfxMessageBox("Baud change did not work!");

        // Ending the process here without sending the confirmations
        // below lets the GPS's 2 second window elapse, and it will
        // automatically revert to the former baud rate.
        return;
    }

    // Display the "standard" baud, not the confirmed number.
    strBaud.Format("%d", baud);
    m_statBaud.SetWindowText(strBaud);

    // Send ACK of baud change msg, at old baud.
    SendAck();

    // Garmin says to wait at least 100 mSec here.
    Sleep(150);

    // Now change our local baud rate to the new baud.
    m_Serial.Setup((CSerial::EBaudrate)baud, CSerial::EData8, CSerial::EParNone, CSerial::EStop1);
    m_Serial.Purge();

    // Transmit a confirmation at the new baud rate.
    ClearMsgBuff(&m_SendMsg);

    m_SendMsg.Start      = 0x10;
    m_SendMsg.CmdId      = MSG_QUERY;
    m_SendMsg.SizeBytes  = 0x02;
    m_SendMsg.Payload[0] = QUERY_CQCQ;
    m_SendMsg.Payload[1] = 0x00;
    m_SendMsg.ChkSum     = CalcChksum(&m_SendMsg);
    m_SendMsg.End1       = 0x10;
    m_SendMsg.End2       = 0x03;

    SendMsg();

    // I'm cheating here, not listening for a response,
    // just blindly sending again, but it seems to work.
    Sleep(100);

    // Transmit a second confirmation at new baud rate.
    ClearMsgBuff(&m_SendMsg);

    m_SendMsg.Start      = 0x10;
    m_SendMsg.CmdId      = MSG_QUERY;
    m_SendMsg.SizeBytes  = 0x02;
    m_SendMsg.Payload[0] = QUERY_CQCQ;
    m_SendMsg.Payload[1] = 0x00;
    m_SendMsg.ChkSum     = CalcChksum(&m_SendMsg);
    m_SendMsg.End1       = 0x10;
    m_SendMsg.End2       = 0x03;

    SendMsg();

    // This whole process must complete in less than 2 seconds
    // or the GPS will revert to the old baud rate.
}

/////////////////////////////////////////////////////////////////////////////
///<summary>GUI button message handler.</summary>
void CGarminBinaryDlg::OnBtnBaudLocal()
{
    if(IsAtLoBaud())
    {
        m_statBaud.SetWindowText(s_StrHiBaud);
        m_Serial.Setup((CSerial::EBaudrate)atoi(s_StrHiBaud), CSerial::EData8, CSerial::EParNone, CSerial::EStop1);
    }
    else
    {
        m_statBaud.SetWindowText(s_StrLoBaud);
        m_Serial.Setup((CSerial::EBaudrate)atoi(s_StrLoBaud), CSerial::EData8, CSerial::EParNone, CSerial::EStop1);
    }
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Change the state machine to control a G12 file capture.</summary>
void CGarminBinaryDlg::G12State(e_STATE_TYPE state)
{
    // Determine if a specific state should be set.
    if(state != STATE_NEXT)
    {
        mG12State = state;
    }

    // Process the current state.
    if(mG12State == STATE_START)
    {
        AddToDisplay("STATE_START", 0);

        // Make sure all periodic messages are off.
        OnBtnAsyncOff();

        // Start the state transition timer.
        SetTimer(_STATE_TIMER, _STATE_TIMER_CMDS_MSECS, NULL);

        // Next state.
        mG12State = STATE_GET_ID;
    }
    else if(mG12State == STATE_GET_ID)
    {
        AddToDisplay("STATE_GET_ID", 0);

        // Request GPS ID String.
        OnBtnGetId();

        // Next state.
        mG12State = STATE_GET_UTC;
    }
    else if(mG12State == STATE_GET_UTC)
    {
        AddToDisplay("STATE_GET_UTC", 0);

        // Request UTC Time from GPS.
        OnBtnGetUTC();

        // Next state.
        mG12State = STATE_GET_POS;
    }
    else if(mG12State == STATE_GET_POS)
    {
        AddToDisplay("STATE_GET_POS", 0);

        // Request position from GPS.
        OnBtnGetLatLon();

        // Next state.
        mG12State = STATE_PVT_ON;
    }
    else if(mG12State == STATE_PVT_ON)
    {
        AddToDisplay("STATE_PVT_ON", 0);

        // Set longer state timer for this state.
        SetTimer(_STATE_TIMER, _STATE_TIMER_CMDS_MSECS * 6, NULL);

        // Turn on once per second PVT messages from GPS.
        OnBtnPvtOn();

        // Next state.
        mG12State = STATE_ASYNC_ON;
    }
    else if(mG12State == STATE_ASYNC_ON)
    {
        // Resume normal period for state timer.
        SetTimer(_STATE_TIMER, _STATE_TIMER_CMDS_MSECS, NULL);

        // Turn off PVT messages.
        OnBtnPvtOff();

        AddToDisplay("STATE_ASYNC_ON", 0);

        // Start asyncronous output of specific messages from GPS.
        AsyncMaskOn();

        CString strValue;
        strValue.Format("%d", mTickDown);
        m_statTick.SetWindowText(strValue);

        // Next state.
        mG12State = STATE_ASYNC_TIC;
    }
    else if(mG12State == STATE_ASYNC_TIC)
    {
        //AddToDisplay("STATE_ASYNC_TIC", 0);

        TickDown();

        // Do not advance the state.
        // State will be advanced by TickDown.
    }
    else if(mG12State == STATE_STOP_REC)
    {
        AddToDisplay("STATE_STOP_REC", 0);
        m_statTick.SetWindowText("0");

        m_bIsLogging = false;
        if(m_OutFile.m_hFile != CFile::hFileNull) m_OutFile.Close();

        OnBtnAsyncOff();

        // Next state.
        mG12State = STATE_GPS_OFF;
    }
    else if(mG12State == STATE_GPS_OFF)
    {
        if(m_chkPwrOff.GetCheck() == BST_CHECKED)
        {
            AddToDisplay("STATE_GPS_OFF", 0);
            OnBtnPwrOff();
        }

        // Next state.
        mG12State = STATE_FINISH;
    }
    else if(mG12State == STATE_FINISH)
    {
        AddToDisplay("STATE_FINISH", 0);

        // Stop the state transition timer.
        KillTimer(_STATE_TIMER);

        // The bandwidth is not measured unless recording file.
        m_statBandwidth.SetWindowText("");

        // Spawn process to convert to Rinex.
        CallGar2rnx();

        G12State(STATE_IDLE);
    }
    else // IDLE state is the end. Once in Idle, stay in Idle.
    {
        // Stay in IDLE.
        mG12State = STATE_IDLE;
    }
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Call the gar2rnx external program to convert the G12 file
/// to a RINEX file.</summary>
void CGarminBinaryDlg::CallGar2rnx()
{
    // get the path to the Rinex creator.
    CString strRinexPath = m_Profile.GetProfileStr("MainConfig", "Gar2RnxPath" , "");
    CString strRinexOpts = m_Profile.GetProfileStr("MainConfig", "Gar2RnxOptions" , "-etrex");

    // Change last letter of G12 filename to "O"
    CString strFileNameRinex = m_strFileNameG12.Left(m_strFileNameG12.GetLength()-1) + "O";

    // Form a string to execute the command.
    CString strRinexExec = strRinexPath + "gar2rnx.exe " + m_strFileNameG12 + " " +
                           strRinexOpts + " > " + strFileNameRinex;

    // Display the full invocation command line.
    AddToDisplay(strRinexExec, 0);

    // spawn a new process to run the Rinex creator.
    int ret = system(strRinexExec);

    // Check and report error.
    if(ret != 0)
    {
        CString str;
        str.Format(
            "Trying to launch gar2rnx.exe returned error code: %d\r\n"\
            "The Rinex file was not created properly. Check the\r\n"\
            "XML configuration file has the correct path information.", ret);
        AfxMessageBox(str, MB_ICONERROR);
    }
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Change the filename extension of provided filename.</summary>
CString CGarminBinaryDlg::ChangeExtension(CString strPath, CString strNewExt)
{
    // The return value.
    CString strUpdated;

    // Parse backward to find the last dot.
    int i = strPath.ReverseFind('.');

    // Now parse backward to find the last backslash.
    int j = strPath.ReverseFind('\\');

    // Make sure the last dot comes after the last backslash.
    // Otherwise the filename doesn't have an extension, and we
    // may have found a dot somewhere else in the path.
    if(i != -1 && j != -1 && j < i)
    {
        // Set index to just after the dot.
        ++i;

        // Put on the new extension.
        strUpdated = strPath.Left(i) + strNewExt;

    }
    else
    {
        // Found no extension, so set index at the end.
        i = strPath.GetLength();

        // Put on the new extension.
        strUpdated = strPath.Left(i) + "." + strNewExt;
    }

    return strUpdated;
}

/////////////////////////////////////////////////////////////////////////////
///<summary>Make a filename based on current UTC following normal
/// RINEX conventions.</summary>
CString CGarminBinaryDlg::GetGarminBinaryFilename()
{
    CTime time(CTime::GetCurrentTime());
    tm tmGmt;
    time.GetGmtTm(&tmGmt);

    int julday = tmGmt.tm_yday + 1;
    char hour = tmGmt.tm_hour + 'a';
    int year = tmGmt.tm_year - 100;
    int minute = tmGmt.tm_min;

    CString strFilename;
    strFilename.Format("grmn%d%c%02d.%02dG", julday, hour, minute, year);

    return strFilename;
}

/////////////////////////////////////////////////////////////////////////////
/// <summary>Determines if serial port is set to low baud.</summary>
bool CGarminBinaryDlg::IsAtLoBaud()
{
    bool retVal = false;
    CString str;
    m_statBaud.GetWindowText(str);

    if(str != s_StrHiBaud)
    {
        retVal = true;
    }

    return retVal;
}
