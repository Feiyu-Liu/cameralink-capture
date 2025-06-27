// BufDlg.cpp : implementation file
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
// Static variables and methods

SapFormatInfo CBufDlg::m_FormatInfo[] =
{
	{SapFormatMono1,    _T("Monochrome 1-bit")},
	{SapFormatMono8,    _T("Monochrome 8-bit")},
	{SapFormatMono16,   _T("Monochrome 16-bit")},
	{SapFormatRGB5551,  _T("RGB 5551")},
	{SapFormatRGB565,   _T("RGB 565")},
	{SapFormatRGB888,   _T("RGB 888 (blue first)")},
	{SapFormatRGBR888,  _T("RGBR 888 (red first)")},
	{SapFormatRGB8888,  _T("RGB 8888")},
	{SapFormatRGB101010,_T("RGB 101010")},
	{SapFormatRGB161616,_T("RGB 161616")},
   {SapFormatRGB16161616,_T("RGB 16161616")},
	{SapFormatRGBP8,	  _T("RGB Planar 8-bit")},
	{SapFormatRGBP16,   _T("RGB Planar 16-bit")},
	{SapFormatHSI,	     _T("HSI")},
	{SapFormatHSIP8,    _T("HSI Planar 8-bit")},
	{SapFormatHSV,	     _T("HSV")},
	{SapFormatUYVY,	  _T("UYVY")},
	{SapFormatYUY2,	  _T("YUY2")},
	{SapFormatYUYV,	  _T("YUYV")},
	{SapFormatLAB,	     _T("LAB")},
	{SapFormatLABP8,    _T("LAB Planar 8-bit")},
	{SapFormatLAB101010,_T("LAB 101010")},
	{SapFormatLABP16,   _T("LAB Planar 16-bit")},
   {SapFormatBICOLOR88,  _T("BICOLOR 8-bit")},
   {SapFormatBICOLOR1212, _T("BICOLOR 12-bit packed")},
   {SapFormatBICOLOR1616,_T("BICOLOR 16-bit")},
   {SapFormatRGB888_MONO8,  _T("RGB-IR 8-bit")},
   {SapFormatRGB161616_MONO16,  _T("RGB-IR 16-bit")},
   {SapFormatMono8P2,  _T("Monochrome 8-bit (2 planes)")},
   {SapFormatMono8P3,  _T("Monochrome 8-bit (3 planes)")},
   {SapFormatMono8P4,  _T("Monochrome 8-bit (4 planes)")},
   {SapFormatMono8P5,  _T("Monochrome 8-bit (5 planes)")},
   {SapFormatMono8P6,  _T("Monochrome 8-bit (6 planes)")},
   {SapFormatMono8P7,  _T("Monochrome 8-bit (7 planes)")},
   {SapFormatMono8P8,  _T("Monochrome 8-bit (8 planes)")},
   {SapFormatMono8P9,  _T("Monochrome 8-bit (9 planes)")},
   {SapFormatMono8P10, _T("Monochrome 8-bit (10 planes)")},
   {SapFormatMono16P2, _T("Monochrome 16-bit (2 planes)")},
   {SapFormatMono16P3, _T("Monochrome 16-bit (3 planes)")},
   {SapFormatMono16P4, _T("Monochrome 16-bit (4 planes)")},
   {SapFormatMono16P5, _T("Monochrome 16-bit (5 planes)")},
   {SapFormatMono16P6, _T("Monochrome 16-bit (6 planes)")},
   {SapFormatMono16P7, _T("Monochrome 16-bit (7 planes)")},
   {SapFormatMono16P8, _T("Monochrome 16-bit (8 planes)")},
   {SapFormatMono16P9, _T("Monochrome 16-bit (9 planes)")},
   {SapFormatMono16P10, _T("Monochrome 16-bit (10 planes)")},
	{SapFormatRGBAP8,	  _T("RGB Alpha Planar 8-bit")},
	{SapFormatRGBAP16,   _T("RGB Alpha Planar 16-bit")},
   {SapFormatRGB888_MONO8P2  ,_T("RGB-IR 8-bit (2 IR planes)")},
   {SapFormatRGB161616_MONO16P2, _T("RGB-IR 16-bit (2 IR planes)")},

};

/////////////////////////////////////////////////////////////////////////////
// CBufDlg dialog

CBufDlg::CBufDlg(CWnd* pParent, SapBuffer *pBuffer, SapDisplay *pDisplay)
	: CDialog(CBufDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CBufDlg)
	m_PixelDepth = 0;
	//}}AFX_DATA_INIT

	SapBuffer buf;
	if (!pBuffer) pBuffer = &buf;

	m_Count = pBuffer->GetCount();
   m_Width = pBuffer->GetWidth();
   m_Height = pBuffer->GetHeight();
   m_Format = pBuffer->GetFormat();
   m_Type = pBuffer->GetType();
   m_PixelDepth = pBuffer->GetPixelDepth();
	m_pSrcNode = pBuffer->GetSrcNode();
   m_Location = pBuffer->GetLocation();
   m_pDisplay = pDisplay;

   m_ScatterGatherType = SapBuffer::TypeScatterGather;

   // For acquisition hardware with DMA transfers using 32-bit addresses in 64-bit Windows (e.g., X64-CL iPro)
   if (m_pSrcNode)
   {
      if (!SapBuffer::IsBufferTypeSupported(m_pSrcNode->GetLocation(), SapBuffer::TypeScatterGather))
         m_ScatterGatherType = SapBuffer::TypeScatterGatherPhysical;
   }
}

BOOL CBufDlg::UpdateBuffer()
{
	// Set new parameters
	m_Buffer.SetCount(m_Count);
   m_Buffer.SetWidth(m_Width);
   m_Buffer.SetHeight(m_Height);
   m_Buffer.SetFormat(m_Format);
   m_Buffer.SetType(m_Type);
   m_Buffer.SetPixelDepth(m_PixelDepth);
	m_Buffer.SetSrcNode(m_pSrcNode);
	m_Buffer.SetLocation(m_Location);
	return TRUE;
}

void CBufDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBufDlg)
	DDX_Control(pDX, IDC_SCG_BUFFER_FORMAT, m_cbFormat);
	DDX_Text(pDX, IDC_SCG_BUFFER_WIDTH, m_Width);
	DDX_Text(pDX, IDC_SCG_BUFFER_HEIGHT, m_Height);
	DDX_Text(pDX, IDC_SCG_BUFFER_COUNT, m_Count);
	DDV_MinMaxUInt(pDX, m_Count, 1, UINT_MAX);
	DDX_Text(pDX, IDC_SCG_BUFFER_PIXEL_DEPTH, m_PixelDepth);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBufDlg, CDialog)
	//{{AFX_MSG_MAP(CBufDlg)
	ON_CBN_SELCHANGE(IDC_SCG_BUFFER_FORMAT, OnSelchangeComboFormat)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBufDlg message handlers

BOOL CBufDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// Initialize format combo
	for (int i= 0; i < sizeof(m_FormatInfo) / sizeof(m_FormatInfo[0]); i++)
	{
		m_cbFormat.AddString(CString(m_FormatInfo[i].m_Name));
		m_cbFormat.SetItemData(i, m_FormatInfo[i].m_Value);
			
		if (m_FormatInfo[i].m_Value == m_Format)
			m_cbFormat.SetCurSel(i);
	}

   EnableControls();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CBufDlg::OnSelchangeComboFormat() 
{
   // Update format
	int index= m_cbFormat.GetCurSel();
	if (index != CB_ERR)
		m_Format = (SapFormat) m_cbFormat.GetItemData(index);

   // Reset pixel depth to max
   UpdateData(TRUE);
   m_PixelDepth = SapManager::GetPixelDepthMax(m_Format);
   UpdateData(FALSE);

   EnableControls();
}

void CBufDlg::OnOK() 
{
	UpdateData(TRUE);

	UpdateBuffer();
	
	CDialog::OnOK();
}

void CBufDlg::EnableControls()
{
	// Lock some parameters
	GetDlgItem(IDC_SCG_BUFFER_WIDTH)->EnableWindow(m_pSrcNode == NULL);
	GetDlgItem(IDC_SCG_BUFFER_HEIGHT)->EnableWindow(m_pSrcNode == NULL);
	GetDlgItem(IDC_SCG_BUFFER_FORMAT)->EnableWindow(m_pSrcNode == NULL);

   // Is pixel depth adjustable?
   if (SapManager::GetPixelDepthMin(m_Format) != SapManager::GetPixelDepthMax(m_Format))
   	GetDlgItem(IDC_SCG_BUFFER_PIXEL_DEPTH)->EnableWindow(TRUE);
   else
   	GetDlgItem(IDC_SCG_BUFFER_PIXEL_DEPTH)->EnableWindow(FALSE);
}
