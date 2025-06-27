#if !defined(AFX_ACQCONFIGDLG_H__1050BCF7_DF38_42E7_9A4E_DFDCABD52116__INCLUDED_)
#define AFX_ACQCONFIGDLG_H__1050BCF7_DF38_42E7_9A4E_DFDCABD52116__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AcqConfigDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAcqConfigDlg dialog

class SAPCLASSGUI_CLASS CAcqConfigDlg : public CDialog, public SapManager
{
// Enumerations
public:
   enum ServerCategory
   {
      ServerAll,
      ServerAcq,
      ServerAcqDevice
   };

// Construction
public:
	CAcqConfigDlg(CWnd* pParent, ServerCategory serverCategory = ServerAcq);
	CAcqConfigDlg(CWnd* pParent, SapLocation loc, const char* configFile, ServerCategory serverCategory = ServerAcq);

   SapLocation GetLocation();
   const char* GetConfigFile();
   ServerCategory GetServerCategory() { return m_serverCategory; }

   void SetCameraLinkServerFlag(bool bFlag) { m_bShowCameraLinkServers = bFlag; }

   // Obsolete functions
	CAcqConfigDlg::CAcqConfigDlg(CWnd* pParent, SapAcquisition *pAcq, const char *productDir= NULL); // Replaced by CAcqConfigDlg(CWnd*, SapLocation, const char*)
	SapAcquisition &GetAcquisition() { return m_Acq; }    // Replaced by GetLocation and GetConfigFile

// Dialog Data
	//{{AFX_DATA(CAcqConfigDlg)
	enum { IDD = IDD_SCG_ACQ_CONFIG };
		// NOTE: the ClassWizard will add data members here
	CComboBox	m_cbResource;
	CComboBox	m_cbServer;
	CEdit	      m_editDescription;
	CListBox	   m_configListFileName;
	CString	   m_serverName;
	int		   m_resourceIndex;
	BOOL		   m_configFileEnable;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAcqConfigDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL UpdateAcquisition();
	void LoadSettings();
	void SaveSettings();
	void InitServerCombo();
	void InitResourceCombo();
	void UpdateDescription();
	void UpdateMenu();
   bool ValidateCCFAgainstServerName(bool bDisplayMessage = false);
   void SplitConfigFilePath();

	// Generated message map functions
	//{{AFX_MSG(CAcqConfigDlg)
	afx_msg void OnConfigBrowse();

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnSelchangeComboServer();
	afx_msg void OnSelchangeComboResource();
   afx_msg void OnBnClickedConfigFileCheck();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
	SapAcquisition m_Acq;		// Internal SapAcquisition object
	CWinApp *m_App;				// Pointer to application object

   CStringA m_productDir;                     // Initial directory for finding CAM and VIC files
   CStringA m_configFile;					      // Complete config file path
   ServerCategory m_serverCategory;          // Category of servers (All, Acq or acqDevice)
   bool m_bShowCameraLinkServers;            // Show the camera links servers whatsoever

	char m_currentConfigDir[MAX_PATH];			// Directory where to find the config files
	char m_currentConfigFileName[MAX_PATH];	// File name (without path) of the current config file selected
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACQCONFIGDLG_H__1050BCF7_DF38_42E7_9A4E_DFDCABD52116__INCLUDED_)
