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

// NotesSettings.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "NotesSettings.h"
#include "MainSyncFrm.h"
#include "ClientUtil.h"
#include "SettingsHelper.h"

#include "winmaincpp.h"
#include "utils.h"
#include "comutil.h"
#include "OutlookPlugin.h"
#include "UICustomization.h"

#include <string>

using namespace std;
// CNotesSettings

static wstring outlookFolder;
static CNotesSettings* wndNotes;
static HANDLE handleThread;

IMPLEMENT_DYNCREATE(CNotesSettings, CDialog)

CNotesSettings::CNotesSettings()
	: CDialog(CNotesSettings::IDD)
{
    handleThread = NULL;
}

CNotesSettings::~CNotesSettings()
{
    // clean stuff used in the select Outlook folder thread
    wndNotes = NULL;
    if (handleThread) {
        TerminateThread(handleThread, -1);
        CloseHandle(handleThread);
        handleThread = NULL;
    }
}

void CNotesSettings::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_NOTES_COMBO_SYNCTYPE, lstSyncType);
    DDX_Control(pDX, IDC_NOTES_EDIT_FOLDER, editFolder);
    DDX_Control(pDX, IDC_NOTES_CHECK_INCLUDE, checkInclude);
    DDX_Control(pDX, IDC_NOTES_BUT_SELECT, butSelectFolder);
    DDX_Control(pDX, IDC_NOTES_CHECK_INCLUDE, checkInclude);
    DDX_Control(pDX, IDC_NOTES_EDIT_REMOTE, editRemote);
    DDX_Control(pDX, IDC_NOTES_GROUP_DIRECTION, groupDirection);
    DDX_Control(pDX, IDC_NOTES_GROUP_FOLDER, groupFolder);
    DDX_Control(pDX, IDC_NOTES_GROUP_ADVANCED, groupAdvanced);
    DDX_Control(pDX, IDC_NOTES_RADIO_SIF, radioSif);
    DDX_Control(pDX, IDC_NOTES_RADIO_VNOTE, radioVNote);

    DDX_Control(pDX, IDC_NOTES_CHECK_SHARED, checkShared);
}

BEGIN_MESSAGE_MAP(CNotesSettings, CDialog)
    ON_BN_CLICKED(IDC_NOTES_OK, &CNotesSettings::OnBnClickedNotesOk)
    ON_BN_CLICKED(IDC_NOTES_CANCEL, &CNotesSettings::OnBnClickedNotesCancel)
    ON_BN_CLICKED(IDC_NOTES_BUT_SELECT, &CNotesSettings::OnBnClickedNotesButSelect)
    ON_BN_CLICKED(IDC_NOTES_RADIO_VNOTE, &CNotesSettings::OnBnClickedNotesRadioVNote)
    ON_BN_CLICKED(IDC_NOTES_RADIO_SIF, &CNotesSettings::OnBnClickedNotesRadioSif)
    ON_BN_CLICKED(IDC_NOTES_CHECK_SHARED, &CNotesSettings::OnBnClickedNotesCheckShared)
END_MESSAGE_MAP()

// CNotesSettings diagnostics

#ifdef _DEBUG
void CNotesSettings::AssertValid() const
{
	CDialog::AssertValid();
}

#ifndef _WIN32_WCE
void CNotesSettings::Dump(CDumpContext& dc) const
{
	CDialog::Dump(dc);
}
#endif
#endif //_DEBUG


// CNotesSettings message handlers

BOOL CNotesSettings::OnInitDialog(){
    CString s1;
    s1.LoadString(IDS_NOTES_DETAILS); SetWindowText(s1);
    CDialog::OnInitDialog();
    
    WindowsSyncSourceConfig* ssconf = ((OutlookConfig*)getConfig())->getSyncSourceConfig(NOTE_);

    editFolder.SetLimitText(EDIT_TEXT_MAXLENGTH);
    editRemote.SetLimitText(EDIT_TEXT_MAXLENGTH);

    // Load the syncmodes in the editbox/dropdown
    loadSyncModesBox(NOTE_);
    
    // load string resources
    s1.LoadString(IDS_SYNC_DIRECTION); SetDlgItemText(IDC_NOTES_GROUP_DIRECTION, s1);
    s1.LoadString(IDS_NOTES_FOLDER); SetDlgItemText(IDC_NOTES_GROUP_FOLDER, s1);
    s1.LoadString(IDS_CURRENT); SetDlgItemText(IDC_NOTES_STATIC_FOLDER, s1);
    s1.LoadString(IDS_INCLUDE_SUBFOLDERS); SetDlgItemText(IDC_NOTES_CHECK_INCLUDE, s1);
    s1.LoadString(IDS_SELECT_FOLDER); SetDlgItemText(IDC_NOTES_BUT_SELECT, s1);
    s1.LoadString(IDS_REMOTE_NAME); SetDlgItemText(IDC_NOTES_STATIC_REMOTE, s1);
    s1.LoadString(IDS_DATA_FORMAT); SetDlgItemText(IDC_NOTES_STATIC_DATAFORMAT, s1);
    s1.LoadString(IDS_USE_SIF); SetDlgItemText(IDC_NOTES_STATIC_DATAFORMAT2, s1);
    s1.LoadString(IDS_USE_SIF);    SetDlgItemText(IDC_NOTES_RADIO_SIF,   s1);
    s1.LoadString(IDS_USE_VNOTE);  SetDlgItemText(IDC_NOTES_RADIO_VNOTE, s1);
    s1.LoadString(IDS_ADVANCED); SetDlgItemText(IDC_NOTES_GROUP_ADVANCED, s1);
    s1.LoadString(IDS_SHARED); SetDlgItemText(IDC_NOTES_CHECK_SHARED, s1);

    s1.LoadString(IDS_OK); SetDlgItemText(IDC_NOTES_OK, s1);
    s1.LoadString(IDS_CANCEL); SetDlgItemText(IDC_NOTES_CANCEL, s1);

    // load settings from Registry
    lstSyncType.SetCurSel(getSyncTypeIndex(ssconf->getSync()));
    
    // Get folder path.
    // Note: use 'toWideChar' because we need UTF-8 conversion.
    WCHAR* olFolder = toWideChar(ssconf->getFolderPath());
    s1 = olFolder;
    delete [] olFolder;
    
    try {
        if(s1 == ""){
            s1 = getDefaultFolderPath(NOTE).data();
        }
    }
    catch (...){
        // an exception occured while trying to get the default folder
    	EndDialog(-1);
    }
    
    SetDlgItemText(IDC_NOTES_EDIT_FOLDER, s1);

    if(ssconf->getUseSubfolders())
        checkInclude.SetCheck(BST_CHECKED);

    // Note: use 'toWideChar' because we need UTF-8 conversion.
    WCHAR* remName = toWideChar(ssconf->getURI());
    s1 = remName;
    delete [] remName;
    SetDlgItemText(IDC_NOTES_EDIT_REMOTE, s1);

    if (s1.Right(wcslen(SHARED_SUFFIX)).Compare(SHARED_SUFFIX) == 0) {
        checkShared.SetCheck(BST_CHECKED);
    }

    wndNotes = this;

    if( strstr(ssconf->getType(),"sif") ){
        radioSif.SetCheck(BST_CHECKED);
        currentRadioChecked = SIF_CHECKED;
    }
    else {
        radioVNote.SetCheck(BST_CHECKED);
        currentRadioChecked = VNOTE_CHECKED;
    }

    // Apply customizations
    bool shared             = UICustomization::shared;
    bool forceUseSubfolders = UICustomization::forceUseSubfolders;
    bool hideDataFormats    = UICustomization::hideDataFormats;
    bool hideAllAdvanced    = !SHOW_ADVANCED_SETTINGS;

    if (!shared) {
        GetDlgItem(IDC_NOTES_CHECK_SHARED)->ShowWindow(SW_HIDE);
    } else {
        editRemote.EnableWindow(false);
    }

    if (forceUseSubfolders) {
        checkInclude.SetCheck(BST_CHECKED);
        checkInclude.ShowWindow(SW_HIDE);

        // Resize things
        CRect rect;
        checkInclude.GetClientRect(&rect);
        int dy = -1 * (rect.Height() + 5);

        resizeItem(GetDlgItem(IDC_NOTES_GROUP_FOLDER), 0, dy);

        moveItem(this, &groupAdvanced, 0, dy);
        moveItem(this, &editRemote,    0, dy);
        moveItem(this, &radioSif,      0, dy);
        moveItem(this, &radioVNote,    0, dy);
        moveItem(this, &checkShared,   0, dy);
        moveItem(this, GetDlgItem(IDC_NOTES_STATIC_REMOTE),     0, dy);
        moveItem(this, GetDlgItem(IDC_NOTES_STATIC_DATAFORMAT), 0, dy);
        moveItem(this, GetDlgItem(IDC_NOTES_STATIC_DATAFORMAT2), 0, dy);
        moveItem(this, GetDlgItem(IDC_NOTES_OK),             0, dy);
        moveItem(this, GetDlgItem(IDC_NOTES_CANCEL),         0, dy);

        setWindowHeight(this, GetDlgItem(IDC_NOTES_OK));
    }

    if (hideAllAdvanced) {
        groupAdvanced.ShowWindow(SW_HIDE);
        editRemote.ShowWindow(SW_HIDE);
        radioSif.ShowWindow(SW_HIDE);
        radioVNote.ShowWindow(SW_HIDE);
        GetDlgItem(IDC_NOTES_STATIC_REMOTE)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_NOTES_STATIC_DATAFORMAT)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_NOTES_STATIC_DATAFORMAT2)->ShowWindow(SW_HIDE);

        CRect rect;
        groupAdvanced.GetClientRect(&rect);
        int dy = -1 * (rect.Height() + 10);

        moveItem(this, GetDlgItem(IDC_NOTES_OK),     0, dy);
        moveItem(this, GetDlgItem(IDC_NOTES_CANCEL), 0, dy);

        setWindowHeight(this, GetDlgItem(IDC_NOTES_OK));
    } else if (hideDataFormats) {
        radioSif.ShowWindow(SW_HIDE);
        radioVNote.ShowWindow(SW_HIDE);
        GetDlgItem(IDC_NOTES_STATIC_DATAFORMAT)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_NOTES_STATIC_DATAFORMAT2)->ShowWindow(SW_HIDE);

        // Resize things
        CRect rect;
        GetDlgItem(IDC_NOTES_STATIC_DATAFORMAT)->GetClientRect(&rect);
        int dy = -1 * (rect.Height() + 5);

        resizeItem(&groupAdvanced, 0, dy);

        moveItem(this, GetDlgItem(IDC_NOTES_OK),     0, dy);
        moveItem(this, GetDlgItem(IDC_NOTES_CANCEL), 0, dy);

        setWindowHeight(this, GetDlgItem(IDC_NOTES_OK));
    }

    // disable windows xp theme, otherwise any color setting for groupbox
    // will be overriden by the theme settings
    if(((COutlookPluginApp*)AfxGetApp())->hLib){
        PFNSETWINDOWTHEME pfnSetWindowTheme =
            (PFNSETWINDOWTHEME)GetProcAddress(((COutlookPluginApp*)AfxGetApp())->hLib, "SetWindowTheme");
        pfnSetWindowTheme (groupDirection.m_hWnd,L" ",L" ");
        pfnSetWindowTheme (groupFolder.m_hWnd,L" ",L" ");
        pfnSetWindowTheme (groupAdvanced.m_hWnd,L" ",L" ");
    };
    
    // Accessing Outlook, could be no more in foreground
    SetForegroundWindow();

    return FALSE;
}


void CNotesSettings::OnBnClickedNotesOk()
{
    // OK Button
    if(saveSettings(false)){
        CDialog::OnOK();
    }
}

void CNotesSettings::OnBnClickedNotesCancel()
{
    // Never read from winreg, will save when 'OK' is clicked on SyncSettings.
    //getConfig()->read();
    CDialog::OnCancel();
}

bool CNotesSettings::saveSettings(bool saveToDisk)
{
    CString remoteName, outlookFolder, syncType;
    CString s1;
    _bstr_t bst;
    WindowsSyncSourceConfig* ssconf = ((OutlookConfig*)getConfig())->getSyncSourceConfig(NOTE_);

    GetDlgItemText(IDC_NOTES_EDIT_REMOTE, remoteName);
    GetDlgItemText(IDC_NOTES_EDIT_FOLDER, outlookFolder);

    // change values
    if(remoteName == ""){
        s1.LoadString(IDS_ERROR_SET_REMOTE_NAME);
        wsafeMessageBox(s1);
        return false;
    };

    if (UICustomization::showWarningOnChangeFromOneWay) {
        int currentSyncType = getSyncTypeIndex(ssconf->getSync());
        int newSyncType = lstSyncType.GetCurSel();
        if (checkOneWayToTwoWay(currentSyncType, newSyncType)) {
           return false;
        }
    }

    // sync source enabled
    ssconf->setSync(getSyncTypeName(lstSyncType.GetCurSel()));

    // Note: use 'toMultibyte' which uses charset UTF-8.
    //       (when writing to winreg, toWideChar is then called)
    char* olFolder = toMultibyte(outlookFolder.GetBuffer());
    if (olFolder) {
        // If folder has changed, clear anchors
        if (UICustomization::clearAnchorsOnFolderChange) {
            const char * original = ssconf->getFolderPath();
            if (strcmp(original, olFolder) != 0 && UICustomization::clearAnchorsOnFolderChange) {
                ssconf->setLast(0);
                ssconf->setEndTimestamp(0);
            }
        }
        
        ssconf->setFolderPath(olFolder);
        delete [] olFolder;
    }

    if(checkInclude.GetCheck() == BST_CHECKED || UICustomization::forceUseSubfolders)
        ssconf->setUseSubfolders(true);
    else
        ssconf->setUseSubfolders(false);

    char* remName = toMultibyte(remoteName.GetBuffer());
    if (remName) {
        ssconf->setURI(remName);
        delete [] remName;
    }

    // Data formats
    if(radioSif.GetCheck() == BST_CHECKED){
        char* version = toMultibyte(SIF_VERSION);
        ssconf->setType("text/x-s4j-sifn");
        ssconf->setVersion(version);
        ssconf->setEncoding("b64");
        delete [] version;
    }
    else{
        char* version = toMultibyte(VNOTE_VERSION);
        ssconf->setType("text/x-vnote"); 
        ssconf->setVersion(version);
        // When encryption is used, encoding is always 'base64'.
        if ( !strcmp(ssconf->getEncryption(), "") ) {
            ssconf->setEncoding("bin");
        }
        else {
            ssconf->setEncoding("b64");
        }
        delete [] version;
    }

    // Never save to winreg, will save when 'OK' is clicked on SyncSettings.
    //if(saveToDisk)
    //    ((OutlookConfig*)getConfig())->save();

    return true;
}

int pickFolderNotes(){

    CString s1;
    try {
        outlookFolder = pickOutlookFolder(NOTE);
    }
    catch (...) {
        printLog("Exception thrown by pickOutlookFolder", LOG_DEBUG);
        outlookFolder = L"";
    }

    if (wndNotes) {     // dialog could have been closed...
        if (outlookFolder != L""){
            s1 = outlookFolder.data();
            wndNotes->SetDlgItemText(IDC_NOTES_EDIT_FOLDER, s1);
        }
        wndNotes->SetForegroundWindow();
        wndNotes->EndModalState();
    }
    return 0;
}

void CNotesSettings::OnBnClickedNotesButSelect()
{
    outlookFolder = L"";
    BeginModalState();
    handleThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)pickFolderNotes, NULL, 0, NULL);
}

void CNotesSettings::OnBnClickedNotesRadioSif() {
    if (currentRadioChecked != SIF_CHECKED) {
        SetDlgItemText(IDC_NOTES_EDIT_REMOTE, SIFN_DEFAULT_NAME);
        currentRadioChecked = SIF_CHECKED;
    }
}

void CNotesSettings::OnBnClickedNotesRadioVNote() {
    if (currentRadioChecked != VNOTE_CHECKED) {
        SetDlgItemText(IDC_NOTES_EDIT_REMOTE, VNOTE_DEFAULT_NAME);
        currentRadioChecked = VNOTE_CHECKED;
    }
}

void CNotesSettings::OnBnClickedNotesCheckShared() {
    long editId = IDC_NOTES_EDIT_REMOTE;

    CString currentValue;
    GetDlgItemText(editId, currentValue);
    CString warningMessage;
    warningMessage.LoadString(IDS_UNCHECK_SHARED);

    CString newValue = processSharedCheckboxClick(NOTES_REMOTE_NAME,
         checkShared.GetCheck() != 0, currentValue, warningMessage);

    SetDlgItemText(editId, newValue);
}


void CNotesSettings::loadSyncModesBox(const char* sourceName)
{
    OutlookConfig* config = getConfig();
    WindowsSyncSourceConfig* ssconf = config->getSyncSourceConfig(sourceName);
    if (!ssconf) return;

    // TODO: use a switch on sourceName when refactoring
    int editBoxResourceID = IDC_NOTES_EDIT_SYNCTYPE;
    int comboBoxResourceID = IDC_NOTES_COMBO_SYNCTYPE;

    CEdit* editbox = (CEdit*)GetDlgItem(editBoxResourceID);
    CComboBox* combobox = (CComboBox*)GetDlgItem(comboBoxResourceID);
    if (!combobox || !editbox) return;

    //
    // Load the syncmodes in the editbox/dropdown
    //
    CString s1 = "";
    StringBuffer syncModes(ssconf->getSyncModes());
    if (syncModes.find(SYNC_MODE_TWO_WAY) != StringBuffer::npos) {
        s1.LoadString(IDS_SYNCTYPE1);
        combobox->AddString(s1);
    }
    if (syncModes.find(SYNC_MODE_ONE_WAY_FROM_SERVER) != StringBuffer::npos ||
        syncModes.find(SYNC_MODE_SMART_ONE_WAY_FROM_SERVER) != StringBuffer::npos) {
        s1.LoadString(IDS_SYNCTYPE2);
        combobox->AddString(s1);
    }
    if (syncModes.find(SYNC_MODE_ONE_WAY_FROM_CLIENT) != StringBuffer::npos ||
        syncModes.find(SYNC_MODE_SMART_ONE_WAY_FROM_CLIENT) != StringBuffer::npos) {
        s1.LoadString(IDS_SYNCTYPE3);
        combobox->AddString(s1);
    }

    if (combobox->GetCount() > 1) {
        // More than 1 syncmode available: use the dropdown box
        editbox->ShowWindow(SW_HIDE);
        combobox->ShowWindow(SW_SHOW);
    }
    else {
        // Only 1 syncmode available: use the editbox
        editbox->ShowWindow(SW_SHOW);
        combobox->ShowWindow(SW_HIDE);
        SetDlgItemText(editBoxResourceID, s1);
    }
}
