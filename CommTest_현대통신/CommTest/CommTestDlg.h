
// CommTestDlg.h : 헤더 파일
//

#pragma once
#include "CommThread.h"


// CCommTestDlg 대화 상자
class CCommTestDlg : public CDialogEx
{
// 생성입니다.
public:
	CCommTestDlg(CWnd* pParent = NULL);	// 표준 생성자입니다.
	~CCommTestDlg();	                // 표준 소멸자입니다.

// 대화 상자 데이터입니다.
	enum { IDD = IDD_COMMTEST_DIALOG };
	enum STATE { NORMAL, INIT_REQ, REGIST_REQ, REMOVE_REQ };

	CommThread m_ComuPort; // 앞에서 만든 통신용 클래스
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
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
