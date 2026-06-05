/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// OptionsPanel.cpp : implementation file
//

#include "StdAfx.h"
#include "WorldBuilder.h"
#include "WorldBuilderDoc.h"
#include "OptionsPanel.h"

/////////////////////////////////////////////////////////////////////////////
// COptionsPanel dialog


COptionsPanel::COptionsPanel(Int dlgid /*=0*/, CWnd* pParent /*=nullptr*/)
	: CDialog(dlgid ? dlgid : COptionsPanel::IDD, pParent)
{
	//{{AFX_DATA_INIT(COptionsPanel)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void COptionsPanel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsPanel)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

static void RebornEnsureOptionsPanelVisible(CWnd* window)
{
	if (!window || !::IsWindow(window->GetSafeHwnd()))
		return;

	CRect frameRect;
	window->GetWindowRect(&frameRect);

	if (::MonitorFromRect(&frameRect, MONITOR_DEFAULTTONULL))
		return;

	CRect workArea;
	::SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);

	window->SetWindowPos(
		nullptr,
		workArea.left + 50,
		workArea.top + 50,
		0,
		0,
		SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

BEGIN_MESSAGE_MAP(COptionsPanel, CDialog)
	//{{AFX_MSG_MAP(COptionsPanel)
	ON_WM_MOVE()
	ON_WM_SHOWWINDOW()
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionsPanel message handlers

void COptionsPanel::OnMove(int x, int y)
{
	CDialog::OnMove(x, y);

	if (this->IsWindowVisible() && !this->IsIconic()) {
		CRect frameRect;
		GetWindowRect(&frameRect);
		::AfxGetApp()->WriteProfileInt(OPTIONS_PANEL_SECTION, "Top", frameRect.top);
		::AfxGetApp()->WriteProfileInt(OPTIONS_PANEL_SECTION, "Left", frameRect.left);
	}

}

void COptionsPanel::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialog::OnShowWindow(bShow, nStatus);

	if (bShow && !IsIconic())
		RebornEnsureOptionsPanelVisible(this);
}

void COptionsPanel::OnEditRedo()
{
	// Redirect undo/redo to the doc so they get executed.
	CWorldBuilderDoc *pDoc = CWorldBuilderDoc::GetActiveDoc();
	if (pDoc) {
		pDoc->OnEditRedo();
	}
}

void COptionsPanel::OnUpdateEditRedo(CCmdUI* pCmdUI)
{
	// Redirect undo/redo to the doc so they get executed.
	CWorldBuilderDoc *pDoc = CWorldBuilderDoc::GetActiveDoc();
	if (pDoc) {
		pDoc->OnUpdateEditRedo(pCmdUI);
	}
}

void COptionsPanel::OnEditUndo()
{
	// Redirect undo/redo to the doc so they get executed.
	CWorldBuilderDoc *pDoc = CWorldBuilderDoc::GetActiveDoc();
	if (pDoc) {
		pDoc->OnEditUndo();
	}
}

void COptionsPanel::OnUpdateEditUndo(CCmdUI* pCmdUI)
{
	// Redirect undo/redo to the doc so they get executed.
	CWorldBuilderDoc *pDoc = CWorldBuilderDoc::GetActiveDoc();
	if (pDoc) {
		pDoc->OnUpdateEditUndo(pCmdUI);
	}
}
