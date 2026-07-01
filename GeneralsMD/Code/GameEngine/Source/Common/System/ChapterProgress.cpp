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
// FILE: ChapterProgress.cpp 
// Author: Gamezerve 7/1/2026
// Description: 
///////////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/ChapterProgress.h"
#include "Common/GlobalData.h"
#include "GameClient/ClientInstance.h"

ChapterProgress* TheChapterProgress = nullptr;

ChapterProgress::ChapterProgress()
{
	loadFromIniFile();
}

Bool ChapterProgress::loadFromIniFile()
{
	AsciiString dir = TheGlobalData->getPath_UserData();
	dir.concat("RebornOmegaStats\\");
	CreateDirectory(dir.str(), nullptr);

	if (rts::ClientInstance::getInstanceId() > 1u)
	{
		AsciiString fname;
		fname.format("RebornOmegaStats\\RebornOmegaCampaignProgress_Instance%.2u.ini", rts::ClientInstance::getInstanceId());
		return load(fname);
	}

	return load("RebornOmegaStats\\RebornOmegaCampaignProgress.ini");
}

static Int DifficultyToProgressValue(GameDifficulty difficulty)
{
	if (difficulty == DIFFICULTY_EASY)
		return 1;

	if (difficulty == DIFFICULTY_HARD)
		return 3;

	return 2;
}

void ChapterProgress::setMissionComplete(AsciiString campaignName, Int missionIndex, GameDifficulty difficulty)
{
	AsciiString key;
	campaignName.toLower();
	key.format("%s_%d", campaignName.str(), missionIndex);

	Int oldValue = getInt(key, 0);
	Int newValue = DifficultyToProgressValue(difficulty);

	if (newValue > oldValue)
	{
		setInt(key, newValue);
		write();
	}
}

Int ChapterProgress::getMissionCompletedDifficulty(AsciiString campaignName, Int missionIndex) const
{
	AsciiString key;
	campaignName.toLower();
	key.format("%s_%d", campaignName.str(), missionIndex);
	return getInt(key, 0);
}

Bool ChapterProgress::isMissionComplete(AsciiString campaignName, Int missionIndex) const
{
	return getMissionCompletedDifficulty(campaignName, missionIndex) > 0;
}
