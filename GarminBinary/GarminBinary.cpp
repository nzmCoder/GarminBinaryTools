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
// GarminBinary.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "GarminBinary.h"
#include "GarminBinaryDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGarminBinaryApp

BEGIN_MESSAGE_MAP(CGarminBinaryApp, CWinApp)
    ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGarminBinaryApp construction

CGarminBinaryApp::CGarminBinaryApp()
{
    // Add construction code here
    // Place all significant initialization in InitInstance

    // Force this process to use only a single CPU on a multi-core PC.
    // This makes serial I/O more reliable.
    HANDLE process = GetCurrentProcess();
    (void)SetProcessAffinityMask(process, 1);
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CGarminBinaryApp object

CGarminBinaryApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CGarminBinaryApp initialization

BOOL CGarminBinaryApp::InitInstance()
{
    CGarminBinaryDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    return FALSE;
}
