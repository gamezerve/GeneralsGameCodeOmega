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
// FILE: ChapterProgress.h 
// Author: Gamezerve 7/1/2026
// Description: 
///////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Common/UserPreferences.h"
#include "Common/GameType.h"

class ChapterProgress : public UserPreferences
{
public:
	ChapterProgress();

	Bool loadFromIniFile();

	void setMissionComplete(AsciiString campaignName, Int missionIndex, GameDifficulty difficulty);
	Int getMissionCompletedDifficulty(AsciiString campaignName, Int missionIndex) const;
	Bool isMissionComplete(AsciiString campaignName, Int missionIndex) const;
};

extern ChapterProgress* TheChapterProgress;
