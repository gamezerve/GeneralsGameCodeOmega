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
#include "GameClient/Gadget.h"
#include "GameClient/GameWindow.h"
#include "GameClient/GameWindowManager.h"
#include "GameClient/GameWindowTransitions.h"
#include "GameClient/Shell.h"
#include "GameClient/WindowLayout.h"

static NameKeyType buttonBackID = NAMEKEY_INVALID;
static Bool buttonPushed = FALSE;

void ChaptersMenuInit(WindowLayout* layout, void* userData)
{
  buttonBackID = TheNameKeyGenerator->nameToKey("ChaptersMenu.wnd:ButtonBack");
  buttonPushed = FALSE;

  layout->hide(FALSE);
  layout->bringForward();

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
  }

  return MSG_IGNORED;
}

WindowMsgHandledType ChaptersMenuInput(GameWindow* window, UnsignedInt msg, WindowMsgData mData1, WindowMsgData mData2)
{
  return MSG_IGNORED;
}
