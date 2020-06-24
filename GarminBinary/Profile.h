/****************************************************************************
GARMIN BINARY EXPLORER for Garmin GPS Receivers that support serial I/O.

Copyright (C) 2016-2020 Norm Moulton

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
// Profile.h: interface for the CProfile class.

#pragma once
#include "TinyXml2.h"

class CProfile
{
public:

   // Ctor/dtor.
   CProfile();
   virtual ~CProfile();

   // Client must call upon start-up.
   void Initialize(CString strDataFilename, bool bUseAppData = true);

   // Clear out any existing stored data.
   void Clear();

   // Get and set string persistent data.
   bool WriteProfileStr(CString strSection, CString strEntry, CString strValue);
   CString GetProfileStr(CString strSection, CString strEntry, CString strDefault);

   // Get and set integer persistent data.
   bool WriteProfileInt(CString strSection, CString strEntry, int nValue);
   int GetProfileInt(CString strSection, CString strEntry, int nDefault);

private:

   // Helper methods.
   tinyxml2::XMLElement* FindElement(CString strSection, CString strEntry);
   void AddElement(CString strSection, CString strEntry, CString strValue);
   void AddElement(CString strSection, CString strEntry, int nValue);

   // Convert the provided file name to a fully qualified path and name.
   CString PrepareAppDataFilename(CString strFileName);

   // Data members
   tinyxml2::XMLDocument* mXmlDocPtr;
   CString mStrDataFilename;
   bool mIsInitialized;
};
