// Profile.cpp: implementation of the CProfile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Profile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CProfile::CProfile(CString strDataFileName):
    m_strDataFileName(strDataFileName),
    m_bNeedsSave(false)
{
    // Init function must be called
    // in the the Windows OnInitDialog function
}

CProfile::~CProfile()
{
    // only worry about saving to file if the profile was written during this lifetime
    if(m_bNeedsSave)
    {
        CFile file;

        // try to save the profile array to the storage file
        if(!file.Open(m_strDataFullPath, CFile::modeCreate | CFile::modeWrite))
        {
            AfxMessageBox("Data settings could not be saved at: "
                          + m_strDataFullPath, MB_ICONSTOP);
        }
        else
        {
            //      AfxMessageBox("Data settings were saved at: "
            //          + m_strDataFullPath, MB_ICONINFORMATION);

            //  write XML header
            CString str;
            str = "<?xml version=\"1.0\"?>\r\n<!-- This file contains persistent data for the application -->\r\n<data>\r\n";
            file.Write(str, str.GetLength());

            int i;
            for(i=0; i<TABLE_SIZE; ++i)
            {
                _WriteElement(file,
                              m_ProfileTable[i].strSection,
                              m_ProfileTable[i].strEntry,
                              m_ProfileTable[i].strValue);
            }

            //  write XML footer

            str = "</data>\r\n";
            file.Write(str, str.GetLength());

            file.Close();
        }
    }
}

void CProfile::Init()
{
    // keep track of whether the profile was written to during this object lifetime
    m_bNeedsSave = false;
    CFile file;

    // try to load the profile array from the storage file
    if(!file.Open(m_strDataFileName, CFile::modeRead))
    {
        m_strDataFullPath = file.GetFilePath();
        //AfxMessageBox("New data file will be created at: "
        //  + m_strDataFullPath, MB_ICONINFORMATION);
    }
    else
    {
        m_strDataFullPath = file.GetFilePath();

        int i;

        for(i=0; i<TABLE_SIZE; ++i)
        {
            _ReadElement(file, m_ProfileTable[i].strSection, m_ProfileTable[i].strEntry, m_ProfileTable[i].strValue);
        }

        file.Close();
    }
//  _Dump();
}

void CProfile::ClearProfile()
{
    CFile file;

    // delete the INI file
    file.Remove(m_strDataFullPath);

//  AfxMessageBox("Data settings were cleared at: "
//          + m_strDataFullPath, MB_ICONINFORMATION);
}


UINT CProfile::GetProfileInt(CString strSection, CString strEntry, int nDefault)
{
    UINT nValue = nDefault;

    int i = 0;

    if((i = _FindEntry(strSection, strEntry)) != -1)
    {
        nValue = atoi(m_ProfileTable[i].strValue);
    }

    return nValue;
}

bool CProfile::WriteProfileInt(CString strSection, CString strEntry, int nValue)
{
    bool bResult = true;

    // flag as 'dirty', needs to be saved to file
    m_bNeedsSave = true;

    int i;
    CString strValue;
    strValue.Format("%d", nValue);

    if((i = _FindEntry(strSection, strEntry)) == -1)
    {
        // not found, so add new entry
        _AddEntry(strSection, strEntry, strValue);
    }
    else
    {
        // update the found entry
        m_ProfileTable[i].strSection = strSection;
        m_ProfileTable[i].strEntry = strEntry;
        m_ProfileTable[i].strValue = strValue;

        bResult = true;
    }

    return bResult;
}

CString CProfile::GetProfileStr(CString strSection, CString strEntry, CString strDefault)
{
    CString strValue = strDefault;

    int i = 0;

    if((i = _FindEntry(strSection, strEntry)) != -1)
    {
        strValue = m_ProfileTable[i].strValue;
    }

    return strValue;
}

bool CProfile::WriteProfileStr(CString strSection, CString strEntry, CString strValue)
{
    bool bResult = true;

    // flag as 'dirty', needs to be saved to file
    m_bNeedsSave = true;

    int i;
    if((i = _FindEntry(strSection, strEntry)) == -1)
    {
        // not found, so add new entry
        _AddEntry(strSection, strEntry, strValue);
    }
    else
    {
        // update the found entry
        m_ProfileTable[i].strSection = strSection;
        m_ProfileTable[i].strEntry = strEntry;
        m_ProfileTable[i].strValue = strValue;

        bResult = true;
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS

// return index of found entry, -1 if not found
int CProfile::_FindEntry(CString strSection, CString strEntry)
{
    int nFoundIdx = -1;

    int i;

    for(i=0; i<TABLE_SIZE; ++i)
    {
        if(m_ProfileTable[i].strSection == strSection
                && m_ProfileTable[i].strEntry == strEntry)
        {
            nFoundIdx = i;
            break;
        }
    }

    return nFoundIdx;
}

bool CProfile::_AddEntry(CString strSection, CString strEntry, CString strValue)
{
    bool bResult = false;

    int i;

    for(i=0; i<TABLE_SIZE; ++i)
    {
        if(m_ProfileTable[i].strSection.IsEmpty())
        {
            // copy the strings in
            m_ProfileTable[i].strSection = strSection;
            m_ProfileTable[i].strEntry = strEntry;
            m_ProfileTable[i].strValue = strValue;

            bResult = true;
            break;
        }
    }

    return bResult;
}

// For debugging only
void CProfile::_Dump()
{
    int i;
    CString str;
    CString strIdx;

    str = m_strDataFullPath;
    str += "\r\n";

    for(i=0; i<40; ++i)
    {
        strIdx.Format("[%d]", i);
        str += strIdx;

        str += m_ProfileTable[i].strSection;
        str += ", ";

        str += m_ProfileTable[i].strEntry;
        str += ", ";

        str += m_ProfileTable[i].strValue;
        str += "\r\n";
    }

    AfxMessageBox(str);
}

void CProfile::_ReadElement(CFile& file, CString& strSection, CString& strEntry, CString& strValue)
{
    // assume valid until an error is found
    bool bValid = true;

    CString str;
    char c;
    int n;
    int i, j;

    // read one full element line from the file
    do
    {
        n = file.Read(&c, 1);
        str += c;

    }
    while(c != '/' && n == 1);

    if(n != 1) bValid = false;

    // parse the "section" tag
    n = str.Find("section=", 0);

    // find the opening quote
    i = str.Find("\"", n);

    // find the closing quote
    j = str.Find("\"", i+1);

    // found the full string
    strSection = str.Mid(i+1, j-i-1);


    // parse the "entry" tag
    n = str.Find("entry=", j);

    // find the opening quote
    i = str.Find("\"", n);

    // find the closing quote
    j = str.Find("\"", i+1);

    // found the full string
    strEntry = str.Mid(i+1, j-i-1);


    // parse the "value" tag
    n = str.Find("value=", j);

    // find the opening quote
    i = str.Find("\"", n);

    // find the closing quote
    j = str.Find("\"", i+1);

    // found the full string
    strValue = str.Mid(i+1, j-i-1);

}

void CProfile::_WriteElement(CFile& file, CString strSection, CString strEntry, CString strValue)
{
    CString str;
    str.Format("<element section=\"%s\" entry=\"%s\" value=\"%s\"></element>\r\n",
               strSection,
               strEntry,
               strValue);

    file.Write(str, str.GetLength());
}

CString CProfile::GetFileName() const
{
    return m_strDataFullPath;
}
