// ColorConversionOptionsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SapClassBasic.h"
#include "SapClassGui.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

SapColorConversion::Align CColorConversionOptionsDlg::m_Aligns[NumAlignValues] =
{
   // bayer
   SapColorConversion::AlignGBRG, 
   SapColorConversion::AlignBGGR, 
   SapColorConversion::AlignRGGB, 
   SapColorConversion::AlignGRBG,

   // bicolor
   SapColorConversion::AlignRGBG,
   SapColorConversion::AlignBGRG
};

SapColorConversionMethodDictionary CColorConversionOptionsDlg::m_Methods[NumMethodValues] =
{  
   { 0, _T("No Method") },

   { SapColorConversion::Method1, _T("Bayer Method 1") },
   { SapColorConversion::Method2, _T("Bayer Method 2") },
   { SapColorConversion::Method3, _T("Bayer Method 3") },
   { SapColorConversion::Method4, _T("Bayer Method 4") },
   { SapColorConversion::Method5, _T("Bayer Method 5") },

   { SapColorConversion::Method6, _T("Bi-Color Method 6 (compatible with Basler Sprint)") },
   { SapColorConversion::Method7, _T("Bi-Color Method 7 (compatible with TDALSA P4)") }
};

/////////////////////////////////////////////////////////////////////////////
// CColorConversionOptionsDlg dialog

CColorConversionOptionsDlg::CColorConversionOptionsDlg(CWnd *pParent, SapColorConversion *pColorConv, SapTransfer *pXfer, CImageWnd *pImageWnd, SapProcessing* pPro)
   : CDialog(CColorConversionOptionsDlg::IDD, pParent)
{
   //{{AFX_DATA_INIT(CColorConversionOptionsDlg)
   m_BlueGain = 1.0f;
   m_Gamma = 1.0f;
   m_GreenGain = 1.0f;
   m_RedGain = 1.0f;
	m_GammaEnabled = FALSE;
	//}}AFX_DATA_INIT

   m_pColorConv = pColorConv;
   m_pXfer = pXfer;
	m_pPro = pPro;
   m_pImageWnd = pImageWnd;
}

CColorConversionOptionsDlg::~CColorConversionOptionsDlg()
{
}

void CColorConversionOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CColorConversionOptionsDlg)
   DDX_Text(pDX, IDC_SCG_COLOR_CONV_BLUE_GAIN, m_BlueGain);
   DDX_Text(pDX, IDC_SCG_COLOR_CONV_GAMMA, m_Gamma);
   DDX_Text(pDX, IDC_SCG_COLOR_CONV_GREEN_GAIN, m_GreenGain);
   DDX_Text(pDX, IDC_SCG_COLOR_CONV_RED_GAIN, m_RedGain);
   DDX_Text(pDX, IDC_SCG_COLOR_CONV_METHOD_DESC, m_MethodDesc);
	DDX_Check(pDX, IDC_SCG_COLOR_CONV_GAMMA_ENABLE, m_GammaEnabled);
	//}}AFX_DATA_MAP
   DDX_Radio(pDX, IDC_SCG_COLOR_CONV_ALIGN_GB_RG, m_Align);
   DDX_Radio(pDX, IDC_SCG_COLOR_CONV_METHOD_NONE, m_Method);
   
}

BEGIN_MESSAGE_MAP(CColorConversionOptionsDlg, CDialog)
   //{{AFX_MSG_MAP(CColorConversionOptionsDlg)
   ON_BN_CLICKED(IDC_SCG_COLOR_CONV_AUTO_WHITE_BALANCE, OnAutoWhiteBalance)
   ON_BN_CLICKED(IDC_SCG_COLOR_CONV_APPLY, OnApply)
   ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDOK, OnClose)
   ON_EN_CHANGE(IDC_SCG_COLOR_CONV_BLUE_GAIN, OnChangeNumber)
   ON_EN_CHANGE(IDC_SCG_COLOR_CONV_GREEN_GAIN, OnChangeNumber)
   ON_EN_CHANGE(IDC_SCG_COLOR_CONV_RED_GAIN, OnChangeNumber)
   ON_EN_CHANGE(IDC_SCG_COLOR_CONV_GAMMA, OnChangeNumber)
	ON_BN_CLICKED(IDC_SCG_COLOR_CONV_GAMMA_ENABLE, OnGammaEnable)
	//}}AFX_MSG_MAP
   ON_CONTROL_RANGE(BN_CLICKED, IDC_SCG_COLOR_CONV_ALIGN_GB_RG, IDC_SCG_COLOR_CONV_ALIGN_GR_BG, OnChangeRange)
   ON_CONTROL_RANGE(BN_CLICKED, IDC_SCG_COLOR_CONV_ALIGN_RG_BG, IDC_SCG_COLOR_CONV_ALIGN_BG_RG, OnChangeRange)
   ON_CONTROL_RANGE(BN_CLICKED, IDC_SCG_COLOR_CONV_METHOD_1, IDC_SCG_COLOR_CONV_METHOD_7, OnChangeRange)
   ON_BN_CLICKED(IDC_SCG_COLOR_CONV_METHOD_NONE, OnChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColorConversionOptionsDlg message handlers

BOOL CColorConversionOptionsDlg::OnInitDialog() 
{
   CDialog::OnInitDialog();

   if (m_pColorConv == NULL)
   {
      AfxMessageBox(_T("No Color Conversion object specified"));
      CDialog::OnCancel();
      return FALSE;
   }

   UpdateInterface();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CColorConversionOptionsDlg::UpdateInterface()
{
	SapColorConversion::Align  AlignCap = m_pColorConv->GetAvailAlign();
	SapColorConversion::Method MethodCap = m_pColorConv->GetAvailMethod();

   // Do not declare in 'for' statement, as we get an error with Visual Studio 2005 in the next loop
   int i;

   // Initialize settings for color alignment
   m_Align = 0;
   for (i = 0; i < NumAlignValues; i++)
   {
      if (m_Aligns[i] == m_pColorConv->GetAlign())
      {
         m_Align = i;
         break;
      }
   }
   
	// Check which alignment is available
   GetDlgItem(IDC_SCG_COLOR_CONV_ALIGN_GB_RG)->EnableWindow( AlignCap & SapColorConversion::AlignGBRG);
   GetDlgItem(IDC_SCG_COLOR_CONV_ALIGN_BG_GR)->EnableWindow( AlignCap & SapColorConversion::AlignBGGR);
   GetDlgItem(IDC_SCG_COLOR_CONV_ALIGN_RG_GB)->EnableWindow( AlignCap & SapColorConversion::AlignRGGB);
   GetDlgItem(IDC_SCG_COLOR_CONV_ALIGN_GR_BG)->EnableWindow(AlignCap & SapColorConversion::AlignGRBG);
   GetDlgItem(IDC_SCG_COLOR_CONV_ALIGN_RG_BG)->EnableWindow(AlignCap & SapColorConversion::AlignRGBG);
   GetDlgItem(IDC_SCG_COLOR_CONV_ALIGN_BG_RG)->EnableWindow(AlignCap & SapColorConversion::AlignBGRG);

   // Initialize settings for color method
   m_Method = 0;
   for (i = 0; i < NumMethodValues; i++)
   {
      if (m_Methods[i].value == m_pColorConv->GetMethod())
      {
         m_Method = i;
         break;
      }
   }

	// Check which interpolation method is available
   GetDlgItem(IDC_SCG_COLOR_CONV_METHOD_NONE)->EnableWindow(FALSE);
   GetDlgItem(IDC_SCG_COLOR_CONV_METHOD_1)->EnableWindow( MethodCap & SapColorConversion::Method1);
   GetDlgItem(IDC_SCG_COLOR_CONV_METHOD_2)->EnableWindow( MethodCap & SapColorConversion::Method2);
   GetDlgItem(IDC_SCG_COLOR_CONV_METHOD_3)->EnableWindow( MethodCap & SapColorConversion::Method3);
   GetDlgItem(IDC_SCG_COLOR_CONV_METHOD_4)->EnableWindow( MethodCap & SapColorConversion::Method4);
   GetDlgItem(IDC_SCG_COLOR_CONV_METHOD_5)->EnableWindow( MethodCap & SapColorConversion::Method5);
   GetDlgItem(IDC_SCG_COLOR_CONV_METHOD_6)->EnableWindow( MethodCap & SapColorConversion::Method6);
   GetDlgItem(IDC_SCG_COLOR_CONV_METHOD_7)->EnableWindow( MethodCap & SapColorConversion::Method7);

   // Initialize gain values
   SapDataFRGB wbGain = m_pColorConv->GetWBGain();

   m_RedGain   = wbGain.Red();
   m_GreenGain = wbGain.Green();
   m_BlueGain  = wbGain.Blue();

   GetDlgItem(IDC_SCG_COLOR_CONV_AUTO_WHITE_BALANCE)->EnableWindow( m_pImageWnd != NULL);

   // Initialize Gamma correction factor
   m_Gamma = m_pColorConv->GetGamma();
	m_GammaEnabled= m_pColorConv->IsLutEnabled();

	// Check if color decoder is enabled and if a lookup table is available
   GetDlgItem(IDC_SCG_COLOR_CONV_GAMMA)->EnableWindow( m_pColorConv->IsEnabled() && m_GammaEnabled);
   GetDlgItem(IDC_SCG_COLOR_CONV_GAMMA_ENABLE)->EnableWindow( m_pColorConv->IsEnabled());

	// Disable Apply button until something change
   GetDlgItem(IDC_SCG_COLOR_CONV_APPLY)->EnableWindow(FALSE);

   // Tell the friendly name of the currently selected method
   SetSelectedMethodDescription();

   UpdateData(FALSE);
}

void CColorConversionOptionsDlg::OnChange() 
{
   UpdateData(TRUE);

	// Something change so enable the Apply button
   GetDlgItem(IDC_SCG_COLOR_CONV_APPLY)->EnableWindow(TRUE);

   SetSelectedMethodDescription();
}

void CColorConversionOptionsDlg::OnChangeNumber()
{
   // Something change so enable the Apply button
   GetDlgItem(IDC_SCG_COLOR_CONV_APPLY)->EnableWindow(TRUE);
}

void CColorConversionOptionsDlg::SetSelectedMethodDescription()
{
   m_MethodDesc = m_Methods[m_Method].description + _T(" is selected");
   UpdateData(FALSE);
}

void CColorConversionOptionsDlg::OnGammaEnable() 
{
   UpdateData(TRUE);

   GetDlgItem(IDC_SCG_COLOR_CONV_GAMMA)->EnableWindow( m_pColorConv->IsEnabled() && m_GammaEnabled);

	// Something change so enable the Apply button
   GetDlgItem(IDC_SCG_COLOR_CONV_APPLY)->EnableWindow(TRUE);		
}

void CColorConversionOptionsDlg::OnChangeRange(UINT id)
{
   OnChange();
}

void CColorConversionOptionsDlg::OnAutoWhiteBalance()
{
   if (m_pImageWnd == NULL)
      return;

   if (!m_pImageWnd->IsRoiTrackerActive())
   {
      AfxMessageBox(_T("You must select a ROI containing white pixels"));
      return;
   }

   CRect rect = m_pImageWnd->GetSelectedRoi();

   if (rect.Width() > 1 && rect.Height() > 1)
   {
		// Compute new white balance factors from region of interest
      if (m_pColorConv->WhiteBalance(rect.left, rect.top, rect.Width(), rect.Height()))
      {
			// Update user interface
         UpdateInterface();

			// Redraw the image
         UpdateView();
      }
   }
}

void CColorConversionOptionsDlg::OnApply() 
{
   UpdateData(TRUE);
   
   // Set alignment 
   if (m_pColorConv->GetAvailAlign() & m_Aligns[m_Align])
      m_pColorConv->SetAlign(m_Aligns[m_Align]);

	// Set interpolation method
   if (m_pColorConv->GetAvailMethod() & m_Methods[m_Method].value)
	   m_pColorConv->SetMethod(m_Methods[m_Method].value);

	// Set white balance's gain
   if( !m_pColorConv->SetWBGain( SapDataFRGB(m_RedGain, m_GreenGain, m_BlueGain)))
	{
		SapDataFRGB wbGain= m_pColorConv->GetWBGain();

		m_RedGain  = wbGain.Red();
		m_GreenGain= wbGain.Green();
		m_BlueGain = wbGain.Blue();
		
		UpdateData( FALSE);
	}

	if( m_GammaEnabled)
	{
		// Set new gamma factor
		m_pColorConv->SetGamma(m_Gamma);
	}

	// Enable/Disable lookup table
   m_pColorConv->EnableLut( m_GammaEnabled);


	// Redraw the image
   UpdateView();

	// Disable apply button
   GetDlgItem(IDC_SCG_COLOR_CONV_APPLY)->EnableWindow(FALSE);
}

void CColorConversionOptionsDlg::OnShowWindow(BOOL bShow, UINT nStatus) 
{
   CDialog::OnShowWindow(bShow, nStatus);
   
   if (bShow)
	{
		if( m_pImageWnd != NULL)
		{
			// Redraw the image
			m_pImageWnd->Invalidate( FALSE);
			m_pImageWnd->OnPaint();
		}

		// Update user interface
      UpdateInterface();
	}
}

void CColorConversionOptionsDlg::UpdateView() 
{
   if (m_pColorConv->IsEnabled())
	{
		// Check if we are operating on-line
		if( m_pXfer && *m_pXfer)
		{
			// Check if we are grabbing
			if( m_pXfer->IsGrabbing())
				// The view will be automatically updated on the next acquired frame
				return;

			// Check if we are using an hardware color decoder
			if(m_pColorConv->IsHardwareEnabled())
			{
				// Acquire one frame
				m_pXfer->Snap();

				return;
			}
		}

		// Else, apply color conversion to current buffer's content
		if( m_pPro != NULL)
		{
			m_pPro->Execute();
		}
		else
		{
			m_pColorConv->Convert();
			if( m_pImageWnd != NULL)
			{
				// Redraw the color decoded image
				m_pImageWnd->Invalidate( FALSE);
				m_pImageWnd->OnPaint();
			}
		}
	}
}

void CColorConversionOptionsDlg::OnClose() 
{
	CDialog::OnOK();

	if( m_pImageWnd !=NULL)
	{
		// Hide ROI tracker
		m_pImageWnd->HideRoiTracker();

		// Redraw the image to remove tracker
		m_pImageWnd->Invalidate( FALSE);
		m_pImageWnd->OnPaint();
	}
}


