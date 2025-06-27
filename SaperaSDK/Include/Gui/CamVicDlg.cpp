// CamVicDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SapClassBasic.h"
#include "SapClassGui.h"
#include "BrowseDirDlg.h"
#include <strsafe.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Configuration parameters
//
static TCHAR *FileExt[2] = {_T("cca"), _T("cvi")};
static TCHAR *CameraKeyName[2] = {_T("Camera Name"), _T("Vic Name")};
static TCHAR *CompanyKeyName = {_T("Company Name")};
static TCHAR *ModelKeyName = {_T("Model Name")};

/////////////////////////////////////////////////////////////////////////////
// CCamVicDlg dialog


CCamVicDlg::CCamVicDlg(CWnd* pParent, SapAcquisition *pAcq, const char *productDir)
	: CDialog(CCamVicDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCamVicDlg)
	m_ServerName = "";
	m_ResourceIndex = 0;
	//}}AFX_DATA_INIT

	m_App = AfxGetApp();
   m_ProductDir = productDir;

	if (pAcq)
	{
      // Copy to internal object
      m_Acq = *pAcq;

		// Get parameters from acquisition object
		char serverName[CORSERVER_MAX_STRLEN];
		SapManager::GetServerName(pAcq->GetLocation(), serverName, sizeof(serverName));
		m_ServerName = serverName;
		m_ResourceIndex = pAcq->GetLocation().GetResourceIndex();

      _tCorStrncpy(m_CamFile, CString(pAcq->GetCamFile()), sizeof(m_CamFile));
      _tCorStrncpy(m_VicFile, CString(pAcq->GetVicFile()), sizeof(m_VicFile));
	}
	else
	{
		// Load parameters from registry
		LoadSettings();
	}
}

BOOL CCamVicDlg::UpdateAcquisition()
{
	// Set new parameters
	SapLocation loc(CStringA(m_ServerName), m_ResourceIndex);
	m_Acq.SetLocation(loc);
   m_Acq.SetCamFile((LPCSTR)CStringA(m_CamFile));
   m_Acq.SetVicFile((LPCSTR)CStringA(m_VicFile));
	return TRUE;
}

void CCamVicDlg::LoadSettings() 
{
	// Read location (server and resource)
	m_ServerName = m_App->GetProfileString(_T("SapAcquisition"), _T("Server"), _T(""));
	m_ResourceIndex = m_App->GetProfileInt(_T("SapAcquisition"), _T("Resource"), 0);

	// Read CAM and VIC file names
	CString str;
	str = m_App->GetProfileString(_T("SapAcquisition"), _T("CamFile"), _T(""));
   _tCorStrncpy(m_CamFile, str, sizeof(m_CamFile));
	str = m_App->GetProfileString(_T("SapAcquisition"), _T("VicFile"), _T(""));
   _tCorStrncpy(m_VicFile, str, sizeof(m_VicFile));
}

void CCamVicDlg::SaveSettings()
{
	// Write location (server and resource)
	m_App->WriteProfileString(_T("SapAcquisition"), _T("Server"), m_ServerName);
	m_App->WriteProfileInt(_T("SapAcquisition"), _T("Resource"), m_ResourceIndex);

	// Write CAM and VIC file names
	m_App->WriteProfileString(_T("SapAcquisition"), _T("CamFile"), m_CamFile);
	m_App->WriteProfileString(_T("SapAcquisition"), _T("VicFile"), m_VicFile);
}

void CCamVicDlg::SetDirectories()
{
	// Initialize directories
	m_currentCamDir[0] = m_currentVicDir[0] = '\0';
	m_currentCamFileIndex = m_currentVicFileIndex = -1;
	m_currentCamFileName[0] = m_currentVicFileName[0] = '\0';

	// Get installation directories
	TCHAR productDir[_MAX_PATH], saperaDir[_MAX_PATH];
   _tCorStrncpy(productDir, _T(""), sizeof(productDir));
	GetEnvironmentVariable(m_ProductDir, productDir, sizeof(productDir));
   _tCorStrncpy(saperaDir, _T(""), sizeof(saperaDir));
	GetEnvironmentVariable(_T("SAPERADIR"), saperaDir, sizeof(saperaDir));

	CString pathName;
	char acDrive[_MAX_DRIVE];
	char acDir[_MAX_DIR];
	char acFname[_MAX_FNAME];
	char acExt[_MAX_EXT];

	// Get last CAM file accessed
   if (_tcslen(m_CamFile))
	{
      CorSplitpath(CStringA(m_CamFile), acDrive, _MAX_DRIVE, acDir, _MAX_DIR, acFname, _MAX_FNAME, acExt, _MAX_EXT);
		_tCorSnprintf(m_currentCamDir, sizeof(m_currentCamDir), _T("%s%s"), acDrive, acDir);
		_tCorSnprintf(m_currentCamFileName, sizeof(m_currentCamFileName), _T("%s%s"), acFname, acExt);
	}
	else
	{
      if (_tcslen(productDir))
		{
         _tCorStrncpy(m_currentCamDir, productDir, sizeof(m_currentCamDir));
         _tCorStrncat(m_currentCamDir, _T("\\CamFiles"), _tcslen(m_currentCamDir) - _tcslen(m_currentCamDir));
		}
		else
		{
			GetCurrentDirectory(sizeof(m_currentCamDir), m_currentCamDir);
		}
	}

	// Get last VIC file accessed
   if (_tcslen(m_VicFile))
	{

      CorSplitpath((LPCSTR)CStringA(m_VicFile), acDrive, _MAX_DRIVE, acDir, _MAX_DIR, acFname, _MAX_FNAME, acExt, _MAX_EXT);
		_tCorSnprintf(m_currentVicDir, sizeof(m_currentVicDir), _T("%s%s"), acDrive, acDir);
		_tCorSnprintf(m_currentVicFileName, sizeof(m_currentVicFileName), _T("%s%s"), acFname, acExt);
	}
	else
	{
      if (_tcslen(productDir))
		{
         _tCorStrncpy(m_currentVicDir, productDir, sizeof(m_currentVicDir));
         _tCorStrncat(m_currentVicDir, _T("\\CamFiles"), sizeof(m_currentVicDir) - _tcslen(m_currentVicDir));
		}
		else
		{
			GetCurrentDirectory(sizeof(m_currentVicDir), m_currentVicDir);
		}
	}

	// Remove trailing backslash
	if (m_currentCamDir[_tcslen(m_currentCamDir) - 1] == '\\')
		m_currentCamDir[_tcslen(m_currentCamDir) - 1] = '\0';

	if (m_currentVicDir[_tcslen(m_currentVicDir) - 1] == '\\')
		m_currentVicDir[_tcslen(m_currentVicDir) - 1] = '\0';

	// Get MakeVic utility filename
   if (_tcslen(saperaDir))
	{
      _tCorStrncpy(m_makeVicFile, saperaDir, sizeof(m_makeVicFile));
      _tCorStrncat(m_makeVicFile, _T("\\Bin\\MakeVic.exe"), sizeof(m_makeVicFile) - _tcslen(m_makeVicFile));
	}
	else
	{
      _tCorStrncpy(m_makeVicFile, _T(""), sizeof(m_makeVicFile));
	}
}

void CCamVicDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCamVicDlg)
	DDX_Control(pDX, IDC_SCG_CV_COMBO_RESOURCE, m_cbResource);
	DDX_Control(pDX, IDC_SCG_CV_COMBO_SERVER, m_cbServer);
	DDX_Control(pDX, IDC_SCG_CV_COMBO_VIC, m_cbVic);
	DDX_Control(pDX, IDC_SCG_CV_COMBO_CAMERA, m_cbCamera);
	DDX_Control(pDX, IDC_SCG_CV_VIC_LIST_FILENAME, m_vicListFileName);
	DDX_Control(pDX, IDC_SCG_CV_CAM_LIST_FILENAME, m_camListFileName);
	DDX_CBString(pDX, IDC_SCG_CV_COMBO_SERVER, m_ServerName);
	DDX_CBIndex(pDX, IDC_SCG_CV_COMBO_RESOURCE, m_ResourceIndex);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCamVicDlg, CDialog)
	//{{AFX_MSG_MAP(CCamVicDlg)
	ON_BN_CLICKED(IDC_SCG_CV_CAM_BROWSE, OnCamBrowse)
	ON_BN_CLICKED(IDC_SCG_CV_VIC_BROWSE, OnVicBrowse)
	ON_EN_KILLFOCUS(IDC_SCG_CV_EDIT_CAM_PATH, OnKillfocusEditCamPath)
	ON_EN_KILLFOCUS(IDC_SCG_CV_EDIT_VIC_PATH, OnKillfocusEditVicPath)
	ON_CBN_SELCHANGE(IDC_SCG_CV_COMBO_CAMERA, OnSelchangeComboCamera)
	ON_CBN_SELCHANGE(IDC_SCG_CV_COMBO_VIC, OnSelchangeComboVic)
	ON_BN_CLICKED(IDC_SCG_CV_VIC_GENERATE, OnGenerateVic)
	ON_CBN_SELCHANGE(IDC_SCG_CV_COMBO_SERVER, OnSelchangeComboServer)
	ON_CBN_SELCHANGE(IDC_SCG_CV_COMBO_RESOURCE, OnSelchangeComboResource)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCamVicDlg message handlers

BOOL CCamVicDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// Initialize location combo boxes
	InitServerCombo();

	// Initialize directories
	SetDirectories();

	// Display the current directory to look for the files
	SetDlgItemText(IDC_SCG_CV_EDIT_CAM_PATH, m_currentCamDir);
	SetDlgItemText(IDC_SCG_CV_EDIT_VIC_PATH, m_currentVicDir);

	// Scan all files in the current directory and fill the list box
	UpdateNames(TRUE);
	UpdateNames(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCamVicDlg::InitServerCombo()
{
	for (int i=0; i < GetServerCount(); i++)
	{
		// Does this server support acquisition ?
		if (GetResourceCount(i, ResourceAcq))
		{
			// Add server name to combo
			char name[128];
			if (GetServerName(i, name, sizeof(name)))
				m_cbServer.AddString((LPCTSTR)CString(name));
		}
	}

	// At least one server must be available
	if (m_cbServer.GetCount() <= 0)
	{
		OnCancel();
		return;
	}

	// Select server name
	if (m_cbServer.SelectString(-1, m_ServerName) == CB_ERR)
	{
		// If not found, select first one
		m_cbServer.SetCurSel(0);
		m_cbServer.GetLBText(0, m_ServerName);
	}

	// Initialize resource combo box
	InitResourceCombo();
}
	
void CCamVicDlg::InitResourceCombo()
{
	// Clear combo context
	m_cbResource.ResetContent();

	// Fill resource combo
	for (int i=0; i < GetResourceCount(CStringA(m_ServerName), ResourceAcq); i++)
	{
		char name[128];
		if (GetResourceName(CStringA(m_ServerName), ResourceAcq, i, name, sizeof(name)))
			m_cbResource.AddString((LPCTSTR)CString(name));
	}

	// Select resource index
	if (m_cbResource.SetCurSel(m_ResourceIndex) == CB_ERR)
	{
		// If not found, select first one
		m_ResourceIndex = 0;
		m_cbResource.SetCurSel(m_ResourceIndex);
	}
}

//
// Browse for a directory where to look for the files
//
void CCamVicDlg::OnCamBrowse() 
{
	CBrowseDirDlg dlg((LPCSTR)CStringA(m_currentCamDir));
	if (dlg.DoModal() == IDOK)
	{
		UpdateCurrentDir((LPTSTR)(LPCTSTR)dlg.GetDir(), TRUE);
		UpdateNames(TRUE);
	}
}

void CCamVicDlg::OnVicBrowse() 
{
	CBrowseDirDlg dlg((LPCSTR)CStringA(m_currentVicDir));
	if (dlg.DoModal() == IDOK)
	{
		UpdateCurrentDir((LPTSTR)(LPCTSTR)dlg.GetDir(), FALSE);
		UpdateNames(FALSE);
	}
}

void CCamVicDlg::OnKillfocusEditCamPath() 
{
	TCHAR newCurrentDir[MAX_PATH];

	// Get new directory name
	GetDlgItemText(IDC_SCG_CV_EDIT_CAM_PATH, newCurrentDir, sizeof(newCurrentDir));

	// Update the dialog box
	UpdateCurrentDir(newCurrentDir, TRUE);
	UpdateNames(TRUE);	
}

void CCamVicDlg::OnKillfocusEditVicPath() 
{
	TCHAR newCurrentDir[MAX_PATH];

	// Get new directory name
	GetDlgItemText(IDC_SCG_CV_EDIT_VIC_PATH, newCurrentDir, sizeof(newCurrentDir));

	// Update the dialog box
	UpdateCurrentDir(newCurrentDir, FALSE);
	UpdateNames(FALSE);	
}

//
// Update the current CCA or CVI directory in the dialog box, and if necessary reset some member variables
// to default values.
//
void CCamVicDlg::UpdateCurrentDir(TCHAR *newCurrentDir, BOOL bCamFile)
{
	// Remove trailing backslash
	if (newCurrentDir[_tcslen( newCurrentDir) - 1] == '\\')
		newCurrentDir[_tcslen( newCurrentDir) - 1] = '\0';

	TCHAR *currentDir			= bCamFile ? m_currentCamDir : m_currentVicDir;
	TCHAR *currentFileName	= bCamFile ? m_currentCamFileName : m_currentVicFileName;
	UINT ctrlId					= bCamFile ? IDC_SCG_CV_EDIT_CAM_PATH : IDC_SCG_CV_EDIT_VIC_PATH;

   if (_tCorStricmp(newCurrentDir, currentDir))
	{
		// New Directory selected, update the list box selection
      _tCorStrncpy(currentDir, newCurrentDir, bCamFile ? sizeof(m_currentCamDir) : sizeof(m_currentVicDir));
		SetDlgItemText(ctrlId, newCurrentDir);
		currentFileName[0] = '\0';
	}
}

//
// Update the design list box
//
void CCamVicDlg::UpdateNames(BOOL bCamFile)
{
	TCHAR *pathName = new TCHAR[_MAX_PATH];
	TCHAR *fileName = new TCHAR[_MAX_PATH];
	TCHAR *cameraName = new TCHAR[_MAX_PATH];
	TCHAR *companyName = new TCHAR[_MAX_PATH];
	TCHAR *modelName = new TCHAR[_MAX_PATH];
	TCHAR *cameraDesc = new TCHAR[_MAX_PATH * 2];

   memset(pathName, 0, _MAX_PATH);
   memset(fileName, 0, _MAX_PATH);
   memset(cameraName, 0, _MAX_PATH);
   memset(companyName, 0, _MAX_PATH);
   memset(modelName, 0, _MAX_PATH);
   memset(cameraDesc, 0, _MAX_PATH * 2);

	int index = 0;
	int fileIndex = 0;
   int curDescLength = 0;
   int maxDescLength = 0;

	CComboBox *combo			= bCamFile ? &m_cbCamera : &m_cbVic;
	CListBox	*listFileName	= bCamFile ? &m_camListFileName : &m_vicListFileName;
	TCHAR *currentDir			= bCamFile ? m_currentCamDir : m_currentVicDir;
	TCHAR *currentFileName	= bCamFile ? m_currentCamFileName : m_currentVicFileName;
	int *currentFileIndex	= bCamFile ? &m_currentCamFileIndex : &m_currentVicFileIndex;
	TCHAR *fileExt				= bCamFile ? FileExt[0] : FileExt[1];
	TCHAR *keyName				= bCamFile ? CameraKeyName[0] : CameraKeyName[1];
	BOOL *fileAvailable	= bCamFile ? &m_camFileAvailable : &m_vicFileAvailable;

	// fill listbox (hidden) with the design file names
	_tCorSnprintf(pathName, _MAX_PATH, _T("%s\\*.%s"), currentDir, fileExt);

	listFileName->ResetContent();
	listFileName->Dir( 0, pathName);

	// Clear all entries
	combo->ResetContent();

	for (fileIndex = 0; fileIndex < listFileName->GetCount(); fileIndex++)
	{
		// For every file, create an entry in the combo box with the name of the camera
		listFileName->GetText( fileIndex, fileName);
		
		_tCorSnprintf( pathName, _MAX_PATH, _T("%s\\%s"), currentDir, fileName);

		GetPrivateProfileString(_T("General"), keyName, _T("Unknown"), cameraName, _MAX_PATH, pathName);

		if( bCamFile)
		{
			GetPrivateProfileString( _T("General"), CompanyKeyName, _T(""), companyName, _MAX_PATH, pathName);
			GetPrivateProfileString( _T("General"), ModelKeyName, _T(""), modelName, _MAX_PATH, pathName);

         if( _tcslen( companyName) && _tcslen( modelName))
			{
            _tCorStrncat( companyName, _T(", "), _MAX_PATH - _tcslen(companyName));
			}

         if( _tcslen( modelName) && _tcslen( cameraName))
			{
            _tCorStrncat( modelName, _T(", "), _MAX_PATH - _tcslen(modelName));
			}
		}
		else
		{
         _tCorStrncpy( companyName, _T(""), _MAX_PATH);
         _tCorStrncpy( modelName, _T(""), _MAX_PATH);
		}

		_tCorSnprintf( cameraDesc, _MAX_PATH * 2, _T("%s%s%s"), companyName, modelName, cameraName);

		index = combo->AddString( cameraDesc);
		combo->SetItemData( index, fileIndex);

      // Make drop-down list wide enough to accomodate long entries.
      // Assume default system font width of 8 pixels per character.
      curDescLength = (int)_tcslen(cameraDesc);
      if (curDescLength > maxDescLength)
      {
         maxDescLength = curDescLength;
	      combo->SetDroppedWidth(maxDescLength * 8);
      }
	}

	if (combo->GetCount())
	{
		int newFileIndex = -1;

      if (_tcslen(currentFileName))
		{
			// Try to find the current camera file selected
			for (index = 0; index < combo->GetCount(); index++)
			{
				fileIndex = (int)combo->GetItemData( index);
				listFileName->GetText( fileIndex, fileName);
	
            if (!CorStricmp( (LPCSTR)CStringA(fileName), (LPCSTR)CStringA(currentFileName)))
				{
					newFileIndex = index;
					break;
				}
			}
		}

		if (newFileIndex == -1) newFileIndex = 0;
	   combo->SetCurSel( newFileIndex);

      *currentFileIndex = (int)combo->GetItemData( newFileIndex);
		listFileName->GetText(*currentFileIndex, currentFileName);
		*fileAvailable = TRUE;
	}
	else
	{
		*currentFileIndex = -1;
		*fileAvailable = FALSE;
	}

	UpdateBoxAvailability();

	delete [] pathName;
	delete [] fileName;
	delete [] cameraName;
	delete [] companyName;
	delete [] modelName;
	delete [] cameraDesc;
}

void CCamVicDlg::UpdateBoxAvailability()
{
	GetDlgItem(IDC_SCG_CV_COMBO_CAMERA)->EnableWindow(m_camFileAvailable);
	GetDlgItem(IDC_SCG_CV_COMBO_VIC)->EnableWindow(m_vicFileAvailable);
	GetDlgItem(IDC_SCG_CV_VIC_GENERATE)->EnableWindow(MakeVicAvailable());
	GetDlgItem(IDOK)->EnableWindow(m_camFileAvailable && m_vicFileAvailable);
}

void CCamVicDlg::OnSelchangeComboCamera() 
{
	int index;			// List box selection
	int newFileIndex;	// File selection 

	// Get new selection
	index = m_cbCamera.GetCurSel();
   newFileIndex = (int)m_cbCamera.GetItemData( index);

	// Check if selection is different than current one
	if( newFileIndex != m_currentCamFileIndex)
	{
		m_currentCamFileIndex = newFileIndex;
		m_camListFileName.GetText( m_currentCamFileIndex, m_currentCamFileName);
	}
}

void CCamVicDlg::OnSelchangeComboVic() 
{
	int index;			// List box selection
	int newFileIndex;	// File selection 

	// Get new selection
	index = m_cbVic.GetCurSel();
   newFileIndex = (int)m_cbVic.GetItemData( index);

	// Check if selection is different than current one
	if( newFileIndex != m_currentVicFileIndex)
	{
		m_currentVicFileIndex = newFileIndex;
		m_vicListFileName.GetText( m_currentVicFileIndex, m_currentVicFileName);
	}
}

BOOL CCamVicDlg::MakeVicAvailable() 
{
   // Check for presence of the MakeVic utility
	OFSTRUCT ofStruct;
	if (OpenFile((LPCSTR)CStringA(m_makeVicFile), &ofStruct, OF_EXIST) == HFILE_ERROR)
      return FALSE;

   // Check that acquisition resource is available
   return !m_ServerName.IsEmpty() && IsResourceAvailable(CStringA(m_ServerName), ResourceAcq, m_ResourceIndex);
}

void CCamVicDlg::OnGenerateVic() 
{
	// Server name
	CString serverName(m_ServerName);

	// Resource index
	CString resourceIndex;
	resourceIndex.Format(_T("%d"), m_ResourceIndex);

	// Default VIC file name
	CString vicFileName = m_currentCamFileName;
	if (vicFileName.Replace(_T(".cca"), _T(".cvi")) == 0)
		vicFileName += _T(".cvi");

	// Create command line
	CString commandLine = CString(m_makeVicFile)
		+ " " + serverName 
		+ " " + resourceIndex 
		+ " \"" + m_currentCamDir + "\\" + m_currentCamFileName + "\""
		+ " \"" + m_currentVicDir + "\\" + vicFileName + "\"";

	// Launch MakeVic utility
	STARTUPINFO startUpInfo;
	PROCESS_INFORMATION processInfo;
	GetStartupInfo(&startUpInfo);
	startUpInfo.dwFlags = STARTF_USESHOWWINDOW;
	startUpInfo.wShowWindow = SW_HIDE;

	BeginWaitCursor();

   if (!CreateProcess(NULL, (LPTSTR)(LPCTSTR)commandLine, NULL, NULL, TRUE, CREATE_NEW_PROCESS_GROUP | NORMAL_PRIORITY_CLASS, NULL, NULL, &startUpInfo, &processInfo))
	{
		AfxMessageBox(_T("The MakeVic utility cannot be started."));
	}
	else
	{
		WaitForSingleObject(processInfo.hProcess, 10000);
	}

	EndWaitCursor();

	commandLine.ReleaseBuffer();

	// Update VIC name and list box
   _tCorStrncpy(m_currentVicFileName, vicFileName, sizeof(m_currentVicFileName));
	UpdateNames(FALSE);
}

void CCamVicDlg::OnOK() 
{
	CDialog::OnOK();

	// Create complete strings
	_tCorSnprintf(m_CamFile, sizeof(m_CamFile), _T("%s\\%s"), m_currentCamDir, m_currentCamFileName);
	_tCorSnprintf(m_VicFile, sizeof(m_VicFile), _T("%s\\%s"), m_currentVicDir, m_currentVicFileName);

	// Update acquisition object
	UpdateAcquisition();
	
	// Save to registry
	SaveSettings();
}

void CCamVicDlg::OnSelchangeComboServer() 
{
	UpdateData(TRUE);
	InitResourceCombo();
	UpdateBoxAvailability();
}

void CCamVicDlg::OnSelchangeComboResource() 
{
	UpdateData(TRUE);
}
