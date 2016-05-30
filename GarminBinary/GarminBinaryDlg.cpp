// GarminBinaryDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GarminBinary.h"
#include "GarminBinaryDlg.h"
#include "Serial.h"
#include "Profile.h"
#include "Windows.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define _RECV_SERIAL_TIMER 1
#define _RECV_SERIAL_TIMER_MSECS 15  // at 9600 baud, max 14.4 chars pent up


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
    //{{AFX_DATA(CAboutDlg)
    enum { IDD = IDD_ABOUTBOX };
    //}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAboutDlg)
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    //{{AFX_MSG(CAboutDlg)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
    //{{AFX_DATA_INIT(CAboutDlg)
    //}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAboutDlg)
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
    //{{AFX_MSG_MAP(CAboutDlg)
    // No message handlers
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CGarminBinaryDlg is the main window for the dialog app.
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

CGarminBinaryDlg::CGarminBinaryDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CGarminBinaryDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CGarminBinaryDlg)
    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    // Note that LoadIcon does not require a subsequent DestroyIcon in Win32
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CGarminBinaryDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CGarminBinaryDlg)
    DDX_Control(pDX, IDC_STAT_GPS_ID_RSP, m_statGpsId);
    DDX_Control(pDX, IDC_CHECK1, m_chkTimer);
    DDX_Control(pDX, IDC_EDIT_MSGS, m_editMsgs);
    DDX_Control(pDX, IDC_CMBO_PORT, m_cmboPort);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CGarminBinaryDlg, CDialog)
    //{{AFX_MSG_MAP(CGarminBinaryDlg)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BTN_GET_ID, OnBtnGetId)
    ON_CBN_CLOSEUP(IDC_CMBO_PORT, OnCloseupCmboPort)
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_CHECK1, OnCheck)
    ON_BN_CLICKED(IDC_BTN_START_ASYNC, OnBtnStartAsync)
    ON_BN_CLICKED(IDC_BTN_STOP_ASYNC, OnBtnStopAsync)
    ON_BN_CLICKED(IDC_BTN_ACK, OnBtnAck)
    ON_BN_CLICKED(IDC_BTN_CLEAR, OnBtnClear)
    ON_BN_CLICKED(IDC_BTN_POS_ON, OnBtnPosOn)
    ON_BN_CLICKED(IDC_BTN_POS_OFF, OnBtnPosOff)
    ON_BN_CLICKED(IDC_BTN_POS_RMD, OnBtnPosRmd)
    //}}AFX_MSG_MAP
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
// Function from: "Programming Windows MFC", Jeff Prosise
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
void CGarminBinaryDlg::Test()
{
    CString strEdit;

    m_editMsgs.GetWindowText(strEdit);
    strEdit += "*";
    m_editMsgs.SetWindowText(strEdit);

}

/////////////////////////////////////////////////////////////////////////////
// CGarminBinaryDlg Windows message handler functions.
/////////////////////////////////////////////////////////////////////////////

void CGarminBinaryDlg::OnOK()
{
    // Don't let the OK button exit the dialog application.
    // CDialog::OnOK();
}

/////////////////////////////////////////////////////////////////////////////
void CGarminBinaryDlg::OnCancel()
{
    // There is nothing to do.
    // The destructors will clean up everything.

    CDialog::OnCancel();
}

/////////////////////////////////////////////////////////////////////////////
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
        strAboutMenu.LoadString(IDS_ABOUTBOX);
        if(!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // Set the big icon. This auto scales to small also.
    SetIcon(m_hIcon, TRUE);

    // End of boilerplate Windows initialization.

    // Required initialization of the profile object before use.
    m_Profile.Init();

    // Use data from profile to set sticky fields.
    m_strSerialPort = m_Profile.GetProfileStr("MainConfig", "ComPort", "None");
    m_cmboPort.SelectString(-1, m_strSerialPort);

    // Initialize the serial port.
    SetupPort();
    m_pRcv = (char*)&m_RecvMsg;
    m_lastRecv = 0;

    // Format the protocol message window with a fixed width font.
    m_Font.CreateFont(12, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                      DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS,
                      DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Fixedsys");
    m_editMsgs.SetFont(&m_Font, TRUE);

    // Start the serial receive polling timer.
    SetTimer(_RECV_SERIAL_TIMER, _RECV_SERIAL_TIMER_MSECS, NULL);
    m_chkTimer.SetCheck(BST_CHECKED);

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
void CGarminBinaryDlg::OnTimer(UINT nIDEvent)
{
    // In case someday there are multiple timers.
    if(nIDEvent == _RECV_SERIAL_TIMER)
    {
        RecvMsg();
    }

    CDialog::OnTimer(nIDEvent);
}

/////////////////////////////////////////////////////////////////////////////
void CGarminBinaryDlg::OnCloseupCmboPort()
{
    SetupPort();
    m_Profile.WriteProfileStr("MainConfig", "ComPort", m_strSerialPort);
}

/////////////////////////////////////////////////////////////////////////////
void CGarminBinaryDlg::OnCheck()
{
    if(BST_CHECKED == m_chkTimer.GetCheck())
    {
        // Start the serial receive polling timer.
        SetTimer(_RECV_SERIAL_TIMER, _RECV_SERIAL_TIMER_MSECS, NULL);
    }
    else
    {
        KillTimer(_RECV_SERIAL_TIMER);
    }
}

/////////////////////////////////////////////////////////////////////////////
void CGarminBinaryDlg::OnBtnClear()
{
    m_editMsgs.SetWindowText("");
}

/////////////////////////////////////////////////////////////////////////////
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
void CGarminBinaryDlg::OnBtnStartAsync()
{
    ClearMsgBuff(&m_SendMsg);

    m_SendMsg.Start      = 0x10;
    m_SendMsg.CmdId      = MSG_ASYNC;
    m_SendMsg.SizeBytes  = 0x02;
    m_SendMsg.Payload[0] = 0xFF;
    m_SendMsg.Payload[1] = 0xFF;
    m_SendMsg.ChkSum     = CalcChksum(&m_SendMsg);
    m_SendMsg.End1       = 0x10;
    m_SendMsg.End2       = 0x03;

    SendMsg();
}

/////////////////////////////////////////////////////////////////////////////
void CGarminBinaryDlg::OnBtnStopAsync()
{
    ClearMsgBuff(&m_SendMsg);

    m_SendMsg.Start      = 0x10;
    m_SendMsg.CmdId      = MSG_ASYNC;
    m_SendMsg.SizeBytes  = 0x02;
    m_SendMsg.Payload[0] = 0x00;
    m_SendMsg.Payload[1] = 0x00;
    m_SendMsg.ChkSum     = CalcChksum(&m_SendMsg);
    m_SendMsg.End1       = 0x10;
    m_SendMsg.End2       = 0x03;

    SendMsg();
}

/////////////////////////////////////////////////////////////////////////////
void CGarminBinaryDlg::OnBtnAck()
{
    ClearMsgBuff(&m_SendMsg);

    m_SendMsg.Start      = 0x10;
    m_SendMsg.CmdId      = MSG_ACK;
    m_SendMsg.SizeBytes  = 0x02;
    m_SendMsg.Payload[0] = m_lastRecv;
    m_SendMsg.Payload[1] = 0x00;
    m_SendMsg.ChkSum     = CalcChksum(&m_SendMsg);
    m_SendMsg.End1       = 0x10;
    m_SendMsg.End2       = 0x03;

    SendMsg();
}

/////////////////////////////////////////////////////////////////////////////
// CGarminBinaryDlg functions.
/////////////////////////////////////////////////////////////////////////////

void CGarminBinaryDlg::SetupPort()
{
    // Open the Serial Port.

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
        str.Format("\\\\.\\%s", m_strSerialPort);
        if(0 != m_Serial.Open(str))
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
// Sends an outgoing message
void CGarminBinaryDlg::SendMsg()
{
    AddToDisplay("TX: ", &m_SendMsg);

    /*
        CString str = DecodeMsgBuff(&m_SendMsg);
        CString strEdit;
        m_editMsgs.GetWindowText(strEdit);
        strEdit += "TX: ";
        strEdit += str;
        strEdit += "\r\n";
        m_editMsgs.SetWindowText(strEdit);
    */

    // This does not do DLE stuffing, because we only care about
    // sending 3 commands, and none of them need doubled DLEs.

    m_Serial.Write(&m_SendMsg.Start, 1);
    m_Serial.Write(&m_SendMsg.CmdId, 1);
    m_Serial.Write(&m_SendMsg.SizeBytes, 1);
    for(int i = 0; i < m_SendMsg.SizeBytes; ++i)
    {
        m_Serial.Write(&m_SendMsg.Payload[i], 1);
    }
    m_Serial.Write(&m_SendMsg.ChkSum, 1);
    m_Serial.Write(&m_SendMsg.End1, 1);
    m_Serial.Write(&m_SendMsg.End2, 1);
}

/////////////////////////////////////////////////////////////////////////////
// Reads, decodes, and displays any messages coming in
void CGarminBinaryDlg::RecvMsg()
{
    DWORD bytesRead = 0;
    t_UINT8 byte = 0;
    int nTime = 0;
    CString str, strEdit;
    static t_UINT8 lastbyte = 0;

    //  m_editMsgs.GetWindowText(strEdit);

    // see if there is a byte to read
    m_Serial.Read(&byte, 1, &bytesRead);

    // if we read any bytes, keep reading until buffer is empty.
    while(bytesRead)
    {
        // This part finds DLEs and end of frames
        if(lastbyte == 0x10 && byte == 0x10)
        {
            // A second DLE was found.
            str = "";

            // Set lastbyte to some neutral value
            lastbyte = 0;
        }
        else if(lastbyte == 0x10 && byte == 0x03)
        {
            // It is the end of frame.

            // add CR, LF.
            str.Format("%02X\r\n", byte);

            // Call helper function.
            ProcessFrame();

            // Remember lastbyte
            lastbyte = byte;
        }
        else
        {
            // It is a normal mid-frame byte.

            str.Format("%02X ", byte);

            // Add byte, then advance pointer.
            *m_pRcv++ = byte;

            // Remember lastbyte
            lastbyte = byte;
        }

        // strEdit += str;
        // m_editMsgs.SetWindowText(strEdit);

        m_Serial.Read(&byte, 1, &bytesRead);
    }
}

/////////////////////////////////////////////////////////////////////////////
// Calculate the checksum
t_UINT8 CGarminBinaryDlg::CalcChksum(t_MSG_FORMAT *pMsg)
{
    t_UINT8

    // Chksum calculated over CmdId, Size, and payload only.

    // Sum up the byte values.
    val = pMsg->CmdId;
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
void CGarminBinaryDlg::ClearMsgBuff(t_MSG_FORMAT* pMsg)
{
    memset(pMsg, 0, sizeof(t_MSG_FORMAT));
}

/////////////////////////////////////////////////////////////////////////////
void CGarminBinaryDlg::DisplayMsg(t_MSG_FORMAT *pMsg)
{
    CString str = DecodeMsgBuff(pMsg);

    AfxMessageBox(str);
}

/////////////////////////////////////////////////////////////////////////////
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

void CGarminBinaryDlg::ProcessFrame()
{
    CString str;

    // Verify received checksum
    t_UINT8 size = m_RecvMsg.SizeBytes;

    // Copy checksum byte into checksum field.
    m_RecvMsg.ChkSum = m_RecvMsg.Payload[size];

    // Set end flags: DLE and ETX
    m_RecvMsg.End1 = 0x10;
    m_RecvMsg.End2 = 0x03;

    // The checksum in a received message is verified this way.
    if(CalcChksum(&m_RecvMsg) - m_RecvMsg.ChkSum == 0)
    {
        // Prepare to update output window.
        CString strEdit;
        m_editMsgs.GetWindowText(strEdit);

        CString strMsg = DecodeMsgBuff(&m_RecvMsg);


        // Decode the GPS ID response and display in dedicated field.
        if(m_RecvMsg.CmdId == MSG_ID_RSP)
        {
            str.Format("%s", &m_RecvMsg.Payload[4]);
            m_statGpsId.SetWindowText(str);
            //AfxMessageBox(str);
        }

        struct manyFloats
        {
            double n[12];

        };

        // add decoded hex strings
        //AddToDisplay("", &m_RecvMsg);

        // try to decode this msg
        if(m_RecvMsg.CmdId == 0x99)
        {
            // add decoded hex strings
            AddToDisplay("", &m_RecvMsg);

            manyFloats* fPtr = (manyFloats*)&m_RecvMsg.Payload[12];

            for(int i = 0; i < 1; ++i)
            {
                double f = fPtr->n[i];
                str.Format("%g,", f);
                AddToDisplay(str);
            }
            AddToDisplay("\r\n");
        }


        /*
        switch(m_RecvMsg.CmdId)
        {
        //case 0x72:
        case 0x99:
        //case MSG_ACK:
        //case MSG_ID_RSP:
        //case MSG_ID_EXT:
        //case MSG_PSEUD:
          AddToDisplay("", &m_RecvMsg);
        }
        */
    }

    // Save last command for possible ACK
    m_lastRecv = m_RecvMsg.CmdId;

    // Reset message buffer.
    ClearMsgBuff(&m_RecvMsg);
    m_pRcv = (char*)&m_RecvMsg;



}

void CGarminBinaryDlg::AddToDisplay(CString strHdr, t_MSG_FORMAT *pMsg)
{
    // Format the new line: Optional Hdr + Decoded ASCII hex + CRLF.
    CString strLine = strHdr + DecodeMsgBuff(pMsg) + "\r\n";

    // Get the initial text length already in.
    int nLength = m_editMsgs.GetWindowTextLength();

    // Set selection at the end of existing text.
    m_editMsgs.SetSel(nLength, nLength);

    // Replace the selection, this adds new text at the end.
    m_editMsgs.ReplaceSel(strLine);
}

void CGarminBinaryDlg::OnBtnPosOn()
{
    ClearMsgBuff(&m_SendMsg);

    m_SendMsg.Start      = 0x10;
    m_SendMsg.CmdId      = 0x0A;
    m_SendMsg.SizeBytes  = 0x02;
    m_SendMsg.Payload[0] = 0x31;  // PVT On
    m_SendMsg.Payload[1] = 0x00;
    m_SendMsg.ChkSum     = CalcChksum(&m_SendMsg);
    m_SendMsg.End1       = 0x10;
    m_SendMsg.End2       = 0x03;

    SendMsg();
}

void CGarminBinaryDlg::OnBtnPosOff()
{
    ClearMsgBuff(&m_SendMsg);

    m_SendMsg.Start      = 0x10;
    m_SendMsg.CmdId      = 0x0A;
    m_SendMsg.SizeBytes  = 0x02;
    m_SendMsg.Payload[0] = 0x32;  // PVT Off
    m_SendMsg.Payload[1] = 0x00;
    m_SendMsg.ChkSum     = CalcChksum(&m_SendMsg);
    m_SendMsg.End1       = 0x10;
    m_SendMsg.End2       = 0x03;

    SendMsg();
}

void CGarminBinaryDlg::OnBtnPosRmd()
{
    ClearMsgBuff(&m_SendMsg);

    m_SendMsg.Start      = 0x10;
    m_SendMsg.CmdId      = 0x0A;
    m_SendMsg.SizeBytes  = 0x02;
    m_SendMsg.Payload[0] = 0x41; //0x6E;  // RMD On
    m_SendMsg.Payload[1] = 0x00;
    m_SendMsg.ChkSum     = CalcChksum(&m_SendMsg);
    m_SendMsg.End1       = 0x10;
    m_SendMsg.End2       = 0x03;

    SendMsg();
}

void CGarminBinaryDlg::AddToDisplay(CString str)
{
    // Add string to line.
    CString strLine = str;

    // Get the initial text length already in.
    int nLength = m_editMsgs.GetWindowTextLength();

    // Set selection at the end of existing text.
    m_editMsgs.SetSel(nLength, nLength);

    // Replace the selection, this adds new text at the end.
    m_editMsgs.ReplaceSel(strLine);
}
