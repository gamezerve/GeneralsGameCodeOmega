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

// FILE: RebornOmegaUpdater.h /////////////////////////////////////////////////////////////////////////////////
// Author: Gamezerve, August 2026
// Modified: -
// Desc:   Updater Checker for Reborn Omega
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#include <vector>

struct RebornOmegaVersionInfo
{
	int major;
	int minor;
	int patch;
	int beta;
	int buildRank;
	std::string displayName;
	std::string pageUrl;
	std::string downloadStartUrl;
	std::string changeLog;
};

// Update check functions for Reborn Omega

enum RebornOmegaUpdateCheckState
{
	REBORN_UPDATE_IDLE,
	REBORN_UPDATE_CHECKING,
	REBORN_UPDATE_CONNECTION_FAILED,
	REBORN_UPDATE_DATA_INVALID,
	REBORN_UPDATE_UP_TO_DATE,
	REBORN_UPDATE_AVAILABLE
};

bool DownloadRebornOmegaUpdateFeed(std::string& feed);
bool DownloadRebornOmegaChangeLog(const std::string& pageUrl, std::string& changeLog);
bool FindLatestRebornOmegaVersion(const std::string& feed, RebornOmegaVersionInfo& version);
bool FindAllRebornOmegaVersions(const std::string& feed, std::vector<RebornOmegaVersionInfo>& versions);
bool ResolveRebornOmegaMirrorUrl(const std::string& startUrl,	std::string& mirrorUrl);

bool StartRebornOmegaUpdateCheck(int installedBuildRank);
RebornOmegaUpdateCheckState GetRebornOmegaUpdateCheckState();
bool GetRebornOmegaUpdateCheckResult(RebornOmegaVersionInfo& version);
bool GetRebornOmegaVersionList(std::vector<RebornOmegaVersionInfo>& versions);
void FinishRebornOmegaUpdateCheck();

void CancelRebornOmegaUpdateCheck();

// Download functions for Reborn Omega installer

enum RebornOmegaDownloadState
{
	REBORN_DOWNLOAD_IDLE,
	REBORN_DOWNLOAD_STARTING,
	REBORN_DOWNLOAD_DOWNLOADING,
	REBORN_DOWNLOAD_PAUSED,
	REBORN_DOWNLOAD_COMPLETED,
	REBORN_DOWNLOAD_FAILED,
	REBORN_DOWNLOAD_CANCELED
};

struct RebornOmegaDownloadProgress
{
	int percent;
	unsigned long downloadedBytes;
	unsigned long totalBytes;
};

bool StartRebornOmegaInstallerDownload(const std::string& mirrorUrl);

void CancelRebornOmegaInstallerDownload();

RebornOmegaDownloadState GetRebornOmegaDownloadState();

RebornOmegaDownloadProgress
GetRebornOmegaDownloadProgress();

const std::string&
GetRebornOmegaDownloadedInstallerPath();

const std::string&
GetRebornOmegaInstallerFileName();

void SetRebornOmegaDownloadDisplayName(
	const std::string& displayName);

const std::string&
GetRebornOmegaDownloadDisplayName();

void FinishRebornOmegaInstallerDownload();

void PauseRebornOmegaInstallerDownload();
void ResumeRebornOmegaInstallerDownload();
bool RetryRebornOmegaInstallerDownload();

Bool GetRebornOmegaAutomaticUpdateChecksEnabled();
Bool SetRebornOmegaAutomaticUpdateChecksEnabled(Bool enabled);

// Change Log Menu functions for Reborn Omega

enum RebornOmegaChangeLogDownloadState
{
	REBORN_CHANGELOG_IDLE,
	REBORN_CHANGELOG_DOWNLOADING,
	REBORN_CHANGELOG_COMPLETED,
	REBORN_CHANGELOG_FAILED
};

bool StartRebornOmegaChangeLogDownload(
	const std::string& pageUrl);

RebornOmegaChangeLogDownloadState
GetRebornOmegaChangeLogDownloadState();

bool GetRebornOmegaChangeLogDownloadResult(
	std::string& changeLog);

void FinishRebornOmegaChangeLogDownload();

void CacheRebornOmegaVersionChangeLog(
	int buildRank,
	const std::string& changeLog);
