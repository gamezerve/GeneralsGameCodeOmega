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

// FILE: MaxHealthUpgrade.cpp /////////////////////////////////////////////////////////////////////////////
// Author: Kris Morness, September 2002
// Desc:	 UpgradeModule that adds a scalar to the object's experience gain.
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#define DEFINE_MAXHEALTHCHANGETYPE_NAMES
#include "Common/Xfer.h"
#include "GameLogic/Object.h"
#include "GameLogic/ExperienceTracker.h"
#include "GameLogic/Module/MaxHealthUpgrade.h"
#include "GameLogic/Module/BodyModule.h"

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
MaxHealthUpgradeModuleData::MaxHealthUpgradeModuleData()
{
	m_addMaxHealth = 0.0f;
	m_maxHealthChangeType = SAME_CURRENTHEALTH;
	m_tooltipTriggerUpgradeName.clear();
}
static void parseMaxHealthTriggeredBy(INI* ini, void* instance, void* store, const void* /*userData*/)
{
	MaxHealthUpgradeModuleData* data = static_cast<MaxHealthUpgradeModuleData*>(store);

	INI::parseAsciiStringVector(
		ini,
		instance,
		&data->m_upgradeMuxData.m_activationUpgradeNames,
		nullptr);

	if (data->m_tooltipTriggerUpgradeName.isEmpty() && !data->m_upgradeMuxData.m_activationUpgradeNames.empty())
	{
		data->m_tooltipTriggerUpgradeName = data->m_upgradeMuxData.m_activationUpgradeNames[0];
	}
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void MaxHealthUpgradeModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	ModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "TriggeredBy", parseMaxHealthTriggeredBy, nullptr, 0 },
		{ "ConflictsWith", INI::parseAsciiStringVector, nullptr, offsetof(MaxHealthUpgradeModuleData, m_upgradeMuxData.m_conflictingUpgradeNames) },
		{ "RemovesUpgrades", INI::parseAsciiStringVector, nullptr, offsetof(MaxHealthUpgradeModuleData, m_upgradeMuxData.m_removalUpgradeNames) },
		{ "FXListUpgrade", INI::parseFXList, nullptr, offsetof(MaxHealthUpgradeModuleData, m_upgradeMuxData.m_fxListUpgrade) },
		{ "RequiresAllTriggers", INI::parseBool, nullptr, offsetof(MaxHealthUpgradeModuleData, m_upgradeMuxData.m_requiresAllTriggers) },
		{ "AddMaxHealth", INI::parseReal, nullptr, offsetof(MaxHealthUpgradeModuleData, m_addMaxHealth) },
		{ "ChangeType", INI::parseIndexList, TheMaxHealthChangeTypeNames, offsetof(MaxHealthUpgradeModuleData, m_maxHealthChangeType) },
		{ nullptr, nullptr, nullptr, 0 }
	};

	p.add(dataFieldParse);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
MaxHealthUpgrade::MaxHealthUpgrade( Thing *thing, const ModuleData* moduleData ) : UpgradeModule( thing, moduleData )
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
MaxHealthUpgrade::~MaxHealthUpgrade()
{
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void MaxHealthUpgrade::upgradeImplementation()
{
	const MaxHealthUpgradeModuleData *data = getMaxHealthUpgradeModuleData();

	//Simply add the xp scalar to the xp tracker!
	Object *obj = getObject();

	BodyModuleInterface *body = obj->getBodyModule();

	//if( body )
	//{
	//	body->setMaxHealth( body->getMaxHealth() + data->m_addMaxHealth, data->m_maxHealthChangeType );
	//}
	
	// Reborn: MaxHealthUpgrade applies a flat health bonus, while difficulty and veterancy
	// bonuses are applied multiplicatively.
	//
	// Without temporarily removing the active health multipliers, researching a
	// health upgrade after an object has already received its difficulty and/or
	// veterancy bonuses produces a different result than producing a new object
	// after the upgrade is researched.
	//
	// Existing object:   (Base * Multipliers) + Upgrade
	// New object:        (Base + Upgrade) * Multipliers
	//
	// To keep health values consistent regardless of upgrade order, temporarily
	// remove the active health multipliers, apply the flat health increase to the
	// base value, then reapply the multipliers.
	//
	// As a result, health values now always follow:
	//
	// (BaseHealth + UpgradeBonus) * DifficultyMultiplier * VeterancyMultiplier
	//
	// Compared to the previous behavior, existing units will now gain more health
	// from MaxHealthUpgrade on health multipliers greater than 1.0 (e.g. Easy),
	// and less health on health multipliers below 1.0 (e.g. Hard). Existing and
	// newly produced units will always end up with identical maximum health.
	if (body)
	{
		Bool wasReceivingDifficultyBonus = obj->isReceivingDifficultyBonus();

		if (wasReceivingDifficultyBonus)
			obj->setReceivingDifficultyBonus(FALSE);

		ExperienceTracker* experienceTracker = obj->getExperienceTracker();
		VeterancyLevel veterancyLevel = experienceTracker ? experienceTracker->getVeterancyLevel() : LEVEL_REGULAR;
		Real veterancyBonus = TheGlobalData->m_healthBonus[veterancyLevel];

		if (veterancyBonus != 1.0f)
			body->setMaxHealth(body->getMaxHealth() / veterancyBonus, PRESERVE_RATIO);

		body->setMaxHealth(
			body->getMaxHealth() + data->m_addMaxHealth,
			data->m_maxHealthChangeType);

		if (veterancyBonus != 1.0f)
			body->setMaxHealth(body->getMaxHealth() * veterancyBonus, PRESERVE_RATIO);

		if (wasReceivingDifficultyBonus)
			obj->setReceivingDifficultyBonus(TRUE);
	}
}

// ------------------------------------------------------------------------------------------------
/** CRC */
// ------------------------------------------------------------------------------------------------
void MaxHealthUpgrade::crc( Xfer *xfer )
{

	// extend base class
	UpgradeModule::crc( xfer );

}

// ------------------------------------------------------------------------------------------------
/** Xfer method
	* Version Info:
	* 1: Initial version */
// ------------------------------------------------------------------------------------------------
void MaxHealthUpgrade::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// extend base class
	UpgradeModule::xfer( xfer );

}


// ------------------------------------------------------------------------------------------------
/** Load post process */
// ------------------------------------------------------------------------------------------------
void MaxHealthUpgrade::loadPostProcess()
{

	// extend base class
	UpgradeModule::loadPostProcess();

}
