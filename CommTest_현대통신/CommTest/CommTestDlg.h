
// CommTestDlg.h : ��� ����
//

#pragma once
#include "CommThread.h"


// CCommTestDlg ��ȭ ����
class CCommTestDlg : public CDialogEx
{
// �����Դϴ�.
public:
	CCommTestDlg(CWnd* pParent = NULL);	// ǥ�� �������Դϴ�.
	~CCommTestDlg();	                // ǥ�� �Ҹ����Դϴ�.

// ��ȭ ���� �������Դϴ�.
	enum { IDD = IDD_COMMTEST_DIALOG };
	enum STATE { NORMAL, INIT_REQ, REGIST_REQ, REMOVE_REQ };

	CommThread m_ComuPort; // �տ��� ���� ��ſ� Ŭ����
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV �����Դϴ�.


// �����Դϴ�.
protected:
	HICON m_hIcon;

	// ������ �޽��� �� �Լ�
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnConnect();
    bool m_bSerialConnected;
	int m_nIdxPort;
	afx_msg void OnBnClickedBtnRegist();
	afx_msg void OnBnClickedBtnRemove();

	CListCtrl m_List;
protected:
//	afx_msg LRESULT OnMymessage(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnReceive(WPARAM wParam, LPARAM lParam);
public:
	int readn(unsigned char* buf, int len);
	int dispatch();
	bool mFirst;
	HANDLE m_hThreadDisp;
	CEdit mId;
	CEdit mName;
	STATE mState;
	afx_msg void OnLvnItemchangedRegist(NMHDR* pNMHDR, LRESULT* pResult);
	CButton mRegist;
	CButton mRemove;
//	CComboBox mConnect;
	CButton mConnect;

	CString mIdSave;
	CString mNameSave;
	int mIdx;
	afx_msg void OnBnClickedBtnFeature();
//	virtual HRESULT accDoDefaultAction(VARIANT varChild);
	int CalcCRC(unsigned char* buf);
	afx_msg void OnBnClickedBtnNotice();
	CEdit mNotice;
};

DWORD ThreadDisp(CCommTestDlg* pDlg);
