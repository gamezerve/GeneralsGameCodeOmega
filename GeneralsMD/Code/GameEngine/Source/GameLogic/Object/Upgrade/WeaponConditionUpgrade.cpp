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

// FILE: WeaponConditionUpgrade.cpp /////////////////////////////////////////////////////////////////////////////
// Author: Gamezerve
// Desc:	 
///////////////////////////////////////////////////////////////////////////////////////////////////

// INCLUDES ///////////////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/Xfer.h"
#include "GameLogic/Object.h"
#include "GameLogic/Module/WeaponConditionUpgrade.h"

WeaponConditionUpgradeModuleData::WeaponConditionUpgradeModuleData()
{
}

void WeaponConditionUpgradeModuleData::buildFieldParse(MultiIniFieldParse& p)
{
	UpgradeModuleData::buildFieldParse(p);

	static const FieldParse dataFieldParse[] =
	{
		{ "AddConditionFlags", WeaponSetFlags::parseFromINI, nullptr, offsetof(WeaponConditionUpgradeModuleData, m_addConditionFlags) },
		{ "RemoveConditionFlags", WeaponSetFlags::parseFromINI, nullptr, offsetof(WeaponConditionUpgradeModuleData, m_removeConditionFlags) },
		{ nullptr, nullptr, nullptr, 0 }
	};

	p.add(dataFieldParse);
}

WeaponConditionUpgrade::WeaponConditionUpgrade(Thing* thing, const ModuleData* moduleData) : UpgradeModule(thing, moduleData)
{
}

WeaponConditionUpgrade::~WeaponConditionUpgrade()
{
}

void WeaponConditionUpgrade::upgradeImplementation()
{
	const WeaponConditionUpgradeModuleData* data = static_cast<const WeaponConditionUpgradeModuleData*>(getModuleData());
	Object* obj = getObject();

	for (Int i = 0; i < WEAPONSET_COUNT; ++i)
	{
		WeaponSetType flag = static_cast<WeaponSetType>(i);

		if (data->m_removeConditionFlags.test(flag))
			obj->clearWeaponSetFlag(flag);
	}

	for (Int i = 0; i < WEAPONSET_COUNT; ++i)
	{
		WeaponSetType flag = static_cast<WeaponSetType>(i);

		if (data->m_addConditionFlags.test(flag))
			obj->setWeaponSetFlag(flag);
	}
}

void WeaponConditionUpgrade::crc(Xfer* xfer)
{
	UpgradeModule::crc(xfer);
}

void WeaponConditionUpgrade::xfer(Xfer* xfer)
{
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion(&version, currentVersion);

	UpgradeModule::xfer(xfer);
}

void WeaponConditionUpgrade::loadPostProcess()
{
	UpgradeModule::loadPostProcess();
}
