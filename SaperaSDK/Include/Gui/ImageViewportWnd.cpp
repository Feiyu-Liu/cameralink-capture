#include "stdafx.h"
#include "SapClassGui.h"
#include "ImageWnd.h"
#include "ImageViewportWnd.h"

/* This viewport's intent is to serve as the HWND slave for SapView.
   It's only role is to render parts of a buffer (or a whole buffer)
   inside its boundaries. A viewport is a "dumb" image viewer.
   Decorating controls are added into the CImageExWnd class.
   One should use this latest class instead of the raw CImageViewportWnd.
   */

#pragma region construction-related methods

IMPLEMENT_DYNAMIC(CImageViewportWnd, CStatic)

CImageViewportWnd::CImageViewportWnd()
{
   m_pView = NULL;
   m_pHandler = NULL;
   m_bCenterImage = false;
   m_bIsTrackingMouseLeave = false;
   m_bCanMoveInView = false;
   m_bIsMovingInView = false;
   m_bIsMovingWholeROI = false;
   m_bCtrlModeOn = false;
   m_bRoiSupported = false;
   m_bRoiActive = false;
   m_bPixelStringFormatIsDecimal = false;
   m_bCrosshairEnabled = false;

   Reset();

   // create drawing resources
   m_bkBrush.CreateSolidBrush(RGB(235, 235, 235));
   m_ROI.m_penROI.CreatePen(PS_SOLID, 1, RGB(20, 190, 60));
   m_ROI.m_brushROI.CreateStockObject(HOLLOW_BRUSH);
   m_ROI.m_penAnchor.CreatePen(PS_SOLID, 1, RGB(255, 0, 255));
   m_ROI.m_brushAnchor.CreateSolidBrush(RGB(255, 0, 255));

}

CImageViewportWnd::~CImageViewportWnd()
{
   // destroy drawing resources
   m_bkBrush.DeleteObject();
   m_ROI.m_penROI.DeleteObject();
   m_ROI.m_brushROI.DeleteObject();
   m_ROI.m_penAnchor.DeleteObject();
   m_ROI.m_brushAnchor.DeleteObject();
}

bool CImageViewportWnd::AttachSapView(SapView* pView)
{
   // attach the sapview master object (before SapView::Create() is called)
   m_pView = pView;

   if (pView && pView->SetWindow(GetSafeHwnd()))
   {
      m_pView->SetCallbackInfo(&ViewCallback, this);
      return true;
   }

   return false;
}

void CImageViewportWnd::ViewCallback(SapViewCallbackInfo* pInfo)
{
   if (pInfo == NULL)
      return;

   CImageViewportWnd* pThis = static_cast<CImageViewportWnd*>(pInfo->GetContext());
   if (pThis)
   {
      pThis->PaintROI();
      pThis->PaintCrosshair();
   }
}

void CImageViewportWnd::AttachEventHandler(CImageViewportWndEventHandler* pHandler)
{
   m_pHandler = pHandler;
}

void CImageViewportWnd::CenterImage(bool bCenter)
{
   m_bCenterImage = bCenter;
   UpdateScales(m_currZoomH, m_currZoomV);
}

bool CImageViewportWnd::IsImageCentered() const
{
   return m_bCenterImage;
}

#pragma endregion

#pragma region painting-related methods

BOOL CImageViewportWnd::OnEraseBkgnd(CDC* pDC)
{
   return TRUE;
}

void CImageViewportWnd::OnPaint()
{
   CPaintDC dc(this);

   enum{
      BORDER_LEFT = 0,
      BORDER_TOP,
      BORDER_RIGHT,
      BORDER_BOTTOM,
      BORDER_CNT
   };

   CRect rcBorders[BORDER_CNT];

   bool bValidView = (m_pView && *m_pView);

   CRect rcClient;
   GetClientRect(rcClient);

   CRect rcView = GetDstRectFromCurrentScalingOptions();

   if (bValidView)
   {
      // repaint the view
      m_pView->OnPaint();

      // block on the left of the view
      rcBorders[BORDER_LEFT].left = rcClient.left;
      rcBorders[BORDER_LEFT].top = rcView.top;
      rcBorders[BORDER_LEFT].right = rcView.left;
      rcBorders[BORDER_LEFT].bottom = rcView.bottom;

      // block at the top
      rcBorders[BORDER_TOP].left = rcClient.left;
      rcBorders[BORDER_TOP].top = rcClient.top;
      rcBorders[BORDER_TOP].right = rcClient.right;
      rcBorders[BORDER_TOP].bottom = rcBorders[BORDER_LEFT].top;

      // block on the right
      rcBorders[BORDER_RIGHT].left = rcView.right;
      rcBorders[BORDER_RIGHT].top = rcBorders[BORDER_LEFT].top;
      rcBorders[BORDER_RIGHT].right = rcClient.right;
      rcBorders[BORDER_RIGHT].bottom = rcBorders[BORDER_LEFT].bottom;

      // block at the bottom
      rcBorders[BORDER_BOTTOM].left = rcBorders[BORDER_TOP].left;
      rcBorders[BORDER_BOTTOM].top = rcBorders[BORDER_LEFT].bottom;
      rcBorders[BORDER_BOTTOM].right = rcBorders[BORDER_TOP].right;
      rcBorders[BORDER_BOTTOM].bottom = rcClient.bottom;

      // valid view, draw the individual blocks around it
      for (unsigned int i = 0; i < BORDER_CNT; i++)
         dc.FillRect(rcBorders[i], &m_bkBrush);

      // draw ROI if needed
      PaintROI();

      // draw crosshair if needed
      PaintCrosshair();
   }
   else
   {
      // no valid view, draw the whole client area
      dc.FillRect(rcClient, &m_bkBrush);
   }
}

void CImageViewportWnd::PaintROI()
{
   static const int ANCHOR_DIM = 6;

   // skip if ROI is not supported nor active
   if (!IsRoiSupported() || !IsRoiActive())
      return;

   CDC* pDC = GetDC();
   if (pDC == NULL)
      return;

   CRect rcView = GetDstRectFromCurrentScalingOptions();

   // set clipping box to prevent drawing outside the destination view borders
   CRgn clipRegion;
   clipRegion.CreateRectRgnIndirect(rcView);
   pDC->SelectClipRgn(&clipRegion);

   // ROI coordinates are related to the view itself, not this window
   // translate those to the viewWnd perspective
   CPoint ptTopLeft, ptBottomRight;
   ptTopLeft = TranslatePointToDestinationView(m_ROI.m_rcInView.TopLeft());
   ptBottomRight = TranslatePointToDestinationView(m_ROI.m_rcInView.BottomRight());
   CRect rcRoiDstView;
   rcRoiDstView.SetRect(ptTopLeft, ptBottomRight);

   // compute location of the anchors
   m_ROI.m_rcAnchors[SImageViewportROI::ANCHOR_TOPLEFT].SetRect(rcRoiDstView.left, rcRoiDstView.top, rcRoiDstView.left + ANCHOR_DIM, rcRoiDstView.top + ANCHOR_DIM);
   m_ROI.m_rcAnchors[SImageViewportROI::ANCHOR_TOPRIGHT].SetRect(rcRoiDstView.right - ANCHOR_DIM, rcRoiDstView.top, rcRoiDstView.right, rcRoiDstView.top + ANCHOR_DIM);
   m_ROI.m_rcAnchors[SImageViewportROI::ANCHOR_BOTTOMLEFT].SetRect(rcRoiDstView.left, rcRoiDstView.bottom - ANCHOR_DIM, rcRoiDstView.left + ANCHOR_DIM, rcRoiDstView.bottom);
   m_ROI.m_rcAnchors[SImageViewportROI::ANCHOR_BOTTOMRIGHT].SetRect(rcRoiDstView.right - ANCHOR_DIM, rcRoiDstView.bottom - ANCHOR_DIM, rcRoiDstView.right, rcRoiDstView.bottom);
   m_ROI.m_rcAnchors[SImageViewportROI::ANCHOR_LEFT].SetRect(rcRoiDstView.left, (rcRoiDstView.top + rcRoiDstView.bottom - ANCHOR_DIM) / 2, rcRoiDstView.left + ANCHOR_DIM, (rcRoiDstView.top + rcRoiDstView.bottom + ANCHOR_DIM) / 2);
   m_ROI.m_rcAnchors[SImageViewportROI::ANCHOR_TOP].SetRect((rcRoiDstView.left + rcRoiDstView.right - ANCHOR_DIM) / 2, rcRoiDstView.top, (rcRoiDstView.left + rcRoiDstView.right + ANCHOR_DIM) / 2, rcRoiDstView.top + ANCHOR_DIM);
   m_ROI.m_rcAnchors[SImageViewportROI::ANCHOR_RIGHT].SetRect(rcRoiDstView.right - ANCHOR_DIM, (rcRoiDstView.top + rcRoiDstView.bottom - ANCHOR_DIM) / 2, rcRoiDstView.right, (rcRoiDstView.top + rcRoiDstView.bottom + ANCHOR_DIM) / 2);
   m_ROI.m_rcAnchors[SImageViewportROI::ANCHOR_BOTTOM].SetRect((rcRoiDstView.left + rcRoiDstView.right - ANCHOR_DIM) / 2, rcRoiDstView.bottom - ANCHOR_DIM, (rcRoiDstView.left + rcRoiDstView.right + ANCHOR_DIM) / 2, rcRoiDstView.bottom);

   // use an offset rectangle to draw relatively to the destination coordinates
   CRect rcInDstView;

   // draw ROI
   CPen* pOldPen = pDC->SelectObject(&(m_ROI.m_penROI));
   CBrush* pOldBrush = pDC->SelectObject(&(m_ROI.m_brushROI));
   rcInDstView = rcRoiDstView;
   rcInDstView.OffsetRect(rcView.TopLeft());
   pDC->Rectangle(rcInDstView);

   // draw the anchors
   pDC->SelectObject(&(m_ROI.m_penAnchor));
   pDC->SelectObject(&(m_ROI.m_brushAnchor));
   for (int i = 0; i < SImageViewportROI::ANCHOR_CNT; i++)
   {
      rcInDstView = m_ROI.m_rcAnchors[i];
      rcInDstView.OffsetRect(rcView.TopLeft());
      pDC->Rectangle(rcInDstView);
   }

   // clean GDI objects
   pDC->SelectObject(pOldPen);
   pDC->SelectObject(pOldBrush);
   ReleaseDC(pDC);
}

void CImageViewportWnd::PaintCrosshair()
{
   // skip if crosshair is not active
   if (!IsCrosshairEnabled())
      return;

   if (!m_pView || !*m_pView)
      return;

   SapBuffer *pBuffer = m_pView->GetBuffer();

   if (!pBuffer || !*pBuffer)
      return;

   CDC* pDC = GetDC();
   if (pDC == NULL)
      return;

   CRect rcView = GetDstRectFromCurrentScalingOptions();

   // set clipping box to prevent drawing outside the destination view borders
   CRgn clipRegion;
   clipRegion.CreateRectRgnIndirect(rcView);
   pDC->SelectClipRgn(&clipRegion);

   // Crosshair coordinates are related to the view itself, not this window
   // translate those to the viewWnd perspective
   CPoint topLeft = TranslatePointToDestinationView(CPoint(0, 0));
   int top = topLeft.y;
   int left = topLeft.x;

   CPoint bottomRight = TranslatePointToDestinationView(CPoint(pBuffer->GetWidth(), pBuffer->GetHeight()));
   int bottom = bottomRight.y;
   int right = bottomRight.x;

   // draw crosshair (use same pen and brush as ROI)
   CPen* pOldPen = pDC->SelectObject(&(m_ROI.m_penROI));
   CBrush* pOldBrush = pDC->SelectObject(&(m_ROI.m_brushROI));
   pDC->MoveTo(left, (top + bottom) / 2);
   pDC->LineTo(right, (top + bottom) / 2);
   pDC->MoveTo((left + right) / 2, top);
   pDC->LineTo((left + right) / 2, bottom);

   pDC->SelectObject(pOldPen);
   pDC->SelectObject(pOldBrush);
   ReleaseDC(pDC);
}

#pragma endregion

#pragma region message handling

BEGIN_MESSAGE_MAP(CImageViewportWnd, CStatic)
   ON_WM_ERASEBKGND()
   ON_WM_PAINT()
   ON_WM_SIZE()
   ON_WM_MOUSEWHEEL()
   ON_WM_LBUTTONDOWN()
   ON_WM_LBUTTONUP()
   ON_WM_MOUSEMOVE()
   ON_WM_SETCURSOR()
   ON_WM_LBUTTONDBLCLK()
   ON_WM_KEYDOWN()
   ON_WM_KEYUP()
   ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave) // ON_WM_MOUSELEAVE is only defined with VS2008 and later
   ON_WM_HSCROLL()
   ON_WM_SYSKEYDOWN()
END_MESSAGE_MAP()

void CImageViewportWnd::PreSubclassWindow()
{
   // grab system-defined UI resources
   m_cursorCross = AfxGetApp()->LoadStandardCursor(IDC_CROSS);
   m_cursorMove = AfxGetApp()->LoadStandardCursor(IDC_SIZEALL);
   m_cursorSizeNWSE = AfxGetApp()->LoadStandardCursor(IDC_SIZENWSE);
   m_cursorSizeNESW = AfxGetApp()->LoadStandardCursor(IDC_SIZENESW);
   m_cursorSizeWE = AfxGetApp()->LoadStandardCursor(IDC_SIZEWE);
   m_cursorSizeNS = AfxGetApp()->LoadStandardCursor(IDC_SIZENS);

   // allow sending messages to its parent
   ModifyStyle(0, SS_NOTIFY);

   CStatic::PreSubclassWindow();
}

BOOL CImageViewportWnd::PreTranslateMessage(MSG* pMsg)
{
   if (pMsg->message == WM_KEYDOWN)
   {
      // handle the ESC key to hide the ROI
      if (pMsg->wParam == VK_ESCAPE)
      {
         ActivateROI(false);
         return TRUE;
      }
   }

   return CStatic::PreTranslateMessage(pMsg);
}


void CImageViewportWnd::OnSize(UINT nType, int cx, int cy)
{
   CStatic::OnSize(nType, cx, cy);
   UpdateScales(m_currZoomH, m_currZoomV);
   if (m_pView)
      m_pView->OnSize();
}

void CImageViewportWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
   SetFocus();

   CRect rcView = GetDstRectFromCurrentScalingOptions();

   if (!rcView.PtInRect(point))
      return CStatic::OnLButtonDown(nFlags, point);

   CPoint ptInDstView = point - rcView.TopLeft();
   CPoint ptInNatView = TranslatePointToNativeView(ptInDstView);

   // when hovering any ROI anchor
   bool bTweakingROI = false;
   if (IsRoiSupported() && IsRoiActive())
   {
      // when hovering any ROI anchor
      m_ROI.m_anchorMovingIndex = -1;
      for (int i = 0; i < SImageViewportROI::ANCHOR_CNT; i++)
      {
         if (m_ROI.m_rcAnchors[i].PtInRect(ptInDstView))
         {
            m_ROI.m_anchorMovingIndex = i;
            bTweakingROI = true;
            break;
         }
      }

      // when inside the ROI
      if (!bTweakingROI && m_ROI.m_rcInView.PtInRect(ptInNatView))
      {
         m_bIsMovingWholeROI = true;
         bTweakingROI = true;
         m_ptOrigin = ptInDstView;
      }
   }

   // otherwise, set the flag 'moving in view'
   if (!bTweakingROI && m_bCanMoveInView)
   {
      m_bIsMovingInView = true;
      m_ptOrigin = ptInDstView;
   }

   CStatic::OnLButtonDown(nFlags, point);
}

void CImageViewportWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
   // validate new roi if needed
   if (m_ROI.m_anchorMovingIndex >= 0 || m_bIsMovingWholeROI)
   {
      ValidateRoi();

      // force redraw to paint validated ROI
      Invalidate(TRUE);
      RedrawWindow();

      // reset the moving anchor to its 'disabled' value
      m_ROI.m_anchorMovingIndex = -1;

      // reset moving flag
      m_bIsMovingWholeROI = false;
   }

   // reset the m_bIsMovingInView flag
   if (m_bCanMoveInView && m_bIsMovingInView)
   {
      m_bIsMovingInView = false;
   }

   CStatic::OnLButtonUp(nFlags, point);
}

void CImageViewportWnd::OnLButtonDblClk(UINT nFlags, CPoint point)
{
   if (!IsRoiSupported())
      return CStatic::OnLButtonDblClk(nFlags, point);

   CRect rcView = GetDstRectFromCurrentScalingOptions();

   if (!rcView.PtInRect(point))
      return CStatic::OnLButtonDblClk(nFlags, point);

   // save the enable flag
   bool bPrevEnabled = IsRoiActive();

   // hide the ROI while setting its coordinates
   ActivateROI(false);

   // change ROI coordinates when enabling 
   if (!bPrevEnabled)
   {
      // get current cursor position relatively to the view
      CPoint ptInDstView = point - rcView.TopLeft();
      CPoint ptInView = TranslatePointToNativeView(ptInDstView);
      if (m_pView && *m_pView)
         SetROI(ptInView.x, ptInView.y, max(1, (int)(100 / m_pView->GetScaleParamsSrc().ZoomHorz())), max(1, (int)(100 / m_pView->GetScaleParamsSrc().ZoomVert())));
   }
   // Clear ROI coordinates
   else
   {
      if (m_pView && *m_pView)
         SetROI(0, 0, 0, 0);
   }

   // toggle the ROI enable flag
   ActivateROI(!bPrevEnabled);

   CStatic::OnLButtonDblClk(nFlags, point);
}

void CImageViewportWnd::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
   switch (nChar)
   {
   case VK_CONTROL:
      m_bCtrlModeOn = true;
      break;

   case VK_NUMPAD0:
   case 0x30:
      if (m_bCtrlModeOn)
         Reset(); // 0 (key or numpad) is down, reset zoom
      break;

   case VK_ADD:
   case VK_OEM_PLUS:
      if (m_bCtrlModeOn)
         ChangeZoom(true, false, m_ptOrigin); // increase zoom
      break;

   case VK_SUBTRACT:
   case VK_OEM_MINUS:
      if (m_bCtrlModeOn)
         ChangeZoom(false, false, m_ptOrigin); // decrease zoom
      break;

   default:
      break;
   }

   CStatic::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CImageViewportWnd::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
   if (nChar == VK_CONTROL && m_bCtrlModeOn)
      m_bCtrlModeOn = false;

   CStatic::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CImageViewportWnd::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
   switch (nChar)
   {
   case VK_LEFT:
      if (!m_bCtrlModeOn)
         MoveMouseCursor(-1, 0); // move one point left
      break;

   case VK_RIGHT:
      if (!m_bCtrlModeOn)
         MoveMouseCursor(+1, 0); // move one point right
      break;

   case VK_UP:
      if (!m_bCtrlModeOn)
         MoveMouseCursor(0, -1); // move one point up
      break;

   case VK_DOWN:
      if (!m_bCtrlModeOn)
         MoveMouseCursor(0, +1); // move one point down
      break;

   default:
      CStatic::OnSysKeyDown(nChar, nRepCnt, nFlags);
   }
}


void CImageViewportWnd::OnMouseMove(UINT nFlags, CPoint point)
{
   CRect rcView = GetDstRectFromCurrentScalingOptions();

   // track the mouse leave event
   if (!m_bIsTrackingMouseLeave)
   {
      TRACKMOUSEEVENT tme = { 0 };
      tme.cbSize = sizeof(tme);
      tme.dwFlags = TME_LEAVE;
      tme.hwndTrack = m_hWnd;
      ::TrackMouseEvent(&tme);
   }

   // make sure we are moving in the view itself
   if (rcView.PtInRect(point))
   {
      // get current cursor position relatively to the view
      CPoint ptInDstView = point - rcView.TopLeft();

      // regular navigation in the view
      CPoint pt = TranslatePointToNativeView(ptInDstView);
      NotifyPixelChanged(pt.x, pt.y);

      if (m_bCanMoveInView && m_bIsMovingInView)
      {
         // move in view if allowed to
         MoveInView(ptInDstView);
      }
      else if (m_ROI.m_anchorMovingIndex >= 0)
      {
         // move selected anchor around
         MoveRoiAnchor(ptInDstView);
      }
      else if (m_bIsMovingWholeROI)
      {
         // move whole ROI around
         MoveRoi(ptInDstView);
      }
   }
   else
   {
      // notifies invalid pixel
      NotifyPixelChanged(-1, -1);
   }

   CStatic::OnMouseMove(nFlags, point);
}

LRESULT CImageViewportWnd::OnMouseLeave(WPARAM wParam, LPARAM lParam)
{
   // notifies invalid pixel
   NotifyPixelChanged(-1, -1);

   // stop current moving operation if any
   // reset the m_bIsMovingInView flag
   if (m_bCanMoveInView && m_bIsMovingInView)
   {
      m_bIsMovingInView = false;
   }

   // tracking has stopped (it is one time only)
   m_bIsTrackingMouseLeave = false;

   return (LRESULT)0;
}

BOOL CImageViewportWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
   CRect rcView = GetDstRectFromCurrentScalingOptions();
   ClientToScreen(rcView);

   if (rcView.PtInRect(pt))
   {
      // MK_RBUTTON down = horizontal zoom
      // MK_RBUTTON + MK_SHIFT both down = vertical zoom
      // MK_CONTROL down = 'quick zoom'

      bool bQuickZoom = (nFlags & MK_CONTROL) > 0;
      bool bVertZoom = false;
      bool bHorzZoom = false;

      if ((nFlags & (MK_RBUTTON | MK_SHIFT)) == (MK_RBUTTON | MK_SHIFT))
         bVertZoom = true;
      else if (nFlags & MK_RBUTTON)
         bHorzZoom = true;

      ChangeZoom(zDelta > 0, bQuickZoom, pt - rcView.TopLeft(), bHorzZoom, bVertZoom);
   }

   return CStatic::OnMouseWheel(nFlags, zDelta, pt);
}

BOOL CImageViewportWnd::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
   CRect rcView = GetDstRectFromCurrentScalingOptions();

   // get cursor position
   CPoint pt;
   GetCursorPos(&pt);
   ScreenToClient(&pt);

   if (!rcView.PtInRect(pt))
      return CStatic::OnSetCursor(pWnd, nHitTest, message);

   // get points relatively to the native and the destination views
   CPoint ptInDstView = pt - rcView.TopLeft();
   CPoint ptInNatView = TranslatePointToNativeView(ptInDstView);

   // when moving in view
   if (m_bIsMovingInView)
   {
      ::SetCursor(m_cursorMove);
      return TRUE;
   }

   // when hovering any ROI anchor
   if (IsRoiSupported() && IsRoiActive())
   {
      for (int i = 0; i < SImageViewportROI::ANCHOR_CNT; i++)
      {
         if (m_ROI.m_rcAnchors[i].PtInRect(ptInDstView))
         {
            switch (i)
            {
            case SImageViewportROI::ANCHOR_TOPLEFT:
            case SImageViewportROI::ANCHOR_BOTTOMRIGHT:
               ::SetCursor(m_cursorSizeNWSE);
               break;

            case SImageViewportROI::ANCHOR_TOPRIGHT:
            case SImageViewportROI::ANCHOR_BOTTOMLEFT:
               ::SetCursor(m_cursorSizeNESW);
               break;

            case SImageViewportROI::ANCHOR_LEFT:
            case SImageViewportROI::ANCHOR_RIGHT:
               ::SetCursor(m_cursorSizeWE);
               break;

            case SImageViewportROI::ANCHOR_TOP:
            case SImageViewportROI::ANCHOR_BOTTOM:
               ::SetCursor(m_cursorSizeNS);
               break;
            }

            return TRUE;
         }
      }

      // when inside the ROI (but not over an anchor)
      if (m_ROI.m_rcInView.PtInRect(ptInNatView))
      {
         ::SetCursor(m_cursorMove);
         return TRUE;
      }
   }

   // default cursor
   ::SetCursor(m_cursorCross);
   return TRUE;
}

#pragma endregion

#pragma region event handlers

void CImageViewportWnd::NotifyPixelChanged(int x, int y)
{
   if (!m_pHandler)
      return;
   m_pHandler->PixelChanged(x, y);
}

void CImageViewportWnd::NotifyScaleChanged()
{
   if (!m_pHandler)
      return;
   m_pHandler->ScaleChanged();
}

void CImageViewportWnd::NotifyRoiChanged()
{
   if (!m_pHandler)
      return;
   m_pHandler->RoiChanged();
}

void CImageViewportWnd::NotifyViewChanged()
{
   if (!m_pHandler)
      return;
   m_pHandler->ViewChanged();
}

#pragma endregion

#pragma region geometrical operations

void CImageViewportWnd::UpdateScales(float zoomH, float zoomV)
{
   if (!m_pView || !*m_pView)
      return;

   SapBuffer *pBuffer;
   if ((pBuffer = m_pView->GetBuffer()) == NULL)
      return;

   // call this to compute new scaling options given
   // the specified zoom levels
   // and a reference point

   // get max display area
   CRect rClient;
   GetClientRect(rClient);

   // source and destination coordinates
   CRect rSrc, rDst;

   UINT clientWidth = rClient.Width();
   UINT clientHeight = rClient.Height();

   UINT imageWidth = pBuffer->GetWidth();
   UINT imageHeight = pBuffer->GetHeight();

   UINT zoomedImageWidth = (UINT)(imageWidth * zoomH);
   UINT zoomedImageHeight = (UINT)(imageHeight * zoomV);

   // determine raw source and destination coordinates
   if ((zoomedImageWidth <= clientWidth) && (zoomedImageHeight <= clientHeight))
   {
      // zoomed image fits in the display area
      rSrc.SetRect(0, 0, imageWidth, imageHeight);
      rDst.SetRect(0, 0, zoomedImageWidth, zoomedImageHeight);
      if (m_bCenterImage)
         rDst.MoveToXY((clientWidth - zoomedImageWidth) / 2, (clientHeight - zoomedImageHeight) / 2);
      m_bCanMoveInView = false;
   }
   else if ((zoomedImageWidth > clientWidth) && (zoomedImageHeight > clientHeight))
   {
      // zoomed image is wider and taller and display area
      rSrc.SetRect(0, 0, (int)(clientWidth / zoomH), (int)(clientHeight / zoomV));
      rDst = rClient;
      m_bCanMoveInView = true;
   }
   else if (zoomedImageWidth > clientWidth)
   {
      // zoomed image is wider than display area
      rSrc.SetRect(0, 0, (int)(clientWidth / zoomH), imageHeight);
      rDst.SetRect(0, 0, clientWidth, zoomedImageHeight);
      if (m_bCenterImage)
         rDst.MoveToY((clientHeight - zoomedImageHeight) / 2);
      m_bCanMoveInView = true;
   }
   else
   {
      // zoomed image is taller than display area
      rSrc.SetRect(0, 0, imageWidth, (int)(clientHeight / zoomV));
      rDst.SetRect(0, 0, zoomedImageWidth, clientHeight);
      if (m_bCenterImage)
         rDst.MoveToX((clientWidth - zoomedImageWidth) / 2);
      m_bCanMoveInView = true;
   }

   // we know the destination scaling options
   SapViewScaleParams scaleDst;
   scaleDst.SetTop(rDst.top);
   scaleDst.SetLeft(rDst.left);
   scaleDst.SetWidth(rDst.Width());
   scaleDst.SetHeight(rDst.Height());

   // handle point-relative zoom

   // get reference location in buffer
   CPoint ptZoomRef = TranslatePointToNativeView(m_ptOrigin);

   // move rSrc around the reference point
   // by keeping the same proportions
   float ptInViewLeftMarginPct = (float)m_ptOrigin.x / (float)clientWidth;
   float ptInViewTopMarginPct = (float)m_ptOrigin.y / (float)clientHeight;
   int ptInSourceLeftMarginPct = (int)(ptInViewLeftMarginPct * rSrc.Width());
   int ptInSourceTopMarginPct = (int)(ptInViewTopMarginPct * rSrc.Height());
   int ptLeft = ptZoomRef.x - ptInSourceLeftMarginPct;
   int ptTop = ptZoomRef.y - ptInSourceTopMarginPct;
   rSrc.MoveToXY(ptLeft, ptTop);

   // fix negative argins
   if (rSrc.top < 0)
      rSrc.MoveToY(0);

   if (rSrc.left < 0)
      rSrc.MoveToX(0);

   // fix positive margins
   if ((UINT)(rSrc.bottom) >= imageHeight)
      rSrc.OffsetRect(0, imageHeight - rSrc.bottom);

   if ((UINT)(rSrc.right) >= imageWidth)
      rSrc.OffsetRect(imageWidth - rSrc.right, 0);

   // we now know the source scaling options
   SapViewScaleParams scaleSrc;
   scaleSrc.SetTop(rSrc.top);
   scaleSrc.SetLeft(rSrc.left);
   scaleSrc.SetWidth(rSrc.Width());
   scaleSrc.SetHeight(rSrc.Height());

   // set new scaling options to sapview
   m_pView->SetScalingMode(scaleSrc, scaleDst, FALSE);

   // repaint the whole window
   Invalidate(TRUE);
   RedrawWindow();

   // tell the outside world
   NotifyScaleChanged();
}

void CImageViewportWnd::MoveInView(CPoint ptInDestinationView)
{
   // pt is relative to the view

   if (!m_pView || !*m_pView)
      return;

   SapBuffer *pBuffer;
   if ((pBuffer = m_pView->GetBuffer()) == NULL)
      return;

   // get start and end points into the buffer
   CPoint start = TranslatePointToNativeView(m_ptOrigin);
   CPoint end = TranslatePointToNativeView(ptInDestinationView);

   // get translation vector
   CPoint trans = start - end;

   // get dimensions of the buffer
   UINT imageHeight = pBuffer->GetHeight();
   UINT imageWidth = pBuffer->GetWidth();

   // set the source scaling options
   CRect rSrc;
   SapViewScaleParams scaleSrc = m_pView->GetScaleParamsSrc();
   rSrc.top = scaleSrc.Top();
   rSrc.left = scaleSrc.Left();
   rSrc.bottom = rSrc.top + scaleSrc.Height();
   rSrc.right = rSrc.left + scaleSrc.Width();

   rSrc += trans;

   if (rSrc.top < 0)
      rSrc.MoveToY(0);

   if (rSrc.left < 0)
      rSrc.MoveToX(0);

   if ((UINT)(rSrc.bottom) > imageHeight)
      rSrc.OffsetRect(0, imageHeight - rSrc.bottom);

   if ((UINT)(rSrc.right) > imageWidth)
      rSrc.OffsetRect(imageWidth - rSrc.right, 0);

   scaleSrc.SetTop(rSrc.top);
   scaleSrc.SetLeft(rSrc.left);
   scaleSrc.SetHeight(rSrc.Height());
   scaleSrc.SetWidth(rSrc.Width());

   // set new reference point for the subsequent move operation
   m_ptOrigin = ptInDestinationView;

   // apply new scaling options to the view

   m_pView->SetScalingMode(scaleSrc, m_pView->GetScaleParamsDst(), FALSE);

   // repaint the whole window
   Invalidate(TRUE);
   RedrawWindow();

   // tell the outside world
   NotifyViewChanged();
}

void CImageViewportWnd::MoveRoi(CPoint ptInDestinationView)
{
   // get start and end points into the buffer
   CPoint start = TranslatePointToNativeView(m_ptOrigin);
   CPoint end = TranslatePointToNativeView(ptInDestinationView);

   // get translation vector
   CPoint trans = end - start;

   // set new reference point for the move operation
   m_ptOrigin = ptInDestinationView;

   // set new ROI coordinates
   CRect rcCopy = m_ROI.m_rcInView;
   m_ROI.m_rcInView.SetRect(rcCopy.TopLeft() + trans, rcCopy.BottomRight() + trans);

   // note: validation of ROI occurs upon release of mouse button

   // invalidate and redraw
   Invalidate(TRUE);
   RedrawWindow();
}

void CImageViewportWnd::MoveRoiAnchor(CPoint ptInDestinationView)
{
   if (m_ROI.m_anchorMovingIndex < 0 || m_ROI.m_anchorMovingIndex >= SImageViewportROI::ANCHOR_CNT)
      return;

   // translate to the point relative to the overall view
   CPoint ptInView = TranslatePointToNativeView(ptInDestinationView);

   // change the official ROI coordinates
   switch (m_ROI.m_anchorMovingIndex)
   {
   case SImageViewportROI::ANCHOR_TOPLEFT:
      m_ROI.m_rcInView.left = ptInView.x;
      m_ROI.m_rcInView.top = ptInView.y;
      break;
   case SImageViewportROI::ANCHOR_TOPRIGHT:
      m_ROI.m_rcInView.right = ptInView.x;
      m_ROI.m_rcInView.top = ptInView.y;
      break;
   case SImageViewportROI::ANCHOR_BOTTOMLEFT:
      m_ROI.m_rcInView.left = ptInView.x;
      m_ROI.m_rcInView.bottom = ptInView.y;
      break;
   case SImageViewportROI::ANCHOR_BOTTOMRIGHT:
      m_ROI.m_rcInView.right = ptInView.x;
      m_ROI.m_rcInView.bottom = ptInView.y;
      break;
   case SImageViewportROI::ANCHOR_LEFT:
      m_ROI.m_rcInView.left = ptInView.x;
      break;
   case SImageViewportROI::ANCHOR_TOP:
      m_ROI.m_rcInView.top = ptInView.y;
      break;
   case SImageViewportROI::ANCHOR_RIGHT:
      m_ROI.m_rcInView.right = ptInView.x;
      break;
   case SImageViewportROI::ANCHOR_BOTTOM:
      m_ROI.m_rcInView.bottom = ptInView.y;
      break;
   }

   // note: validation of ROI occurs upon release of mouse button

   // invalidate and redraw
   Invalidate(TRUE);
   RedrawWindow();
}

CPoint CImageViewportWnd::TranslatePointToNativeView(CPoint ptInDestinationView) const
{
   CPoint pt;
   SapBuffer *pBuffer;

   if (m_pView && *m_pView && ((pBuffer = m_pView->GetBuffer()) != NULL))
   {
      SapViewScaleParams scaleSrc = m_pView->GetScaleParamsSrc();
      pt.x = (LONG)(ptInDestinationView.x / scaleSrc.ZoomHorz()) + scaleSrc.Left();
      pt.y = (LONG)(ptInDestinationView.y / scaleSrc.ZoomVert()) + scaleSrc.Top();

      pt.x = max(0, pt.x);
      pt.x = min(pt.x, pBuffer->GetWidth() - 1);

      pt.y = max(0, pt.y);
      pt.y = min(pt.y, pBuffer->GetHeight() - 1);
   }

   return pt;
}

CPoint CImageViewportWnd::TranslatePointToDestinationView(CPoint pointInNativeView) const
{
   CPoint pt;
   if (m_pView && *m_pView)
   {
      SapViewScaleParams scaleSrc = m_pView->GetScaleParamsSrc();
      pt.x = (LONG)((pointInNativeView.x - scaleSrc.Left()) * scaleSrc.ZoomHorz());
      pt.y = (LONG)((pointInNativeView.y - scaleSrc.Top()) * scaleSrc.ZoomVert());
   }
   return pt;
}

CRect CImageViewportWnd::GetDstRectFromCurrentScalingOptions() const
{
   CRect rcDst;
   if (m_pView && *m_pView)
   {
      SapViewScaleParams scaleDst = m_pView->GetScaleParamsDst();
      rcDst.top = scaleDst.Top();
      rcDst.left = scaleDst.Left();
      rcDst.bottom = rcDst.top + scaleDst.Height();
      rcDst.right = rcDst.left + scaleDst.Width();
   }
   return rcDst;
}

void CImageViewportWnd::ChangeZoom(bool bIncrease, bool bQuickZoom, CPoint ptOriginInView, bool bHorzZoom, bool bVertZoom)
{
   bool bGlobalZoom = !bHorzZoom && !bVertZoom;

   // new zoom factors
   // note: we need a local copy of the accumulated zoom factors
   // i.e. m_currZoomH and m_currZoomV
   // because we can't rely on the ones returned by the view since 
   // the view modifies these factors we set to it when it renders 
   // on the screen and STRETCHES to the size of the window
   float fNewZoomH = m_currZoomH;
   float fNewZoomV = m_currZoomV;

   // zoom factor
   float fFactor = (bQuickZoom) ? 0.5f : 0.125f;

   if (bIncrease)
   {
      // zoom in
      if (bHorzZoom || bGlobalZoom)
         fNewZoomH += fFactor;
      if (bVertZoom || bGlobalZoom)
         fNewZoomV += fFactor;
   }
   else
   {
      // zoom out
      if (bHorzZoom || bGlobalZoom)
         fNewZoomH -= fFactor;
      if (bVertZoom || bGlobalZoom)
         fNewZoomV -= fFactor;
   }

   // set limits on zoom factors
   fNewZoomH = max(fNewZoomH, 0.125f);
   fNewZoomV = max(fNewZoomV, 0.125f);

   // save the reference point
   m_ptOrigin = ptOriginInView;

   m_currZoomH = fNewZoomH;
   m_currZoomV = fNewZoomV;

   // update the source and destination scaling options
   UpdateScales(fNewZoomH, fNewZoomV);
}

#pragma endregion

#pragma region ROI

void CImageViewportWnd::SupportROI(bool bSupport)
{
   m_bRoiSupported = bSupport;
}

void CImageViewportWnd::ActivateROI(bool bActive)
{
   // call this to activate (show) or disactivate (hide) the ROI
   // call SetROI first if you want to manually set the coordinates 

   m_bRoiActive = bActive;

   // validate and notify change to user
   ValidateRoi();

   // repaint the whole window
   Invalidate(TRUE);
   RedrawWindow();
}

void CImageViewportWnd::SetROI(int left, int top, unsigned int width, unsigned int height)
{
   // call this to manually set the ROI coordinates

   // arguments are related to the overall buffer/view
   m_ROI.m_rcInView.SetRect(left, top, left + width, top + height);
   ValidateRoi();

   if (IsRoiSupported() && IsRoiActive())
   {
      // repaint the whole window
      Invalidate(TRUE);
      RedrawWindow();
   }
}

void CImageViewportWnd::SetROI(CPoint topLeft, CPoint bottomRight)
{
   SetROI(topLeft.x, topLeft.y, bottomRight.x - topLeft.x, bottomRight.y - topLeft.y);
}

void CImageViewportWnd::SetROI(CRect roi)
{
   SetROI(roi.TopLeft(), roi.BottomRight());
}

bool CImageViewportWnd::IsRoiSupported() const
{
   return m_bRoiSupported;
}

bool CImageViewportWnd::IsRoiActive() const
{
   return m_bRoiSupported && m_bRoiActive;
}

void CImageViewportWnd::GetROI(unsigned int& left, unsigned int& top, unsigned int& width, unsigned int& height) const
{
   left = m_ROI.m_rcInView.left;
   top = m_ROI.m_rcInView.top;
   width = m_ROI.m_rcInView.Width();
   height = m_ROI.m_rcInView.Height();
}

void CImageViewportWnd::GetROI(CPoint& topLeft, CPoint& bottomRight) const
{
   topLeft = m_ROI.m_rcInView.TopLeft();
   bottomRight = m_ROI.m_rcInView.BottomRight();
}

void CImageViewportWnd::GetROI(CRect& roi) const
{
   roi = m_ROI.m_rcInView;
}

void CImageViewportWnd::ValidateRoi()
{
   if (!IsRoiSupported())
      return;

   if (!m_pView || !*m_pView)
      return;

   SapBuffer *pBuffer = m_pView->GetBuffer();

   if (!pBuffer || !*pBuffer)
      return;

   CRect outRect(m_ROI.m_rcInView);
   outRect.NormalizeRect();

   int viewWidth = pBuffer->GetWidth();
   int viewHeight = pBuffer->GetHeight();

   if (outRect.left < 0)
      outRect.left = 0;

   if (outRect.right > viewWidth - 1)
      outRect.right = viewWidth - 1;

   if (outRect.top < 0)
      outRect.top = 0;

   if (outRect.bottom > viewHeight - 1)
      outRect.bottom = viewHeight - 1;

   m_ROI.m_rcInView = outRect;

   // tell the outside world
   NotifyRoiChanged();
}

#pragma endregion

#pragma region move mouse

void CImageViewportWnd::MoveMouseCursor(int horizontalDelta, int verticalDelta) const
{
   CPoint ptCurrentInScreen;
   ::GetCursorPos(&ptCurrentInScreen);

   CPoint ptCurrentInClient = ptCurrentInScreen;
   ScreenToClient(&ptCurrentInClient);

   CRect rcView = GetDstRectFromCurrentScalingOptions();

   if (!rcView.PtInRect(ptCurrentInClient))
      return;

   CPoint ptNewInScreen = ptCurrentInScreen;
   ptNewInScreen.Offset(horizontalDelta, verticalDelta);
   ::SetCursorPos(ptNewInScreen.x, ptNewInScreen.y);
}

#pragma endregion

#pragma region utilities

void CImageViewportWnd::SetPixelStringFormatDecimal(bool decimal)
{
   m_bPixelStringFormatIsDecimal = decimal;
}

bool CImageViewportWnd::IsPixelStringFormatDecimal() const
{
   return m_bPixelStringFormatIsDecimal;
}

CString CImageViewportWnd::GetPixelString(CPoint ptInBuffer) const
{
   if (!m_pView || !*m_pView || m_pView->GetBuffer() == NULL || !*m_pView->GetBuffer() || !m_pView->GetBuffer()->IsMapped())
      return CImageWnd::GetPixelString(NULL, CPoint(0, 0));

   return CImageWnd::GetPixelString(m_pView->GetBuffer(), ptInBuffer, m_bPixelStringFormatIsDecimal);
}

void CImageViewportWnd::EnableCrosshair(bool enable)
{
   m_bCrosshairEnabled = enable;
}

bool CImageViewportWnd::IsCrosshairEnabled() const
{
   return m_bCrosshairEnabled;
}

void CImageViewportWnd::Refresh()
{
   if (!m_pView || !*m_pView)
      return;

   // call this function whenever scaling options change 
   // in the outside world

   m_currZoomH = m_pView->GetScaleParamsSrc().ZoomHorz();
   m_currZoomV = m_pView->GetScaleParamsSrc().ZoomVert();
   m_ptOrigin.SetPoint(0, 0);

   UpdateScales(m_currZoomH, m_currZoomV);
   NotifyViewChanged();
}

void CImageViewportWnd::Reset()
{
   m_currZoomH = m_currZoomV = 1.0f;
   m_ptOrigin.SetPoint(0, 0);

   // call this to reset the scaling options
   if (!m_pView || !*m_pView)
      return;

   m_pView->SetScalingMode(SapView::ScalingNone, TRUE);

   m_currZoomH = m_pView->GetScaleParamsSrc().ZoomHorz();
   m_currZoomV = m_pView->GetScaleParamsSrc().ZoomVert();

   UpdateScales();
   NotifyViewChanged();
}

#pragma endregion



