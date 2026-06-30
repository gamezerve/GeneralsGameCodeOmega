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

#include "Common/NameKeyGenerator.h"
#include "GameClient/Display.h"
#include "GameClient/Gadget.h"
#include "GameClient/GadgetListBox.h"
#include "GameClient/GameWindow.h"
#include "GameClient/GameWindowManager.h"
#include "GameClient/GameWindowTransitions.h"
#include "GameClient/MapUtil.h"
#include "GameClient/Shell.h"
#include "GameClient/WindowLayout.h"

#include <fstream>
#include <string>
#include <vector>
#include <cctype>

static NameKeyType buttonBackID = NAMEKEY_INVALID;
static Bool buttonPushed = FALSE;
static GameWindow* chapterButtons[8];
static Int selectedChapter = 0;

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

      row++;
    }
  }

  if (row > 0)
  {
    GadgetListBoxSetSelected(listbox, 0);

    TheWindowManager->winSendSystemMsg(
      listbox,
      GLM_SELECTED,
      (WindowMsgData)listbox,
      0
    );
  }

  file.close();
}

static void SelectChapter(Int index)
{

  selectedChapter = index;

  for (Int i = 0; i < 8; ++i)
  {
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
    LoadCampaignMaps("TRAINING");
  }
  else if (index == 2) // ChinaGen
  {
    LoadCampaignMaps("China_Gen");
  }


}

WindowMsgHandledType ChaptersMenuDividerInput(GameWindow* window, UnsignedInt msg, WindowMsgData mData1, WindowMsgData mData2)
{
  return MSG_IGNORED;
}

void ChaptersMenuInit(WindowLayout* layout, void* userData)
{
  buttonBackID = TheNameKeyGenerator->nameToKey("ChaptersMenu.wnd:ButtonBack");
  buttonPushed = FALSE;

  layout->hide(FALSE);
  layout->bringForward();

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


  for (Int i = 0; i < 8; ++i)
  {
    chapterButtons[i] = TheWindowManager->winGetWindowFromId(
      nullptr,
      TheNameKeyGenerator->nameToKey(names[i]));

    if (chapterButtons[i])
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


      // Reborn: Only Training (0) and ChinaGen (2) will be active atm.
      if (i == 0 || i == 2)
        chapterButtons[i]->winEnable(TRUE);
      else
        chapterButtons[i]->winEnable(FALSE);

    }
  }

  SelectChapter(0);

  LoadCampaignMaps("TRAINING");

  TheTransitionHandler->setGroup("ChaptersMenuFade");
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

  TheShell->reverseAnimatewindow();
  TheTransitionHandler->reverse("ChaptersMenuFade");
}

void ChaptersMenuUpdate(WindowLayout* layout, void* userData)
{
  if (TheShell->isAnimFinished() && TheTransitionHandler->isFinished())
    TheShell->shutdownComplete(layout);
}

WindowMsgHandledType ChaptersMenuSystem(GameWindow* window, UnsignedInt msg, WindowMsgData mData1, WindowMsgData mData2)
{
  DEBUG_LOG(("ChaptersMenuSystem: msg=%u\n", msg));
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

    if (controlID == buttonBackID)
    {
      buttonPushed = TRUE;
      TheShell->pop();
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

  }

  return MSG_IGNORED;
}

WindowMsgHandledType ChaptersMenuInput(GameWindow* window, UnsignedInt msg, WindowMsgData mData1, WindowMsgData mData2)
{
  return MSG_IGNORED;
}
