/****************************************************************************
GARMIN BINARY EXPLORER for Garmin GPS Receivers that support serial I/O.

Copyright (C) 2016 Norm Moulton

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

1.00   30 May 2016, First version

****************************************************************************/

// GarminBinaryDlg.h : header file
//

#if !defined(AFX_GARMINBINARYDLG_H__5A848C27_8495_4931_9A93_EFE2B39822D5__INCLUDED_)
#define AFX_GARMINBINARYDLG_H__5A848C27_8495_4931_9A93_EFE2B39822D5__INCLUDED_

// Added by ClassView
#include "GarminBinary.h"
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


/////////////////////////////////////////////////////////////////////////////
// CGarminBinaryDlg dialog

class CGarminBinaryDlg : public CDialog
{
public:

    // Construction
    CGarminBinaryDlg(CWnd* pParent = NULL); // standard constructor

    BOOL PeekAndPump();

    void SetupPort();
    void RecvMsg();
    void SendMsg();

    void ProcessFrame();
    CString DecodeMsgBuff(t_MSG_FORMAT* pMsg);
    void ClearMsgBuff(t_MSG_FORMAT* pMsg);
    void DisplayMsg(t_MSG_FORMAT *pMsg);
    t_UINT8 CalcChksum(t_MSG_FORMAT* pMsg);
    void AddToDisplay(CString strHdr, t_MSG_FORMAT *pMsg);
    void UpdateMsgSeen();
    void UpdateErrSeen();
    void ConfirmNewBaud();
    void DecodeLatLon();
    CString DecodeUTC();
    void DecodePVT();
    void AddToLogFile();
    void UpdateHighWater();
    void UpdateAsyncMask();
    CString Latitude2Str(double lat);
    CString Longitude2Str(double lon);
    void G12State(e_STATE_TYPE state = STATE_NEXT);
    CString ChangeExtension(CString strPath, CString strNewExt);
    CString GetGarminBinaryFilename();
    void TickDown();



// Dialog Data
    //{{AFX_DATA(CGarminBinaryDlg)
    enum { IDD = IDD_MAIN_DLG };
    CEdit   m_editRecTime;
    CStatic m_statTick;
    CStatic m_statEPE;
    CButton m_chkPwrOff;
    CStatic m_statWaterLn;
    CButton m_btnBaudLocal;
    CStatic m_statMask;
    CStatic m_statGpsFix;
    CStatic m_statLatLon;
    CStatic m_statUTC;
    CStatic m_statErrFrames;
    CStatic m_statBaud;
    CStatic m_statMsgIdSeen;
    CStatic m_statGpsId;
    CEdit   m_editMsgs;
    CComboBox   m_cmboPort;
    //}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CGarminBinaryDlg)
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    HICON m_hIcon;

    // Generated message map functions
    //{{AFX_MSG(CGarminBinaryDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnBtnGetId();
    virtual void OnCancel();
    virtual void OnOK();
    afx_msg void OnCloseupCmboPort();
    afx_msg void OnTimer(UINT nIDEvent);
    afx_msg void OnBtnStartAsync();
    afx_msg void OnBtnStopAsync();
    afx_msg void OnBtnAck();
    afx_msg void OnBtnClear();
    afx_msg void OnBtnPosOn();
    afx_msg void OnBtnPosOff();
    afx_msg void OnBtnAbout();
    afx_msg void OnBtnLogOn();
    afx_msg void OnBtnLogOff();
    afx_msg void OnBtnGetLatLon();
    afx_msg void OnBtnGetUTC();
    afx_msg void OnBtnBaudUp();
    afx_msg void OnBtnBaudDn();
    afx_msg void OnBtnBaudLocal();
    afx_msg void OnBtnPwrOff();
    afx_msg void OnBtnGar2rnx();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:

    // Data
    CString m_strSerialPort;
    CString m_strFileNameG12;

    t_MSG_FORMAT m_SendMsg;
    t_MSG_FORMAT m_RecvMsg;
    char* m_pRcv;
    t_UINT8 m_lastRecv;
    unsigned int mMsgsSeen[0x100];
    unsigned int mErrFrames;

    t_UINT16 mMask;
    bool m_bIsLogging;
    e_STATE_TYPE mG12State;
    int mTickDown;

    unsigned int mHighWater;
    CFile m_OutFile;
    HICON m_hIconBtn;
    CFont m_Font;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GARMINBINARYDLG_H__5A848C27_8495_4931_9A93_EFE2B39822D5__INCLUDED_)
