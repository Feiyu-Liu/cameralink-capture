// GioOutputDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SapClassBasic.h"
#include "SapClassGui.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CGioOutputDlg dialog


CGioOutputDlg::CGioOutputDlg(CWnd* pParent,UINT iDevice, SapGio *pGio)
	: CDialog(CGioOutputDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGioOutputDlg)
	//}}AFX_DATA_INIT
	UINT32 iIO;
	m_pParent = pParent;
	m_iDevice = iDevice;
	m_pGio = pGio;

	for (iIO=0; iIO<MAX_OUTPUT_GIO_IO; iIO++)
	{
		m_gioContext[iIO].pDlg = this;
		m_gioContext[iIO].io = iIO;
		m_GioEventCount[iIO] = 0;
	}
}


void CGioOutputDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGioOutputDlg)
	DDX_Control(pDX, IDC_SCG_GIO_OUTPUT, m_Static_Label);
	DDX_Control(pDX, IDC_SCG_POWER_COLOR_GREEN, m_Picture);
	DDX_Control(pDX, IDC_SCG_POWER_COLOR_RED, m_Picture2);
	DDX_Control(pDX, IDC_SCG_COMBO_OUTPUT_STATE, m_Combo_Output_State);
	//}}AFX_DATA_MAP
}

BOOL CGioOutputDlg::PreTranslateMessage(MSG* pMsg) 
{ 
	if(pMsg->message==WM_KEYDOWN && pMsg->wParam == VK_ESCAPE) 
		return true; 

	return CDialog::PreTranslateMessage(pMsg); 
} 


UINT CGioOutputDlg::MoveWindow(UINT offset)
{
	RECT rect;
	UINT h;

	GetClientRect(&rect);
	h = rect.bottom-rect.top ;
	rect.top = 5;
	rect.left = offset; 
	rect.right = rect.left + rect.right;
	CDialog::MoveWindow(&rect,TRUE);
	return rect.right + 10;
}


BEGIN_MESSAGE_MAP(CGioOutputDlg, CDialog)
	//{{AFX_MSG_MAP(CGioOutputDlg)
	ON_CBN_SELCHANGE(IDC_SCG_COMBO_OUTPUT_STATE, OnSelchangeComboOutputState)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_SCG_PIN1, OnPin1)
	ON_BN_CLICKED(IDC_SCG_PIN2, OnPin2)
	ON_BN_CLICKED(IDC_SCG_PIN3, OnPin3)
	ON_BN_CLICKED(IDC_SCG_PIN4, OnPin4)
	ON_BN_CLICKED(IDC_SCG_PIN5, OnPin5)
	ON_BN_CLICKED(IDC_SCG_PIN6, OnPin6)
	ON_BN_CLICKED(IDC_SCG_PIN7, OnPin7)
	ON_BN_CLICKED(IDC_SCG_PIN8, OnPin8)
	ON_BN_CLICKED(IDC_SCG_PIN9, OnPin9)
	ON_BN_CLICKED(IDC_SCG_PIN10, OnPin10)
	ON_BN_CLICKED(IDC_SCG_PIN11, OnPin11)
	ON_BN_CLICKED(IDC_SCG_PIN12, OnPin12)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGioOutputDlg message handlers

// Used by Set/Get ItemData
#define OUTPUT_TYPE_TRISTATE  0
#define OUTPUT_TYPE_ACTIVE    0xFFFF

BOOL CGioOutputDlg::Update()
{

UINT32 capFaultDetect=0,bFaultDetect=0;


	if (m_pGio == NULL)
		return FALSE;


	if (m_pGio->IsCapabilityValid(CORGIO_CAP_FAULT_DETECT))
		m_pGio->GetCapability(CORGIO_CAP_FAULT_DETECT,&capFaultDetect);

	if (capFaultDetect)
	{
		if (m_pGio->IsParameterValid(CORGIO_PRM_FAULT_DETECT))
			m_pGio->GetParameter(CORGIO_PRM_FAULT_DETECT,&bFaultDetect);

			CWnd *PowerNA=GetDlgItem(IDC_STATIC_NA); 
			PowerNA->ModifyStyle(WS_VISIBLE, 0);
			PowerNA->RedrawWindow();

			CWnd *PowerGreen= GetDlgItem(IDC_SCG_POWER_COLOR_GREEN); 
			CWnd *PowerRed= GetDlgItem(IDC_SCG_POWER_COLOR_RED); 
			if (!bFaultDetect)
			{
				PowerGreen->EnableWindow(true);
				PowerRed->EnableWindow(false);
				PowerGreen->ModifyStyle(0, WS_VISIBLE);
				PowerGreen->RedrawWindow();
			}
			else
			{
				PowerRed->EnableWindow(true);
				PowerGreen->EnableWindow(false);
				PowerRed->ModifyStyle(0, WS_VISIBLE);
				PowerRed->RedrawWindow();
			}

		}
	else
	{
		CWnd *PowerLabel=GetDlgItem(IDC_SCG_POWER_LABEL); 
		PowerLabel->EnableWindow(false);
		PowerLabel->RedrawWindow();
	}
return TRUE;
}


BOOL CGioOutputDlg::Create()
{
	BOOL bCreate;
	bCreate = CDialog::Create(CGioOutputDlg::IDD,m_pParent);
	if (!bCreate)
		return FALSE;

	return TRUE;
}


BOOL CGioOutputDlg::OnInitDialog() 
{
	UINT32 capOutput=0,capTristate=0,capOutputType=0,capEventType=0,capPowerGood=0,capFaultDetect=0;
	UINT32 bFaultDetect=0;
	UINT32 prmOutput=0,prmTristate=0;
	UINT32 iSelection,iIO=0;
   UINT32 capReadOnly=0;
	char label[128];

	CDialog::OnInitDialog();

	if( m_pGio == NULL || !*m_pGio)
	{
		AfxMessageBox(_T("Invalid Gio object"));
		CDialog::OnCancel();
		return FALSE;
	}

	// Get signal status reporting capability. Note that it is not supported on all
   // products, and we need to first check if the capability is valid.
	if (m_pGio->IsCapabilityValid(CORGIO_CAP_OUTPUT_TYPE))
		m_pGio->GetCapability(CORGIO_CAP_OUTPUT_TYPE, &capOutputType);

	if (m_pGio->IsCapabilityValid(CORGIO_CAP_DIR_TRISTATE))
		m_pGio->GetCapability(CORGIO_CAP_DIR_TRISTATE,&capTristate);

	if (m_pGio->IsCapabilityValid(CORGIO_CAP_DIR_OUTPUT))
		m_pGio->GetCapability(CORGIO_CAP_DIR_OUTPUT,&capOutput); 

	if (m_pGio->IsParameterValid(CORGIO_PRM_LABEL))
		m_pGio->GetParameter(CORGIO_PRM_LABEL, label);

	if (m_pGio->IsCapabilityValid(CORGIO_CAP_EVENT_TYPE))
		m_pGio->GetCapability(CORGIO_CAP_EVENT_TYPE,&capEventType);

	if (m_pGio->IsCapabilityValid(CORGIO_CAP_POWER_GOOD))
		m_pGio->GetCapability(CORGIO_CAP_POWER_GOOD,&capPowerGood);

	if (m_pGio->IsCapabilityValid(CORGIO_CAP_FAULT_DETECT))
		m_pGio->GetCapability(CORGIO_CAP_FAULT_DETECT,&capFaultDetect);

	if (m_pGio->IsParameterValid(CORGIO_PRM_OUTPUT_TYPE))
		m_pGio->GetParameter(CORGIO_PRM_OUTPUT_TYPE,&prmOutput);

	if (m_pGio->IsParameterValid(CORGIO_PRM_DIR_TRISTATE))
		m_pGio->GetParameter(CORGIO_PRM_DIR_TRISTATE,&prmTristate);

	if (m_pGio->IsCapabilityValid(CORGIO_CAP_READ_ONLY))
		m_pGio->GetCapability(CORGIO_CAP_READ_ONLY,&capReadOnly);


   label[sizeof(label) - 1] = '\0';   
   if( !strstr( label, "output") && !strstr( label, "Output"))
   {
      CorStrncat(label," (output)", sizeof(label));
   }
	
	m_Static_Label.SetWindowText(CString(label));

	// Enable only the IO available
	for (iIO=0; iIO < (UINT32)m_pGio->GetNumPins(); iIO++)
	{
      if(!(capReadOnly & (1 << iIO)))
      {
   		((CButton*)GetDlgItem(IDC_SCG_PIN1 + iIO))->EnableWindow(capOutput & (1<<iIO));
      }
	}

	if (capTristate)
	{
		iSelection = m_Combo_Output_State.AddString(_T("Tristate"));
      m_Combo_Output_State.SetItemData( iSelection, OUTPUT_TYPE_TRISTATE);   // A value of 0 will be used for Tristate

      if( prmTristate)
      {
	   	m_Combo_Output_State.SetCurSel(iSelection);
      }
   }

	if (capOutputType & CORGIO_VAL_OUTPUT_TYPE_PNP)
	{
		iSelection = m_Combo_Output_State.AddString(_T("PNP"));
      m_Combo_Output_State.SetItemData(iSelection, CORGIO_VAL_OUTPUT_TYPE_PNP);

      if( !prmTristate && (prmOutput == CORGIO_VAL_OUTPUT_TYPE_PNP))
      {
	   	m_Combo_Output_State.SetCurSel(iSelection);
      }
	}

	if (capOutputType & CORGIO_VAL_OUTPUT_TYPE_NPN)
	{
		iSelection = m_Combo_Output_State.AddString(_T("NPN"));
      m_Combo_Output_State.SetItemData( iSelection, CORGIO_VAL_OUTPUT_TYPE_NPN);

      if( !prmTristate && (prmOutput == CORGIO_VAL_OUTPUT_TYPE_NPN))
      {
	   	m_Combo_Output_State.SetCurSel(iSelection);
      }
	}

	if (capOutputType & CORGIO_VAL_OUTPUT_TYPE_LED)
	{
		iSelection = m_Combo_Output_State.AddString(_T("LED"));
      m_Combo_Output_State.SetItemData( iSelection, CORGIO_VAL_OUTPUT_TYPE_LED);

      if( !prmTristate && (prmOutput == CORGIO_VAL_OUTPUT_TYPE_LED))
      {
	   	m_Combo_Output_State.SetCurSel(iSelection);
      }
	}

	if (capOutputType & CORGIO_VAL_OUTPUT_TYPE_OPTOCOUPLE)
	{
		iSelection = m_Combo_Output_State.AddString(_T("OPTO"));
      m_Combo_Output_State.SetItemData( iSelection, CORGIO_VAL_OUTPUT_TYPE_OPTOCOUPLE);

      if( !prmTristate && (prmOutput == CORGIO_VAL_OUTPUT_TYPE_OPTOCOUPLE))
      {
	   	m_Combo_Output_State.SetCurSel(iSelection);
      }
	}

	if (capOutputType & CORGIO_VAL_OUTPUT_TYPE_TTL)
	{
		iSelection = m_Combo_Output_State.AddString(_T("TTL"));
      m_Combo_Output_State.SetItemData( iSelection, CORGIO_VAL_OUTPUT_TYPE_TTL);

      if( !prmTristate && (prmOutput == CORGIO_VAL_OUTPUT_TYPE_TTL))
      {
	   	m_Combo_Output_State.SetCurSel(iSelection);
      }
	}

	if (capOutputType & CORGIO_VAL_OUTPUT_TYPE_LVTTL)
	{
		iSelection = m_Combo_Output_State.AddString(_T("LVTTL"));
      m_Combo_Output_State.SetItemData( iSelection, CORGIO_VAL_OUTPUT_TYPE_LVTTL);

      if( !prmTristate && (prmOutput == CORGIO_VAL_OUTPUT_TYPE_LVTTL))
      {
	   	m_Combo_Output_State.SetCurSel(iSelection);
      }
	}

	if (!capOutputType)
	{
		iSelection = m_Combo_Output_State.AddString(_T("Active"));
      m_Combo_Output_State.SetItemData( iSelection, OUTPUT_TYPE_ACTIVE);   // A value of 0xFFFF will be used for Active

      if( !prmTristate)
      {
      	m_Combo_Output_State.SetCurSel(iSelection);
      }
	}


	if (capFaultDetect)
	{
		if (m_pGio->IsParameterValid(CORGIO_PRM_FAULT_DETECT))
			m_pGio->GetParameter(CORGIO_PRM_FAULT_DETECT,&bFaultDetect);

			CWnd *PowerNA=GetDlgItem(IDC_STATIC_NA); 
			PowerNA->ModifyStyle(WS_VISIBLE, 0);
			PowerNA->RedrawWindow();

			CWnd *PowerGreen=GetDlgItem(IDC_SCG_POWER_COLOR_GREEN); 
			CWnd *PowerRed=GetDlgItem(IDC_SCG_POWER_COLOR_RED); 
			if (!bFaultDetect)
			{
				PowerGreen->EnableWindow(true);
				PowerRed->EnableWindow(false);
				PowerGreen->ModifyStyle(0, WS_VISIBLE);
				PowerGreen->RedrawWindow();
			}
			else
			{
				PowerRed->EnableWindow(true);
				PowerGreen->EnableWindow(false);
				PowerRed->ModifyStyle(0, WS_VISIBLE);
				PowerRed->RedrawWindow();
			}

		}
	else
	{
		CWnd *PowerLabel=GetDlgItem(IDC_SCG_POWER_LABEL); 
		PowerLabel->EnableWindow(false);
		PowerLabel->RedrawWindow();
	}

	// Output buttons captions initialized
	InitOutputDlg();

	return TRUE;
}


BOOL CGioOutputDlg::InitOutputDlg()
{
	SapGio::PinState state = SapGio::PinLow;
	BOOL status = false;
   UINT32 capReadOnly = 0;

   if( m_pGio->IsCapabilityValid(CORGIO_CAP_READ_ONLY))
      status = m_pGio->GetCapability(CORGIO_CAP_READ_ONLY, &capReadOnly);

	for (UINT32 iIO=0; iIO < (UINT32)m_pGio->GetNumPins(); iIO++)
	{
		m_pGio->SetDisplayStatusMode(SapManager::StatusLog, NULL);
		status = m_pGio->GetPinState(iIO, &state);
		m_pGio->SetDisplayStatusMode(SapManager::StatusNotify, NULL);
		
		// No Output pin state available : can't read it.
		if (status == 0)
			((CButton*)GetDlgItem(IDC_SCG_PIN1 + iIO))->SetWindowText(_T("N/A"));
		else
		{
         if( !(capReadOnly & (1 << iIO)))
         {
   			// Pins Output buttons identifiers is a sequence.
			   if (state == SapGio::PinLow)
   				((CButton*)GetDlgItem(IDC_SCG_PIN1 + iIO))->SetWindowText(_T("LOW"));
			   else if (state == SapGio::PinHigh)
   				((CButton*)GetDlgItem(IDC_SCG_PIN1 + iIO))->SetWindowText(_T("HIGH"));
	   	}
         else
         {
   			((CButton*)GetDlgItem(IDC_SCG_PIN1 + iIO))->SetWindowText(_T("N/A"));
         }
      }
	}

	InitComboOutputState();

	return status;
}


void CGioOutputDlg::InitComboOutputState() 
{
	CComboBox *p=NULL;
	CORSTATUS status=0;
	UINT32 capOutputType=0, capTristate=0,prmOutputType=0;
	int iSelection=0;
   UINT32 capReadOnly = 0;

	UpdateData(FALSE);


	p=(CComboBox *)GetDlgItem(IDC_SCG_COMBO_OUTPUT_STATE);
	iSelection = p->GetCurSel();
	if (CB_ERR == iSelection)
	{
		return;
	}

	if (m_pGio->IsCapabilityValid(CORGIO_CAP_DIR_TRISTATE))
		m_pGio->GetCapability(CORGIO_CAP_DIR_TRISTATE, &capTristate);
	if (m_pGio->IsCapabilityValid(CORGIO_CAP_OUTPUT_TYPE))
		m_pGio->GetCapability(CORGIO_CAP_OUTPUT_TYPE, &capOutputType);
   if( m_pGio->IsCapabilityValid(CORGIO_CAP_READ_ONLY))
      status = m_pGio->GetCapability(CORGIO_CAP_READ_ONLY, &capReadOnly);

   prmOutputType = (UINT32)p->GetItemData( iSelection);

   if( prmOutputType == OUTPUT_TYPE_TRISTATE)
   {
      status = m_pGio->SetParameter(CORGIO_PRM_DIR_TRISTATE, capTristate & ~capReadOnly);
   }
   else if( prmOutputType == OUTPUT_TYPE_ACTIVE)
   {
   	status = m_pGio->SetParameter(CORGIO_PRM_DIR_TRISTATE,0);
   }
   else
   {
      if( capOutputType != prmOutputType)
      {
         // The outputs have more than one value, set the value. Otherwise do not set it as it cannot be changed anyway.
         status = m_pGio->SetParameter(CORGIO_PRM_OUTPUT_TYPE, prmOutputType);
      }

      // Remove from Tristate
		status = m_pGio->SetParameter(CORGIO_PRM_DIR_TRISTATE,0);
   }
}

void CGioOutputDlg::OnSelchangeComboOutputState() 
{
InitComboOutputState();
}


void CGioOutputDlg::UpdateIO() 
{
	UINT32 output = 0;
	UINT32 state = 0;
   UINT32 capReadOnly = 0;
	BOOL status;

	if (m_pGio->IsParameterValid(CORGIO_PRM_DIR_OUTPUT))
		status = m_pGio->GetParameter(CORGIO_PRM_DIR_OUTPUT,&output);

   if( m_pGio->IsCapabilityValid(CORGIO_CAP_READ_ONLY))
      status = m_pGio->GetCapability(CORGIO_CAP_READ_ONLY, &capReadOnly);

	for (UINT32 iIO=0; iIO < (UINT32)m_pGio->GetNumPins(); iIO++)
	{
		CString caption;
		CButton * pinButton = (CButton*)GetDlgItem(IDC_SCG_PIN1 + iIO);
		pinButton->GetWindowText(caption);
		if( caption.CompareNoCase(_T("HIGH")) == 0)
			 state = 1;
		 else
			 state = 0;
		
      if( !(capReadOnly & (1 << iIO)))
      {
         status = m_pGio->SetPinState(iIO, (SapGio::PinState)state);
      }
	}
}


HBRUSH CGioOutputDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	switch ( nCtlColor )
	{
		case CTLCOLOR_STATIC:
           break;
		case CTLCOLOR_DLG:
		return (HBRUSH)(m_bkBrush.GetSafeHandle());	
		default:
			break;
	}
	
	return hbr;
}


void CGioOutputDlg::ChangePinState(CButton* pinButton)
{
	CString pinButtonText;
	pinButton->GetWindowText(pinButtonText);
	if (pinButtonText.CompareNoCase(_T("HIGH")) == 0)
		pinButton->SetWindowText(_T("LOW"));
	else if (pinButtonText.CompareNoCase(_T("LOW")) == 0)
	{
		pinButton->SetWindowText(_T("HIGH"));
		pinButton->SetButtonStyle(1,true);
	}
	else if (pinButtonText.CompareNoCase(_T("N/A")) == 0)
	{
		pinButton->SetWindowText(_T("LOW"));		
		pinButton->SetButtonStyle(1,true);
	}
}

void CGioOutputDlg::OnPin1() 
{
	ChangePinState((CButton*)GetDlgItem(IDC_SCG_PIN1));	
	UpdateIO();
}
void CGioOutputDlg::OnPin2() 
{
	ChangePinState((CButton*)GetDlgItem(IDC_SCG_PIN2));	
	UpdateIO();
}
void CGioOutputDlg::OnPin3() 
{
	ChangePinState((CButton*)GetDlgItem(IDC_SCG_PIN3));	
	UpdateIO();
}
void CGioOutputDlg::OnPin4() 
{
	ChangePinState((CButton*)GetDlgItem(IDC_SCG_PIN4));	
	UpdateIO();
}
void CGioOutputDlg::OnPin5() 
{
	ChangePinState((CButton*)GetDlgItem(IDC_SCG_PIN5));	
	UpdateIO();
}
void CGioOutputDlg::OnPin6() 
{
	ChangePinState((CButton*)GetDlgItem(IDC_SCG_PIN6));	
	UpdateIO();
}
void CGioOutputDlg::OnPin7() 
{
	ChangePinState((CButton*)GetDlgItem(IDC_SCG_PIN7));	
	UpdateIO();
}
void CGioOutputDlg::OnPin8() 
{
	ChangePinState((CButton*)GetDlgItem(IDC_SCG_PIN8));	
	UpdateIO();
}
void CGioOutputDlg::OnPin9() 
{
	ChangePinState((CButton*)GetDlgItem(IDC_SCG_PIN9));	
	UpdateIO();
}
void CGioOutputDlg::OnPin10() 
{
	ChangePinState((CButton*)GetDlgItem(IDC_SCG_PIN10));	
	UpdateIO();
}
void CGioOutputDlg::OnPin11() 
{
	ChangePinState((CButton*)GetDlgItem(IDC_SCG_PIN11));	
	UpdateIO();
}
void CGioOutputDlg::OnPin12() 
{
	ChangePinState((CButton*)GetDlgItem(IDC_SCG_PIN12));	
	UpdateIO();
}

