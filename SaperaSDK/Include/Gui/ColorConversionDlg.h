// ColorConversionOptionsDlg.h : header file
//

#if !defined(AFX_COLORCONVDLG_H__32005EA1_2AF6_46F7_8F9E_57815D5457E7__INCLUDED_)
#define AFX_COLORCONVDLG_H__32005EA1_2AF6_46F7_8F9E_57815D5457E7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CColorConversionOptionsDlg dialog

struct SapColorConversionMethodDictionary
{
   SapColorConversion::Method value;
   CString description;
};

class SAPCLASSGUI_CLASS CColorConversionOptionsDlg : public CDialog, public SapManager
{
public:
   // Number of possible values for Color Conversion parameters
   enum LogTypes
   {
      NumAlignValues  = 6,
      NumMethodValues = 7+1,
   };

// Construction
public:
	CColorConversionOptionsDlg(CWnd* pParent, SapColorConversion *pColorConv, SapTransfer *pXfer= NULL, CImageWnd *pImageWnd= NULL, SapProcessing* m_pPro= NULL);
   virtual ~CColorConversionOptionsDlg();

   void UpdateInterface();

// Dialog Data
	//{{AFX_DATA(CColorConversionOptionsDlg)
	enum { IDD = IDD_SCG_COLOR_CONVERSION_OPTIONS };
	float	m_BlueGain;
	float	m_Gamma;
	float	m_GreenGain;
	float	m_RedGain;
	BOOL	m_GammaEnabled;
   CString m_MethodDesc;
	//}}AFX_DATA

	SapColorConversion::Align  m_Align;
	SapColorConversion::Method m_Method;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColorConversionOptionsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CColorConversionOptionsDlg)
   afx_msg void OnChange();
   afx_msg void OnChangeNumber();
	afx_msg void OnAutoWhiteBalance();
	virtual BOOL OnInitDialog();
	afx_msg void OnApply();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnClose();
	afx_msg void OnGammaEnable();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void OnChangeRange(UINT id);
	void OnChangeConversion(UINT id);
	void UpdateView();
   void SetSelectedMethodDescription();

private:
	SapColorConversion*  m_pColorConv;
	SapTransfer*	      m_pXfer;
	SapProcessing*	      m_pPro;
	CImageWnd*		      m_pImageWnd;

	static SapColorConversion::Align  m_Aligns[NumAlignValues];
   static SapColorConversionMethodDictionary m_Methods[NumMethodValues];
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COLORCONV_H__32005EA1_2AF6_46F7_8F9E_57815D5457E7__INCLUDED_)
