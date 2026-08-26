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
#include "GameClient/GadgetPushButton.h"
#include "GameClient/GadgetStaticText.h"
#include "GameClient/GameText.h"
#include "GameClient/GameWindowManager.h"
#include "GameClient/GUICallbacks.h"
#include "GameClient/KeyDefs.h"
#include "GameClient/MessageBox.h"
#include "GameClient/RebornOmegaUpdater.h"
#include "GameClient/Shell.h"
#include "GameClient/WindowLayout.h"

#include <string>
#include <vector>

static GameWindow* s_changeLogMenuROParent = nullptr;
static GameWindow* s_changeLogMenuROListBox = nullptr;
static Bool s_changeLogMenuROCloseRequested = FALSE;
static Bool s_changeLogMenuROOptionsOverlay = FALSE;

static GameWindow* s_changeLogMenuROVersionLabel = nullptr;
static GameWindow* s_changeLogMenuROPreviousButton = nullptr;
static GameWindow* s_changeLogMenuRONextButton = nullptr;

static std::vector<RebornOmegaVersionInfo>
s_changeLogMenuROVersions;

static Int s_changeLogMenuROSelectedVersion = -1;
static Int s_changeLogMenuRODownloadingVersion = -1;

static DWORD s_changeLogMenuROLastLoadingUpdate = 0;
static Int s_changeLogMenuROLoadingDots = 0;

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

static void SetChangeLogMenuROStatus(
	const UnicodeString& text)
{
	if (!s_changeLogMenuROListBox)
		return;

	GadgetListBoxReset(
		s_changeLogMenuROListBox);

	GadgetListBoxAddEntryText(
		s_changeLogMenuROListBox,
		text,
		GameMakeColor(254, 254, 254, 255),
		-1);
}

static void UpdateChangeLogMenuRONavigation()
{
	Bool downloading =
		GetRebornOmegaChangeLogDownloadState() ==
		REBORN_CHANGELOG_DOWNLOADING;

	if (s_changeLogMenuROPreviousButton)
	{
		s_changeLogMenuROPreviousButton->winEnable(
			!downloading &&
			s_changeLogMenuROSelectedVersion > 0);
	}

	if (s_changeLogMenuRONextButton)
	{
		s_changeLogMenuRONextButton->winEnable(
			!downloading &&
			s_changeLogMenuROSelectedVersion >= 0 &&
			s_changeLogMenuROSelectedVersion <
			static_cast<Int>(
				s_changeLogMenuROVersions.size()) - 1);
	}
}

static void UpdateChangeLogMenuROLoadingText()
{
	UnicodeString loadingText =
		TheGameText->fetch(
			"GUI:RebornOmegaChangeLogLoading");

	switch (s_changeLogMenuROLoadingDots)
	{
	case 1:
		loadingText.concat(L".");
		break;

	case 2:
		loadingText.concat(L"..");
		break;

	case 3:
		loadingText.concat(L"...");
		break;
	}

	SetChangeLogMenuROStatus(
		loadingText);
}

static void ShowSelectedChangeLogMenuROVersion()
{
	if (s_changeLogMenuROSelectedVersion < 0 ||
		s_changeLogMenuROSelectedVersion >=
		static_cast<Int>(
			s_changeLogMenuROVersions.size()))
	{
		return;
	}

	RebornOmegaVersionInfo& version =
		s_changeLogMenuROVersions[
			s_changeLogMenuROSelectedVersion];

	if (s_changeLogMenuROVersionLabel)
	{
		UnicodeString versionText;
		versionText.translate(
			version.displayName.c_str());

		GadgetStaticTextSetText(
			s_changeLogMenuROVersionLabel,
			versionText);
	}

	if (!version.changeLog.empty())
	{
		PopulateChangeLogMenuRO(
			version.changeLog.c_str());

		UpdateChangeLogMenuRONavigation();
		return;
	}

	s_changeLogMenuROLoadingDots = 0;
	s_changeLogMenuROLastLoadingUpdate =
		timeGetTime();

	UpdateChangeLogMenuROLoadingText();

	s_changeLogMenuRODownloadingVersion =
		s_changeLogMenuROSelectedVersion;

	if (!StartRebornOmegaChangeLogDownload(
		version.pageUrl))
	{
		s_changeLogMenuRODownloadingVersion = -1;

		SetChangeLogMenuROStatus(
			TheGameText->fetch(
				"GUI:RebornOmegaChangeLogUnavailable"));
	}

	UpdateChangeLogMenuRONavigation();
}

Bool IsChangeLogMenuROOptionsOverlayActive()
{
	return s_changeLogMenuROOptionsOverlay;
}

static void CloseChangeLogMenuRO()
{
	if (!s_changeLogMenuROParent)
		return;

	s_changeLogMenuROCloseRequested = FALSE;

	if (!s_changeLogMenuROOptionsOverlay)
	{
		RequestChangeLogMessageBoxRestore();
		TheShell->pop();
		return;
	}

	GameWindow* changeLogWindow =
		s_changeLogMenuROParent;

	s_changeLogMenuROOptionsOverlay = FALSE;
	s_changeLogMenuROListBox = nullptr;
	s_changeLogMenuROParent = nullptr;
	s_changeLogMenuROVersionLabel = nullptr;
	s_changeLogMenuROPreviousButton = nullptr;
	s_changeLogMenuRONextButton = nullptr;
	s_changeLogMenuROVersions.clear();
	s_changeLogMenuROSelectedVersion = -1;
	s_changeLogMenuRODownloadingVersion = -1;
	s_changeLogMenuROLastLoadingUpdate = 0;
	s_changeLogMenuROLoadingDots = 0;

	TheWindowManager->winDestroy(
		changeLogWindow);

	GameWindow* mainMenuWindow =
		TheWindowManager->winGetWindowFromId(
			nullptr,
			NAMEKEY(
				"MainMenu.wnd:MainMenuParent"));

	if (mainMenuWindow)
	{
		mainMenuWindow->winHide(FALSE);
		mainMenuWindow->winEnable(TRUE);
	}

	RestoreChangeLogMessageBox();
}

void ShowChangeLogMenuRO()
{
	if (!IsChangeLogMessageBoxFromOptions())
	{
		TheShell->push(
			"Menus/ChangeLogMenuRO.wnd");

		return;
	}

	GameWindow* mainMenuWindow =
		TheWindowManager->winGetWindowFromId(
			nullptr,
			NAMEKEY(
				"MainMenu.wnd:MainMenuParent"));

	if (mainMenuWindow)
	{
		mainMenuWindow->winHide(TRUE);
		mainMenuWindow->winEnable(FALSE);
	}

	s_changeLogMenuROOptionsOverlay = TRUE;

	GameWindow* changeLogWindow =
		TheWindowManager->winCreateFromScript(
			"Menus/ChangeLogMenuRO.wnd");

	if (!changeLogWindow)
	{
		s_changeLogMenuROOptionsOverlay = FALSE;

		if (mainMenuWindow)
		{
			mainMenuWindow->winHide(FALSE);
			mainMenuWindow->winEnable(TRUE);
		}

		RestoreChangeLogMessageBox();
		return;
	}

	ChangeLogMenuROInit(
		nullptr,
		nullptr);

	changeLogWindow->winHide(FALSE);
	changeLogWindow->winEnable(TRUE);
	changeLogWindow->winBringToTop();

	TheWindowManager->winSetModal(
		changeLogWindow);
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

	s_changeLogMenuROVersionLabel =
		TheWindowManager->winGetWindowFromId(
			s_changeLogMenuROParent,
			NAMEKEY(
				"ChangeLogMenuRO.wnd:StaticTextVersion"));

	s_changeLogMenuROPreviousButton =
		TheWindowManager->winGetWindowFromId(
			s_changeLogMenuROParent,
			NAMEKEY(
				"ChangeLogMenuRO.wnd:ButtonPreviousVersion"));

	s_changeLogMenuRONextButton =
		TheWindowManager->winGetWindowFromId(
			s_changeLogMenuROParent,
			NAMEKEY(
				"ChangeLogMenuRO.wnd:ButtonNextVersion"));

	s_changeLogMenuROVersions.clear();
	s_changeLogMenuROSelectedVersion = -1;
	s_changeLogMenuRODownloadingVersion = -1;

	RebornOmegaChangeLogDownloadState downloadState =
		GetRebornOmegaChangeLogDownloadState();

	if (downloadState ==
		REBORN_CHANGELOG_COMPLETED ||
		downloadState ==
		REBORN_CHANGELOG_FAILED)
	{
		FinishRebornOmegaChangeLogDownload();
	}

	if (!GetRebornOmegaVersionList(
		s_changeLogMenuROVersions))
	{
		if (s_changeLogMenuROVersionLabel)
		{
			GadgetStaticTextSetText(
				s_changeLogMenuROVersionLabel,
				TheGameText->fetch(
					"GUI:RebornOmegaChangeLogUnavailable"));
		}

		SetChangeLogMenuROStatus(
			TheGameText->fetch(
				"GUI:RebornOmegaChangeLogUnavailable"));

		UpdateChangeLogMenuRONavigation();
		return;
	}

	s_changeLogMenuROSelectedVersion =
		static_cast<Int>(
			s_changeLogMenuROVersions.size()) - 1;

	ShowSelectedChangeLogMenuROVersion();
	
}

void ChangeLogMenuROUpdate(
	WindowLayout* layout,
	void* userData)
{
	if (s_changeLogMenuROCloseRequested)
	{
		CloseChangeLogMenuRO();
		return;
	}

	RebornOmegaChangeLogDownloadState state =
		GetRebornOmegaChangeLogDownloadState();

	if (state == REBORN_CHANGELOG_DOWNLOADING)
	{
		DWORD currentTime =
			timeGetTime();

		if (currentTime -
			s_changeLogMenuROLastLoadingUpdate >= 350)
		{
			s_changeLogMenuROLastLoadingUpdate =
				currentTime;

			s_changeLogMenuROLoadingDots =
				(s_changeLogMenuROLoadingDots + 1) % 4;

			UpdateChangeLogMenuROLoadingText();
		}

		return;
	}

	if (state == REBORN_CHANGELOG_COMPLETED)
	{
		std::string changeLog;

		if (GetRebornOmegaChangeLogDownloadResult(
			changeLog) &&
			s_changeLogMenuRODownloadingVersion >= 0 &&
			s_changeLogMenuRODownloadingVersion <
			static_cast<Int>(
				s_changeLogMenuROVersions.size()))
		{
			RebornOmegaVersionInfo& version =
				s_changeLogMenuROVersions[
					s_changeLogMenuRODownloadingVersion];

			version.changeLog =
				changeLog;

			if (s_changeLogMenuROSelectedVersion ==
				s_changeLogMenuRODownloadingVersion)
			{
				PopulateChangeLogMenuRO(
					version.changeLog.c_str());
			}
		}

		s_changeLogMenuRODownloadingVersion = -1;

		FinishRebornOmegaChangeLogDownload();
		UpdateChangeLogMenuRONavigation();
		return;
	}

	if (state == REBORN_CHANGELOG_FAILED)
	{
		s_changeLogMenuRODownloadingVersion = -1;

		FinishRebornOmegaChangeLogDownload();

		SetChangeLogMenuROStatus(
			TheGameText->fetch(
				"GUI:RebornOmegaChangeLogUnavailable"));

		UpdateChangeLogMenuRONavigation();
	}
}

void ChangeLogMenuROShutdown(
	WindowLayout* layout,
	void* userData)
{
	if (layout)
		layout->hide(TRUE);

	s_changeLogMenuROParent = nullptr;
	s_changeLogMenuROListBox = nullptr;
	s_changeLogMenuROVersionLabel = nullptr;
	s_changeLogMenuROPreviousButton = nullptr;
	s_changeLogMenuRONextButton = nullptr;
	s_changeLogMenuROVersions.clear();
	s_changeLogMenuROSelectedVersion = -1;
	s_changeLogMenuRODownloadingVersion = -1;
	s_changeLogMenuROLastLoadingUpdate = 0;
	s_changeLogMenuROLoadingDots = 0;
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
		NAMEKEY(
			"ChangeLogMenuRO.wnd:ButtonPreviousVersion"))
	{
		if (s_changeLogMenuROSelectedVersion > 0 &&
			GetRebornOmegaChangeLogDownloadState() !=
			REBORN_CHANGELOG_DOWNLOADING)
		{
			--s_changeLogMenuROSelectedVersion;
			ShowSelectedChangeLogMenuROVersion();
		}

		return MSG_HANDLED;
	}

	if (controlID ==
		NAMEKEY(
			"ChangeLogMenuRO.wnd:ButtonNextVersion"))
	{
		if (s_changeLogMenuROSelectedVersion >= 0 &&
			s_changeLogMenuROSelectedVersion <
			static_cast<Int>(
				s_changeLogMenuROVersions.size()) - 1 &&
			GetRebornOmegaChangeLogDownloadState() !=
			REBORN_CHANGELOG_DOWNLOADING)
		{
			++s_changeLogMenuROSelectedVersion;
			ShowSelectedChangeLogMenuROVersion();
		}

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
