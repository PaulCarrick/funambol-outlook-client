/*
 * Funambol is a mobile platform developed by Funambol, Inc. 
 * Copyright (C) 2003 - 2007 Funambol, Inc.
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License version 3 as published by
 * the Free Software Foundation with the addition of the following permission 
 * added to Section 15 as permitted in Section 7(a): FOR ANY PART OF THE COVERED
 * WORK IN WHICH THE COPYRIGHT IS OWNED BY FUNAMBOL, FUNAMBOL DISCLAIMS THE 
 * WARRANTY OF NON INFRINGEMENT  OF THIRD PARTY RIGHTS.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Affero General Public License 
 * along with this program; if not, see http://www.gnu.org/licenses or write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA.
 * 
 * You can contact Funambol, Inc. headquarters at 643 Bair Island Road, Suite 
 * 305, Redwood City, CA 94063, USA, or at email address info@funambol.com.
 * 
 * The interactive user interfaces in modified source and object code versions
 * of this program must display Appropriate Legal Notices, as required under
 * Section 5 of the GNU Affero General Public License version 3.
 * 
 * In accordance with Section 7(b) of the GNU Affero General Public License
 * version 3, these Appropriate Legal Notices must retain the display of the
 * "Powered by Funambol" logo. If the display of the logo is not reasonably 
 * feasible for technical reasons, the Appropriate Legal Notices must display
 * the words "Powered by Funambol".
 */

#if !defined(AFX_OP_H__FA98B70B_D0B7_11D3_BC39_00C04F602FEE__INCLUDED_)
#define AFX_OP_H__FA98B70B_D0B7_11D3_BC39_00C04F602FEE__INCLUDED_

/** @cond OLPLUGIN */
/** @addtogroup UI */
/** @{ */

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols


/**
 * Main class for Outlook Plug-in UI.
 */
class COutlookPluginApp : public CWinApp
{
public:
    COutlookPluginApp();
    CSingleDocTemplate* docSettings;
    CSingleDocTemplate* docMain;
    HINSTANCE hInst;
    HINSTANCE   hLib;

    HANDLE hScheduleSyncThread;
    DWORD dwScheduleThreadId;

    BOOL registerFunClass();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COutlookPluginApp)
	public:
          virtual BOOL InitInstance();
          virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(COutlookPluginApp)
	afx_msg void OnAppAbout();
    afx_msg void OnViewGuide();

    // manually start the update procedure
    afx_msg void OnUpdateSw();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

/** @} */
/** @endcond */
#endif
