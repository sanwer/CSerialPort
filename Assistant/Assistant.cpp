// SCOMM.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Assistant.h"
#include "MainDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainApp

BEGIN_MESSAGE_MAP(CMainApp, CWinApp)
	//{{AFX_MSG_MAP(CMainApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainApp construction

CMainApp::CMainApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMainApp object

CMainApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CMainApp initialization

//CMainDlg* m_ctrlComId;

BOOL CMainApp::InitInstance()
{
	AfxEnableControlContainer();

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	CMainDlg dlg;
	m_pMainWnd = &dlg;

	SetDialogBkColor(RGB(204,204,153), RGB(0, 0, 0));//ÍÁ»ÆÉ«

	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		//dlg.m_Port.ClosePort();
	}
	else if (nResponse == IDCANCEL)
	{
		//dlg.m_Port.ClosePort();
	}

	return FALSE;
}
