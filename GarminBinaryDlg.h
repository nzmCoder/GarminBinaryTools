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
    void AddToDisplay(CString str);
    t_UINT8 m_lastRecv;
    void AddToDisplay(CString strHdr, t_MSG_FORMAT *pMsg);
    void ProcessFrame();
    // Construction
    CGarminBinaryDlg(CWnd* pParent = NULL); // standard constructor

    void SetupPort();

    void RecvMsg();
    void SendMsg();

    CString DecodeMsgBuff(t_MSG_FORMAT* pMsg);
    void DisplayMsg(t_MSG_FORMAT *pMsg);
    void ClearMsgBuff(t_MSG_FORMAT* pMsg);

    t_UINT8 CalcChksum(t_MSG_FORMAT* pMsg);

    BOOL PeekAndPump();

    void Test();

    // Data
    t_MSG_FORMAT m_SendMsg;
    t_MSG_FORMAT m_RecvMsg;
    char* m_pRcv;


// Dialog Data
    //{{AFX_DATA(CGarminBinaryDlg)
    enum { IDD = IDD_MAIN_DLG };
    CStatic m_statGpsId;
    CButton m_chkTimer;
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
    afx_msg void OnCheck();
    afx_msg void OnBtnStartAsync();
    afx_msg void OnBtnStopAsync();
    afx_msg void OnBtnAck();
    afx_msg void OnBtnClear();
    afx_msg void OnBtnPosOn();
    afx_msg void OnBtnPosOff();
    afx_msg void OnBtnPosRmd();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:

    CString m_strSerialPort;
    CFont m_Font;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GARMINBINARYDLG_H__5A848C27_8495_4931_9A93_EFE2B39822D5__INCLUDED_)
