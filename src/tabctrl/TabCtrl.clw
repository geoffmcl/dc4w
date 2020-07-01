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

ResourceCount=6
Resource1=IDD_TABCTRL_DIALOG
Resource2=IDD_OUTLINE
Resource3=IDR_MENU1
Resource4=IDD_EXPANDED
Resource5=IDD_PREFERENCES
Resource6=IDD_COMPARE

[CLS:CCompPg]
Type=0
BaseClass=CPropertyPage
HeaderFile=CompPg.h
ImplementationFile=CompPg.cpp
LastObject=IDC_IGNORESP
Filter=D
VirtualFilter=idWC

[CLS:CExpPag]
Type=0
BaseClass=CPropertyPage
HeaderFile=ExpPag.h
ImplementationFile=ExpPag.cpp
Filter=D
VirtualFilter=idWC
LastObject=CExpPag

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
LastObject=CTabCtrlApp
Filter=N
VirtualFilter=AC

[CLS:CTabCtrlDlg]
Type=0
BaseClass=CDialog
HeaderFile=TabCtrlDlg.h
ImplementationFile=TabCtrlDlg.cpp
Filter=D
VirtualFilter=dWC

[CLS:CTabSheet]
Type=0
BaseClass=CPropertySheet
HeaderFile=TabSheet.h
ImplementationFile=TabSheet.cpp
Filter=W
VirtualFilter=hWC
LastObject=IDM_COMPARE

[DLG:IDD_COMPARE]
Type=1
Class=CCompPg
ControlCount=6
Control1=IDC_NOCASE,button,1342242819
Control2=IDC_STATIC,button,1342177287
Control3=IDC_IGNORESP,button,1342242819
Control4=IDC_SKIPCCOMM,button,1342242819
Control5=IDC_SKIPQTXT,button,1342242819
Control6=IDC_IGNORETERM,button,1342242819

[DLG:IDD_EXPANDED]
Type=1
Class=CExpPag
ControlCount=6
Control1=IDC_RADIO1,button,1342308361
Control2=IDC_RADIO2,button,1342177289
Control3=IDC_RADIO3,button,1342177289
Control4=IDC_RADIO4,button,1342177289
Control5=IDC_STATIC,button,1342177287
Control6=IDC_COLOURS,button,1342242817

[DLG:IDD_OUTLINE]
Type=1
Class=COutPg
ControlCount=1
Control1=IDC_SHOWIDENT,button,1342242819

[DLG:IDD_TABCTRL_DIALOG]
Type=1
Class=CTabCtrlDlg
ControlCount=6
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_TAB1,SysTabControl32,1342177280
Control4=IDM_SHOW,button,1342210048
Control5=ID_APPLY_NOW,button,1342242816
Control6=ID_CONTEXT_HELP,button,1342242816

[DLG:IDD_PREFERENCES]
Type=1
Class=?
ControlCount=4
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_SSTAB1,{BDC217C5-ED16-11CD-956C-0000C04E4C0A},1342242816
Control4=IDM_SHOW,button,1342210048

[MNU:IDR_MENU1]
Type=1
Class=?
Command1=IDM_SHOW
Command2=IDM_COMPARE
Command3=IDM_OUTLINE
Command4=IDM_EXPANDED
CommandCount=4

