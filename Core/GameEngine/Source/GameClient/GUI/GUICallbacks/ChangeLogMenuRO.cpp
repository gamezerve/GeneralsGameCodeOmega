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

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

// FILE: ChangeLogMenuRO.cpp ///////////////////////////////////////////////////////////////////////
// Author: Gamezerve, August 2026
// Desc: GUI callbacks for the Reborn Omega version changelog menu
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/NameKeyGenerator.h"
#include "GameClient/GadgetListBox.h"
#include "GameClient/GadgetRadioButton.h"
#include "GameClient/GameWindowManager.h"
#include "GameClient/GUICallbacks.h"
#include "GameClient/KeyDefs.h"
#include "GameClient/MessageBox.h"
#include "GameClient/Shell.h"
#include "GameClient/WindowLayout.h"

#include <string>

static GameWindow* s_changeLogMenuROParent = nullptr;
static GameWindow* s_changeLogMenuROListBox = nullptr;
static GameWindow* s_changeLogMenuROCurrentVersion = nullptr;
static GameWindow* s_changeLogMenuRONewVersion = nullptr;
static Bool s_changeLogMenuROCloseRequested = FALSE;

static void PopulateChangeLogMenuRO(const char* changeLog)
{
	if (!s_changeLogMenuROListBox)
		return;

	GadgetListBoxReset(s_changeLogMenuROListBox);

	if (!changeLog || !*changeLog)
		return;

	std::string text = changeLog;
	size_t position = 0;

	while (position <= text.size())
	{
		size_t lineEnd = text.find('\n', position);

		std::string line =
			text.substr(
				position,
				lineEnd == std::string::npos
				? std::string::npos
				: lineEnd - position);

		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);

		UnicodeString translated;
		translated.translate(line.c_str());

		GadgetListBoxAddEntryText(
			s_changeLogMenuROListBox,
			translated,
			GameMakeColor(254, 254, 254, 255),
			-1);

		if (lineEnd == std::string::npos)
			break;

		position = lineEnd + 1;
	}
}

static void CloseChangeLogMenuRO()
{
	if (!s_changeLogMenuROParent)
		return;

	s_changeLogMenuROCloseRequested = FALSE;

	if (IsChangeLogMessageBoxFromOptions())
		SkipNextMainMenuTransition();

	RequestChangeLogMessageBoxRestore();
	TheShell->pop();
}

void ShowChangeLogMenuRO()
{
	TheShell->push("Menus/ChangeLogMenuRO.wnd");
}

void ChangeLogMenuROInit(
	WindowLayout* layout,
	void* userData)
{
	s_changeLogMenuROCloseRequested = FALSE;

	s_changeLogMenuROParent =
		TheWindowManager->winGetWindowFromId(
			nullptr,
			NAMEKEY("ChangeLogMenuRO.wnd:Parent"));

	s_changeLogMenuROListBox =
		TheWindowManager->winGetWindowFromId(
			s_changeLogMenuROParent,
			NAMEKEY("ChangeLogMenuRO.wnd:ListboxChangeLog"));

	s_changeLogMenuROCurrentVersion =
		TheWindowManager->winGetWindowFromId(
			s_changeLogMenuROParent,
			NAMEKEY("ChangeLogMenuRO.wnd:RadioButtonCurrentVersion"));

	s_changeLogMenuRONewVersion =
		TheWindowManager->winGetWindowFromId(
			s_changeLogMenuROParent,
			NAMEKEY("ChangeLogMenuRO.wnd:RadioButtonNewVersion"));

	const char* currentChangeLog =
		GetRebornOmegaCurrentChangeLog();

	const char* newChangeLog =
		GetRebornOmegaNewChangeLog();

	Bool hasCurrentChangeLog =
		currentChangeLog && *currentChangeLog;

	Bool hasNewChangeLog =
		newChangeLog && *newChangeLog;

	if (s_changeLogMenuROCurrentVersion)
		s_changeLogMenuROCurrentVersion->winEnable(
			hasCurrentChangeLog);

	if (s_changeLogMenuRONewVersion)
		s_changeLogMenuRONewVersion->winEnable(
			hasNewChangeLog);

	if (hasNewChangeLog)
	{
		GadgetRadioSetSelection(
			s_changeLogMenuRONewVersion,
			TRUE);

		PopulateChangeLogMenuRO(newChangeLog);
	}
	else if (hasCurrentChangeLog)
	{
		GadgetRadioSetSelection(
			s_changeLogMenuROCurrentVersion,
			TRUE);

		PopulateChangeLogMenuRO(currentChangeLog);
	}
}

void ChangeLogMenuROUpdate(
	WindowLayout* layout,
	void* userData)
{
	if (s_changeLogMenuROCloseRequested)
		CloseChangeLogMenuRO();
}

void ChangeLogMenuROShutdown(
	WindowLayout* layout,
	void* userData)
{
	if (layout)
		layout->hide(TRUE);

	s_changeLogMenuROParent = nullptr;
	s_changeLogMenuROListBox = nullptr;
	s_changeLogMenuROCurrentVersion = nullptr;
	s_changeLogMenuRONewVersion = nullptr;
	s_changeLogMenuROCloseRequested = FALSE;

	TheShell->shutdownComplete(layout);
}

WindowMsgHandledType ChangeLogMenuROSystem(
	GameWindow* window,
	UnsignedInt msg,
	WindowMsgData mData1,
	WindowMsgData mData2)
{
	if (msg != GBM_SELECTED)
		return MSG_IGNORED;

	GameWindow* control =
		reinterpret_cast<GameWindow*>(mData1);

	if (!control)
		return MSG_IGNORED;

	Int controlID = control->winGetWindowId();

	if (controlID ==
		NAMEKEY("ChangeLogMenuRO.wnd:RadioButtonCurrentVersion"))
	{
		PopulateChangeLogMenuRO(
			GetRebornOmegaCurrentChangeLog());

		return MSG_HANDLED;
	}

	if (controlID ==
		NAMEKEY("ChangeLogMenuRO.wnd:RadioButtonNewVersion"))
	{
		PopulateChangeLogMenuRO(
			GetRebornOmegaNewChangeLog());

		return MSG_HANDLED;
	}

	if (controlID ==
		NAMEKEY("ChangeLogMenuRO.wnd:ButtonClose"))
	{
		s_changeLogMenuROCloseRequested = TRUE;
		return MSG_HANDLED;
	}

	return MSG_IGNORED;
}

WindowMsgHandledType ChangeLogMenuROInput(
	GameWindow* window,
	UnsignedInt msg,
	WindowMsgData mData1,
	WindowMsgData mData2)
{
	if (msg == GWM_CHAR &&
		static_cast<UnsignedByte>(mData1) == KEY_ESC &&
		BitIsSet(
			static_cast<UnsignedByte>(mData2),
			KEY_STATE_UP))
	{
		s_changeLogMenuROCloseRequested = TRUE;
		return MSG_HANDLED;
	}

	return MSG_IGNORED;
}
