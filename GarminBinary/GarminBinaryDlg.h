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

1.20   17 Jun 2017, General cleanup and conversion to VS2017

1.00   30 May 2016, First version

****************************************************************************/

// GarminBinaryDlg.h : header file
//

#pragma once

#include "afxwin.h"
#include "GarminBinary.h"

/////////////////////////////////////////////////////////////////////////////
// CGarminBinaryDlg dialog

class CGarminBinaryDlg : public CDialog
{
public:

    CGarminBinaryDlg(CWnd* pParent = NULL);
    BOOL PeekAndPump();
    void UpdateBandwidth();
    void SetupPort();
    void RecvMsg();
    void SendMsg();
    void ProcessFrame();

    void G12State(e_STATE_TYPE state = STATE_NEXT);

    CString DecodeMsgBuff(t_MSG_FORMAT* pMsg);
    void DecodeLatLon();
    CString DecodeUTC();
    void DecodePVT();

    void ClearMsgBuff(t_MSG_FORMAT* pMsg);
    void DisplayMsg(t_MSG_FORMAT *pMsg);
    uint8_t CalcChksum(t_MSG_FORMAT* pMsg);
    void AddToDisplay(CString strHdr, t_MSG_FORMAT *pMsg);
    void UpdateMsgSeen();
    void UpdateErrSeen();
    void ConfirmNewBaud();
    void AddToLogFile();
    void UpdateHighWater();

    CString Latitude2Str(double lat);
    CString Longitude2Str(double lon);
    CString ChangeExtension(CString strPath, CString strNewExt);
    CString GetGarminBinaryFilename();
    void TickDown();
    void AsyncMaskOn();
    void CallGar2rnx();
    void SendAck();
    bool IsAtLoBaud();

// Dialog Controls
    enum { IDD = IDD_MAIN_DLG };

    CComboBox   m_cmboPort;
    CEdit       m_editRecTime;
    CEdit       m_editMsgs;

    CStatic m_statBandwidth;
    CStatic m_statTick;
    CStatic m_statEPE;
    CStatic m_statWaterLn;
    CStatic m_statGpsFix;
    CStatic m_statLatLon;
    CStatic m_statUTC;
    CStatic m_statErrFrames;
    CStatic m_statBaud;
    CStatic m_statMsgIdSeen;
    CStatic m_statGpsId;
    CStatic m_statTickLabel;
    CStatic m_statEpeLabel;
    CStatic m_statFrameErrsLabel;
    CStatic m_statMsgsSeenLabel;
    CStatic m_statRecTimeLabel;
    CStatic m_statWaterLineLabel;
    CStatic m_statFixQuality;

    CButton m_chkPwrOff;
    CButton m_btnBaudLocal;
    CButton m_btnBaudUp;
    CButton m_btnBaudDn;
    CButton m_btnGetId;
    CButton m_btnGetUtc;
    CButton m_btnGetLatLon;
    CButton m_btnRecOn;
    CButton m_btnRecOff;
    CButton m_btnPvtOn;
    CButton m_btnPvtOff;
    CButton m_btnAsyncOn;
    CButton m_btnAsyncOff;
    CButton m_btnClearStats;
    CButton m_btnPwrOff;

protected:

    HICON m_hIcon;
    CToolTipCtrl m_ToolTip;

    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual void OnOK();
    virtual void OnCancel();

    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnBtnGetId();
    afx_msg void OnCloseupCmboPort();
    afx_msg void OnTimer(UINT nIDEvent);
    afx_msg void OnBtnAsyncOn();
    afx_msg void OnBtnAsyncOff();
    afx_msg void OnBtnClear();
    afx_msg void OnBtnPvtOn();
    afx_msg void OnBtnPvtOff();
    afx_msg void OnBtnAbout();
    afx_msg void OnBtnRecOn();
    afx_msg void OnBtnRecOff();
    afx_msg void OnBtnGetLatLon();
    afx_msg void OnBtnGetUTC();
    afx_msg void OnBtnBaudUp();
    afx_msg void OnBtnBaudDn();
    afx_msg void OnBtnBaudLocal();
    afx_msg void OnBtnPwrOff();
    afx_msg void OnDropdownCmboPort();

    DECLARE_MESSAGE_MAP()

private:

    // Data
    CString m_strSerialPort;
    CString m_strFileNameG12;

    t_MSG_FORMAT m_SendMsg;
    t_MSG_FORMAT m_RecvMsg;
    char* m_pRcv;
    uint8_t m_lastRecv;

    unsigned int mMsgsSeen[0x100];
    unsigned int mErrFrames;
    unsigned int mHighWater;

    bool m_bIsLogging;
    e_STATE_TYPE mG12State;
    unsigned int mTickDown;

    CFile m_OutFile;
    HICON m_hIconBtn;
    CFont m_Font;

};

