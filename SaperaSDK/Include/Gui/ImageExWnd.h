#ifndef _IMAGEEXWND_H_
#define _IMAGEEXWND_H_

// Refer to ImageViewportWnd.h for more details
#include "ImageViewportWnd.h"
#include "SapClassBasic.h"

// when building the "regular" MFC DLL
// refrain from exposing MFC derived data types
#if SAPCONTROLSINTEROP_EXPORT
#define SAPCLASSGUI_CLASS
#endif

#pragma region events

class SAPCLASSGUI_CLASS CImageExWndEventHandler : public CImageViewportWndEventHandler
{
public:
   virtual void MarkerChanged() {};
   virtual void InteractiveMarkerChanged(int lineIndex) {};
};

#pragma endregion

#pragma region control

class SAPCLASSGUI_CLASS CImageExWnd : public CStatic, public CImageViewportWndEventHandler
{
   DECLARE_DYNAMIC(CImageExWnd)

public:
   CImageExWnd();
   virtual ~CImageExWnd();

   // see declaration of CImageViewportWnd for details about the following
   // interface

   bool AttachSapView(SapView* pView);
   void AttachEventHandler(CImageExWndEventHandler* pHandler);

   void CenterImage(bool bCenter = true);
   bool IsImageCentered() const;

   void Refresh();
   void Reset();

   void SupportROI(bool bSupport = true);
   void ActivateROI(bool bActive = true);
   void SetROI(int left, int top, unsigned int width, unsigned int height);
   void SetROI(CPoint topLeft, CPoint bottomRight);
   void SetROI(CRect roi);
   bool IsRoiSupported() const;
   bool IsRoiActive() const;
   void GetROI(unsigned int& left, unsigned int& top, unsigned int& width, unsigned int& height) const;
   void GetROI(CPoint& topLeft, CPoint& bottomRight) const;
   void GetROI(CRect& roi) const;

   void SetPixelStringFormatDecimal(bool decimal = true);
   bool IsPixelStringFormatDecimal() const;
   CString GetPixelString(CPoint ptInBuffer) const;

   // enable/disable line marker
   void EnableMarker(bool enable = true);
   bool IsMarkerEnabled() const;

   // set/get line index to be highlighted with marker
   void SetMarkerLine(unsigned int line);
   unsigned int GetMarkerLine() const;

   // set/get the image crosshair
   void EnableCrosshair(bool enable = true);
   bool IsCrosshairEnabled() const;

private:
   bool m_bIsDynamicCreation;
   CImageViewportWnd m_viewportWnd;
   CScrollBar m_scrollbars[SB_CTL]; // 2 scrollbars
   CImageExWndEventHandler* m_pHandler;
   SapView* m_pView;
   CRgn m_region;
   CBrush m_bkBrush;
   
   // flag for tracking mouse leave event
   bool m_bIsTrackingMouseLeave;

   // marker-related things
   bool m_bMarkerEnabled;
   int m_nMarkerLineIndex;
   int m_nInteractiveMarkerLineIndex;
   int m_nMaxLineIndex;
   CPen m_markerPen;
   CPen m_interactiveMarkerPen;
   CBrush m_markerBrush;
   CBrush m_interactiveMarkerBrush;

protected:
   BOOL CreateChildControls();
   void UpdateLayout();
   void UpdateScrollBars();

   int ValidateMarkerLineIndex(int lineIndex);
   void PaintMarker(CDC& dc, int lineIndex);

   void NotifyMarkerChanged();
   void NotifyInteractiveMarkerChanged(int lineIndex);

   virtual void PixelChanged(int x, int y);
   virtual void ScaleChanged();
   virtual void RoiChanged();
   virtual void ViewChanged();

   virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
   virtual void PreSubclassWindow();
   virtual BOOL PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()
   afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
   afx_msg void OnDestroy();
   afx_msg void OnPaint();
   afx_msg BOOL OnEraseBkgnd(CDC* pDC);
   afx_msg void OnSize(UINT nType, int cx, int cy);
   afx_msg void OnSetFocus(CWnd* pOldWnd);
   afx_msg void OnMouseMove(UINT nFlags, CPoint point);
   afx_msg LRESULT OnMouseLeave(WPARAM wParam, LPARAM lParam);
   afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
   afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
   afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};

#pragma endregion

#endif	// _IMAGEEXWND_H_