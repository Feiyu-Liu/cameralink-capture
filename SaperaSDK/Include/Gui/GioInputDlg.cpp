// GioInputDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SapClassBasic.h"
#include "SapClassGui.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define RISING_EDGE_CALLBACK "Rising Edge"
#define FALLING_EDGE_CALLBACK "Falling Edge"
#define FAULT_CALLBACK "fault"

#ifndef CORGIO_VAL_EVENT_TYPE_FAULT
#define CORGIO_VAL_EVENT_TYPE_FAULT 4
#endif

/////////////////////////////////////////////////////////////////////////////
// CGioInputDlg dialog


CGioInputDlg::CGioInputDlg(CWnd* pParent,UINT iDevice, SapGio *pGio)
	: CDialog(CGioInputDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGioInputDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	int iIO;
	m_pParent = pParent;
	m_iDevice = iDevice;
	m_pGio = pGio;

	for (iIO=0; iIO<MAX_INPUT_GIO_IO; iIO++)
	{
		m_gioContext[iIO].pDlg = this;
		m_gioContext[iIO].io = iIO;
		m_GioEventCount[iIO] = 0;
	}
}


void CGioInputDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGioInputDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	DDX_Control(pDX, IDC_SCG_GIO_INPUT, m_Static_Label);
	DDX_Control(pDX, IDC_SCG_COMBO_SIGNAL_EVENT, m_Combo_Signal_Event);
	DDX_Control(pDX, IDC_SCG_COMBO_INPUT_LEVEL, m_Combo_Input_Level);
	//}}AFX_DATA_MAP
}

UINT CGioInputDlg::MoveWindow(UINT offset)
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

BEGIN_MESSAGE_MAP(CGioInputDlg, CDialog)
	//{{AFX_MSG_MAP(CGioInputDlg)
	ON_BN_CLICKED(IDC_SCG_ENABLE1, OnEnableCallback)
	ON_CBN_SELCHANGE(IDC_SCG_COMBO_INPUT_LEVEL, OnSelchangeComboInputLevel)
	ON_CBN_SELCHANGE(IDC_SCG_COMBO_SIGNAL_EVENT, OnSelchangeComboSignalEvent)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_SCG_ENABLE2, OnEnableCallback)
	ON_BN_CLICKED(IDC_SCG_ENABLE3, OnEnableCallback)
	ON_BN_CLICKED(IDC_SCG_ENABLE4, OnEnableCallback)
	ON_BN_CLICKED(IDC_SCG_ENABLE5, OnEnableCallback)
	ON_BN_CLICKED(IDC_SCG_ENABLE6, OnEnableCallback)
	ON_BN_CLICKED(IDC_SCG_ENABLE7, OnEnableCallback)
	ON_BN_CLICKED(IDC_SCG_ENABLE8, OnEnableCallback)
	ON_BN_CLICKED(IDC_SCG_ENABLE9, OnEnableCallback)
	ON_BN_CLICKED(IDC_SCG_ENABLE10, OnEnableCallback)
	ON_BN_CLICKED(IDC_SCG_ENABLE11, OnEnableCallback)
	ON_BN_CLICKED(IDC_SCG_ENABLE12, OnEnableCallback)
	ON_BN_CLICKED(IDC_SCG_BT_RESET, OnBtReset)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGioInputDlg message handlers

BOOL CGioInputDlg::PreTranslateMessage(MSG* pMsg) 
{ 
	if(pMsg->message==WM_KEYDOWN && pMsg->wParam == VK_ESCAPE) 
		return true; 

	return CDialog::PreTranslateMessage(pMsg); 
} 

void CGioInputDlg::GioCallbackInfo(SapGioCallbackInfo *pInfo)
{
CGioInputDlg* pInputDlg;

    pInputDlg = (CGioInputDlg*)pInfo->GetContext();

    // Set the Input event counter
	pInputDlg->m_GioEventCount[pInfo->GetPinNumber()]++;
}


BOOL CGioInputDlg::Create()
{
	BOOL bCreate;
	bCreate = CDialog::Create(CGioInputDlg::IDD,m_pParent);
	if (!bCreate)
		return FALSE;

	return TRUE;
}


BOOL CGioInputDlg::OnInitDialog() 
{
	BOOL status;
	UINT32 capInputLevel=0,capInput=0,prmInputLevel=0,capInputControl=0,prmInputControl=0;
	UINT32 capInputControlLevel=0,prmInputControlLevel=0;
	UINT32 capConnector=0,prmConnector=0,capEventType=0;
   UINT32 capReadOnly=0;
	UINT32 iSelection,iIO;
	char label[128];

	CDialog::OnInitDialog();

	// Association of CallBack function for Input Module
	m_pGio->SetCallbackInfo(GioCallbackInfo,this);

	if (m_pGio->IsCapabilityValid(CORGIO_CAP_INPUT_LEVEL))
	   status = m_pGio->GetCapability(CORGIO_CAP_INPUT_LEVEL,&capInputLevel);
	if (m_pGio->IsCapabilityValid(CORGIO_CAP_DIR_INPUT))
	   status = m_pGio->GetCapability(CORGIO_CAP_DIR_INPUT,&capInput);
	if (m_pGio->IsParameterValid(CORGIO_PRM_INPUT_LEVEL))
	   status = m_pGio->GetParameter(CORGIO_PRM_INPUT_LEVEL,&prmInputLevel); 
	if (m_pGio->IsCapabilityValid(CORGIO_CAP_INPUT_CONTROL_METHOD))
	   status = m_pGio->GetCapability(CORGIO_CAP_INPUT_CONTROL_METHOD,&capInputControl); 
	if (m_pGio->IsParameterValid(CORGIO_PRM_INPUT_CONTROL_METHOD))
	   status = m_pGio->GetParameter(CORGIO_PRM_INPUT_CONTROL_METHOD,&prmInputControl); 

	if (m_pGio->IsCapabilityValid(CORGIO_CAP_CONNECTOR))
	   status = m_pGio->GetCapability(CORGIO_CAP_CONNECTOR,&capConnector); 
	if (m_pGio->IsParameterValid(CORGIO_PRM_CONNECTOR))
	   status = m_pGio->GetParameter(CORGIO_PRM_CONNECTOR,&prmConnector); 
	if (m_pGio->IsCapabilityValid(CORGIO_CAP_EVENT_TYPE))
	   status = m_pGio->GetCapability(CORGIO_CAP_EVENT_TYPE,&capEventType); 
	if (m_pGio->IsCapabilityValid(CORGIO_CAP_READ_ONLY))
		status = m_pGio->GetCapability(CORGIO_CAP_READ_ONLY,&capReadOnly);


	if (m_pGio->IsParameterValid(CORGIO_PRM_LABEL))
	   status = m_pGio->GetParameter(CORGIO_PRM_LABEL,label);
	if (m_pGio->IsCapabilityValid(CORGIO_CAP_INPUT_CONTROL_POLARITY))
	   status = m_pGio->GetCapability(CORGIO_CAP_INPUT_CONTROL_POLARITY,&capInputControlLevel); 
	if (m_pGio->IsParameterValid(CORGIO_PRM_INPUT_CONTROL_POLARITY))
	   status = m_pGio->GetParameter(CORGIO_PRM_INPUT_CONTROL_POLARITY,&prmInputControlLevel); 
   
   label[sizeof(label) - 1] = '\0';   
   if( !strstr( label, "input") && !strstr( label, "Input"))
   {
      CorStrncat(label," (input)", sizeof(label));
   }

	m_Static_Label.SetWindowText(CString(label));

	// enable only the IO available
	for (iIO=0; iIO < (UINT32)m_pGio->GetNumPins(); iIO++)
	{
		((CButton*)GetDlgItem(IDC_SCG_ST_PIN1 + iIO))->EnableWindow(capInput & (1<<iIO));
		((CButton*)GetDlgItem(IDC_SCG_ENABLE1  + iIO))->EnableWindow(capInput & (1<<iIO));		 
		((CButton*)GetDlgItem(IDC_SCG_ST_COUNT1 + iIO))->EnableWindow(capInput & (1<<iIO));		 
	}


	// Fill Input Level combo box
   // Can only change Input Level if none of the Inputs are reserved for other modules (such as ACQ)
   if (capInputLevel & CORGIO_VAL_INPUT_LEVEL_LVTTL)
   {
      if( !capReadOnly || (prmInputLevel & CORGIO_VAL_INPUT_LEVEL_LVTTL))
      {
		   iSelection = m_Combo_Input_Level.AddString(_T("3.3-Volts Single Ended"));
         m_Combo_Input_Level.SetItemData( iSelection, CORGIO_VAL_INPUT_LEVEL_LVTTL);
		   if (prmInputLevel & CORGIO_VAL_INPUT_LEVEL_LVTTL)
   		   m_Combo_Input_Level.SetCurSel(iSelection);
      }
   }
   if (capInputLevel & CORGIO_VAL_INPUT_LEVEL_TTL)
   {
      if( !capReadOnly || (prmInputLevel & CORGIO_VAL_INPUT_LEVEL_TTL))
      {
		   iSelection = m_Combo_Input_Level.AddString(_T("5-Volts Single Ended"));
         m_Combo_Input_Level.SetItemData( iSelection, CORGIO_VAL_INPUT_LEVEL_TTL);
   	   if (prmInputLevel & CORGIO_VAL_INPUT_LEVEL_TTL)
	   	   m_Combo_Input_Level.SetCurSel(iSelection);
      }
   }
   if (capInputLevel & CORGIO_VAL_INPUT_LEVEL_12VOLTS)
   {
      if( !capReadOnly || (prmInputLevel & CORGIO_VAL_INPUT_LEVEL_12VOLTS))
      {
		   iSelection = m_Combo_Input_Level.AddString(_T("12-Volts Single Ended"));
         m_Combo_Input_Level.SetItemData( iSelection, CORGIO_VAL_INPUT_LEVEL_12VOLTS);
   	   if (prmInputLevel & CORGIO_VAL_INPUT_LEVEL_12VOLTS)
	   	   m_Combo_Input_Level.SetCurSel(iSelection);
      }
   }
   if (capInputLevel & CORGIO_VAL_INPUT_LEVEL_24VOLTS)
   {
      if( !capReadOnly || (prmInputLevel & CORGIO_VAL_INPUT_LEVEL_24VOLTS))
      {
		   iSelection = m_Combo_Input_Level.AddString(_T("24-Volts Single Ended"));
         m_Combo_Input_Level.SetItemData( iSelection, CORGIO_VAL_INPUT_LEVEL_24VOLTS);
   	   if (prmInputLevel & CORGIO_VAL_INPUT_LEVEL_24VOLTS)
	   	   m_Combo_Input_Level.SetCurSel(iSelection);
      }
   }
   if (capInputLevel & CORGIO_VAL_INPUT_LEVEL_LVDS)
   {
      if( !capReadOnly || (prmInputLevel & CORGIO_VAL_INPUT_LEVEL_LVDS))
      {
		   iSelection = m_Combo_Input_Level.AddString(_T("LVDS RS-644"));
         m_Combo_Input_Level.SetItemData( iSelection, CORGIO_VAL_INPUT_LEVEL_LVDS);
	      if (prmInputLevel & CORGIO_VAL_INPUT_LEVEL_LVDS)
		      m_Combo_Input_Level.SetCurSel(iSelection);
      }
   }
   if (capInputLevel & CORGIO_VAL_INPUT_LEVEL_422)
   {
      if( !capReadOnly || (prmInputLevel & CORGIO_VAL_INPUT_LEVEL_422))
      {
		   iSelection = m_Combo_Input_Level.AddString(_T("RS-422"));
         m_Combo_Input_Level.SetItemData( iSelection, CORGIO_VAL_INPUT_LEVEL_422);
   	   if (prmInputLevel & CORGIO_VAL_INPUT_LEVEL_422)
	   	   m_Combo_Input_Level.SetCurSel(iSelection);
      }
   }

	m_Combo_Input_Level.EnableWindow(prmInputLevel != 0);

	// Fill Combo Signal Event
	if (capEventType & CORGIO_VAL_EVENT_TYPE_RISING_EDGE)
   {
		iSelection = m_Combo_Signal_Event.AddString(_T(RISING_EDGE_CALLBACK));
      m_Combo_Signal_Event.SetItemData( iSelection, CORGIO_VAL_EVENT_TYPE_RISING_EDGE);
   }
	if (capEventType & CORGIO_VAL_EVENT_TYPE_FALLING_EDGE)
   {
		iSelection = m_Combo_Signal_Event.AddString(_T(FALLING_EDGE_CALLBACK));
      m_Combo_Signal_Event.SetItemData( iSelection, CORGIO_VAL_EVENT_TYPE_FALLING_EDGE);
   }
	if (capEventType & CORGIO_VAL_EVENT_TYPE_FAULT)
   {
		iSelection = m_Combo_Signal_Event.AddString(_T(FAULT_CALLBACK));
      m_Combo_Signal_Event.SetItemData( iSelection, CORGIO_VAL_EVENT_TYPE_FAULT);
   }
	if (capEventType)
		m_Combo_Signal_Event.SetCurSel(0);
	else
		m_Combo_Signal_Event.EnableWindow(FALSE);

	if (m_Combo_Signal_Event.GetCount()==0)
	{
		m_Combo_Signal_Event.EnableWindow(FALSE);
		// enable only the IO available
		for (iIO=0; iIO < (UINT32)m_pGio->GetNumPins(); iIO++)
		{
			((CButton*)GetDlgItem(IDC_SCG_ENABLE1  + iIO))->EnableWindow(FALSE);		 
		}
	}
	if (m_Combo_Input_Level.GetCount()==0)
		m_Combo_Input_Level.EnableWindow(FALSE);

	return TRUE;
}

void CGioInputDlg::OnSelchangeComboSignalEvent() 
{
	UpdateData(TRUE);
	UINT32 iSelection;

	iSelection = m_Combo_Signal_Event.GetCurSel();

	OnEnableCallback();
}



void CGioInputDlg::OnEnableCallback() 
{
	BOOL status;
	UINT32 iIO, jIO;
	int iSelection = -1;
   UINT32 eventType;

	UpdateData(TRUE);

	iSelection = m_Combo_Signal_Event.GetCurSel();
   eventType = (UINT32)m_Combo_Signal_Event.GetItemData( iSelection);


	for (iIO=0; iIO < (UINT32)m_pGio->GetNumPins(); iIO++)
	{
			status = m_pGio->DisableCallback(iIO);
	}

	for (jIO=0; jIO < (UINT32)m_pGio->GetNumPins(); jIO++)
	{
		if (((CButton*)GetDlgItem(IDC_SCG_ENABLE1+jIO))->GetCheck())
		{
			status = m_pGio->EnableCallback(jIO, eventType);
		}
	}
}

	
BOOL CGioInputDlg::Update()
{
	SapGio::PinState *states = NULL;
	BOOL status = true;
	UINT32 iIO;
	UINT32 jIO;

	if (m_pGio == NULL)
		return FALSE;

   // Get the state of all input pins of the current I/O group.
   // Note: It is also possible to get the state of a single input pin if
	//       desired. See SapGio::GetPinState(int pinNumber, PinState *pPinState)
	states = new SapGio::PinState[m_pGio->GetNumPins()];
	m_pGio->SetDisplayStatusMode(SapManager::StatusLog, NULL);
	status = m_pGio->GetPinState(states);
	m_pGio->SetDisplayStatusMode(SapManager::StatusNotify, NULL);

	for (iIO=0; iIO < (UINT32)m_pGio->GetNumPins(); iIO++)
	{
		// Pins Input Edit controls identifiers is a sequence.
		if (states[iIO] == SapGio::PinLow)
			((CButton*)GetDlgItem(IDC_SCG_ST_PIN1 + iIO))->SetWindowText(_T("LOW"));
		else if (states[iIO] == SapGio::PinHigh)
			((CButton*)GetDlgItem(IDC_SCG_ST_PIN1 + iIO))->SetWindowText(_T("HIGH"));
	}

	delete [] states;
	states = NULL;

	for (jIO=0; jIO < (UINT32)m_pGio->GetNumPins(); jIO++)
	{
		CString str;
		str.Format(_T("%d"),this->m_GioEventCount[jIO]);
		((CButton*)GetDlgItem(IDC_SCG_ST_COUNT1 + jIO))->SetWindowText(str);
	}
	return status;
}


void CGioInputDlg::OnSelchangeComboInputLevel() 
{
	UINT32 iSelection=0,prmInputLevel=0,currentPrmInputLevel=0;
	CORSTATUS status=0;

	UpdateData(TRUE);

	iSelection = m_Combo_Input_Level.GetCurSel();

   prmInputLevel = (UINT32)m_Combo_Input_Level.GetItemData( iSelection);

   status = m_pGio->GetParameter(CORGIO_PRM_INPUT_LEVEL,&currentPrmInputLevel);

   if (prmInputLevel != currentPrmInputLevel)
      status = m_pGio->SetParameter(CORGIO_PRM_INPUT_LEVEL, prmInputLevel);
}


HBRUSH CGioInputDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
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

BOOL CGioInputDlg::ResetInterruptCount()
{
	UINT32 iIO;

	if (m_pGio == NULL)
		return FALSE;

	for (iIO=0; iIO < (UINT32)m_pGio->GetNumPins(); iIO++)
	{
		CString str;
		m_GioEventCount[iIO]=0;
		str.Format(_T("%d"),m_GioEventCount[iIO]);
		((CButton*)GetDlgItem(IDC_SCG_ST_COUNT1 + iIO))->SetWindowText(str);
	}
	return TRUE;
}



void CGioInputDlg::OnBtReset() 
{
	ResetInterruptCount();
}
