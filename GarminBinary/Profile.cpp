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
// Profile.cpp: implementation of the CProfile class.

#include "stdafx.h"
#include "Profile.h"

// XML strings.
const CString XML_DIR_NAME = "nzmCoder";
const CString XML_COMMENT = "This file contains persistent data for the application";

// XML tag elements.
const char* XML_ROOT = "data";
const char* XML_ELEMENT = "element";
const char* XML_SECTION = "section";
const char* XML_ENTRY = "entry";
const char* XML_VALUE = "value";

CProfile::CProfile() :
   mXmlDocPtr(0),
   mIsInitialized(false)
{
   // Initialize function must be called on every start-up prior to use.
}

CProfile::~CProfile()
{
   delete mXmlDocPtr;
   mXmlDocPtr = 0;
}

void CProfile::Initialize(CString strDataFilename, bool useAppDataLoc)
{
   // Only allow initialization once.
   if (!mIsInitialized)
   {
      mIsInitialized = true;
      mStrDataFilename = strDataFilename;

      if (useAppDataLoc)
      {
         // Adjust the name to place the file in the user's %appdata% folder.
         mStrDataFilename = PrepareAppDataFilename(mStrDataFilename);
      }

      mXmlDocPtr = new tinyxml2::XMLDocument();

      if (mXmlDocPtr->LoadFile(mStrDataFilename) != tinyxml2::XML_SUCCESS)
      {
         Clear();
      }
   }
}

void CProfile::Clear()
{
   mXmlDocPtr->Clear();

   // Add the default declaration header.
   tinyxml2::XMLDeclaration* pDeclaration = mXmlDocPtr->NewDeclaration(0);
   mXmlDocPtr->InsertEndChild(pDeclaration);
   mXmlDocPtr->SetBOM(true);

   // Add our standard global comment.
   tinyxml2::XMLComment* pComment = mXmlDocPtr->NewComment(XML_COMMENT);
   mXmlDocPtr->InsertEndChild(pComment);

   // Add the root element.
   tinyxml2::XMLElement* pRoot = mXmlDocPtr->NewElement(XML_ROOT);
   mXmlDocPtr->InsertEndChild(pRoot);

   mXmlDocPtr->SaveFile(mStrDataFilename);
}

int CProfile::GetProfileInt(CString strSection, CString strEntry, int nDefault)
{
   // Return value.
   int value = nDefault;

   tinyxml2::XMLElement* elementPtr = FindElement(strSection, strEntry);
   if (elementPtr)
   {
      // Element existed, so get value.
      CString strXmlValue(elementPtr->Attribute(XML_VALUE));
      value = atoi(strXmlValue);
   }
   else
   {
      // Element didn't exist, so add it using default.
      AddElement(strSection, strEntry, nDefault);
      mXmlDocPtr->SaveFile(mStrDataFilename);
   }

   return value;
}

bool CProfile::WriteProfileInt(CString strSection, CString strEntry, int nValue)
{
   // Return value. True means attribute already existed.
   bool isExisting = false;

   tinyxml2::XMLElement* elementPtr = FindElement(strSection, strEntry);
   if (elementPtr)
   {
      // Element existed, so update it.
      elementPtr->SetAttribute(XML_VALUE, nValue);
      isExisting = true;
   }
   else
   {
      // Element didn't exist, so add it.
      AddElement(strSection, strEntry, nValue);
   }

   mXmlDocPtr->SaveFile(mStrDataFilename);

   return isExisting;
}

CString CProfile::GetProfileStr(CString strSection, CString strEntry, CString strDefault)
{
   // Return value.
   CString value = strDefault;

   tinyxml2::XMLElement* elementPtr = FindElement(strSection, strEntry);

   if (elementPtr)
   {
      // Element existed, so get value.
      CString strXmlValue(elementPtr->Attribute(XML_VALUE));
      value = strXmlValue;
   }
   else
   {
      // Element didn't exist, so add it using default.
      AddElement(strSection, strEntry, strDefault);
      mXmlDocPtr->SaveFile(mStrDataFilename);
   }

   return value;
}

bool CProfile::WriteProfileStr(CString strSection, CString strEntry, CString strValue)
{
   // Return value. True means attribute already existed.
   bool isExisting = false;

   tinyxml2::XMLElement* elementPtr = FindElement(strSection, strEntry);
   if (elementPtr)
   {
      // Element existed, so update it.
      elementPtr->SetAttribute(XML_VALUE, strValue);
      isExisting = true;
   }
   else
   {
      // Element didn't exist, so add it.
      AddElement(strSection, strEntry, strValue);
   }

   mXmlDocPtr->SaveFile(mStrDataFilename);

   return isExisting;
}

tinyxml2::XMLElement* CProfile::FindElement(CString strSection, CString strEntry)
{
   // Return value.
   tinyxml2::XMLElement* foundElementPtr = 0;

   tinyxml2::XMLElement* pRoot = mXmlDocPtr->FirstChildElement(XML_ROOT);
   if (pRoot)
   {
      tinyxml2::XMLElement* elementPtr = pRoot->FirstChildElement(XML_ELEMENT);
      while (elementPtr)
      {
         CString strXmlSection(elementPtr->Attribute(XML_SECTION));
         CString strXmlEntry(elementPtr->Attribute(XML_ENTRY));

         if (strXmlSection == strSection && strXmlEntry == strEntry)
         {
            foundElementPtr = elementPtr;
            break;
         }

         elementPtr = elementPtr->NextSiblingElement(XML_ELEMENT);
      }
   }

   return foundElementPtr;
}

void CProfile::AddElement(CString strSection, CString strEntry, int nValue)
{
   CString strValue;
   strValue.Format("%d", nValue);

   AddElement(strSection, strEntry, strValue);
}

void CProfile::AddElement(CString strSection, CString strEntry, CString strValue)
{
   tinyxml2::XMLElement* pRoot = mXmlDocPtr->FirstChildElement(XML_ROOT);
   if (pRoot)
   {
      tinyxml2::XMLElement* pNewElement = mXmlDocPtr->NewElement(XML_ELEMENT);
      pNewElement->SetAttribute(XML_SECTION, strSection);
      pNewElement->SetAttribute(XML_ENTRY, strEntry);
      pNewElement->SetAttribute(XML_VALUE, strValue);
      pRoot->InsertEndChild(pNewElement);
   }
}

CString CProfile::PrepareAppDataFilename(CString strFileName)
{
   // Return value.
   CString strFullPathFilename;

   // Get the user's specific %appdata% path. This is an environment variable,
   // so the user can adjust this value in the OS if they desire. (Add local
   // environment var with same name to override the OS default if necessary.)

   // Form the path name.
   CString strAppDataPath = getenv("AppData");
   strAppDataPath += "\\" + XML_DIR_NAME;

   // This will create a folder in AppData only if it does not exist.
   if (CreateDirectory(strAppDataPath, NULL) ||
      ERROR_ALREADY_EXISTS == GetLastError())
   {
      // One way or another, the folder now exists.
      // Concatenate to form the fully qualified user filename.
      strFullPathFilename = strAppDataPath + "\\" + strFileName;
   }
   else
   {
      // Error using AppData folder, so set the filename to the passed-in
      // value. This will safely fall-back to making the file live in the
      // current folder, which is set to the same folder as the EXE.
      strFullPathFilename = strFileName;
   }

   // Return the full path and filename.
   return strFullPathFilename;
}
