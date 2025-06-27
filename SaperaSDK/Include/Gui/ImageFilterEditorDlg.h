#pragma once

// CImageFilterEditorDlg dialog

class SAPCLASSGUI_CLASS CImageFilterEditorDlg : public CDialog
{
	DECLARE_DYNAMIC(CImageFilterEditorDlg)

public:
   CImageFilterEditorDlg(SapAcquisition* pAcq, CWnd* pParent = NULL);
	virtual ~CImageFilterEditorDlg();

protected:

   SapAcquisition* mp_Acq;
   
   // hardware values (not floating points)
   UINT32 m_Divisor;
   INT32 m_MinValue;
   INT32 m_MaxValue;

   bool CreateCoeffEditControls(int count);
   void DestroyCoeffEditControls();

   void InitControls();
   void ArrangeCoeffEditControls();

   bool UpdateState();
   void UpdateKernelSizeCombo();

   bool ReadCoefficients();
   bool ReadCoefficientsFromBuffer(SapBuffer& buffer);
   bool WriteCoefficients();
   bool WriteCoefficientsToBuffer(SapBuffer& buffer);

   static bool ConvertToDecimal(CStringA string, double& decimal);

   enum { IDD = IDD_SCG_IMAGE_FILTER_EDITOR };

   CButton m_EnableCheckbox;
   CComboBox m_FilterIndexCombo;
   CComboBox m_KernelSizeCombo;
   CString m_CoeffMinValue;
   CString m_CoeffMaxValue;
   CStatic m_CoeffArea;

   int m_CoeffEditCount;
   CEdit* mp_CoeffEditBoxes;

   virtual BOOL OnInitDialog();
   virtual void OnOK();
   virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
   afx_msg void OnCheckImageFilterEnable();
   afx_msg void OnChangeImageFilterIndex();
   afx_msg void OnChangeImageFilterSize();
   afx_msg void OnBnClickedBtnImport();
   afx_msg void OnBnClickedBtnExport();
};
