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

///////////////////////////////////////////////////////////////////////////////////////
// FILE: ChaptersMenu.cpp 
// Author: Chris Brue, August 2002
// Description: 
///////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/ChapterProgress.h"
#include "Common/GameEngine.h"
#include "Common/GameState.h"
#include "Common/GlobalData.h"
#include "Common/MessageStream.h"
#include "Common/NameKeyGenerator.h"
#include "Common/OptionPreferences.h"
#include "Common/RandomValue.h"
#include "GameClient/CampaignManager.h"
#include "GameClient/Display.h"
#include "GameClient/Gadget.h"
#include "GameClient/GadgetListBox.h"
#include "GameClient/GameWindow.h"
#include "GameClient/GameWindowManager.h"
#include "GameClient/GameWindowTransitions.h"
#include "GameClient/MapUtil.h"
#include "GameClient/Shell.h"
#include "GameClient/WindowLayout.h"
#include "GameLogic/GameLogic.h"
#include "GameLogic/ScriptEngine.h"

#include <fstream>
#include <string>
#include <vector>
#include <cctype>

static NameKeyType buttonBackID = NAMEKEY_INVALID;
static NameKeyType buttonStartID = NAMEKEY_INVALID;
static NameKeyType radioEasyID = NAMEKEY_INVALID;
static NameKeyType radioMediumID = NAMEKEY_INVALID;
static NameKeyType radioHardID = NAMEKEY_INVALID;

static GameDifficulty selectedDifficulty = DIFFICULTY_NORMAL;


static Int initialGadgetDelay = 2;
static Bool justEntered = FALSE;
static Bool buttonPushed = FALSE;
static Bool chapterImagesCached = FALSE;
static Bool startGame = FALSE;
static Bool isShuttingDown = FALSE;
static Bool ignoreNextMapSelectionForDoubleClick = FALSE;
static Bool resetDifficultyOnNextInit = FALSE;
static Bool resetChapterImagesOnNextInit = FALSE;
static Bool missionLaunchPending = FALSE;

static UnsignedInt lastMapClickTime = 0;
static Int lastMapClickRow = -1;

static GameWindow* chapterButtons[8];
static Int selectedChapter = -1;

static const Image* normalImages[8][3];
static const Image* selectedImages[8][3];

static Color normalTextColor[8];
static Color normalTextBorderColor[8];
static Color normalHiliteTextColor[8];
static Color normalHiliteTextBorderColor[8];

static Color selectedTextColor[8];
static Color selectedTextBorderColor[8];

static const Image* normalHiliteImages[8][3];

static std::vector<std::string> campaignMapItemData;

static Int lastSelectedMapRow = 0;

static void StartChapterMission(AsciiString mapName, GameDifficulty diff)
{

  if (missionLaunchPending || startGame)
    return;

  missionLaunchPending = TRUE;

  startGame = TRUE;

  TheCampaignManager->setGameDifficulty(diff);

  OptionPreferences pref;
  pref.setCampaignDifficulty(diff);
  pref.write();

  TheScriptEngine->setGlobalDifficulty(diff);

  TheWritableGlobalData->m_pendingFile = mapName;

  TheShell->reverseAnimatewindow();
  TheTransitionHandler->setGroup("FadeWholeScreen");
}

static void ListboxClear(GameWindow* listbox)
{
  GadgetListBoxReset(listbox);
}

static void ListboxAdd(GameWindow* listbox, const char* text, Int row)
{
  UnicodeString utext;
  utext.translate(text);

  GadgetListBoxAddEntryText(listbox, utext, 0xFFFFFFFF, row);
}

static void DrawMapListboxDividers(GameWindow* window, WinInstanceData* instData)
{
  if (!window)
    return;

  NameKeyType listID = TheNameKeyGenerator->nameToKey("ChaptersMenu.wnd:ListboxMap");
  GameWindow* listbox = TheWindowManager->winGetWindowFromId(nullptr, listID);

  if (!listbox)
    return;

  Int winx;
  Int winy;
  listbox->winGetScreenPosition(&winx, &winy);

  ICoord2D size;
  listbox->winGetSize(&size.x, &size.y);

  const Int lineLeft = winx + 2;
  const Int lineRight = winx + size.x - 22;
  const Color lineColor = GameMakeColor(255, 255, 255, 55);

  Int lastDividerY = -1;

  // İlk iki divider'ı bulmak için
  Int firstDividerY = -1;
  Int secondDividerY = -1;

  for (Int y = 1; y < size.y; ++y)
  {
    Int rowA = -1;
    Int colA = -1;
    Int rowB = -1;
    Int colB = -1;

    GadgetListBoxGetEntryBasedOnXY(listbox, winx + 4, winy + y - 1, rowA, colA);
    GadgetListBoxGetEntryBasedOnXY(listbox, winx + 4, winy + y, rowB, colB);

    if (rowA >= 0 && rowB >= 0 && rowA != rowB)
    {
      Int drawY = winy + y + 4;

      // Çizgiyi çiz
      TheDisplay->drawLine(
        lineLeft,
        drawY,
        lineRight,
        drawY,
        1,
        lineColor
      );

      // Divider'ları kaydet
      if (firstDividerY == -1)
        firstDividerY = drawY;
      else if (secondDividerY == -1)
        secondDividerY = drawY;

      lastDividerY = drawY;
    }
  }

  // ✅ Ekstra çizgi (doğru hizalı)
  if (lastDividerY != -1 && firstDividerY != -1 && secondDividerY != -1)
  {
    Int rowHeight = secondDividerY - firstDividerY;

    TheDisplay->drawLine(
      lineLeft,
      lastDividerY + rowHeight,
      lineRight,
      lastDividerY + rowHeight,
      1,
      lineColor
    );
  }
}

static void LoadCampaignMaps(const char* campaignName)
{
  campaignMapItemData.clear();

  NameKeyType listID = TheNameKeyGenerator->nameToKey("ChaptersMenu.wnd:ListboxMap");
  GameWindow* listbox = TheWindowManager->winGetWindowFromId(nullptr, listID);

  if (!listbox)
    return;

  ListboxClear(listbox);

  std::ifstream file("Data\\INI\\Campaign.ini");
  if (!file.is_open())
    return;

  std::string line;
  bool inCorrectCampaign = false;
  Int row = 0;
  int depth = 0;

  while (std::getline(file, line))
  {
    // campaign start
    if (line.rfind("Campaign ", 0) == 0)
    {
      size_t pos = line.find("Campaign ");
      std::string foundName = line.substr(pos + 9);

      while (!foundName.empty() && isspace(foundName.back()))
        foundName.pop_back();

      while (!foundName.empty() && isspace(foundName.front()))
        foundName.erase(0, 1);

      if (foundName == campaignName)
      {
        inCorrectCampaign = true;
        depth = 0;
      }
      else
      {
        inCorrectCampaign = false;
      }

      continue;
    }

    if (!inCorrectCampaign)
      continue;

    // track blocks
    if (line.find("Mission") != std::string::npos)
      depth++;

    if (line.find("END") != std::string::npos)
    {
      if (depth > 0)
        depth--;        // mission end
      else
        break;          // campaign end ✅
    }

    // map parse
    size_t pos = line.find("Map ");
    if (pos != std::string::npos)
    {
      std::string mapPath = line.substr(pos + 4);

      while (!mapPath.empty() && isspace(mapPath.back()))
        mapPath.pop_back();

      UnicodeString utext;
      utext.translate(mapPath.c_str());

      GadgetListBoxAddEntryText(listbox, utext, 0xFFFFFFFF, row);

      campaignMapItemData.push_back(mapPath);
      GadgetListBoxSetItemData(listbox, (void*)campaignMapItemData.back().c_str(), row);

      if (row > 0)
      {
        ChapterProgress progress;

        AsciiString campaign;
        campaign.set(campaignName);
        campaign.toLower();

        Int previousMissionNumber = row;

        if (progress.getMissionCompletedDifficulty(campaign, previousMissionNumber) <= 0)
          GadgetListBoxSetItemEnabled(listbox, row, FALSE);
      }

      row++;
    }
  }

  if (row > 0)
  {
    Int rowToSelect = lastSelectedMapRow;

    if (rowToSelect < 0 || rowToSelect >= row)
      rowToSelect = 0;

    if (!GadgetListBoxIsItemEnabled(listbox, rowToSelect))
      rowToSelect = 0;

    GadgetListBoxSetSelected(listbox, rowToSelect);
    TheWindowManager->winSetFocus(listbox);

    ignoreNextMapSelectionForDoubleClick = TRUE;

    TheWindowManager->winSendSystemMsg(
      listbox,
      GLM_SELECTED,
      (WindowMsgData)listbox,
      rowToSelect
    );
  }

  file.close();
}

static void SelectChapter(Int index)
{
  if (selectedChapter != index)
    lastSelectedMapRow = 0;

  DEBUG_LOG(("SelectChapter(%d)\n", index));

  selectedChapter = index;

  for (Int i = 0; i < 8; ++i)
  {
    DEBUG_LOG(("Reset %d\n", i));

    if (!chapterButtons[i])
      continue;

    if (!chapterButtons[i]->winGetEnabled())
      continue;

    if (i == index)
    {
      chapterButtons[i]->winSetEnabledImage(0, selectedImages[i][0]);
      chapterButtons[i]->winSetEnabledImage(5, selectedImages[i][1]);
      chapterButtons[i]->winSetEnabledImage(6, selectedImages[i][2]);

      chapterButtons[i]->winSetHiliteImage(0, selectedImages[i][0]);
      chapterButtons[i]->winSetHiliteImage(5, selectedImages[i][1]);
      chapterButtons[i]->winSetHiliteImage(6, selectedImages[i][2]);

      chapterButtons[i]->winSetEnabledTextColors(
        selectedTextColor[i],
        selectedTextBorderColor[i]
      );

      chapterButtons[i]->winSetHiliteTextColors(
        selectedTextColor[i],
        selectedTextBorderColor[i]
      );
    }
    else
    {
      chapterButtons[i]->winSetEnabledImage(0, normalImages[i][0]);
      chapterButtons[i]->winSetEnabledImage(5, normalImages[i][1]);
      chapterButtons[i]->winSetEnabledImage(6, normalImages[i][2]);

      chapterButtons[i]->winSetHiliteImage(0, normalHiliteImages[i][0]);
      chapterButtons[i]->winSetHiliteImage(5, normalHiliteImages[i][1]);
      chapterButtons[i]->winSetHiliteImage(6, normalHiliteImages[i][2]);

      chapterButtons[i]->winSetEnabledTextColors(
        normalTextColor[i],
        normalTextBorderColor[i]
      );

      chapterButtons[i]->winSetHiliteTextColors(
        normalHiliteTextColor[i],
        normalHiliteTextBorderColor[i]
      );
    }
  }


	// Reborn: Fill map listbox with maps for the selected chapter
  if (index == 0) // TRAINING
  {
    TheCampaignManager->setCampaign("TRAINING");
    LoadCampaignMaps("TRAINING");
  }
  else if (index == 2) // ChinaGen
  {
    TheCampaignManager->setCampaign("China_Gen");
    LoadCampaignMaps("China_Gen");
  }


}

WindowMsgHandledType ChaptersMenuDividerInput(GameWindow* window, UnsignedInt msg, WindowMsgData mData1, WindowMsgData mData2)
{
  return MSG_IGNORED;
}

void ChaptersMenuInit(WindowLayout* layout, void* userData)
{

  missionLaunchPending = FALSE;
  startGame = FALSE;

  if (resetChapterImagesOnNextInit)
  {
		memset(chapterButtons, 0, sizeof(chapterButtons));
    chapterImagesCached = FALSE;
    resetChapterImagesOnNextInit = FALSE;
	}

  if (selectedChapter < 0)
  {
    memset(chapterButtons, 0, sizeof(chapterButtons));
  }

  TheShell->showShellMap(TRUE);

  radioEasyID = TheNameKeyGenerator->nameToKey("ChaptersMenu.wnd:RadioButtonEasyAI");
  radioMediumID = TheNameKeyGenerator->nameToKey("ChaptersMenu.wnd:RadioButtonMediumAI");
  radioHardID = TheNameKeyGenerator->nameToKey("ChaptersMenu.wnd:RadioButtonHardAI");
  buttonBackID = TheNameKeyGenerator->nameToKey("ChaptersMenu.wnd:ButtonBack");
  buttonStartID = TheNameKeyGenerator->nameToKey("ChaptersMenu.wnd:ButtonStart");
  buttonPushed = FALSE;

  isShuttingDown = FALSE;

  NameKeyType selectedRadioID = radioMediumID;

  if (selectedDifficulty == DIFFICULTY_EASY)
    selectedRadioID = radioEasyID;
  else if (selectedDifficulty == DIFFICULTY_HARD)
    selectedRadioID = radioHardID;

  GameWindow* radio = TheWindowManager->winGetWindowFromId(nullptr, selectedRadioID);
  if (radio)
    TheWindowManager->winSendSystemMsg(radio, GBM_SET_SELECTION, TRUE, 0);

  layout->hide(FALSE);
  layout->bringForward();

  GameWindow* subParent = TheWindowManager->winGetWindowFromId(
    nullptr,
    TheNameKeyGenerator->nameToKey("ChaptersMenu.wnd:SubParent")
  );

  if (subParent)
  {
    subParent->winHide(TRUE);
  }

  justEntered = TRUE;
  initialGadgetDelay = 2;

  //NameKeyType listID = TheNameKeyGenerator->nameToKey("ChaptersMenu.wnd:ListboxMap");
  //GameWindow* listbox = TheWindowManager->winGetWindowFromId(nullptr, listID);

  NameKeyType dividerID = TheNameKeyGenerator->nameToKey("ChaptersMenu.wnd:ListboxDividerOverlay");
  GameWindow* divider = TheWindowManager->winGetWindowFromId(nullptr, dividerID);

  if (divider)
  {
    divider->winEnable(FALSE);
    divider->winHide(FALSE);
    divider->winSetDrawFunc(DrawMapListboxDividers);
  }

  if (TheMapCache)
    TheMapCache->updateCache();

  for (Int i = 0; i < 8; ++i)
  {
    AsciiString name;
    name.format("ChaptersMenu.wnd:ButtonMapStartPosition%d", i);

    GameWindow* button = TheWindowManager->winGetWindowFromId(
      nullptr,
      TheNameKeyGenerator->nameToKey(name));

    if (button)
      button->winHide(TRUE);
  }

  const char* names[8] =
  {
      "ChaptersMenu.wnd:ButtonTRAINING",   // 0
      "ChaptersMenu.wnd:ButtonChallenge",  // 1
      "ChaptersMenu.wnd:ButtonChinaGen",   // 2
      "ChaptersMenu.wnd:ButtonChina",      // 3
      "ChaptersMenu.wnd:ButtonGLAGen",     // 4
      "ChaptersMenu.wnd:ButtonGLA",        // 5
      "ChaptersMenu.wnd:ButtonUSAGen",     // 6
      "ChaptersMenu.wnd:ButtonUSA"         // 7
  };


  Int chapterToSelect = selectedChapter;
  if (chapterToSelect < 0)
    chapterToSelect = 0;

  for (Int i = 0; i < 8; ++i)
  {
    chapterButtons[i] = TheWindowManager->winGetWindowFromId(
      nullptr,
      TheNameKeyGenerator->nameToKey(names[i]));

    if (chapterButtons[i])
    {
      if (!chapterImagesCached)
      {
        normalImages[i][0] = chapterButtons[i]->winGetEnabledImage(0);
        normalImages[i][1] = chapterButtons[i]->winGetEnabledImage(5);
        normalImages[i][2] = chapterButtons[i]->winGetEnabledImage(6);

        normalHiliteImages[i][0] = chapterButtons[i]->winGetHiliteImage(0);
        normalHiliteImages[i][1] = chapterButtons[i]->winGetHiliteImage(5);
        normalHiliteImages[i][2] = chapterButtons[i]->winGetHiliteImage(6);

        selectedImages[i][0] = chapterButtons[i]->winGetHiliteImage(1);
        selectedImages[i][1] = chapterButtons[i]->winGetHiliteImage(3);
        selectedImages[i][2] = chapterButtons[i]->winGetHiliteImage(4);

        normalTextColor[i] = chapterButtons[i]->winGetEnabledTextColor();
        normalTextBorderColor[i] = chapterButtons[i]->winGetEnabledTextBorderColor();

        normalHiliteTextColor[i] = chapterButtons[i]->winGetHiliteTextColor();
        normalHiliteTextBorderColor[i] = chapterButtons[i]->winGetHiliteTextBorderColor();

        selectedTextColor[i] = chapterButtons[i]->winGetHiliteTextColor();
        selectedTextBorderColor[i] = chapterButtons[i]->winGetHiliteTextBorderColor();
      }


      // Reborn: Only Training (0) and ChinaGen (2) will be active atm.
      if (i == 0 || i == 2)
        chapterButtons[i]->winEnable(TRUE);
      else
        chapterButtons[i]->winEnable(FALSE);

    }
  }
  chapterImagesCached = TRUE;


  SelectChapter(chapterToSelect);

  //TheTransitionHandler->setGroup("ChaptersMenuFade");
}

static void shutdownComplete(WindowLayout* layout)
{
  layout->hide(TRUE);
  TheShell->shutdownComplete(layout);
}

void ChaptersMenuShutdown(WindowLayout* layout, void* userData)
{

  Bool popImmediate = *(Bool*)userData;

  if (popImmediate)
  {
    shutdownComplete(layout);
    return;
  }

  isShuttingDown = TRUE;
  TheShell->reverseAnimatewindow();
  TheTransitionHandler->reverse("ChaptersMenuFade");
}

void ChaptersMenuUpdate(WindowLayout* layout, void* userData)
{
  if (justEntered)
  {
    if (initialGadgetDelay == 1)
    {
      GameWindow* subParent = TheWindowManager->winGetWindowFromId(
        nullptr,
        TheNameKeyGenerator->nameToKey("ChaptersMenu.wnd:SubParent")
      );

      if (subParent)
        subParent->winHide(FALSE);

      TheTransitionHandler->setGroup("ChaptersMenuFade");
      initialGadgetDelay = 2;
      justEntered = FALSE;
    }
    else
    {
      initialGadgetDelay--;
    }
  }

  if (startGame && TheShell->isAnimFinished() && TheTransitionHandler->isFinished())
  {
    startGame = FALSE;

    if (TheGameLogic->isInGame())
      TheGameLogic->clearGameData();

    GameMessage* msg = TheMessageStream->appendMessage(GameMessage::MSG_NEW_GAME);
    msg->appendIntegerArgument(GAME_SINGLE_PLAYER);
    msg->appendIntegerArgument(TheCampaignManager->getGameDifficulty());
    msg->appendIntegerArgument(TheCampaignManager->getRankPoints());
    InitRandom(0);

    TheShell->shutdownComplete(layout);
    return;
  }

  if (isShuttingDown && TheShell->isAnimFinished() && TheTransitionHandler->isFinished())
    shutdownComplete(layout);
}

static Bool LaunchSelectedChapterMission(GameWindow* listbox)
{
  if (!listbox)
    return FALSE;

  Int selected = -1;
  GadgetListBoxGetSelected(listbox, &selected);

  if (selected < 0)
    return FALSE;

  if (!GadgetListBoxIsItemEnabled(listbox, selected))
    return FALSE;

  const char* mapFname = (const char*)GadgetListBoxGetItemData(listbox, selected);

  if (!mapFname)
    return FALSE;

  AsciiString mapName = mapFname;
  mapName.toLower();

  StartChapterMission(mapName, selectedDifficulty);
  return TRUE;
}

WindowMsgHandledType ChaptersMenuSystem(GameWindow* window, UnsignedInt msg, WindowMsgData mData1, WindowMsgData mData2)
{
  DEBUG_LOG(("ChaptersMenuSystem: msg=%u\n", msg));

  if (missionLaunchPending)
    return MSG_HANDLED;

  switch (msg)
  {
  case GBM_SELECTED:
  {
    GameWindow* control = (GameWindow*)mData1;

    if (!control)
      break;

    for (Int i = 0; i < 8; ++i)
    {
      if (control == chapterButtons[i])
      {
        SelectChapter(i);
        return MSG_HANDLED;
      }
    }

    Int controlID = control->winGetWindowId();

    if (buttonPushed)
      break;

    if (controlID == buttonStartID)
    {
      NameKeyType listID = TheNameKeyGenerator->nameToKey("ChaptersMenu.wnd:ListboxMap");
      GameWindow* listbox = TheWindowManager->winGetWindowFromId(nullptr, listID);

      LaunchSelectedChapterMission(listbox);
      return MSG_HANDLED;
    }
    if (controlID == buttonBackID)
    {
      buttonPushed = TRUE;
			resetChapterImagesOnNextInit = TRUE;
      resetDifficultyOnNextInit = TRUE;
      TheShell->pop();
      return MSG_HANDLED;
    }
    if (controlID == radioEasyID)
    {
      selectedDifficulty = DIFFICULTY_EASY;
      return MSG_HANDLED;
    }
    else if (controlID == radioMediumID)
    {
      selectedDifficulty = DIFFICULTY_NORMAL;
      return MSG_HANDLED;
    }
    else if (controlID == radioHardID)
    {
      selectedDifficulty = DIFFICULTY_HARD;
      return MSG_HANDLED;
    }

    break;
  }

  case GLM_SELECTED:
  {
    GameWindow* control = (GameWindow*)mData1;
    if (!control)
      break;

    NameKeyType listID = TheNameKeyGenerator->nameToKey("ChaptersMenu.wnd:ListboxMap");

    if (control->winGetWindowId() == listID)
    {
      int rowSelected = mData2;

      if (rowSelected < 0)
      {
        GadgetListBoxSetSelected(control, lastSelectedMapRow);
        return MSG_HANDLED;
      }

      if (!GadgetListBoxIsItemEnabled(control, rowSelected))
      {
        GadgetListBoxSetSelected(control, lastSelectedMapRow);
        return MSG_HANDLED;
      }

      if (ignoreNextMapSelectionForDoubleClick)
      {
        ignoreNextMapSelectionForDoubleClick = FALSE;
      }

      lastMapClickTime = 0;
      lastMapClickRow = -1;

      lastSelectedMapRow = rowSelected;

      GameWindow* listbox = control;

      const char* mapFname = (const char*)GadgetListBoxGetItemData(listbox, rowSelected);

      if (!mapFname)
        break;

      std::string fixedPath = mapFname;

      for (size_t i = 0; i < fixedPath.size(); ++i)
      {
        if (fixedPath[i] == '\\')
          fixedPath[i] = '/';
      }

      for (size_t i = 0; i < fixedPath.size(); ++i)
      {
        fixedPath[i] = (char)tolower(fixedPath[i]);
      }

      AsciiString asciiMap = fixedPath.c_str();
      asciiMap.toLower();

      NameKeyType previewID = TheNameKeyGenerator->nameToKey("ChaptersMenu.wnd:MapWindow");
      GameWindow* preview = TheWindowManager->winGetWindowFromId(nullptr, previewID);

      if (!preview)
        break;

      preview->winSetStatus(WIN_STATUS_IMAGE);

      Image* image = getMapPreviewImage(asciiMap);
      preview->winSetUserData((void*)TheMapCache->findMap(asciiMap));

      if (image)
      {
        preview->winSetEnabledImage(0, image);
      }
      else
      {
        preview->winClearStatus(WIN_STATUS_IMAGE);
      }

    }

    break;
  }


  case GLM_DOUBLE_CLICKED:
  {
    DEBUG_LOG(("ChaptersMenuSystem: GLM_DOUBLE_CLICKED mData2=%d\n", (Int)mData2));

    GameWindow* listbox = (GameWindow*)mData1;

    if (!listbox)
      return MSG_HANDLED;

    NameKeyType listID = TheNameKeyGenerator->nameToKey("ChaptersMenu.wnd:ListboxMap");

    if (listbox->winGetWindowId() != listID)
      return MSG_HANDLED;

    Int selected = (Int)mData2;

    if (selected < 0)
      GadgetListBoxGetSelected(listbox, &selected);

    if (selected < 0)
      return MSG_HANDLED;

    if (!GadgetListBoxIsItemEnabled(listbox, selected))
      return MSG_HANDLED;

    const char* mapFname = (const char*)GadgetListBoxGetItemData(listbox, selected);

    if (!mapFname)
      return MSG_HANDLED;

    GadgetListBoxSetSelected(listbox, selected);

    AsciiString mapName = mapFname;
    mapName.toLower();

    StartChapterMission(mapName, selectedDifficulty);

    return MSG_HANDLED;
  }


  }

  return MSG_IGNORED;
}

WindowMsgHandledType ChaptersMenuInput(GameWindow* window, UnsignedInt msg, WindowMsgData mData1, WindowMsgData mData2)
{
  return MSG_IGNORED;
}
