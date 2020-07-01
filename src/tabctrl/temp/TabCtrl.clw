; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CCompPg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "tabctrl.h"
LastPage=0

ClassCount=6
Class1=CCompPg
Class2=CExpPag
Class3=COutPg
Class4=CTabCtrlApp
Class5=CTabCtrlDlg
Class6=CTabSheet

ResourceCount=5
Resource1=IDD_COMPARE
Resource2=IDD_OUTLINE
Resource3=IDD_EXPANDED
Resource4=IDD_TABCTRL_DIALOG
Resource5=IDD_PREFERENCES

[CLS:CCompPg]
Type=0
BaseClass=CPropertyPage
HeaderFile=CompPg.h
ImplementationFile=CompPg.cpp
LastObject=CCompPg

[CLS:CExpPag]
Type=0
BaseClass=CPropertyPage
HeaderFile=ExpPag.h
ImplementationFile=ExpPag.cpp

[CLS:COutPg]
Type=0
BaseClass=CPropertyPage
HeaderFile=OutPg.h
ImplementationFile=OutPg.cpp

[CLS:CTabCtrlApp]
Type=0
BaseClass=CWinApp
HeaderFile=TabCtrl.h
ImplementationFile=TabCtrl.cpp

[CLS:CTabCtrlDlg]
Type=0
BaseClass=CDialog
HeaderFile=TabCtrlDlg.h
ImplementationFile=TabCtrlDlg.cpp

[CLS:CTabSheet]
Type=0
BaseClass=CPropertySheet
HeaderFile=TabSheet.h
ImplementationFile=TabSheet.cpp

[DLG:IDD_COMPARE]
Type=1
Class=CCompPg
ControlCount=1
Control1=IDC_NOCASE,button,1342242819

[DLG:IDD_EXPANDED]
Type=1
Class=CExpPag
ControlCount=4
Control1=IDC_RADIO1,button,1342177289
Control2=IDC_RADIO2,button,1342177289
Control3=IDC_RADIO3,button,1342177289
Control4=IDC_RADIO4,button,1342177289

[DLG:IDD_OUTLINE]
Type=1
Class=COutPg
ControlCount=1
Control1=IDC_SHOWIDENT,button,1342242819

[DLG:IDD_TABCTRL_DIALOG]
Type=1
Class=CTabCtrlDlg
ControlCount=4
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_TAB1,SysTabControl32,1342177280
Control4=IDM_SHOW,button,1342210048

[DLG:IDD_PREFERENCES]
Type=1
Class=?
ControlCount=4
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_SSTAB1,{BDC217C5-ED16-11CD-956C-0000C04E4C0A},1342242816
Control4=IDM_SHOW,button,1342210048

