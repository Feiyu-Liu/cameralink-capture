// RawFileDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SapClassBasic.h"
#include "SapClassGui.h"
#include "RawFileDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRawFileDlg dialog
SapFormatInfo CRawFileDlg::m_FormatInfo[] =
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
	{SapFormatHSIP8,   _T("HSI Planar 8-bit")},
	{SapFormatHSV,	    _T("HSV")},
	{SapFormatUYVY,	 _T("UYVY")},
	{SapFormatYUY2,	 _T("YUY2")},
	{SapFormatYUYV,	 _T("YUYV")},
	{SapFormatLAB,	    _T("LAB")},
	{SapFormatLABP8,   _T("LAB Planar 8-bit")},
	{SapFormatLAB101010,_T("LAB 101010")},
	{SapFormatLABP16,  _T("LAB Planar 16-bit")},
   {SapFormatBICOLOR88,    _T("BICOLOR 8-bit")},
   {SapFormatBICOLOR1616,  _T("BICOLOR 16-bit")},
   {SapFormatBICOLOR1212,  _T("BICOLOR 12-bit")},
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
   {SapFormatRGB161616_MONO16P2,_T("RGB-IR 16-bit (2 IR planes)")},
};

CRawFileDlg::CRawFileDlg(CWnd* pParent, int width, int height, int offset, SapFormat format)
	: CDialog(CRawFileDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRawFileDlg)
	//}}AFX_DATA_INIT

	m_App = AfxGetApp();
	m_FileRawWidth = m_App->GetProfileInt(_T("SapFile"), _T("Raw Width"), width);
	m_FileRawHeight = m_App->GetProfileInt(_T("SapFile"), _T("Raw Height"), height);
	m_FileRawOffset = m_App->GetProfileInt(_T("SapFile"), _T("Raw Offset"), offset);
   m_FileRawFormat = format;
}


void CRawFileDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRawFileDlg)
	DDX_Text(pDX, IDC_SCG_RAW_EDIT_HEIGHT, m_FileRawHeight);
	DDX_Text(pDX, IDC_SCG_RAW_EDIT_WIDTH, m_FileRawWidth);
	DDX_Text(pDX, IDC_SCG_RAW_EDIT_OFFSET, m_FileRawOffset);
   DDX_Control(pDX, IDC_SCG_RAW_COMBO_FORMAT, m_cbFormat);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CRawFileDlg, CDialog)
	//{{AFX_MSG_MAP(CRawFileDlg)
   ON_CBN_SELCHANGE(IDC_SCG_RAW_COMBO_FORMAT, OnSelchangeComboFormat)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRawFileDlg message handlers

void CRawFileDlg::OnOK() 
{
	CDialog::OnOK();

   m_FileRawFormat = (SapFormat) m_cbFormat.GetItemData(m_cbFormat.GetCurSel());

	m_App->WriteProfileInt(_T("SapFile"), _T("Raw Width"), m_FileRawWidth);
	m_App->WriteProfileInt(_T("SapFile"), _T("Raw Height"), m_FileRawHeight);
	m_App->WriteProfileInt(_T("SapFile"), _T("Raw Offset"), m_FileRawOffset);
   m_App->WriteProfileInt(_T("SapFile"), _T("Raw Format"), m_FileRawFormat);
}

BOOL CRawFileDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// Initialize format combo
	for (int i= 0; i < sizeof(m_FormatInfo) / sizeof(m_FormatInfo[0]); i++)
	{
		int index = m_cbFormat.AddString(CString(m_FormatInfo[i].m_Name));
		m_cbFormat.SetItemData(index, m_FormatInfo[i].m_Value);
			
		if ((m_FormatInfo[i].m_Value == m_FileRawFormat) || ((m_FileRawFormat == 0) && (m_FormatInfo[i].m_Value == SapFormatMono8)))
			m_cbFormat.SetCurSel(index);
	}

   if (m_FileRawFormat != 0)
      m_cbFormat.EnableWindow(FALSE);

   return TRUE;
}

void CRawFileDlg::OnSelchangeComboFormat() 
{
   // Update format
	int index= m_cbFormat.GetCurSel();
	if (index != CB_ERR)
		m_FileRawFormat = (SapFormat) m_cbFormat.GetItemData(index);
}