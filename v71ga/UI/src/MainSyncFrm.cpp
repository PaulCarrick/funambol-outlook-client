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

#include "stdafx.h"
#include "OutlookPlugin.h"
#include "OutlookPlugindoc.h"
#include "LeftView.h"
#include "SyncForm.h"
#include "MainSyncFrm.h"
#include "AccountSettings.h"
#include "FullSync.h"
#include "LogSettings.h"
#include "AnimatedIcon.h"

#include "ClientUtil.h"
#include "utils.h"
#include "SyncException.h"

#include "HwndFunctions.h"
#include "comutil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "winmaincpp.h"

#include "Tlhelp32.h"

/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CMainSyncFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainSyncFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CConfigFrame)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code !
    ON_WM_CREATE()
	//}}AFX_MSG_MAP
    ON_MESSAGE(ID_MYMSG_SYNC_BEGIN, &CMainSyncFrame::OnMsgSyncBegin)
    ON_MESSAGE(ID_MYMSG_SYNC_END, &CMainSyncFrame::OnMsgSyncEnd)
    ON_MESSAGE(ID_MYMSG_SYNCSOURCE_BEGIN, &CMainSyncFrame::OnMsgSyncSourceBegin)
    ON_MESSAGE(ID_MYMSG_SYNCSOURCE_END, &CMainSyncFrame::OnMsgSyncSourceEnd)
    ON_MESSAGE(ID_MYMSG_SYNC_ITEM_SYNCED, &CMainSyncFrame::OnMsgItemSynced)
    ON_MESSAGE(ID_MYMSG_SYNC_TOTALITEMS, &CMainSyncFrame::OnMsgTotalItems) 
    ON_MESSAGE(ID_MYMSG_SYNC_STARTSYNC_BEGIN, &CMainSyncFrame::OnMsgStartSyncBegin) 
    ON_MESSAGE(ID_MYMSG_STARTSYNC_ENDED, &CMainSyncFrame::OnMsgStartsyncEnded) 
    ON_MESSAGE(ID_MYMSG_REFRESH_STATUSBAR, &CMainSyncFrame::OnMsgRefreshStatusBar) 
    ON_MESSAGE(ID_MYMSG_SOURCE_STATE, &CMainSyncFrame::OnMsgSyncSourceState) 
    ON_MESSAGE(ID_MYMSG_UNLOCK_BUTTONS, &CMainSyncFrame::OnMsgUnlockButtons)
    ON_COMMAND(ID_FILE_CONFIGURATION, &CMainSyncFrame::OnFileConfiguration)
    ON_COMMAND(ID_TOOLS_FULLSYNC, &CMainSyncFrame::OnToolsFullSync)
    ON_COMMAND(ID_FILE_SYNCHRONIZE, &CMainSyncFrame::OnFileSynchronize)
    ON_COMMAND(ID_TOOLS_SETLOGLEVEL, &CMainSyncFrame::OnToolsSetloglevel)
    ON_WM_NCACTIVATE()
    ON_WM_CLOSE()
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	//ID_INDICATOR_CAPS,
	//ID_INDICATOR_NUM,
	//ID_INDICATOR_SCRL,
};


// Flag used to lock/unlock the statusbar (and other objects).
// During canceling sync, we don't want bar to be updated / object enabled.
bool cancelingSync = false;


/**
 * Function used to refresh the statusbar.
 * Statusbar is not updated if locked by the flag 'cancelingSync'.
 */
void refreshStatusBar(CString& msg) {

    // Avoid updating the statusbar when canceling sync.
    if (!cancelingSync && msg.GetLength()) {
        ((CMainSyncFrame*)AfxGetMainWnd())->wndStatusBar.SetPaneText(0, msg);
    }
}


/**
 * Function used to refresh labels of each source.
 * Labels are not updated if locked by the flag 'cancelingSync'.
 */
void refreshSourceLabels(CString& msg, int sourceIndex) {

    // Don't update source labels for scheduled sync
    if (getConfig()->getScheduledSync()) {
        return;
    }

    if (!cancelingSync && msg.GetLength()) {
        switch(sourceIndex){
            case SYNCSOURCE_CONTACTS:
                ((CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1))->changeContactsStatus(msg); 
                ((CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1))->paneContacts.Invalidate();
                break;
            case SYNCSOURCE_CALENDAR:
                ((CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1))->changeCalendarStatus(msg); 
                ((CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1))->paneCalendar.Invalidate();
                break;
            case SYNCSOURCE_TASKS:
                ((CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1))->changeTasksStatus(msg);    
                ((CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1))->paneTasks.Invalidate();
                break;
            case SYNCSOURCE_NOTES:
                ((CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1))->changeNotesStatus(msg);
                ((CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1))->paneNotes.Invalidate();
                break;  
            default:
                break;
        }
    }
}



/////////////////////////////////////////////////////////////////////////////

CMainSyncFrame::CMainSyncFrame() {
    hSyncThread = NULL;
    dwThreadId  = NULL;
    configOpened = false;
    cancelingSync = false;
    
    syncModeContacts = -1; syncModeCalendar = -1; 
    syncModeTasks = -1; syncModeNotes = -1; 
    dpiX = 0; dpiY =0;

    // load bitmaps
    hBmpDarkBlue = LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_BK_DARK_BLUE));
    hBmpBlue = LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_BK_BLUE));
    hBmpDark = LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_BK_DARK));
    hBmpLight = LoadBitmap(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_BK_LIGHT));
}

CMainSyncFrame::~CMainSyncFrame() {
    if (dwThreadId) {
        CloseHandle(hSyncThread);
    } 
    //closeClient();
    DeleteObject(hBmpDarkBlue);
    DeleteObject(hBmpBlue);
    DeleteObject(hBmpDark);
    DeleteObject(hBmpLight);
}

int CMainSyncFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!wndStatusBar.Create(this) ||
		!wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

    // TODO: hide splitter here
    EnableDocking(CBRS_ALIGN_ANY);
    wndSplitter.SetActivePane(0,1);
    wndSplitter.SetColumnInfo(0,0,0); 
    RecalcLayout();
    SetWindowText(WPROGRAM_NAME); 

    bSyncStarted = false;

	return 0;
}

BOOL CMainSyncFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;

    // TODO: set here main window size and style
    cs.style =  WS_SYSMENU  | WS_VISIBLE | WS_MINIMIZEBOX;
    // cs.dwExStyle = 0 ; 
    HDC hdc = ::GetDC(0);
    dpiX = ::GetDeviceCaps(hdc,LOGPIXELSX);
    dpiY = ::GetDeviceCaps(hdc,LOGPIXELSY);
    ::ReleaseDC(0,hdc);

    double dx = FRAME_MAIN_X * ((double)dpiX/96);      // default DPI = 96
    double dy = FRAME_MAIN_Y * ((double)dpiY/96);      // default DPI = 96
    cs.cy = (int)dy;
    cs.cx = (int)dx;


    // Center window
    cs.x = (GetSystemMetrics(SM_CXSCREEN) - cs.cx)/2;
    cs.y = (GetSystemMetrics(SM_CYSCREEN) - cs.cy)/2;

    // Set the class name, previously registered to be now used.
    // Class name is important to correctly use FindWindow() function.
    cs.lpszClass = PLUGIN_UI_CLASSNAME;

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// diagnostics

#ifdef _DEBUG
void CMainSyncFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainSyncFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG


/////////////////////////////////////////////////////////////////////////////

BOOL CMainSyncFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{ 
    if (!wndSplitter.CreateStatic(this,1,2,WS_CHILD | WS_VISIBLE | WS_MINIMIZEBOX))
	{
		TRACE(_T("failed to create the splitter"));
		return FALSE;
	}
	
    if (!wndSplitter.CreateView(0,0,RUNTIME_CLASS(CSyncForm),CSize(100,100),pContext))
	{
		TRACE(_T("Failed to create view in first pane"));
		return FALSE;
	}

    if (!wndSplitter.CreateView(0,1,RUNTIME_CLASS(CSyncForm),CSize(100,100),pContext))
	{
		TRACE(_T("failed to create view in second pane"));
		return FALSE;
	} 

	return TRUE;
}

void CMainSyncFrame::OnFileConfiguration()
{
    if (checkConnectionSettings()) {
        // show config: Sync settings (default)
        showSettingsWindow(1);
    }
    else {
        // show config: Account settings
        showSettingsWindow(0);
    }
}


void CMainSyncFrame::OnToolsFullSync()
{
    // if sync is in progress we don't open the recover panel
    if(checkSyncInProgress()){
        CString s1;
        s1.LoadString(IDS_ERROR_CANNOT_CHANGE_SETTINGS);
        AfxMessageBox(s1);
        return;
    }

    // show full sync dialog
    CFullSync wndFullSync;
    INT_PTR result = wndFullSync.DoModal();  
}



/**
 * Thread to start the sync process.
 */
DWORD WINAPI syncThread(LPVOID lpParam) {
    
    int ret = 0;

    try {
        ret = startSync();
    }
    catch (SyncException* e) {
        // Catch SyncExceptions:
        //   code 2 = aborted by user (soft termination)
        //   code 3 = Outlook fatal exception
        ret = e->getErrorCode();
    }
    catch (std::exception &e) {
        // Catch STL exceptions: code 7
        CStringA s1 = "Unexpected STL exception: ";
        s1.Append(e.what());
        printLog(s1.GetBuffer(), LOG_ERROR);
        ret = 7;
    }
    catch(...) {
        // Catch other unexpected exceptions.
        CStringA s1;
        s1.LoadString(IDS_UNEXPECTED_EXCEPTION);
        printLog(s1.GetBuffer(), LOG_ERROR);
        ret = 6;            // code 6 = unexpected exception
    }


    Sleep(200);     // Just to be sure that everything has been completed...
    SendMessage(HwndFunctions::getWindowHandle(), ID_MYMSG_STARTSYNC_ENDED, NULL, (LPARAM)ret);


    if (ret) {
        // **** Investigate on this ****
        // In case of COM exceptions, the COM library could not be
        // reused with 'CoInitialize()'. While terminating the thread like this seems
        // working fine...
        TerminateThread(GetCurrentThread(), ret);
    }
    return 0;
}


/**
 * Thread used to kill the syncThread after a timeout.
 * @param lpParam : the syncThread HANDLE
 */
DWORD WINAPI syncThreadKiller(LPVOID lpParam) {

    // Wait on the sync thread (timeout = 5sec)
    int ret = 0;
    HANDLE hSyncThread = lpParam;
    DWORD dwWaitResult = WaitForSingleObject(hSyncThread, TIME_OUT_ABORT * 1000);

    switch (dwWaitResult) {
        // Thread exited -> no need to kill it (should be the usual way).
        case WAIT_ABANDONED:
        case WAIT_OBJECT_0: {
            ret = 0;
            break;
        }
        // Sync is still running after timeout -> kill thread.
        case WAIT_TIMEOUT: {
            hardTerminateSync(hSyncThread);
            SendMessage(HwndFunctions::getWindowHandle(), ID_MYMSG_STARTSYNC_ENDED, NULL, (LPARAM)4);
            ret = 0;
            break;
        }
        // Some error occurred (case WAIT_FAILED)
        default: {
            ret = 1;
            break;
        }
    }

    // To enable again the refresh of statusbar.
    cancelingSync = false;

    return ret;
}




void CMainSyncFrame::OnFileSynchronize() {

    CString s1;
    if(  (!checkSyncInProgress()) ){
        // No sync in progress -> StartSync.
        StartSync();
    }
    else{
        if (getConfig()->getScheduledSync()) {
            // It's running a scheduled sync -> error msg.
            s1.LoadString(IDS_TEXT_SYNC_ALREADY_RUNNING);
            MessageBox(s1);
        }
    }
}


int CMainSyncFrame::OnCancelSync() {
    return 0;
}


void CMainSyncFrame::showSettingsWindow(const int paneToDisplay){

    if(checkSyncInProgress()){
        CString s1;
        s1.LoadString(IDS_ERROR_CANNOT_CHANGE_SETTINGS);
        AfxMessageBox(s1);
        return;
    }

    CDocument* pDoc = NULL;
    pConfigFrame = NULL;

    CSingleDocTemplate* docSettings = ((COutlookPluginApp*)AfxGetApp())->docSettings;

    getConfig()->read();
    
    pDoc = docSettings->CreateNewDocument();
    if (pDoc != NULL)
    {
        // If creation worked, use create a new frame for
        // that document.
        pConfigFrame = (CConfigFrame*)docSettings->CreateNewFrame(pDoc, NULL);
        
        if (pConfigFrame != NULL)
        {
            docSettings->SetDefaultTitle(pDoc);

            // If document initialization fails
            if (!pDoc->OnNewDocument())
            {
                pConfigFrame->DestroyWindow();
                pConfigFrame = NULL;
            }
            //else
            //{
            //    docSettings->InitialUpdateFrame(pConfigFrame, pDoc, TRUE);
            //}
        }
    }

    // if it failed
    if (pConfigFrame == NULL || pDoc == NULL)
    {
        delete pDoc;
        AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
    } 

    pConfigFrame->wndSplitter.SetActivePane(0,0);
    pConfigFrame->wndSplitter.SetColumnInfo(0,65,40);
    //pConfigFrame->wndSplitter.RecalcLayout();        
    docSettings->InitialUpdateFrame(pConfigFrame, pDoc, TRUE);
    

    //select the desired pane to be displayed.
    ((CLeftView*)pConfigFrame->wndSplitter.GetPane(0,0))->selectItem(paneToDisplay);
    
    pConfigFrame->wndSplitter.GetPane(0,1)->SendMessage(WM_PAINT);

    this->BeginModalState(); // TODO: this is required
    configOpened = true;
}

void CMainSyncFrame::OnToolsSetloglevel()
{
    // show the Log Level dialog
    CLogSettings wndLog;
    wndLog.DoModal();
}


LRESULT CMainSyncFrame::OnMsgSyncBegin( WPARAM , LPARAM lParam) {  

    CString s1;
    s1.LoadString(IDS_TEXT_STARTING_SYNC);
    wndStatusBar.SetPaneText(0,s1);

    if (!getConfig()->getScheduledSync()) {
        // hide the menu
        HMENU hMenu = ::GetMenu(GetSafeHwnd());
        int nCount = GetMenuItemCount(hMenu);
        for(int i = 0; i < nCount; i++){
            EnableMenuItem(hMenu, i, MF_BYPOSITION | MF_GRAYED);
        }
        DrawMenuBar();

        ((CSyncForm*)wndSplitter.GetPane(0,1))->iconStatusSync.SetIcon(::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_CANCEL)));
        s1.LoadString(IDS_MAIN_PRESS_TO_CANCEL); 
        ((CSyncForm*)wndSplitter.GetPane(0,1))->SetDlgItemText(IDC_MAIN_MSG_PRESS, s1);
    }
    else{
        // scheduled sync: keep black button
    }

    bSyncStarted = true;
    Invalidate();

    return 0;
}

// UI received a sync end message
LRESULT CMainSyncFrame::OnMsgSyncEnd( WPARAM , LPARAM ) {

    //printLog("msg syncEnd received by UI", LOG_DEBUG);
    CString s1;
    s1.LoadString(IDS_TEXT_SYNC_ENDED);
    wndStatusBar.SetPaneText(0,s1);

    bSyncStarted = false; 
    return 0;
}

// UI received sync source begin message
LRESULT CMainSyncFrame::OnMsgSyncSourceBegin( WPARAM wParam, LPARAM lParam) {

    CString s1;
    currentSource = lParam;
    currentItem = 0;
    
    // if it is scheduled, we change only status bar text
    bool isScheduled = getConfig()->getScheduledSync();

    if(!isScheduled){
        // change controls based on what source is currently syncing
        switch(currentSource){
            case SYNCSOURCE_CONTACTS:
                contactsBegin++;
                ((CSyncForm*)wndSplitter.GetPane(0,1))->GetDlgItem(IDC_MAIN_STATIC_CONTACTS)->ShowWindow(SW_SHOW);
                ((CSyncForm*)wndSplitter.GetPane(0,1))->GetDlgItem(IDC_MAIN_STATIC_STATUS_CONTACTS)->ShowWindow(SW_SHOW);
                ((CSyncForm*)wndSplitter.GetPane(0,1))->iconStatusContacts.Animate();
                ((CSyncForm*)wndSplitter.GetPane(0,1))->paneContacts.SetBitmap(hBmpBlue);
                break;

            case SYNCSOURCE_CALENDAR:
                calendarBegin++;
                ((CSyncForm*)wndSplitter.GetPane(0,1))->GetDlgItem(IDC_MAIN_STATIC_CALENDAR)->ShowWindow(SW_SHOW);
                ((CSyncForm*)wndSplitter.GetPane(0,1))->GetDlgItem(IDC_MAIN_STATIC_STATUS_CALENDAR)->ShowWindow(SW_SHOW);
                ((CSyncForm*)wndSplitter.GetPane(0,1))->iconStatusCalendar.Animate();
                ((CSyncForm*)wndSplitter.GetPane(0,1))->paneCalendar.SetBitmap(hBmpBlue);
                break;

            case SYNCSOURCE_TASKS:
                tasksBegin++;
                ((CSyncForm*)wndSplitter.GetPane(0,1))->GetDlgItem(IDC_MAIN_STATIC_TASKS)->ShowWindow(SW_SHOW);
                ((CSyncForm*)wndSplitter.GetPane(0,1))->GetDlgItem(IDC_MAIN_STATIC_STATUS_TASKS)->ShowWindow(SW_SHOW);
                ((CSyncForm*)wndSplitter.GetPane(0,1))->iconStatusTasks.Animate();
                ((CSyncForm*)wndSplitter.GetPane(0,1))->paneTasks.SetBitmap(hBmpBlue);
                break;

            case SYNCSOURCE_NOTES:
                notesBegin++;
                ((CSyncForm*)wndSplitter.GetPane(0,1))->GetDlgItem(IDC_MAIN_STATIC_NOTES)->ShowWindow(SW_SHOW);
                ((CSyncForm*)wndSplitter.GetPane(0,1))->GetDlgItem(IDC_MAIN_STATIC_STATUS_NOTES)->ShowWindow(SW_SHOW);
                ((CSyncForm*)wndSplitter.GetPane(0,1))->iconStatusNotes.Animate();
                ((CSyncForm*)wndSplitter.GetPane(0,1))->paneNotes.SetBitmap(hBmpBlue);
                break;
        };
    };


    //
    // change text on labels to reflect sync status
    //
    CString msg;
    msg.LoadString(IDS_TEXT_CHECKING);
    if(wParam == 1){
        msg += " Server ";
    }
    else {
        msg += " Outlook ";
    }


    CString source;
    switch(lParam){
        case SYNCSOURCE_CONTACTS:
            source.LoadString(IDS_TEXT_CONTACTS);
            msg+=source;
            if(!isScheduled)
                ((CSyncForm*)wndSplitter.GetPane(0,1))->changeContactsStatus(msg);
            ((CSyncForm*)wndSplitter.GetPane(0,1))->paneContacts.Invalidate();
            break;

        case SYNCSOURCE_CALENDAR:
            source.LoadString(IDS_TEXT_APPOINTMENTS);
            msg+=source;
            if(!isScheduled)
                ((CSyncForm*)wndSplitter.GetPane(0,1))->changeCalendarStatus(msg);
            ((CSyncForm*)wndSplitter.GetPane(0,1))->paneCalendar.Invalidate();
            break;

        case SYNCSOURCE_TASKS:
            source.LoadString(IDS_TEXT_TASKS);
            msg+=source;
            if(!isScheduled)
                ((CSyncForm*)wndSplitter.GetPane(0,1))->changeTasksStatus(msg);
            ((CSyncForm*)wndSplitter.GetPane(0,1))->paneTasks.Invalidate();
            break;

        case SYNCSOURCE_NOTES:
            source.LoadString(IDS_TEXT_NOTES);
            msg+=source;
            if(!isScheduled)
                ((CSyncForm*)wndSplitter.GetPane(0,1))->changeNotesStatus(msg);
            ((CSyncForm*)wndSplitter.GetPane(0,1))->paneNotes.Invalidate();
            break;
    }
    

    // Update status-bar with the same message.
    refreshStatusBar(msg);

    return 0; 
}

// UI received a sync source ended message
LRESULT CMainSyncFrame::OnMsgSyncSourceEnd( WPARAM , LPARAM lParam) {

    CString s1;
    currentSource = lParam;

    // if it is scheduled, we change only status bar text
    if(getConfig()->getScheduledSync()) {
        currentSource = 0;
        return 0;
    }

    switch(currentSource){
     case SYNCSOURCE_CONTACTS:
         ((CSyncForm*)wndSplitter.GetPane(0,1))->iconStatusContacts.StopAnim();
         ((CSyncForm*)wndSplitter.GetPane(0,1))->iconContacts.SetIcon(::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_CONTACTS)));
         ((CSyncForm*)wndSplitter.GetPane(0,1))->paneContacts.SetBitmap(hBmpLight);
         if (contactsBegin == 2) {
            ((CSyncForm*)wndSplitter.GetPane(0,1))->iconStatusContacts.SetIcon(LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_COMPLETE)));
            ((CSyncForm*)wndSplitter.GetPane(0,1))->paneContacts.hPrevStatusIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_COMPLETE));
            s1.LoadString(IDS_DONE);
         }
         else {
             ((CSyncForm*)wndSplitter.GetPane(0,1))->iconStatusContacts.SetIcon(NULL);
             s1.LoadString(IDS_FINISHED_SENDING);
         }
         ((CSyncForm*)wndSplitter.GetPane(0,1))->changeContactsStatus(s1);
         ((CSyncForm*)wndSplitter.GetPane(0,1))->paneContacts.Invalidate();
         break;
     
     case SYNCSOURCE_CALENDAR:
         s1.LoadString(IDS_DONE);
         ((CSyncForm*)wndSplitter.GetPane(0,1))->iconStatusCalendar.StopAnim();
         ((CSyncForm*)wndSplitter.GetPane(0,1))->iconCalendar.SetIcon(::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_CALENDAR)));
         ((CSyncForm*)wndSplitter.GetPane(0,1))->paneCalendar.SetBitmap(hBmpLight);
          if (calendarBegin == 2) {
            ((CSyncForm*)wndSplitter.GetPane(0,1))->iconStatusCalendar.SetIcon(LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_COMPLETE)));
            ((CSyncForm*)wndSplitter.GetPane(0,1))->paneCalendar.hPrevStatusIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_COMPLETE));
            s1.LoadString(IDS_DONE);
         }
         else {
             ((CSyncForm*)wndSplitter.GetPane(0,1))->iconStatusCalendar.SetIcon(NULL);
             s1.LoadString(IDS_FINISHED_SENDING);
         }
         ((CSyncForm*)wndSplitter.GetPane(0,1))->changeCalendarStatus(s1);
         ((CSyncForm*)wndSplitter.GetPane(0,1))->paneCalendar.Invalidate();
         break;
      
     case SYNCSOURCE_TASKS:
         s1.LoadString(IDS_DONE);
         ((CSyncForm*)wndSplitter.GetPane(0,1))->iconStatusTasks.StopAnim();
         ((CSyncForm*)wndSplitter.GetPane(0,1))->iconTasks.SetIcon(::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_TASKS)));
         ((CSyncForm*)wndSplitter.GetPane(0,1))->paneTasks.SetBitmap(hBmpLight);
           if (tasksBegin == 2) {
            ((CSyncForm*)wndSplitter.GetPane(0,1))->iconStatusTasks.SetIcon(LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_COMPLETE)));
            ((CSyncForm*)wndSplitter.GetPane(0,1))->paneTasks.hPrevStatusIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_COMPLETE));
            s1.LoadString(IDS_DONE);
         }
         else {
             ((CSyncForm*)wndSplitter.GetPane(0,1))->iconStatusTasks.SetIcon(NULL);
             s1.LoadString(IDS_FINISHED_SENDING);
         }
         ((CSyncForm*)wndSplitter.GetPane(0,1))->changeTasksStatus(s1);
         ((CSyncForm*)wndSplitter.GetPane(0,1))->paneTasks.Invalidate();
         break;

     case SYNCSOURCE_NOTES:
         s1.LoadString(IDS_DONE);
         ((CSyncForm*)wndSplitter.GetPane(0,1))->iconStatusNotes.StopAnim();
         ((CSyncForm*)wndSplitter.GetPane(0,1))->iconNotes.SetIcon(::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_NOTES)));
         ((CSyncForm*)wndSplitter.GetPane(0,1))->paneNotes.SetBitmap(hBmpLight);
            if (notesBegin == 2) {
            ((CSyncForm*)wndSplitter.GetPane(0,1))->iconStatusNotes.SetIcon(LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_COMPLETE)));
            ((CSyncForm*)wndSplitter.GetPane(0,1))->paneNotes.hPrevStatusIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_COMPLETE));
            s1.LoadString(IDS_DONE);
         }
         else {
             ((CSyncForm*)wndSplitter.GetPane(0,1))->iconStatusNotes.SetIcon(NULL);
             s1.LoadString(IDS_FINISHED_SENDING);
         }
         ((CSyncForm*)wndSplitter.GetPane(0,1))->changeNotesStatus(s1);
         ((CSyncForm*)wndSplitter.GetPane(0,1))->paneNotes.Invalidate();
         break;

    };

    //
    // Invalidating the currentSource, here it's finished.
    //
    currentSource = 0;

    return 0;
}

// UI received a item synced message
LRESULT CMainSyncFrame::OnMsgItemSynced( WPARAM wParam, LPARAM ) {

    currentItem++;

    //
    // Format message: "Sending/Receiving contacts x[/y]..."
    //
    CString statusBarText;
    if(wParam == -1) {
        statusBarText = "Sending ";
    }
    else {
        statusBarText = "Receiving ";
    }

    CString s1;
    switch(currentSource){
        case SYNCSOURCE_CONTACTS:
            s1.LoadString(IDS_TEXT_CONTACTS);
            statusBarText += s1;
            break;
        case SYNCSOURCE_CALENDAR:
            s1.LoadString(IDS_TEXT_APPOINTMENTS);
            statusBarText += s1;
            break;
        case SYNCSOURCE_TASKS:
            s1.LoadString(IDS_TEXT_TASKS);
            statusBarText += s1;
            break;
        case SYNCSOURCE_NOTES:
            s1.LoadString(IDS_TEXT_NOTES);
            statusBarText += s1;
            break;
    }
    statusBarText += " ";

    char* temp =  ltow(currentItem);
    statusBarText += temp;
    delete [] temp; temp = NULL;

    // '-1' received when #ofChanges is not supported.
    if(totalItems > 0){
        statusBarText += "/";
        temp = ltow(totalItems);
        statusBarText += temp;
        delete [] temp; temp = NULL;
    }

    refreshStatusBar(statusBarText);



    // if it is scheduled, we change only status bar text
    if(getConfig()->getScheduledSync()) {
        return 0;
    }

    // change source status
    switch(currentSource){
        case SYNCSOURCE_CONTACTS:
            ((CSyncForm*)wndSplitter.GetPane(0,1))->changeContactsStatus(statusBarText); 
            //((CSyncForm*)wndSplitter.GetPane(0,1))->repaintPaneControls(PANE_TYPE_CONTACTS);
            ((CSyncForm*)wndSplitter.GetPane(0,1))->paneContacts.Invalidate();
            break;
        case SYNCSOURCE_CALENDAR:
            ((CSyncForm*)wndSplitter.GetPane(0,1))->changeCalendarStatus(statusBarText); 
            ((CSyncForm*)wndSplitter.GetPane(0,1))->paneCalendar.Invalidate();
            break;
        case SYNCSOURCE_TASKS:
            ((CSyncForm*)wndSplitter.GetPane(0,1))->changeTasksStatus(statusBarText);    
            ((CSyncForm*)wndSplitter.GetPane(0,1))->paneTasks.Invalidate();
            break;
        case SYNCSOURCE_NOTES:
            ((CSyncForm*)wndSplitter.GetPane(0,1))->changeNotesStatus(statusBarText);
            ((CSyncForm*)wndSplitter.GetPane(0,1))->paneNotes.Invalidate();
            break;           
    }

    //Invalidate(FALSE);
    return 0;
}



afx_msg LRESULT CMainSyncFrame::OnMsgRefreshStatusBar( WPARAM wParam, LPARAM lParam) {

    char text[100];
    text[0] = 0;

    // *** TODO: move messages to UI resources! ***
    switch (lParam) {
        case SBAR_CHECK_ALL_ITEMS: {
            sprintf(text, SBAR_READING_ALLITEMS, (int)wParam);
            break;
        }
        case SBAR_CHECK_MOD_ITEMS: {
            sprintf(text, SBAR_CHECKING_MODITEMS);
            break;
        }
        case SBAR_CHECK_MOD_ITEMS2: {
            sprintf(text, SBAR_CHECKING_MODITEMS2, (int)wParam);
            break;
        }
        case SBAR_WRITE_OLD_ITEMS: {
            sprintf(text, SBAR_WRITING_OLDITEMS);
            break;
        }
        case SBAR_DELETE_CLIENT_ITEMS: {
            char* sourceName;
            if      (currentSource == SYNCSOURCE_CONTACTS) sourceName = CONTACT_;
            else if (currentSource == SYNCSOURCE_CALENDAR) sourceName = APPOINTMENT_;
            else if (currentSource == SYNCSOURCE_TASKS)    sourceName = TASK_;
            else if (currentSource == SYNCSOURCE_NOTES)    sourceName = NOTE_;
            sprintf(text, SBAR_DELETING_ITEMS, sourceName);
            break;
        }
        case SBAR_SENDDATA_BEGIN: {
            sprintf(text, SBAR_SENDING_DATA);
            break;
        }
        case SBAR_RECEIVE_DATA_BEGIN: {
            sprintf(text, SBAR_RECEIVING_DATA);
            break;
        }
        case SBAR_SENDDATA_END: {
            sprintf(text, SBAR_WAITING);
            break;
        }
    }

    CString s1 = text;
    refreshStatusBar(s1);

    // Refresh source labels for some case
    if ( lParam == SBAR_SENDDATA_BEGIN ||
         lParam == SBAR_RECEIVE_DATA_BEGIN ||
         lParam == SBAR_SENDDATA_END ||
         lParam == SBAR_DELETE_CLIENT_ITEMS ) {

        refreshSourceLabels(s1, currentSource);
    }

    return 0;
}



afx_msg LRESULT CMainSyncFrame::OnMsgTotalItems( WPARAM wParam, LPARAM lParam)
{
    totalItems = lParam;
   
    CString source;
    CString msg; 
    switch(currentSource){
        case SYNCSOURCE_CONTACTS:
            source.LoadString(IDS_TEXT_CONTACTS);
            break;
        case SYNCSOURCE_CALENDAR:
            source.LoadString(IDS_TEXT_APPOINTMENTS);
            break;
        case SYNCSOURCE_TASKS:
            source.LoadString(IDS_TEXT_TASKS);
            break;
        case SYNCSOURCE_NOTES:
            source.LoadString(IDS_TEXT_NOTES);
            break;
    };
    
    if(wParam == 1){
        msg.LoadString(IDS_TEXT_RECEIVING);
    }
    else
        msg.LoadString(IDS_TEXT_SENDING);
    msg+=" "; msg+=source;
    refreshStatusBar(msg);

    //Invalidate();
    return 0;
}

// the config window has closed, and the user is returned to the main window
void CMainSyncFrame::OnConfigClosed(){
    EndModalState();
    SetForegroundWindow();
    configOpened = false;
    ((CSyncForm*)wndSplitter.GetPane(0,1))->refreshSources();
};

LRESULT CMainSyncFrame::OnMsgStartSyncBegin(WPARAM wParam, LPARAM lParam){
    return 0;
}


/**
 * Message received when sync thread has exited.
 * 'lParam' is the exitThread code (0 if no errors).
 * Here errors of sync process are managed, and then UI refreshed.
 */
LRESULT CMainSyncFrame::OnMsgStartsyncEnded(WPARAM wParam, LPARAM lParam){
    
    CString s1;
    _bstr_t bst;
    int exitCode = lParam;
    const bool isScheduled = getConfig()->getScheduledSync();
    
    // Sync has finished: unlock buttons
    cancelingSync = false;
    ((CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1))->unlockButtons();
    
    //
    // Error occurred: display error message on a msgBox.
    //
    if (exitCode) {
        BeginModalState();

        manageSyncErrorMsg(exitCode);

        EndModalState();
        SetForegroundWindow();
    }
    else{
        // exitCode = 0 : sync finished ok
    }

    //
    // Open settings window if error on invalid credentials.
    //
    if ( (!isScheduled) &&
         (exitCode == 407  ||                   // 407  = Auth failed
          exitCode == 401  ||                   // 401  = Wrong credentials
          exitCode == 404  ||                   // 404  = not found
          exitCode == 2001 ||                   // 2001 = HTTP connection error
          exitCode == 2060 ||                   // 2060 = HTTP resource not found (status 404)
          exitCode == 2102) ) {                 // 2102 = No sources to sync

        if (exitCode == 404 ||
            exitCode == 2102) {
            showSettingsWindow(1);              // -> show Sync settings
        }
        else  {
            showSettingsWindow(0);              // -> show Account settings
        }

    }

    s1.LoadString(IDS_TEXT_SYNC_ENDED);
    refreshStatusBar(s1);

    ((CSyncForm*)wndSplitter.GetPane(0,1))->paneSync.SetBitmap(hBmpDark);
    ((CSyncForm*)wndSplitter.GetPane(0,1))->iconStatusSync.SetIcon(::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_SYNC_ALL_BLUE)));

    s1.LoadString(IDS_MAIN_PRESS_TO_SYNC); 
    ((CSyncForm*)wndSplitter.GetPane(0,1))->SetDlgItemText(IDC_MAIN_MSG_PRESS, s1);


    // Correct source status in case of ClientException (code 3), killed thread (code 4), 
    // unexpected exception (code 6 & 7)
    // *** TODO: change "notSynced" to "Failed"! or add a state ***
    if (exitCode == 3 || exitCode == 4 || exitCode == 6 || exitCode == 7) {
        if(strcmp(getConfig()->getSyncSourceConfig(CONTACT_)->getSync(),"none") != 0)
            ((CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1))->syncSourceContactState = SYNCSOURCE_STATE_NOT_SYNCED;
        if(strcmp(getConfig()->getSyncSourceConfig(APPOINTMENT_)->getSync(),"none") != 0)
            ((CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1))->syncSourceCalendarState = SYNCSOURCE_STATE_NOT_SYNCED;
        if(strcmp(getConfig()->getSyncSourceConfig(TASK_)->getSync(),"none") != 0)
            ((CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1))->syncSourceTaskState = SYNCSOURCE_STATE_NOT_SYNCED;
        if(strcmp(getConfig()->getSyncSourceConfig(NOTE_)->getSync(),"none") != 0)
            ((CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1))->syncSourceNoteState = SYNCSOURCE_STATE_NOT_SYNCED;
    }
    else if (exitCode == 5){
        // user avoided full sync, set canceled state
        // set sync source status
        if(strcmp(getConfig()->getSyncSourceConfig(CONTACT_)->getSync(),"none") != 0){
            ((CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1))->syncSourceContactState = SYNCSOURCE_STATE_CANCELED;
        }
        if(strcmp(getConfig()->getSyncSourceConfig(APPOINTMENT_)->getSync(),"none") != 0){
            ((CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1))->syncSourceCalendarState = SYNCSOURCE_STATE_CANCELED;
        }
        if(strcmp(getConfig()->getSyncSourceConfig(TASK_)->getSync(),"none") != 0){
            ((CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1))->syncSourceTaskState = SYNCSOURCE_STATE_CANCELED;
        }
        if(strcmp(getConfig()->getSyncSourceConfig(NOTE_)->getSync(),"none") != 0){
            ((CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1))->syncSourceNoteState = SYNCSOURCE_STATE_CANCELED;
        }
    }


    // show the menu
    HMENU hMenu = ::GetMenu(GetSafeHwnd());
    int nCount = GetMenuItemCount(hMenu);
    for(int i = 0; i < nCount; i++){
        EnableMenuItem(hMenu, i, MF_BYPOSITION | MF_ENABLED);
    }
    DrawMenuBar();

    UpdateWindow();


    if (isScheduled) {
        // Scheduled sync: current config is out of date -> read ALL from winreg.
        getConfig()->read();
    }
    else {
        if (getConfig()->getFullSync()) {
            // Full sync: read original syncModes from winreg.
            getConfig()->readSyncModes();
        }
        else {
            // Normal sync: restore original syncModes (a pane could be clicked).
            // **** TODO: USE CONFIG IN MEMORY, LIKE IN FULL-SYNC! Don't keep'em in UI ****
            restoreSyncModeSettings();
        }
    }

    // Refresh sources.
    ((CSyncForm*)wndSplitter.GetPane(0,1))->refreshSources();
    SetForegroundWindow();

    Invalidate(FALSE);
    bSyncStarted = false;
    return 0;
}

void CMainSyncFrame::StartSync(){

    CString s1;
    currentSource = 0;
    currentItem = 0;
    totalItems = 0;
    printLog("StartSync begin", LOG_DEBUG);

    // Check on sync in progress.
    if (checkSyncInProgress()) {
        s1.LoadString(IDS_TEXT_SYNC_ALREADY_RUNNING);
        MessageBox(s1);
        return;
    }

    // Check if connection settings are valid.
    if(! checkConnectionSettings()) {
        s1.LoadString(IDS_ERROR_SET_CONNECTION);
        AfxMessageBox(s1);
        showSettingsWindow(0);          // 0 = 'Account Settings' pane.
        return;
    }

    // Lock the UI buttons.
    ((CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1))->lockButtons();

    // Reset counters.
    contactsBegin = 0;
    calendarBegin = 0;
    tasksBegin = 0;
    notesBegin = 0;

    // Hide the menu.
    printLog("Hide menu", LOG_DEBUG);
    
    HMENU hMenu = ::GetMenu(GetSafeHwnd());
    int nCount = GetMenuItemCount(hMenu);
    for(int i = 0; i < nCount; i++){
        EnableMenuItem(hMenu, i, MF_BYPOSITION | MF_GRAYED);
    }
    DrawMenuBar();


    //
    // Clear source state for sources to sync, clear status icons.
    //
    if(strcmp(getConfig()->getSyncSourceConfig(CONTACT_)->getSync(), "none") != 0) {
        ((CSyncForm*)wndSplitter.GetPane(0,1))->syncSourceContactState = SYNCSOURCE_STATE_OK;
        ((CSyncForm*)wndSplitter.GetPane(0,1))->iconStatusContacts.SetIcon(NULL);
    }
    if(strcmp(getConfig()->getSyncSourceConfig(APPOINTMENT_)->getSync(), "none") != 0) {
        ((CSyncForm*)wndSplitter.GetPane(0,1))->syncSourceCalendarState = SYNCSOURCE_STATE_OK;
        ((CSyncForm*)wndSplitter.GetPane(0,1))->iconStatusCalendar.SetIcon(NULL);
    }
    if(strcmp(getConfig()->getSyncSourceConfig(TASK_)->getSync(), "none") != 0) {
        ((CSyncForm*)wndSplitter.GetPane(0,1))->syncSourceTaskState = SYNCSOURCE_STATE_OK;
        ((CSyncForm*)wndSplitter.GetPane(0,1))->iconStatusTasks.SetIcon(NULL);
    }
    if(strcmp(getConfig()->getSyncSourceConfig(NOTE_)->getSync(), "none") != 0) {
        ((CSyncForm*)wndSplitter.GetPane(0,1))->syncSourceNoteState = SYNCSOURCE_STATE_OK;
        ((CSyncForm*)wndSplitter.GetPane(0,1))->iconStatusNotes.SetIcon(NULL);
    }

    //
    // Refresh of main UI.
    //
    ((CSyncForm*)wndSplitter.GetPane(0,1))->refreshSources();
    ((CSyncForm*)wndSplitter.GetPane(0,1))->iconStatusSync.SetIcon(::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_CANCEL)));
    s1.LoadString(IDS_MAIN_PRESS_TO_CANCEL); 
    ((CSyncForm*)wndSplitter.GetPane(0,1))->SetDlgItemText(IDC_MAIN_MSG_PRESS, s1);
    ((CSyncForm*)wndSplitter.GetPane(0,1))->Invalidate();

    // Set state to panes
    ((CSyncForm*)wndSplitter.GetPane(0,1))->paneContacts.state = STATE_SYNC;
    ((CSyncForm*)wndSplitter.GetPane(0,1))->paneCalendar.state = STATE_SYNC;
    ((CSyncForm*)wndSplitter.GetPane(0,1))->paneTasks.state = STATE_SYNC;
    ((CSyncForm*)wndSplitter.GetPane(0,1))->paneNotes.state = STATE_SYNC;
    

    //
    // Start the sync thread.
    //
    printLog("Starting the syncThread...", LOG_DEBUG);
    bSyncStarted = true;
    cancelingSync = false;
    hSyncThread = CreateThread(NULL, 0, syncThread, 0, 0, &dwThreadId);
    if (hSyncThread == NULL) {
        DWORD errorCode = GetLastError();
        CString s1 = "Thread error: syncThread";
        AfxMessageBox(s1);
        return;
    }
}



int CMainSyncFrame::CancelSync(){
    int ret = 1;
    CString msg;
    CString s1;

    // This will avoid clicking 2 times on cancel sync.
    if (cancelingSync) {
        s1.LoadString(IDS_TEXT_SYNC_ALREADY_RUNNING);
        MessageBox(s1);
        return ret;
    }

    //
    // Display warning.
    //
    unsigned int flags = MB_YESNO | MB_ICONQUESTION | MB_SETFOREGROUND | MB_APPLMODAL;
    int selected = MessageBox(WMSG_BOX_CANCEL_SYNC, WPROGRAM_NAME, flags);


    // First check again if sync is running (could be terminated in the meanwhile...)
    if (!checkSyncInProgress()) {
        return ret;
    }

    if (selected == IDYES) {
        ret = 0;

        // Refresh status bar
        CString s1;
        s1.LoadString(IDS_TEXT_CANCELING_SYNC);
        refreshStatusBar(s1);

        // LOCK the statusbar and other controls.
        cancelingSync = true;
        ((CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1))->lockButtons();

        // First we try to terminate the sync in a soft way.
        softTerminateSync();
        
        // If it fails, this thread will kill the syncThread (after a timeout).
        DWORD killerThreadId;
        HANDLE h = CreateThread(NULL, 0, syncThreadKiller, hSyncThread, 0, &killerThreadId);

        bSyncStarted = false;

        ((CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1))->OnNcPaint();

        // show the menu
        HMENU hMenu = ::GetMenu(GetSafeHwnd());
        int nCount = GetMenuItemCount(hMenu);
        for(int i = 0; i < nCount; i++){
            EnableMenuItem(hMenu, i, MF_BYPOSITION | MF_ENABLED);
        }
        DrawMenuBar();

        ((CSyncForm*)wndSplitter.GetPane(0,1))->refreshSources();

        ((CSyncForm*)wndSplitter.GetPane(0,1))->iconStatusSync.SetIcon(::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_CANCEL)));
        s1.LoadString(IDS_MAIN_PRESS_TO_SYNC); 
        ((CSyncForm*)wndSplitter.GetPane(0,1))->SetDlgItemText(IDC_MAIN_MSG_PRESS, s1);

        // set sync source status
        if(strcmp(getConfig()->getSyncSourceConfig(CONTACT_)->getSync(),"none") != 0){
            ((CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1))->syncSourceContactState = SYNCSOURCE_STATE_CANCELED;
        }
        if(strcmp(getConfig()->getSyncSourceConfig(APPOINTMENT_)->getSync(),"none") != 0){
            ((CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1))->syncSourceCalendarState = SYNCSOURCE_STATE_CANCELED;
        }
        if(strcmp(getConfig()->getSyncSourceConfig(TASK_)->getSync(),"none") != 0){
            ((CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1))->syncSourceTaskState = SYNCSOURCE_STATE_CANCELED;
        }
        if(strcmp(getConfig()->getSyncSourceConfig(NOTE_)->getSync(),"none") != 0){
            ((CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1))->syncSourceNoteState = SYNCSOURCE_STATE_CANCELED;
        }

        //
        // ***TODO*** is this call necessary?
        // Restore of syncModes is (always) called on "OnMsgStartsyncEnded()"
        //
        restoreSyncModeSettings(); // restore changes made by clicking 'sync' link

        Invalidate();
        currentSource = 0;
        currentItem = 0;
        totalItems = 0;
    }    
    
    return ret;    
}

// handling for minimizing/restoring the UI when the config is opened
BOOL CMainSyncFrame::OnNcActivate(BOOL bActive){
    // needs special handling only when the config window is opened
    if(configOpened){
        if( (bActive) && (pConfigFrame != NULL))
            if( (! this->IsWindowEnabled()) && 
                (pConfigFrame->IsWindowVisible() ) 
                //(pConfigFrame->IsWindowVisible()) //&&
                //(pConfigFrame->IsWindowEnabled())
                )
            {
                pConfigFrame->SetForegroundWindow();
                pConfigFrame->SetFocus();
            };
    };

    CFrameWnd::OnNcActivate(bActive);
    return TRUE;
} 

void CMainSyncFrame::OnClose(){

    // CancelSync only if sync in progress AND not a scheduled one!
    // (if scheduled, the sync will continue in bkground)
    if( (checkSyncInProgress()) && (!getConfig()->getScheduledSync()) ) {

        if (CancelSync()) {
            // cancelled
            return;
        }
    }

    closeClient();
    CFrameWnd::OnClose();
}


void CMainSyncFrame::backupSyncModeSettings(){
    syncModeContacts = getSyncModeCode(getConfig()->getSyncSourceConfig(CONTACT_)->getSync());
    syncModeCalendar = getSyncModeCode(getConfig()->getSyncSourceConfig(APPOINTMENT_)->getSync());
    syncModeTasks = getSyncModeCode(getConfig()->getSyncSourceConfig(TASK_)->getSync());
    syncModeNotes = getSyncModeCode(getConfig()->getSyncSourceConfig(NOTE_)->getSync());
}

void CMainSyncFrame::restoreSyncModeSettings(){

    if(syncModeContacts != -1)
        getConfig()->getSyncSourceConfig(CONTACT_)->setSync(syncModeName((SyncMode)syncModeContacts));
    if(syncModeCalendar != -1)
        getConfig()->getSyncSourceConfig(APPOINTMENT_)->setSync(syncModeName((SyncMode)syncModeCalendar));
    if(syncModeTasks    != -1)
        getConfig()->getSyncSourceConfig(TASK_)->setSync(syncModeName((SyncMode)syncModeTasks));
    if(syncModeNotes    != -1)
        getConfig()->getSyncSourceConfig(NOTE_)->setSync(syncModeName((SyncMode)syncModeNotes));

    // Save ONLY sync-modes of each source, if necessary.
    if ( syncModeContacts != -1 || 
         syncModeCalendar != -1 ||
         syncModeTasks    != -1 ||
         syncModeNotes    != -1 ) {
        getConfig()->saveSyncModes();
    }
    
    syncModeContacts = -1; syncModeCalendar = -1; syncModeTasks = -1; syncModeNotes = -1;
}


bool CMainSyncFrame::checkConnectionSettings()
{
    bool isOk = true;

    // first check if the server URL is not empty
    if (strcmp(getConfig()->getAccessConfig().getSyncURL(), "") == 0)
        isOk = false;

    if( (strcmp(getConfig()->getAccessConfig().getUsername(), "") == 0) ||
        (strcmp(getConfig()->getAccessConfig().getPassword(), "") == 0) )
        isOk = false;

    return isOk;
}


LRESULT CMainSyncFrame::OnMsgSyncSourceState(WPARAM wParam, LPARAM lParam) {

    CSyncForm* syncForm = (CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1);

    if(wParam == SYNCSOURCE_CONTACTS) {
        syncForm->iconStatusContacts.StopAnim();
        syncForm->syncSourceContactState = lParam;
        // Update the status icon (funzilla #2110)
        if (lParam == SYNCSOURCE_STATE_OK)
            syncForm->iconStatusContacts.SetIcon(LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_OK)));
        else 
            syncForm->iconStatusContacts.SetIcon(LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ALERT)));
    }

    if(wParam == SYNCSOURCE_CALENDAR) {
        syncForm->iconStatusCalendar.StopAnim();
        syncForm->syncSourceCalendarState = lParam;
        // Update the status icon (funzilla #2110)
        if (lParam == SYNCSOURCE_STATE_OK)
            syncForm->iconStatusCalendar.SetIcon(LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_OK)));
        else 
            syncForm->iconStatusCalendar.SetIcon(LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ALERT)));
    }

    if(wParam == SYNCSOURCE_TASKS) {
        syncForm->iconStatusTasks.StopAnim();
        syncForm->syncSourceTaskState = lParam;
        // Update the status icon (funzilla #2110)
        if (lParam == SYNCSOURCE_STATE_OK)
            syncForm->iconStatusTasks.SetIcon(LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_OK)));
        else 
            syncForm->iconStatusTasks.SetIcon(LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ALERT)));
    }

    if(wParam == SYNCSOURCE_NOTES) {
        syncForm->iconStatusNotes.StopAnim();
        syncForm->syncSourceNoteState = lParam;
        // Update the status icon (funzilla #2110)
        if (lParam == SYNCSOURCE_STATE_OK)
            syncForm->iconStatusNotes.SetIcon(LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_OK)));
        else 
            syncForm->iconStatusNotes.SetIcon(LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ALERT)));

    }

    return 0;
}

/**
 * Used to re-enable UI buttons (called after 'continueAfterPrepareSync()' method).
 */
LRESULT CMainSyncFrame::OnMsgUnlockButtons(WPARAM wParam, LPARAM lParam) {

    ((CSyncForm*)((CMainSyncFrame*)AfxGetMainWnd())->wndSplitter.GetPane(0,1))->unlockButtons();

    return 0;
}