#include "stdafx.h"
#include "SapClassGui.h"
#include "ImageExWnd.h"

const uint BORDER_SIZE = 30U;

IMPLEMENT_DYNAMIC(CImageExWnd, CStatic)

CImageExWnd::CImageExWnd()
{
   m_pView = NULL;
   m_pHandler = NULL;
   m_bIsDynamicCreation = false;
   m_bIsTrackingMouseLeave = false;

   m_viewportWnd.AttachEventHandler(this);

   EnableMarker(false);
   m_nInteractiveMarkerLineIndex = -1;
   m_nMaxLineIndex = 0;

   // create GDI objects
   m_region.CreateRectRgnIndirect(CRect(0, 0, 1, 1)); // passing an empty CRect fails with VS2005 (but works with VS2008 and up)
   m_bkBrush.CreateSolidBrush(RGB(235, 235, 235));
   m_markerPen.CreatePen(PS_SOLID, 1, RGB(10, 100, 30));
   m_markerBrush.CreateSolidBrush(RGB(10, 100, 30));
   m_interactiveMarkerPen.CreatePen(PS_SOLID, 1, RGB(20, 190, 60));
   m_interactiveMarkerBrush.CreateSolidBrush(RGB(20, 190, 60));
}

CImageExWnd::~CImageExWnd()
{
   // destroy GDI objects
   m_bkBrush.DeleteObject();
   m_region.DeleteObject();
   m_markerPen.DeleteObject();
   m_markerBrush.DeleteObject();
   m_interactiveMarkerPen.DeleteObject();
   m_interactiveMarkerBrush.DeleteObject();
}

#pragma region interface from CImageViewportWnd

bool CImageExWnd::AttachSapView(SapView* pView)
{
   m_pView = pView;
   return m_viewportWnd.AttachSapView(pView);
}

void CImageExWnd::AttachEventHandler(CImageExWndEventHandler* pHandler)
{
   m_pHandler = pHandler;
}

void CImageExWnd::CenterImage(bool bCenter)
{
   m_viewportWnd.CenterImage(bCenter);
}

bool CImageExWnd::IsImageCentered() const
{
   return m_viewportWnd.IsImageCentered();
}

void CImageExWnd::Refresh()
{
   m_viewportWnd.Refresh();
   UpdateLayout();
}

void CImageExWnd::Reset()
{
   m_viewportWnd.Reset();
   UpdateLayout();
}

void CImageExWnd::SupportROI(bool bSupport)
{
   m_viewportWnd.SupportROI(bSupport);
}

void CImageExWnd::ActivateROI(bool bActive)
{
   m_viewportWnd.ActivateROI(bActive);
}

void CImageExWnd::SetROI(int left, int top, unsigned int width, unsigned int height)
{
   m_viewportWnd.SetROI(left, top, width, height);
}

void CImageExWnd::SetROI(CPoint topLeft, CPoint bottomRight)
{
   m_viewportWnd.SetROI(topLeft, bottomRight);
}

void CImageExWnd::SetROI(CRect roi)
{
   m_viewportWnd.SetROI(roi);
}

bool CImageExWnd::IsRoiSupported() const
{
   return m_viewportWnd.IsRoiSupported();
}

bool CImageExWnd::IsRoiActive() const
{
   return m_viewportWnd.IsRoiActive();
}

void CImageExWnd::GetROI(unsigned int& left, unsigned int& top, unsigned int& width, unsigned int& height) const
{
   m_viewportWnd.GetROI(left, top, width, height);
}

void CImageExWnd::GetROI(CPoint& topLeft, CPoint& bottomRight) const
{
   m_viewportWnd.GetROI(topLeft, bottomRight);
}

void CImageExWnd::GetROI(CRect& roi) const
{
   m_viewportWnd.GetROI(roi);
}

void CImageExWnd::SetPixelStringFormatDecimal(bool decimal)
{
   m_viewportWnd.SetPixelStringFormatDecimal(decimal);
}

bool CImageExWnd::IsPixelStringFormatDecimal() const
{
   return m_viewportWnd.IsPixelStringFormatDecimal();
}

CString CImageExWnd::GetPixelString(CPoint ptInBuffer) const
{
   return m_viewportWnd.GetPixelString(ptInBuffer);
}

#pragma endregion

#pragma region events

void CImageExWnd::NotifyMarkerChanged()
{
   if (m_pHandler != NULL)
      m_pHandler->MarkerChanged();
}

void CImageExWnd::NotifyInteractiveMarkerChanged(int lineIndex)
{
   if (m_pHandler != NULL)
      m_pHandler->InteractiveMarkerChanged(lineIndex);
}

void CImageExWnd::PixelChanged(int x, int y)
{
   if (m_pHandler != NULL)
      m_pHandler->PixelChanged(x, y);
}

void CImageExWnd::ScaleChanged()
{
   if (m_pHandler != NULL)
      m_pHandler->ScaleChanged();

   // redraw marker
   Invalidate(TRUE);
   RedrawWindow();

   UpdateScrollBars();
}

void CImageExWnd::RoiChanged()
{
   if (m_pHandler != NULL)
      m_pHandler->RoiChanged();
}
void CImageExWnd::ViewChanged()
{
   if (m_pHandler != NULL)
      m_pHandler->ViewChanged();

   // redraw marker
   Invalidate(TRUE);
   RedrawWindow();

   UpdateScrollBars();
}

#pragma endregion

#pragma region marker

void CImageExWnd::EnableMarker(bool bEnable)
{
   m_bMarkerEnabled = false;

   if (!m_pView || !*m_pView)
      return;

   SapBuffer *pBuffer = m_pView->GetBuffer();

   if (!pBuffer || !*pBuffer)
      return;

   m_nMaxLineIndex = pBuffer->GetHeight() - 1;

   m_bMarkerEnabled = bEnable;

   // update the layout with the marker area 
   // and the inner image viewer side by side
   UpdateLayout();

   SetMarkerLine(0);
}

bool CImageExWnd::IsMarkerEnabled() const
{
   return m_bMarkerEnabled;
}

void CImageExWnd::SetMarkerLine(unsigned int line)
{
   m_nMarkerLineIndex = line;
   m_nMarkerLineIndex = ValidateMarkerLineIndex(m_nMarkerLineIndex);

   if (IsMarkerEnabled())
   {
      // repaint the whole window
      Invalidate(TRUE);
      RedrawWindow();

      // tell the outside world
      NotifyMarkerChanged();
   }
}

unsigned int CImageExWnd::GetMarkerLine() const
{
   return m_nMarkerLineIndex;
}

int CImageExWnd::ValidateMarkerLineIndex(int lineIndex)
{
   if (lineIndex > m_nMaxLineIndex)
      lineIndex = m_nMaxLineIndex;

   return lineIndex;
}

void CImageExWnd::EnableCrosshair(bool enable)
{
   m_viewportWnd.EnableCrosshair(enable);
}

bool CImageExWnd::IsCrosshairEnabled() const
{
   return m_viewportWnd.IsCrosshairEnabled();
}

#pragma endregion

BEGIN_MESSAGE_MAP(CImageExWnd, CStatic)
   ON_WM_CREATE()
   ON_WM_PAINT()
   ON_WM_SIZE()
   ON_WM_SETFOCUS()
   ON_WM_ERASEBKGND()
   ON_WM_MOUSEMOVE()
   ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave) // ON_WM_MOUSELEAVE is only defined with VS2008 and later
   ON_WM_LBUTTONDOWN()
   ON_WM_DESTROY()
   ON_WM_HSCROLL()
   ON_WM_VSCROLL()
END_MESSAGE_MAP()

BOOL CImageExWnd::PreCreateWindow(CREATESTRUCT& cs)
{
   // MFC has two ways to create and hook a control that is a subclassed window
   // a. the static way is used when the app is dialog-based 
   // (DDX_Control calls SubClassDlgItem(...))
   // and 
   // b. the dynamic way is used when the app relies on a SDI or MDI model
   // (the application explicitly calls the Create(...) method in the CView-derived object)

   // with a. the control already exists when the subclassing occurs 
   // and so only PreSubclassWindow(...) is called
   // with b. it is still under construction and so PreCreateWindow(...) is called,
   // followed by PreSubclassWindow() and finally OnCreate(...)

   // the fact that we have an inner window that we need to create is problematic
   // because this parent window must exist and be hooked before we can create any child window
   // and attach it to the parent window

   // creating the inner window in PreSubclassWindow(...) works with a. but doesn't with b.
   // in this later case we need to put it in OnCreate(...) where the framework garantees
   // this parent window exists and is hooked (but not yet visible)

   // with this in mind we then have to handle these two ways exclusively, 
   // hence the use of the following boolean flag:

   m_bIsDynamicCreation = true;

   return CStatic::PreCreateWindow(cs);
}

BOOL CImageExWnd::CreateChildControls()
{
   BOOL bStatus = TRUE;

   // create image viewer as a child (if not created yet)
   if (bStatus && m_viewportWnd.GetSafeHwnd() == (HWND)0)
      bStatus = m_viewportWnd.Create(_T("inner image window"), WS_CHILD | WS_VISIBLE, CRect(), this);

   // create scroll bars
   if (bStatus && m_scrollbars[SB_HORZ].GetSafeHwnd() == (HWND)0)
      bStatus = m_scrollbars[SB_HORZ].Create(SBS_HORZ | WS_CHILD | WS_VISIBLE, CRect(), this, 1);
   if (bStatus && m_scrollbars[SB_VERT].GetSafeHwnd() == (HWND)0)
      bStatus = m_scrollbars[SB_VERT].Create(SBS_VERT | WS_CHILD | WS_VISIBLE, CRect(), this, 2);

   return bStatus;
}

int CImageExWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
   if (CStatic::OnCreate(lpCreateStruct) == -1)
      return -1;

   if (!CreateChildControls())
      return -1;

   return 0;
}

void CImageExWnd::PreSubclassWindow()
{
   // create children controls
   if (!m_bIsDynamicCreation)
      CreateChildControls();

   // allow control sending messages to its parent
   ModifyStyle(0, SS_NOTIFY);

   CStatic::PreSubclassWindow();
}

void CImageExWnd::OnDestroy()
{
   CStatic::OnDestroy();
   
   m_scrollbars[SB_VERT].DestroyWindow();
   m_scrollbars[SB_HORZ].DestroyWindow();
   m_viewportWnd.DestroyWindow();
}

BOOL CImageExWnd::PreTranslateMessage(MSG* pMsg)
{
   if (pMsg->message == WM_KEYDOWN)
   {
      if (pMsg->wParam == VK_UP)
      {
         // handle the UP ARROW key to move the marker up
         if (IsMarkerEnabled() && (m_nMarkerLineIndex > 0))
         {
            SetMarkerLine(m_nMarkerLineIndex - 1);
         }
         return TRUE;
      }
      else if (pMsg->wParam == VK_DOWN)
      {
         // handle the DOWN ARROW key to move the marker down
         if (IsMarkerEnabled() && (m_nMarkerLineIndex < m_nMaxLineIndex))
         {
            SetMarkerLine(m_nMarkerLineIndex + 1);
         }
         return TRUE;
      }
   }

   return CStatic::PreTranslateMessage(pMsg);
}

void CImageExWnd::OnPaint()
{
   CPaintDC dc(this);   // device context for painting
   CRect rect;          // drawing area
   CDC dcMemory;        // in-memory DC
   CBitmap bitmap;      // in-memory bitmap

   CPen* pOldPen;
   CBrush* pOldBrush;

   // clip to the authorized painting region
   dc.SelectClipRgn(&m_region);

   // use double buffering to paint efficiently
   m_region.GetRgnBox(&rect);
   dcMemory.CreateCompatibleDC(&dc);
   bitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height());
   CBitmap* pOldBitmap = dcMemory.SelectObject(&bitmap);

   // erase with uniform background
   dcMemory.FillRgn(&m_region, &m_bkBrush);

   // marker stuff
   if (IsMarkerEnabled())
   {
      // draw the marker if any
      if (m_nMarkerLineIndex >= 0)
      {
         pOldPen = dcMemory.SelectObject(&m_markerPen);
         pOldBrush = dcMemory.SelectObject(&m_markerBrush);
         PaintMarker(dcMemory, m_nMarkerLineIndex);
         dcMemory.SelectObject(pOldPen);
         dcMemory.SelectObject(pOldBrush);
      }

      // draw the interactive marker if any and different than position of the permanent marker
      if ((m_nInteractiveMarkerLineIndex >= 0) && (m_nInteractiveMarkerLineIndex != m_nMarkerLineIndex))
      {
         pOldPen = dcMemory.SelectObject(&m_interactiveMarkerPen);
         pOldBrush = dcMemory.SelectObject(&m_interactiveMarkerBrush);
         PaintMarker(dcMemory, m_nInteractiveMarkerLineIndex);
         dcMemory.SelectObject(pOldPen);
         dcMemory.SelectObject(pOldBrush);
      }
   }

   // copy the in-memory DC into the on-screen DC
   dc.BitBlt(rect.left, rect.top, rect.Width(), rect.Height(), &dcMemory, rect.left, rect.top, SRCCOPY);
   dcMemory.SelectObject(pOldBitmap);
   bitmap.DeleteObject();
   dcMemory.DeleteDC();

}

void CImageExWnd::PaintMarker(CDC& dc, int lineIndex)
{
   // skip if marker is disabled
   if (!IsMarkerEnabled())
      return;

   // get coordinates for the marker
   CRect rcMarker;
   CPoint ptThisLine, ptNextLine;

   rcMarker.left = 0;
   rcMarker.right = BORDER_SIZE;

   ptThisLine.SetPoint(0, lineIndex);
   ptThisLine = m_viewportWnd.TranslatePointToDestinationView(ptThisLine);
   rcMarker.top = ptThisLine.y;

   if (lineIndex < m_nMaxLineIndex)
   {
      // look at the starting vertical location of the next line
      // to determine the height of the marker
      ptNextLine.SetPoint(0, lineIndex + 1);
      ptNextLine = m_viewportWnd.TranslatePointToDestinationView(ptNextLine);
      rcMarker.bottom = ptNextLine.y;
   }
   else
   {
      // there is no next line
      // so we look at the starting vertical location of the previous line
      // to determine the height of the marker
      ptNextLine.SetPoint(0, lineIndex - 1);
      ptNextLine = m_viewportWnd.TranslatePointToDestinationView(ptNextLine);
      uint height = rcMarker.top - ptNextLine.y;
      rcMarker.bottom = rcMarker.top + height;
   }

   CRect rcView = m_viewportWnd.GetDstRectFromCurrentScalingOptions();
   rcMarker.OffsetRect(0, rcView.top);

   // make sure the height of the marker is at least 1 pixel tall 
   // and doesn't go past the bottom of the image
   if (rcMarker.Height() < 1)
      rcMarker.bottom = rcMarker.top + 1;
   if (rcMarker.bottom > rcView.bottom)
      rcMarker.bottom -= (rcView.bottom - rcMarker.bottom);

   // draw marker
   dc.Rectangle(rcMarker);
}

BOOL CImageExWnd::OnEraseBkgnd(CDC* pDC)
{
   return TRUE;
}

void CImageExWnd::UpdateLayout()
{
   CRect rcClient;
   GetClientRect(rcClient);

   // calculate the region where we do painting
   // (referenced later to improve gdi performance)
   CRgn rgnTemp;
   m_region.SetRectRgn(rcClient);

   // update scroll bars ranges
   UpdateScrollBars();

   // get current system sizes for scrollbars
   const UINT SCROLL_HORZ_HEIGHT = GetSystemMetrics(SM_CYHSCROLL);
   const UINT SCROLL_VERT_WIDTH = GetSystemMetrics(SM_CXVSCROLL);

   // set inner image wnd position
   CRect rcImage = rcClient;
   if (IsMarkerEnabled())
      rcImage.left += BORDER_SIZE;
   rcImage.bottom -= SCROLL_HORZ_HEIGHT;
   rcImage.right -= SCROLL_VERT_WIDTH;

   if (m_viewportWnd.GetSafeHwnd())
      m_viewportWnd.MoveWindow(rcImage);

   rgnTemp.CreateRectRgnIndirect(rcImage);
   m_region.CombineRgn(&m_region, &rgnTemp, RGN_DIFF);

   // set scrollbars positions
   if (m_scrollbars[SB_HORZ].GetSafeHwnd() && m_scrollbars[SB_VERT].GetSafeHwnd())
   {
      // SB_HORZ
      CRect rcBarH = rcClient;
      rcBarH.top = rcBarH.bottom - SCROLL_HORZ_HEIGHT;
      if (IsMarkerEnabled())
         rcBarH.left += BORDER_SIZE;
      rcBarH.right -= SCROLL_VERT_WIDTH;
      m_scrollbars[SB_HORZ].MoveWindow(rcBarH);

      rgnTemp.SetRectRgn(rcBarH);
      m_region.CombineRgn(&m_region, &rgnTemp, RGN_DIFF);
      
      //SB_VERT
      CRect rcBarV = rcClient;
      rcBarV.left = rcBarV.right - SCROLL_VERT_WIDTH;
      rcBarV.bottom -= SCROLL_HORZ_HEIGHT;
      m_scrollbars[SB_VERT].MoveWindow(rcBarV);

      rgnTemp.SetRectRgn(rcBarV);
      m_region.CombineRgn(&m_region, &rgnTemp, RGN_DIFF);
   }

   // release GDI resources
   rgnTemp.DeleteObject();

   // prevent artifacts from showing, force repaint of entire window
   CDC* pDC = GetDC();
   if (pDC)
      pDC->FillRect(rcClient, &m_bkBrush);

   // repaint the whole window
   Invalidate(TRUE);
   RedrawWindow();
}

void CImageExWnd::OnSize(UINT nType, int cx, int cy)
{
   CStatic::OnSize(nType, cx, cy);
   UpdateLayout();
}

void CImageExWnd::OnSetFocus(CWnd* pOldWnd)
{
   m_viewportWnd.SetFocus();
}

void CImageExWnd::OnMouseMove(UINT nFlags, CPoint point)
{
   // play with the interactive marker
   if (IsMarkerEnabled())
   {
      // don't draw marker if mouse moves on the left of the SB_HORZ
      // scrollbar boundaries
      const UINT SCROLL_HORZ_HEIGHT = GetSystemMetrics(SM_CYHSCROLL);
      CRect rcClient;
      GetClientRect(rcClient);
      if (point.y < (LONG) (rcClient.bottom - SCROLL_HORZ_HEIGHT))
      {
         CPoint ptInDestinationView;
         ptInDestinationView.x = 0;
         ptInDestinationView.y = point.y - m_viewportWnd.GetDstRectFromCurrentScalingOptions().top;
         CPoint ptInNativeView = m_viewportWnd.TranslatePointToNativeView(ptInDestinationView);
         m_nInteractiveMarkerLineIndex = ptInNativeView.y;
         m_nInteractiveMarkerLineIndex = ValidateMarkerLineIndex(m_nInteractiveMarkerLineIndex);

         // repaint the whole window
         Invalidate(TRUE);
         RedrawWindow();

         // tell the outside world
         NotifyInteractiveMarkerChanged(m_nInteractiveMarkerLineIndex);

         // track the mouse leave event
         if (!m_bIsTrackingMouseLeave)
         {
            TRACKMOUSEEVENT tme = { 0 };
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = m_hWnd;
            ::TrackMouseEvent(&tme);
         }
      }
   }

   CStatic::OnMouseMove(nFlags, point);
}

LRESULT CImageExWnd::OnMouseLeave(WPARAM wParam, LPARAM lParam)
{
   // invalidate the interactive marker index
   m_nInteractiveMarkerLineIndex = -1;

   // repaint the whole window
   Invalidate(TRUE);
   RedrawWindow();

   // tracking has stopped (it is one time only)
   m_bIsTrackingMouseLeave = false;

   return (LRESULT)0;
}

void CImageExWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
   // set focus to this control to receive keyboard inputs
   SetFocus();

   // make the interactive marker definitive
   if (IsMarkerEnabled())
   {
      if (m_nInteractiveMarkerLineIndex >= 0)
      {
         SetMarkerLine(m_nInteractiveMarkerLineIndex);
         m_nInteractiveMarkerLineIndex = -1;
      }
   }

   CStatic::OnLButtonDown(nFlags, point);
}

#pragma region scrollbars

void CImageExWnd::UpdateScrollBars()
{
   // some code taken from CImageWnd

   if (!m_pView || !*m_pView || !m_pView->GetBuffer())
   {
      m_scrollbars[SB_HORZ].EnableWindow(FALSE);
      m_scrollbars[SB_VERT].EnableWindow(FALSE);
      return;
   }

   const SapViewScaleParams& paramSrc = m_pView->GetScaleParamsSrc();

   int viewWidth = m_pView->GetBuffer()->GetWidth();
   int viewHeight = m_pView->GetBuffer()->GetHeight();
   int pageWidth = paramSrc.Width();
   int pageHeight = paramSrc.Height();

   SCROLLINFO info;
   info.cbSize = sizeof(SCROLLINFO);
   info.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;

   // SB_HORZ
   if (pageWidth < viewWidth)
   {
      info.nMin = 0;
      info.nMax = (int)((float)(viewWidth + 0.5) - 1);
      info.nPage = pageWidth;
      info.nPos = paramSrc.Left();
      m_scrollbars[SB_HORZ].SetScrollInfo(&info);
      m_scrollbars[SB_HORZ].EnableWindow(TRUE);
   }
   else
   {
      m_scrollbars[SB_HORZ].EnableWindow(FALSE);
   }

   // SB_VERT
   if (pageHeight < viewHeight)
   {
      info.nMin = 0;
      info.nMax = (int)(((float)viewHeight + 0.5) - 1);
      info.nPage = pageHeight;
      info.nPos = paramSrc.Top();
      m_scrollbars[SB_VERT].SetScrollInfo(&info);
      m_scrollbars[SB_VERT].EnableWindow(TRUE);
   }
   else
   {
      m_scrollbars[SB_VERT].EnableWindow(FALSE);
   }
}

void CImageExWnd::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
   // some code taken from CImageWnd
   
   if (!m_pView || !*m_pView || !m_pView->GetBuffer())
      return;

   // Get current position and limits
   SCROLLINFO scrollInfo = { 0 };

   scrollInfo.cbSize = sizeof(scrollInfo);
   scrollInfo.fMask = SIF_TRACKPOS | SIF_RANGE | SIF_POS;

   m_scrollbars[SB_HORZ].GetScrollInfo(&scrollInfo);

   int left = scrollInfo.nPos;
   int hMin = scrollInfo.nMin;
   int hMax = scrollInfo.nMax;

   // Calculate new position
   switch (nSBCode)
   {
   case SB_THUMBTRACK:
   case SB_THUMBPOSITION:
      left = scrollInfo.nTrackPos;
      break;
   case SB_LINELEFT:
      if (left > hMin) left--; break;
   case SB_LINERIGHT:
      if (left < hMax) left++; break;
   case SB_PAGELEFT:
      left -= 10;
      if (left < hMin) left = hMin; break;
   case SB_PAGERIGHT:
      left += 10;
      if (left > hMax) left = hMax; break;
   }

   // Store new position and reread it to validate
   scrollInfo.nPos = left;
   m_scrollbars[SB_HORZ].SetScrollInfo(&scrollInfo, TRUE);

   // Update view
   m_pView->OnHScroll(left);
   m_viewportWnd.Refresh();

   CStatic::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CImageExWnd::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
   // some code taken from CImageWnd
   
   if (!m_pView || !*m_pView || !m_pView->GetBuffer())
      return;

   // Get current position and limits
   SCROLLINFO scrollInfo = { 0 };

   scrollInfo.cbSize = sizeof(scrollInfo);
   scrollInfo.fMask = SIF_TRACKPOS | SIF_RANGE | SIF_POS;

   m_scrollbars[SB_VERT].GetScrollInfo(&scrollInfo);

   int top = scrollInfo.nPos;
   int vMin = scrollInfo.nMin;
   int vMax = scrollInfo.nMax;

   // Calculate new position
   switch (nSBCode)
   {
   case SB_THUMBTRACK:
   case SB_THUMBPOSITION:
      top = scrollInfo.nTrackPos;
      break;
   case SB_LINEUP:
      if (top > vMin) top--; break;
   case SB_LINEDOWN:
      if (top < vMax) top++; break;
   case SB_PAGEUP:
      top -= 10;
      if (top < vMin) top = vMin; break;
   case SB_PAGEDOWN:
      top += 10;
      if (top > vMax) top = vMax; break;
   }

   // Store new position and reread it to validate
   scrollInfo.nPos = top;
   m_scrollbars[SB_VERT].SetScrollInfo(&scrollInfo, TRUE);

   // Update view
   m_pView->OnVScroll(top);
   m_viewportWnd.Refresh();

   CStatic::OnVScroll(nSBCode, nPos, pScrollBar);
}

#pragma endregion