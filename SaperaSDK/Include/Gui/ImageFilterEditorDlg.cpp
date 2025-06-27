// ImageFilterEditorDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SapClassGui.h"

#pragma warning(disable:4995)

#include <string>
#include <sstream>

typedef struct
{
   SapAcquisition::ImageFilterKernelSize param;
   int dim;
} FILTER_SIZE_PAIR;

static const FILTER_SIZE_PAIR FILTER_SIZES_PRM[] = { { SapAcquisition::ImageFilterSize1x1, 1 }, { SapAcquisition::ImageFilterSize2x2, 2 },
{ SapAcquisition::ImageFilterSize3x3, 3 }, { SapAcquisition::ImageFilterSize4x4, 4 }, { SapAcquisition::ImageFilterSize5x5, 5 },
{ SapAcquisition::ImageFilterSize6x6, 6 }, { SapAcquisition::ImageFilterSize7x7, 7 } };

static int ConvertKernelSizeToInt(SapAcquisition::ImageFilterKernelSize ksize)
{
   for (int i = 0; i < sizeof(FILTER_SIZES_PRM) / sizeof(FILTER_SIZES_PRM[0]); i++)
   {
      if (FILTER_SIZES_PRM[i].param == ksize)
         return FILTER_SIZES_PRM[i].dim;
   }

   return 0;
}

// CImageFilterEditorDlg dialog

IMPLEMENT_DYNAMIC(CImageFilterEditorDlg, CDialog)

CImageFilterEditorDlg::CImageFilterEditorDlg(SapAcquisition* pAcq, CWnd* pParent /*=NULL*/)
: CDialog(CImageFilterEditorDlg::IDD, pParent)
{
   mp_Acq = pAcq;

   m_Divisor = 1;
   m_MinValue = 0;
   m_MaxValue = 0;
   m_CoeffMinValue = _T("N/A");
   m_CoeffMaxValue = _T("N/A");
   m_CoeffEditCount = 0;
   mp_CoeffEditBoxes = NULL;
}

CImageFilterEditorDlg::~CImageFilterEditorDlg()
{
   DestroyCoeffEditControls();
}

BOOL CImageFilterEditorDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   if (!mp_Acq || !*mp_Acq)
   {
      MessageBox(_T("The acquisition device passed in is invalid."), _T("Image Filter Editor"), MB_ICONERROR);
      EndDialog(TRUE);
      return FALSE;
   }

   if (!mp_Acq->IsImageFilterAvailable())
   {
      MessageBox(_T("Image Filter is incompatible with this device."), _T("Image Filter Editor"), MB_ICONERROR);
      EndDialog(TRUE);
      return FALSE;
   }

   SapAcquisition::ImageFilterKernelSize kernelSize;
   if (mp_Acq->GetImageFilterKernelSize(0, &kernelSize))
   {
      if (kernelSize == SapAcquisition::ImageFilterSize4x3)
      {
         MessageBox(_T("Not available for color correction coefficients.\nUse the Sapera Color Calibration tool instead."), _T("Image Filter Editor"), MB_ICONERROR);
         EndDialog(TRUE);
         return FALSE;
      }
   }

   if (mp_Acq->IsCapabilityValid(CORACQ_CAP_IMAGE_FILTER_KERNEL_DIVISOR))
      mp_Acq->GetCapability(CORACQ_CAP_IMAGE_FILTER_KERNEL_DIVISOR, &m_Divisor);

   if (mp_Acq->IsCapabilityValid(CORACQ_CAP_IMAGE_FILTER_KERNEL_VALUE_MIN))
      mp_Acq->GetCapability(CORACQ_CAP_IMAGE_FILTER_KERNEL_VALUE_MIN, &m_MinValue);

   if (mp_Acq->IsCapabilityValid(CORACQ_CAP_IMAGE_FILTER_KERNEL_VALUE_MAX))
      mp_Acq->GetCapability(CORACQ_CAP_IMAGE_FILTER_KERNEL_VALUE_MAX, &m_MaxValue);

   InitControls();

   m_FilterIndexCombo.SetCurSel(0);

   UpdateKernelSizeCombo();

   if (!UpdateState())
      return FALSE;

   return TRUE;
}

bool CImageFilterEditorDlg::CreateCoeffEditControls(int count)
{
   DestroyCoeffEditControls();

   mp_CoeffEditBoxes = new CEdit[count];
   if (!mp_CoeffEditBoxes)
      return false;

   m_CoeffEditCount = count;

   for (int iEdit = 0; iEdit < m_CoeffEditCount; iEdit++)
   {
      CEdit& edit = mp_CoeffEditBoxes[iEdit];
      if (edit.Create(ES_CENTER | WS_CHILD | WS_TABSTOP | WS_BORDER, CRect(), this, iEdit + 1) == FALSE)
         return false;

      edit.SetFont(m_FilterIndexCombo.GetFont());
   }

   return true;
}

void CImageFilterEditorDlg::DestroyCoeffEditControls()
{
   if (mp_CoeffEditBoxes)
   {
      for (int iEdit = 0; iEdit < m_CoeffEditCount; iEdit++)
         mp_CoeffEditBoxes[iEdit].DestroyWindow();

      delete[] mp_CoeffEditBoxes;
      mp_CoeffEditBoxes = NULL;
      m_CoeffEditCount = 0;
   }
}

void CImageFilterEditorDlg::InitControls()
{
   CString sText;
   int nItem;

   /*
   filter indexes
   */

   INT32 nFilterCountHW = 0;
   if (mp_Acq->IsCapabilityValid(CORACQ_CAP_IMAGE_FILTER_MAX))
      mp_Acq->GetCapability(CORACQ_CAP_IMAGE_FILTER_MAX, &nFilterCountHW);

   for (int i = 0; i < nFilterCountHW; i++)
   {
      sText.Format(_T("%d"), i);
      nItem = m_FilterIndexCombo.InsertString(-1, sText);
      m_FilterIndexCombo.SetItemData(nItem, i);
   }

   /*
   kernel sizes
   */

   UINT32 kernelSizesHW = 0;
   if (mp_Acq->IsCapabilityValid(CORACQ_CAP_IMAGE_FILTER_KERNEL_SIZE))
      mp_Acq->GetCapability(CORACQ_CAP_IMAGE_FILTER_KERNEL_SIZE, &kernelSizesHW);

   for (int i = 0; i < (sizeof(FILTER_SIZES_PRM) / sizeof(FILTER_SIZES_PRM[0])); i++)
   {
      if (kernelSizesHW & FILTER_SIZES_PRM[i].param)
      {
         int nSize = i + 1;
         sText.Format(_T("%d x %d"), nSize, nSize);
         nItem = m_KernelSizeCombo.InsertString(-1, sText);
         m_KernelSizeCombo.SetItemData(nItem, FILTER_SIZES_PRM[i].param);
      }
   }

   /*
   min/max values
   */

   m_CoeffMinValue.Format(_T("%.5f"), (double)m_MinValue / m_Divisor);
   m_CoeffMaxValue.Format(_T("%.5f"), (double)m_MaxValue / m_Divisor);


   UpdateData(FALSE);
}

bool CImageFilterEditorDlg::UpdateState()
{
   BOOL bEnable = mp_Acq->IsImageFilterEnabled();
   m_EnableCheckbox.SetCheck(bEnable ? BST_CHECKED : BST_UNCHECKED);

   ArrangeCoeffEditControls();
   if (!ReadCoefficients())
   {
      MessageBox(_T("Failed to read the coefficients from the device."), _T("Image Filter Editor"), MB_ICONERROR);
      return false;
   }

   return true;
}

void CImageFilterEditorDlg::UpdateKernelSizeCombo()
{
   int nItem = m_FilterIndexCombo.GetCurSel();
   int nIndex = (int)m_FilterIndexCombo.GetItemData(nItem);

   SapAcquisition::ImageFilterKernelSize kernelSize = SapAcquisition::ImageFilterSize1x1;
   if (mp_Acq->GetImageFilterKernelSize(nIndex, &kernelSize))
   {
      int nComboCount = m_KernelSizeCombo.GetCount();
      for (int i = 0; i < nComboCount; i++)
      {
         if ((SapAcquisition::ImageFilterKernelSize)m_KernelSizeCombo.GetItemData(i) == kernelSize)
         {
            m_KernelSizeCombo.SetCurSel(i);
            break;
         }
      }
   }
}

void CImageFilterEditorDlg::ArrangeCoeffEditControls()
{
   int nItem = m_KernelSizeCombo.GetCurSel();
   int nSize = ConvertKernelSizeToInt((SapAcquisition::ImageFilterKernelSize)m_KernelSizeCombo.GetItemData(nItem));
   int nEditCount = nSize*nSize;

   /*
   create the edit boxes
   not visible until placed in the area
   */

   if (!CreateCoeffEditControls(nEditCount))
   {
      MessageBox(_T("Failed to create the edit controls for the coefficients."), _T("Image Filter Editor"), MB_ICONERROR);
      return;
   }

   /*
   place them
   */

   static const int MAX_LINE_COL = sizeof(FILTER_SIZES_PRM) / sizeof(FILTER_SIZES_PRM[0]);
   static const int BOX_MARGIN = 5;

   // get the coordinates where to put the boxes
   CRect rcArea;
   m_CoeffArea.GetWindowRect(&rcArea);
   ScreenToClient(rcArea);

   // calculate the box width
   const int editWidth = (rcArea.Width() / MAX_LINE_COL) - BOX_MARGIN;

   // and the box height
   CRect rcDefaultSize;
   m_FilterIndexCombo.GetClientRect(rcDefaultSize);
   const int editHeight = rcDefaultSize.Height();

   // find top-left coordinate to have the boxes centered in the area
   int nTotalWidth = nSize*editWidth + (nSize - 1)*BOX_MARGIN;
   int nTotalHeight = nSize*editHeight + (nSize - 1)*BOX_MARGIN;
   const int startX = rcArea.left + (rcArea.Width() - nTotalWidth) / 2;
   const int startY = rcArea.top + (rcArea.Height() - nTotalHeight) / 2;

   // move the boxes
   CRect rcEdit;
   rcEdit.left = startX;
   rcEdit.right = rcEdit.left + editWidth;
   rcEdit.top = startY;
   rcEdit.bottom = rcEdit.top + editHeight;

   for (int iEditLine = 0; iEditLine < nSize; iEditLine++)
   {
      for (int iEditColumn = 0; iEditColumn < nSize; iEditColumn++)
      {
         int iEdit = iEditLine*nSize + iEditColumn;
         mp_CoeffEditBoxes[iEdit].MoveWindow(rcEdit, FALSE);

         rcEdit.left = rcEdit.right + BOX_MARGIN;
         rcEdit.right = rcEdit.left + editWidth;
      }

      rcEdit.left = startX;
      rcEdit.right = rcEdit.left + editWidth;
      rcEdit.top = rcEdit.bottom + BOX_MARGIN;
      rcEdit.bottom = rcEdit.top + editHeight;
   }

   /*
   show the boxes now
   */

   for (int iEdit = 0; iEdit < m_CoeffEditCount; iEdit++)
   {
      mp_CoeffEditBoxes[iEdit].ShowWindow(SW_SHOW);
   }
}

bool CImageFilterEditorDlg::ConvertToDecimal(CStringA string, double& decimal)
{
   std::string basicstring(string);
   std::istringstream ss(basicstring);

   ss >> decimal;  // try to read the number
   ss >> std::ws;  // eat whitespace after number

   return !ss.fail() && ss.eof();
}

bool CImageFilterEditorDlg::ReadCoefficients()
{
   bool bStatus = true; // to prevent leaking resources

   int nItem = m_FilterIndexCombo.GetCurSel();
   int nIndex = (int)m_FilterIndexCombo.GetItemData(nItem);

   nItem = m_KernelSizeCombo.GetCurSel();
   int nSize = ConvertKernelSizeToInt((SapAcquisition::ImageFilterKernelSize)m_KernelSizeCombo.GetItemData(nItem));

   SapBuffer buffer;
   buffer.SetFormat(SapFormatInt32);
   buffer.SetHeight(nSize);
   buffer.SetWidth(nSize);

   if (!buffer.Create())
   {
      MessageBox(_T("Failed to create the buffer for the kernel coefficients."), _T("Image Filter Editor"), MB_ICONERROR);
      return false;
   }

   if (!mp_Acq->GetImageFilter(nIndex, &buffer))
   {
      MessageBox(_T("Failed to get the coefficients from the device."), _T("Image Filter Editor"), MB_ICONERROR);
      bStatus = false;
   }

   if (!ReadCoefficientsFromBuffer(buffer))
   {
      MessageBox(_T("Failed to read the coefficients from the buffer."), _T("Image Filter Editor"), MB_ICONERROR);
      bStatus = false;
   }

   buffer.Destroy();

   return bStatus;
}

bool CImageFilterEditorDlg::ReadCoefficientsFromBuffer(SapBuffer& buffer)
{
   CString sText;
   int iEdit;
   SapDataMono data;

   int nWidth = buffer.GetWidth();
   int nHeight = buffer.GetHeight();

   if (nWidth != nHeight)
   {
      MessageBox(_T("The input buffer is not a square matrix."), _T("Image Filter Editor"), MB_ICONERROR);
      return false;
   }

   if (nWidth*nHeight != m_CoeffEditCount)
   {
      CString sMsg;
      sMsg.Format(_T("The input buffer is for a %dx%d kernel. Please select this Filter Kernel Size first."), nWidth, nHeight);
      MessageBox(sMsg, _T("Image Filter Editor"), MB_ICONERROR);
      return false;
   }

   bool bStatus = true;

   for (int iEditLine = 0; (iEditLine < nHeight) && bStatus; iEditLine++)
   {
      for (int iEditColumn = 0; iEditColumn < nWidth; iEditColumn++)
      {
         iEdit = iEditLine*nWidth + iEditColumn;
         if (!buffer.ReadElement(iEditColumn, iEditLine, &data))
         {
            MessageBox(_T("Failed to read a coefficient from the device."), _T("Image Filter Editor"), MB_ICONERROR);
            bStatus = false;
            break;
         }
         sText.Format(_T("%.5f"), data.Mono() / (double)m_Divisor);
         mp_CoeffEditBoxes[iEdit].SetWindowText(sText);
      }
   }

   return bStatus;
}

bool CImageFilterEditorDlg::WriteCoefficients()
{
   bool bStatus = true; // to prevent leaking resources

   int nItem = m_FilterIndexCombo.GetCurSel();
   int nIndex = (int)m_FilterIndexCombo.GetItemData(nItem);

   nItem = m_KernelSizeCombo.GetCurSel();
   int nSize = ConvertKernelSizeToInt((SapAcquisition::ImageFilterKernelSize)m_KernelSizeCombo.GetItemData(nItem));

   SapBuffer buffer;
   buffer.SetFormat(SapFormatInt32);
   buffer.SetHeight(nSize);
   buffer.SetWidth(nSize);

   if (!buffer.Create())
   {
      MessageBox(_T("Failed to create the buffer for the kernel coefficients."), _T("Image Filter Editor"), MB_ICONERROR);
      return false;
   }

   if (!WriteCoefficientsToBuffer(buffer))
   {
      MessageBox(_T("Failed to write the coefficients to the buffer."), _T("Image Filter Editor"), MB_ICONERROR);
      bStatus = false;
   }

   if (bStatus && !mp_Acq->SetImageFilter(nIndex, &buffer))
   {
      MessageBox(_T("Failed to set the coefficients onto the device."), _T("Image Filter Editor"), MB_ICONERROR);
      bStatus = false;
   }

   buffer.Destroy();

   return bStatus;
}

bool CImageFilterEditorDlg::WriteCoefficientsToBuffer(SapBuffer& buffer)
{
   int nWidth = buffer.GetWidth();
   int nHeight = buffer.GetHeight();

   if (nWidth != nHeight)
   {
      MessageBox(_T("The output buffer is not a square matrix."), _T("Image Filter Editor"), MB_ICONERROR);
      return false;
   }

   if (nWidth*nHeight != m_CoeffEditCount)
   {
      CString sMsg;
      sMsg.Format(_T("The output buffer is for a %dx%d kernel. Please enter coefficients for this Filter Kernel Size first."), nWidth, nHeight);
      MessageBox(sMsg, _T("Image Filter Editor"), MB_ICONERROR);
      return false;
   }

   bool bStatus = true;

   for (int iEditLine = 0; (iEditLine < nHeight) && bStatus; iEditLine++)
   {
      for (int iEditColumn = 0; iEditColumn < nWidth; iEditColumn++)
      {
         // get text from the box
         CString sText;
         int iEdit = iEditLine*nWidth + iEditColumn;
         mp_CoeffEditBoxes[iEdit].GetWindowText(sText);

         // validate and convert to double
         double dValue = 0;
         if (!ConvertToDecimal((CStringA)sText, dValue))
         {
            CString sMsg;
            sMsg.Format(_T("Failed to convert '%s' to a signed floating point value."), (LPCTSTR)sText);
            MessageBox(sMsg, _T("Image Filter Editor"), MB_ICONERROR);
            bStatus = false;
            break;
         }

         // apply divisor
         int nValue = (int)(dValue * m_Divisor);

         // prevent narrowing issues (1)
         if (nValue == (m_MinValue - 1))
            nValue = m_MinValue;
         else if (nValue == (m_MaxValue + 1))
            nValue = m_MaxValue;

         // prevent out-of-range values (2)
         if ((nValue < m_MinValue) || (nValue > m_MaxValue))
         {
            int corrected = nValue;
            if (nValue < m_MinValue)
               corrected = m_MinValue;
            else if (nValue > m_MaxValue)
               corrected = m_MaxValue;

            CString sMsg;
            sMsg.Format(_T("User value at location (%d,%d) was out-of-range and corrected."), iEditColumn, iEditLine);
            MessageBox(sMsg, _T("Image Filter Editor"), MB_ICONINFORMATION);

            nValue = corrected;
         }

         // write value to the kernel
         SapDataMono data;
         data.Set(nValue);
         if (!buffer.WriteElement(iEditColumn, iEditLine, data))
         {
            MessageBox(_T("Failed to write a coefficient to the buffer."), _T("Image Filter Editor"), MB_ICONERROR);
            bStatus = false;
            break;
         }
      }
   }

   return bStatus;
}



void CImageFilterEditorDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_CHECK_IMAGE_FILTER_ENABLE, m_EnableCheckbox);
   DDX_Control(pDX, IDC_COMBO_IMAGE_FILTER_INDEX_SELECT, m_FilterIndexCombo);
   DDX_Control(pDX, IDC_COMBO_IMAGE_FILTER_SIZE_SELECT, m_KernelSizeCombo);
   DDX_Text(pDX, IDC_STATIC_IMAGE_FILTER_MIN_VALUE, m_CoeffMinValue);
   DDX_Text(pDX, IDC_STATIC_IMAGE_FILTER_MAX_VALUE, m_CoeffMaxValue);
   DDX_Control(pDX, IDC_STATIC_IMAGE_FILTER_COEFF_AREA, m_CoeffArea);
}

BEGIN_MESSAGE_MAP(CImageFilterEditorDlg, CDialog)
   ON_BN_CLICKED(IDC_CHECK_IMAGE_FILTER_ENABLE, OnCheckImageFilterEnable)
   ON_CBN_SELCHANGE(IDC_COMBO_IMAGE_FILTER_INDEX_SELECT, OnChangeImageFilterIndex)
   ON_CBN_SELCHANGE(IDC_COMBO_IMAGE_FILTER_SIZE_SELECT, OnChangeImageFilterSize)
   ON_BN_CLICKED(IDC_BTN_IMPORT, OnBnClickedBtnImport)
   ON_BN_CLICKED(IDC_BTN_EXPORT, OnBnClickedBtnExport)
END_MESSAGE_MAP()

void CImageFilterEditorDlg::OnCheckImageFilterEnable()
{
   if (!mp_Acq->EnableImageFilter(m_EnableCheckbox.GetCheck() == BST_CHECKED))
   {
      MessageBox(_T("Failed to enable the image filter."), _T("Image Filter Editor"), MB_ICONERROR);
   }
}

void CImageFilterEditorDlg::OnChangeImageFilterIndex()
{
   UpdateKernelSizeCombo();
   UpdateState();
}

void CImageFilterEditorDlg::OnChangeImageFilterSize()
{
   UpdateState();
}

void CImageFilterEditorDlg::OnBnClickedBtnImport()
{
   // Account for new 'bVistaStyle' argument for CFileDialog constructor in Visual C++ 2008 and up
#if _MSC_VER >= 1500
   CFileDialog fileDlg(TRUE, _T("crc"), _T("kernel.crc"), OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, _T("Teledyne DALSA Files(*.crc)|*.crc||"), this, 0, TRUE);
#else
   CFileDialog fileDlg(TRUE, _T("crc"), _T("kernel.crc"), OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, _T("Teledyne DALSA Files(*.crc)|*.crc||"), this);
#endif
   if (fileDlg.DoModal() == IDOK)
   {
      CString path = fileDlg.GetPathName();
      SapBuffer buffer((CStringA)path);
      if (buffer.Create())
      {
         if (!buffer.Load((CStringA)path) || !ReadCoefficientsFromBuffer(buffer))
         {
            MessageBox(_T("Failed to load/read the kernel from the file."), _T("Image Filter Editor"), MB_ICONERROR);
         }

         buffer.Destroy();
      }
   }
}

void CImageFilterEditorDlg::OnBnClickedBtnExport()
{
   // Account for new 'bVistaStyle' argument for CFileDialog constructor in Visual C++ 2008 and up
#if _MSC_VER >= 1500
   CFileDialog fileDlg(FALSE, _T("crc"), _T("kernel.crc"), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Teledyne DALSA Files(*.crc)|*.crc||"), this, 0, TRUE);
#else
   CFileDialog fileDlg(FALSE, _T("crc"), _T("kernel.crc"), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Teledyne DALSA Files(*.crc)|*.crc||"), this);
#endif
   if (fileDlg.DoModal() == IDOK)
   {
      CString path = fileDlg.GetPathName();

      bool bStatus = true; // to prevent leaking resources

      int nItem = m_KernelSizeCombo.GetCurSel();
      int nSize = ConvertKernelSizeToInt((SapAcquisition::ImageFilterKernelSize)m_KernelSizeCombo.GetItemData(nItem));

      SapBuffer buffer;
      buffer.SetFormat(SapFormatInt32);
      buffer.SetHeight(nSize);
      buffer.SetWidth(nSize);

      if (!buffer.Create())
      {
         MessageBox(_T("Failed to create the buffer for the kernel coefficients."), _T("Image Filter Editor"), MB_ICONERROR);
         return;
      }

      if (!WriteCoefficientsToBuffer(buffer))
      {
         MessageBox(_T("Failed to write the coefficients to the buffer."), _T("Image Filter Editor"), MB_ICONERROR);
         bStatus = false;
      }

      if (bStatus && !buffer.Save((CStringA)path, "-format crc"))
      {
         MessageBox(_T("Failed to save the kernel to file."), _T("Image Filter Editor"), MB_ICONERROR);
         bStatus = false;
      }

      buffer.Destroy();
   }
}

void CImageFilterEditorDlg::OnOK()
{
   if (!WriteCoefficients())
   {
      // we failed to write the coefficients,
      // stop request to close the window
      // so the user can re-edit the values

      MessageBox(_T("Failed to apply the coefficients."), _T("Image Filter Editor"), MB_ICONERROR);
      return;
   }

   MessageBox(_T("Coefficients successfully applied."), _T("Image Filter Editor"), MB_ICONINFORMATION);

   CDialog::OnOK();
}

BOOL CImageFilterEditorDlg::PreTranslateMessage(MSG* pMsg)
{
   // prevent closing the window by pressing <enter> or <esc>

   if (pMsg->message == WM_KEYDOWN)
   {
      if (pMsg->wParam == VK_RETURN || pMsg->wParam == VK_ESCAPE)
      {
         return TRUE;
      }
   }

   return CDialog::PreTranslateMessage(pMsg);
}


#pragma warning(default:4995)