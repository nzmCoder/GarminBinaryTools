// Profile.h: interface for the CProfile class.
//
//////////////////////////////////////////////////////////////////////

/*

This class was originally modeled after the Windows API functions
Write/GetProfileInt and Write/GetProfileString.

This class stores its data in a user defined file in a format that
crudely resembles XML.

*/

#if !defined(AFX_PROFILE_H__03005067_261F_4307_82A2_09DE57730117__INCLUDED_)
#define AFX_PROFILE_H__03005067_261F_4307_82A2_09DE57730117__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CProfile
{
public:
    CProfile(CString strDataFileName);
    virtual ~CProfile();

    void Init(void);
    void ClearProfile();

    bool WriteProfileStr(CString strSection, CString strEntry, CString strValue);
    CString GetProfileStr(CString strSection, CString strEntry, CString strDefault);

    bool WriteProfileInt(CString strSection, CString strEntry, int nValue);
    UINT GetProfileInt(CString strSection, CString strEntry, int nDefault);

    CString GetFileName() const;

    typedef struct
    {
        CString strSection;
        CString strEntry;
        CString strValue;
    } t_ProfileEntry;

private:

    void _Dump();
    bool _AddEntry(CString strSection, CString strEntry, CString strValue);
    int _FindEntry(CString strSection, CString strEntry);

    void _ReadElement(CFile& file, CString& strSection, CString& strEntry, CString& strValue);
    void _WriteElement(CFile& file, CString strSection, CString strEntry, CString strValue);

    // The maximum number of stored elements.
    enum { TABLE_SIZE = 16 };

    // Data members
    t_ProfileEntry m_ProfileTable[TABLE_SIZE];
    CString m_strDataFullPath;
    bool m_bNeedsSave;
    const CString m_strDataFileName;
};

#endif // !defined(AFX_PROFILE_H__03005067_261F_4307_82A2_09DE57730117__INCLUDED_)
