#ifndef _IMAGE_VWPT_WND_H_
#define _IMAGE_VWPT_WND_H_

#include "SapClassBasic.h"

// when building the "regular" MFC DLL
// refrain from exposing MFC derived data types
#if SAPCONTROLSINTEROP_EXPORT
#define SAPCLASSGUI_CLASS
#endif

/*
 * The CImageViewportWnd class for Sapera++ and MFC creates a new image viewer
 * for interacting with Sapera buffers and views.
 *
 * It relies heavily on capabilities from the mouse and keyboard and allows
 * for zooming, stretching, moving, defining and tweaking a ROI in the image.
 *
 * Mouse actions are:
 * - scroll mouse wheel to zoom in and out
 * - keep CTRL key down and scroll to zoom faster
 * - keep right button down and scroll to stretch horizontally
 * - keep right button and shift key down and scroll to stretch vertically
 * - double click to create a new ROI
 * - keep left button down and move to navigate the buffer or move the ROI
 *   and its anchors
 *
 * Keyboard actions are:
 * - press CTRL + '0' to reset zoom to 1:1
 * - press CTRL + '+' to zoom in
 * - press CTRL + '-' to zoom out
 * - press ESC to deactivate the ROI (i.e. hide it)
 *
 * Derive the CImageViewportWndEventHandler class and attach it to your instance
 * of CImageViewportWnd to support events.
 *
 */

#pragma region events

class SAPCLASSGUI_CLASS CImageViewportWndEventHandler
{
public:
   virtual void PixelChanged(int x, int y) {};
   virtual void ScaleChanged() {};
   virtual void RoiChanged() {};
   virtual void ViewChanged() {};
};

#pragma endregion

#pragma region internal use only (not exported)

struct SImageViewportROI
{
   enum
   {
      ANCHOR_TOPLEFT = 0,
      ANCHOR_TOPRIGHT,
      ANCHOR_BOTTOMLEFT,
      ANCHOR_BOTTOMRIGHT,
      ANCHOR_LEFT,
      ANCHOR_TOP,
      ANCHOR_RIGHT,
      ANCHOR_BOTTOM,
      ANCHOR_CNT
   };

   CRect m_rcInView;
   CRect m_rcAnchors[ANCHOR_CNT];
   int m_anchorMovingIndex;
   CPen m_penROI;
   CBrush m_brushROI;
   CPen m_penAnchor;
   CBrush m_brushAnchor;
};

#pragma endregion

#pragma region control

class SAPCLASSGUI_CLASS CImageViewportWnd : public CStatic
{
   DECLARE_DYNAMIC(CImageViewportWnd)

   friend class CImageExWnd;

public:

   CImageViewportWnd();
   virtual ~CImageViewportWnd();

   // attach the sapview master object (before SapView::Create() is called)
   bool AttachSapView(SapView* pView);

   // attach object that implements the event handler functions
   // you can attach as many as you need to
   void AttachEventHandler(CImageViewportWndEventHandler* pHandler);

   // specify whether or not we want the image to be centered on screen
   void CenterImage(bool bCenter);
   bool IsImageCentered() const;

   // updates the viewer whenever the view options are manually changed
   void Refresh();

   // reset to 1:1 scaling and no zoom
   void Reset();

   // enable/disable ROI
   void SupportROI(bool bSupport = true);
   void ActivateROI(bool bActive = true);

   // set ROI coordinates (relative to the buffer)
   void SetROI(int left, int top, unsigned int width, unsigned int height);
   void SetROI(CPoint topLeft, CPoint bottomRight);
   void SetROI(CRect roi);

   // get state of ROI
   bool IsRoiSupported() const;
   bool IsRoiActive() const;

   // get ROI coordinates (relative to the buffer)
   void GetROI(unsigned int& left, unsigned int& top, unsigned int& width, unsigned int& height) const;
   void GetROI(CPoint& topLeft, CPoint& bottomRight) const;
   void GetROI(CRect& roi) const;

   // get pixel string representation
   void SetPixelStringFormatDecimal(bool decimal = true);
   bool IsPixelStringFormatDecimal() const;
   CString GetPixelString(CPoint ptInBuffer) const;

   // set/get the image crosshair
   void EnableCrosshair(bool enable = true);
   bool IsCrosshairEnabled() const;

private:

   // outside world notifiers
   void NotifyPixelChanged(int x, int y);
   void NotifyScaleChanged();
   void NotifyRoiChanged();
   void NotifyViewChanged();

   // zoom related
   void ChangeZoom(bool bIncrease, bool bQuickZoom, CPoint ptOriginInView, bool bHorzZoom = false, bool bVertZoom = false);
   void UpdateScales(float zoomH = 1.0f, float zoomV = 1.0f);

   // move related
   void MoveInView(CPoint ptInDestinationView);

   // roi related
   void MoveRoi(CPoint ptInDestinationView);
   void MoveRoiAnchor(CPoint ptInDestinationView);
   void ValidateRoi();
   void PaintROI();

   // crosshair
   void PaintCrosshair();

   // mouse move related
   void MoveMouseCursor(int horizontalDelta, int verticalDelta) const;

   // sapview related
   static void ViewCallback(SapViewCallbackInfo* pInfo);

   // translations
   CPoint TranslatePointToNativeView(CPoint ptInDestinationView) const;
   CPoint TranslatePointToDestinationView(CPoint pointInNativeView) const;

   // utility functions
   CRect GetDstRectFromCurrentScalingOptions() const;

protected:

   // MFC related
   virtual void PreSubclassWindow();
   virtual BOOL PreTranslateMessage(MSG* pMsg);
   DECLARE_MESSAGE_MAP()
   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
   afx_msg void OnPaint();
   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
   afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
   afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
   afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
   afx_msg void OnMouseMove(UINT nFlags, CPoint point);
   afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
   afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
   afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
   afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
   afx_msg LRESULT OnMouseLeave(WPARAM wParam, LPARAM lParam);

private:

   // event handler interface
   CImageViewportWndEventHandler* m_pHandler;

   // location info
   bool m_bCenterImage;

   // view and scaling options for sapera
   SapView* m_pView;
   float m_currZoomH;
   float m_currZoomV;

   // ui enhancements
   HCURSOR m_cursorCross;
   HCURSOR m_cursorMove;
   HCURSOR m_cursorSizeNWSE;
   HCURSOR m_cursorSizeNESW;
   HCURSOR m_cursorSizeWE;
   HCURSOR m_cursorSizeNS;
   CBrush m_bkBrush;

   // flag for tracking mouse leave event
   bool m_bIsTrackingMouseLeave;

   // flags for the moving operation
   bool m_bCanMoveInView;
   bool m_bIsMovingInView;
   bool m_bIsMovingWholeROI;

   // flags for the ctrl mode
   bool m_bCtrlModeOn;

   // the origin of the move/zoom operation
   // relative to the destination view rect
   CPoint m_ptOrigin;

   // ROI-related things
   bool m_bRoiSupported;
   bool m_bRoiActive;
   SImageViewportROI m_ROI;

   // control display format
   bool m_bPixelStringFormatIsDecimal;

   // crosshair
   bool m_bCrosshairEnabled;
};

#pragma endregion

#endif	// _IMAGE_VWPT_WND_H_