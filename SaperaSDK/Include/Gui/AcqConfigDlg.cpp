// AcqConfigDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SapClassBasic.h"
#include "SapClassGui.h"
#include "BrowseDirDlg.h"
#include ".\acqconfigdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Configuration parameters
//
static TCHAR *FileExt = { _T(ACQCFG_DEFAULT_EXTENSION) };
static TCHAR *ConfigKeyName = { _T("Camera Name") };
static TCHAR *CompanyKeyName = { _T("Company Name") };
static TCHAR *ModelKeyName = { _T("Model Name") };
static TCHAR *VicName = { _T("Vic Name") };

/////////////////////////////////////////////////////////////////////////////
// CAcqConfigDlg dialog

CAcqConfigDlg::CAcqConfigDlg(CWnd* pParent, ServerCategory serverCategory)
: CDialog(CAcqConfigDlg::IDD, pParent)
{
   m_App = AfxGetApp();
   m_serverCategory = serverCategory;
   m_bShowCameraLinkServers = false;
   m_configFileEnable = (m_serverCategory == ServerAcq);


   // Load parameters from registry
   LoadSettings();
}

CAcqConfigDlg::CAcqConfigDlg(CWnd* pParent, SapLocation loc, const char* configFile, ServerCategory serverCategory)
: CDialog(CAcqConfigDlg::IDD, pParent)
{
   m_App = AfxGetApp();
   m_serverCategory = serverCategory;
   m_bShowCameraLinkServers = false;
   m_configFileEnable = (m_serverCategory == ServerAcq);

   // Initialize parameters
   char serverName[CORSERVER_MAX_STRLEN];
   SapManager::GetServerName(loc, serverName, sizeof(serverName));

   m_serverName = serverName;
   m_resourceIndex = loc.GetResourceIndex();

   m_configFile = CString(configFile);
   SplitConfigFilePath();
}

// Obsolete constructor, replaced by CAcqConfigDlg(CWnd*, SapLocation, const char*)
CAcqConfigDlg::CAcqConfigDlg(CWnd* pParent, SapAcquisition *pAcq, const char *productDir)
: CDialog(CAcqConfigDlg::IDD, pParent)
{
   m_App = AfxGetApp();
   m_productDir = productDir;
   m_serverCategory = ServerAcq;
   m_bShowCameraLinkServers = false;

   if (pAcq)
   {
      // Copy to internal object
      m_Acq = *pAcq;

      // Get parameters from acquisition object
      char serverName[CORSERVER_MAX_STRLEN];
      SapManager::GetServerName(pAcq->GetLocation(), serverName, sizeof(serverName));

      m_serverName = serverName;
      m_resourceIndex = pAcq->GetLocation().GetResourceIndex();

      m_configFile = CStringA(pAcq->GetConfigFile());
      SplitConfigFilePath();
   }
   else
   {
      // Load parameters from registry
      LoadSettings();
   }
}

SapLocation CAcqConfigDlg::GetLocation()
{
   return SapLocation(CStringA(m_serverName), m_resourceIndex);
}

BOOL CAcqConfigDlg::UpdateAcquisition()
{
   SapLocation loc(CStringA(m_serverName), m_resourceIndex);
   m_Acq.SetLocation(loc);
   m_Acq.SetConfigFile(CStringA(m_configFile));
   return TRUE;
}

void CAcqConfigDlg::LoadSettings()
{
   CString str;

   // Read location (server and resource)
   m_serverName = m_App->GetProfileString(_T("SapAcquisition"), _T("Server"), _T(""));
   m_resourceIndex = m_App->GetProfileInt(_T("SapAcquisition"), _T("Resource"), 0);

   // Read config file name
   str = m_App->GetProfileString(_T("SapAcquisition"), _T("ConfigFileName"), _T(""));
   CorStrncpy(m_currentConfigFileName, (LPCSTR)CStringA(str), sizeof(m_currentConfigFileName));

   // Read config file directory
   str = m_App->GetProfileString(_T("SapAcquisition"), _T("ConfigFileDir"), _T(""));
   CorStrncpy(m_currentConfigDir, (LPCSTR)CStringA(str), sizeof(m_currentConfigDir));

   // Reconstruct config file path
   m_configFile = CString(m_currentConfigDir) + CString(m_currentConfigFileName);
}

void CAcqConfigDlg::SaveSettings()
{
   // Write location (server and resource)
   m_App->WriteProfileString(_T("SapAcquisition"), _T("Server"), m_serverName);
   m_App->WriteProfileInt(_T("SapAcquisition"), _T("Resource"), m_resourceIndex);

   // Write config file name and directory
   m_App->WriteProfileString(_T("SapAcquisition"), _T("ConfigFileName"), CString(m_currentConfigFileName));
   m_App->WriteProfileString(_T("SapAcquisition"), _T("ConfigFileDir"), CString(m_currentConfigDir));
}

void CAcqConfigDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CAcqConfigDlg)
   // NOTE: the ClassWizard will add DDX and DDV calls here
   DDX_Control(pDX, IDC_SCG_CV_COMBO_RESOURCE, m_cbResource);
   DDX_Control(pDX, IDC_SCG_CV_COMBO_SERVER, m_cbServer);
   DDX_Control(pDX, IDC_SCG_CV_CONFIG_LIST_FILENAME, m_configListFileName);
   DDX_CBString(pDX, IDC_SCG_CV_COMBO_SERVER, m_serverName);
   DDX_CBIndex(pDX, IDC_SCG_CV_COMBO_RESOURCE, m_resourceIndex);
   DDX_Check(pDX, IDC_SCG_CV_CONFIG_FILE_CHECK, m_configFileEnable);
   //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAcqConfigDlg, CDialog)
   //{{AFX_MSG_MAP(CAcqConfigDlg)
   ON_BN_CLICKED(IDC_SCG_CV_CONFIG_BROWSE, OnConfigBrowse)
   ON_CBN_SELCHANGE(IDC_SCG_CV_COMBO_SERVER, OnSelchangeComboServer)
   ON_CBN_SELCHANGE(IDC_SCG_CV_COMBO_RESOURCE, OnSelchangeComboResource)
   //}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_SCG_CV_CONFIG_FILE_CHECK, OnBnClickedConfigFileCheck)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAcqConfigDlg message handlers

BOOL CAcqConfigDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   // put instructions in the text box
   SetDlgItemText(IDC_SCG_CV_EDIT_CONFIG_PATH, _T("Please select a CCF file..."));

   // erase with the actual config file path if it exists and is valid
   if (!m_configFile.IsEmpty())
   {
      if (ValidateCCFAgainstServerName(false))
      {
         SetDlgItemText(IDC_SCG_CV_EDIT_CONFIG_PATH, CString(m_configFile));
         UpdateDescription();
      }
   }

   InitServerCombo();
   InitResourceCombo();
   UpdateMenu();

   BringWindowToTop();

   return TRUE;
}

void CAcqConfigDlg::InitServerCombo()
{
   for (int i = 0; i < GetServerCount(); i++)
   {
      if (!IsServerAccessible(i))
         continue;

      // Does this server support "Acq" (frame-grabber) or "AcqDevice" (camera)?

      bool bAcq = (m_serverCategory == ServerAcq || m_serverCategory == ServerAll)
         && (GetResourceCount(i, ResourceAcq) > 0);

      // when m_serverCategory is ServerAcqDevice, show only servers that have only one resource that is an acqdevice and no acq
      bool bAcqDevice = (m_serverCategory == ServerAcqDevice || m_serverCategory == ServerAll)
         && (GetResourceCount(i, ResourceAcqDevice) == 1) && (GetResourceCount(i, ResourceAcq) == 0);

      if (bAcq || bAcqDevice || m_bShowCameraLinkServers)
      {
         char name[128];
         if (GetServerName(i, name, sizeof(name)))
         {
            if (strstr(name, "CameraLink_") != NULL)
            {
               if (bAcq || m_bShowCameraLinkServers)
                  m_cbServer.AddString(CString(name));
            }
            // Exclude Z-Trak from the server list
            else if (strstr(name, "LP1") == NULL && strstr(name, "LP2") == NULL)
            {
               // Add server name to combo
               m_cbServer.AddString(CString(name));

               // Store flag to tell whether or not it's a "AcqDevice" server
               // Only use DWORD_PTR starting with Visual Studio .NET 2003, since its presence depends on the
               // Platform SDK version with Visual Studio 6, and it is only needed for 64-bit compatibility anyway.
#if defined(_MSC_VER) && _MSC_VER >= 1300
               m_cbServer.SetItemData(m_cbServer.GetCount() - 1, (DWORD_PTR)bAcq);
#else
               m_cbServer.SetItemData(m_cbServer.GetCount()-1, bAcq);
#endif
            }
         }
      }
   }

   // At least one server must be available
   if (m_cbServer.GetCount() <= 0)
   {
      OnCancel();
      return;
   }

   // Select server name
   if (m_cbServer.SelectString(-1, m_serverName) == CB_ERR)
   {
      // If not found, select first one
      m_cbServer.SetCurSel(0);
      m_cbServer.GetLBText(0, m_serverName);
   }
}

void CAcqConfigDlg::InitResourceCombo()
{
   int i = 0;

   // Clear combo content
   m_cbResource.ResetContent();
   
   // Check if the server name is present on the system
   if (GetServerIndex(CStringA(m_serverName)) != SapLocation::ServerUnknown)
   {
      // Add "Acq" resources (frame-grabbers) to combo
      for (i = 0; i < GetResourceCount(CStringA(m_serverName), ResourceAcq); i++)
      {
         char name[128];
         if (GetResourceName(CStringA(m_serverName), ResourceAcq, i, name, sizeof(name)))
            if (IsResourceAvailable(CStringA(m_serverName), ResourceAcq, i) == FALSE)
               CorSnprintf(name, sizeof(name), "%s", "Not Available - Resource In Use");
         m_cbResource.AddString(CString(name));
      }

      // Add "AcqDevice" resources (cameras) to combo only if there are no frame grabber resources
      if (GetResourceCount(CStringA(m_serverName), ResourceAcq) == 0)
      {
         for (i = 0; i < GetResourceCount(CStringA(m_serverName), ResourceAcqDevice); i++)
         {
            char name[128];
            if (GetResourceName(CStringA(m_serverName), ResourceAcqDevice, i, name, sizeof(name)))
               m_cbResource.AddString(CString(name));
         }
      }
   }

   // Select resource index
   if (m_cbResource.SetCurSel(m_resourceIndex) == CB_ERR)
   {
      // If not found, select first one
      m_resourceIndex = 0;
      m_cbResource.SetCurSel(m_resourceIndex);
   }
}

//
// Browse for a directory where to look for the files
//
void CAcqConfigDlg::OnConfigBrowse()
{
   CFileDialog fileOpen(TRUE, NULL, NULL, OFN_FILEMUSTEXIST);

   CString sInitialDir(m_currentConfigDir);
   TCHAR* pInitialDir = (LPTSTR)(LPCTSTR)sInitialDir;

   //apply extensions filter based on category (Visual Studio 2005 and up only)
#if defined(_MSC_VER) &&  _MSC_VER >= 1400
   fileOpen.GetOFN().lpstrFilter = _T("Camera Configuration Files(*.ccf)\0*.ccf\0All Files (*.*)\0*.*\0\0");
   fileOpen.GetOFN().lpstrInitialDir = pInitialDir;
#endif

   INT_PTR ret = fileOpen.DoModal();
   sInitialDir.ReleaseBuffer();

   if (ret == IDCANCEL)
      return;

   m_configFile = fileOpen.GetPathName();
   SetDlgItemText(IDC_SCG_CV_EDIT_CONFIG_PATH, CString(m_configFile));

   SplitConfigFilePath();
      
   UpdateDescription();
   UpdateMenu();
}

void CAcqConfigDlg::SplitConfigFilePath()
{
   char acDrive[_MAX_DRIVE];
   char acDir[_MAX_DIR];
   char acFname[_MAX_FNAME];
   char acExt[_MAX_EXT];

   //split path into directory and filename
   CorSplitpath(CStringA(m_configFile), acDrive, _MAX_DRIVE, acDir, _MAX_DIR, acFname, _MAX_FNAME, acExt, _MAX_EXT);
   CorSnprintf(m_currentConfigDir, sizeof(m_currentConfigDir), "%s%s", acDrive, acDir);
   CorSnprintf(m_currentConfigFileName, sizeof(m_currentConfigFileName), "%s%s", acFname, acExt);
}

//
// Update the design list box
//
void CAcqConfigDlg::UpdateDescription()
{
   TCHAR szServerName[CORSERVER_MAX_STRLEN] = { 0 };
   TCHAR szCameraName[_MAX_PATH] = { 0 };
   TCHAR szVicName[_MAX_PATH] = { 0 };
   TCHAR szCompanyName[_MAX_PATH] = { 0 };
   TCHAR szModelName[_MAX_PATH] = { 0 };

   TCHAR *keyName = ConfigKeyName;

   CString strConfigFile(m_configFile);
   GetPrivateProfileString(_T("Board"), _T("Server name"), _T("Unknow"), szServerName, CORSERVER_MAX_STRLEN, strConfigFile);
   GetPrivateProfileString(_T("General"), CString(keyName), _T("Unknown"), szCameraName, _MAX_PATH, strConfigFile);
   GetPrivateProfileString(_T("General"), CString(CompanyKeyName), _T(""), szCompanyName, _MAX_PATH, strConfigFile);
   GetPrivateProfileString(_T("General"), CString(ModelKeyName), _T(""), szModelName, _MAX_PATH, strConfigFile);
   GetPrivateProfileString(_T("General"), CString(VicName), _T(""), szVicName, _MAX_PATH, strConfigFile);

   SetDlgItemText(IDC_STATIC_FILENAME, CString(m_currentConfigFileName));
   SetDlgItemText(IDC_STATIC_COMPANY, szCompanyName);
   SetDlgItemText(IDC_STATIC_MODEL, szModelName);
   SetDlgItemText(IDC_STATIC_CAMERA, szCameraName);
   SetDlgItemText(IDC_STATIC_VIC, szVicName);
   SetDlgItemText(IDC_STATIC_SERVER, szServerName);
}

void CAcqConfigDlg::UpdateMenu()
{
   // Is config file required by this type of server?
   BOOL bConfigFileRequired = m_cbServer.GetItemData(m_cbServer.GetCurSel()) ? true : false;
   GetDlgItem(IDC_SCG_CV_CONFIG_FILE_CHECK)->EnableWindow(!bConfigFileRequired);
   if (bConfigFileRequired)
   {
      // Force loading a config file
      m_configFileEnable = bConfigFileRequired;
      UpdateData(FALSE);
   }

   // Is config file specified?
   bool bIsConfigFileSpecified = !m_configFile.IsEmpty();

   // Update the controls related to the config file
   GetDlgItem(IDC_SCG_CV_EDIT_CONFIG_PATH)->EnableWindow(m_configFileEnable);
   GetDlgItem(IDC_SCG_CV_CONFIG_BROWSE)->EnableWindow(m_configFileEnable);
   GetDlgItem(IDOK)->EnableWindow(!m_configFileEnable || bIsConfigFileSpecified);
}

bool CAcqConfigDlg::ValidateCCFAgainstServerName(bool bDisplayMessage)
{
   TCHAR szServerName[CORSERVER_MAX_STRLEN] = { 0 };
   CString strConfigFile(m_configFile);
   GetPrivateProfileString(_T("Board"), _T("Server name"), _T("Unknow"), szServerName, CORSERVER_MAX_STRLEN, strConfigFile);
   
   // Get current server name
   CStringA curServerName = m_serverName;

   // Remove trailing board index from the server name
   char* pTmp = strrchr(curServerName.GetBuffer(CORSERVER_MAX_STRLEN), '_');
   if (pTmp != 0) *pTmp = '\0';
   curServerName.ReleaseBuffer();

   // Check if the current configuration file has been created for the current server 
   if (curServerName.CompareNoCase(CStringA(szServerName)) == 0)
      return true;

   // Display a message
   if (bDisplayMessage)
   {
      MessageBox(_T("The selected file does not match the selected server. To avoid loading problems, please use CamExpert to create a new file for the selected server."), _T("Camera Configuration File Mismatch"), MB_ICONWARNING | MB_OK);
   }

   return false;
}

void CAcqConfigDlg::OnOK()
{
   // Validate the CCF file
   BOOL bConfigFileRequired = m_cbServer.GetItemData(m_cbServer.GetCurSel()) ? true : false;
   if (bConfigFileRequired && !ValidateCCFAgainstServerName(true))
      return;

   // Update acquisition object
   UpdateAcquisition();

   // Save to registry
   SaveSettings();

   // Update server category if necessary
   if (m_serverCategory == ServerAll)
   {
      if (GetResourceCount(m_Acq.GetLocation(), ResourceAcq) > 0)
         m_serverCategory = ServerAcq;
      else if (GetResourceCount(m_Acq.GetLocation(), ResourceAcqDevice) > 0)
         m_serverCategory = ServerAcqDevice;
   }

   CDialog::OnOK();
}

void CAcqConfigDlg::OnSelchangeComboServer()
{
   UpdateData(TRUE);
   InitResourceCombo();
   UpdateMenu();
}

void CAcqConfigDlg::OnSelchangeComboResource()
{
   UpdateData(TRUE);
   UpdateMenu();
}

void CAcqConfigDlg::OnBnClickedConfigFileCheck()
{
   UpdateData(TRUE);
   UpdateMenu();
}


const char* CAcqConfigDlg::GetConfigFile()
{ 
   if (m_configFileEnable)
      return m_configFile;
   else
      return NULL; 
}