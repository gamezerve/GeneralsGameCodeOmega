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

// FILE: RebornOmegaUpdater.cpp /////////////////////////////////////////////////////////////////////////////////
// Author: Gamezerve, August 2026
// Modified: -
// Desc:   Updater Checker for Reborn Omega
///////////////////////////////////////////////////////////////////////////////////////////////////

#include "GameClient/RebornOmegaUpdater.h"

#include <cstdlib>
#include <string>
#include <vector>
#include <windows.h>
#include <winhttp.h>

#include "Common/GlobalData.h"
#include "Common/UserPreferences.h"

static bool DownloadHttpsText(const std::string& url, std::string& content)
{
	content.clear();

	const std::string prefix = "https://";

	if (url.compare(0, prefix.size(), prefix) != 0)
		return false;

	size_t hostStart = prefix.size();
	size_t pathStart = url.find('/', hostStart);

	if (pathStart == std::string::npos)
		return false;

	std::string hostText = url.substr(hostStart, pathStart - hostStart);
	std::string pathText = url.substr(pathStart);

	std::wstring host(hostText.begin(), hostText.end());
	std::wstring path(pathText.begin(), pathText.end());
	
	HINTERNET session = WinHttpOpen(
		L"Reborn Omega Update Checker",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS,
		0);

	if (!session)
		return false;

	WinHttpSetTimeouts(session, 3000, 3000, 3000, 5000);

	HINTERNET connection = WinHttpConnect(
		session,
		host.c_str(),
		INTERNET_DEFAULT_HTTPS_PORT,
		0);

	if (!connection)
	{
		WinHttpCloseHandle(session);
		return false;
	}

	DWORD flags = WINHTTP_FLAG_SECURE;

	HINTERNET request = WinHttpOpenRequest(
		connection,
		L"GET",
		path.c_str(),
		nullptr,
		WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES,
		flags);

	bool success = false;

	if (request &&
		WinHttpSendRequest(
			request,
			WINHTTP_NO_ADDITIONAL_HEADERS,
			0,
			WINHTTP_NO_REQUEST_DATA,
			0,
			0,
			0) &&
		WinHttpReceiveResponse(request, nullptr))
	{
		DWORD statusCode = 0;
		DWORD statusCodeSize = sizeof(statusCode);

		if (WinHttpQueryHeaders(
			request,
			WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
			WINHTTP_HEADER_NAME_BY_INDEX,
			&statusCode,
			&statusCodeSize,
			WINHTTP_NO_HEADER_INDEX) &&
			statusCode >= 200 &&
			statusCode < 300)
		{
			DWORD available = 0;

			while (WinHttpQueryDataAvailable(request, &available) &&
				available > 0)
			{
				std::vector<char> buffer(available);
				DWORD bytesRead = 0;

				if (!WinHttpReadData(
					request,
					buffer.data(),
					available,
					&bytesRead))
				{
					content.clear();
					break;
				}

				content.append(buffer.data(), bytesRead);
			}

			success = !content.empty();
		}
	}

	if (request)
		WinHttpCloseHandle(request);

	WinHttpCloseHandle(connection);
	WinHttpCloseHandle(session);

	return success;
}

bool DownloadRebornOmegaUpdateFeed(std::string& feed)
{
	return DownloadHttpsText(
		"https://rss.moddb.com/mods/zero-hour-reborn-omega/downloads/feed/rss.xml",
		feed);
}

bool ResolveRebornOmegaMirrorUrl(
	const std::string& startUrl,
	std::string& mirrorUrl)
{
	mirrorUrl.clear();

	std::string html;

	if (!DownloadHttpsText(startUrl, html))
		return false;

	const std::string relativeMarker = "href=\"/downloads/mirror/";
	size_t urlStart = html.find(relativeMarker);

	if (urlStart != std::string::npos)
	{
		urlStart += 6;
		size_t urlEnd = html.find('"', urlStart);

		if (urlEnd == std::string::npos)
			return false;

		mirrorUrl =
			"https://www.moddb.com" +
			html.substr(urlStart, urlEnd - urlStart);

		return true;
	}

	const std::string absoluteMarker =
		"href=\"https://www.moddb.com/downloads/mirror/";

	urlStart = html.find(absoluteMarker);

	if (urlStart == std::string::npos)
		return false;

	urlStart += 6;

	size_t urlEnd = html.find('"', urlStart);

	if (urlEnd == std::string::npos)
		return false;

	mirrorUrl = html.substr(urlStart, urlEnd - urlStart);

	return true;
}

static bool ParseRebornOmegaVersion(
	const std::string& title,
	RebornOmegaVersionInfo& version)
{
	const std::string prefix = "Reborn Omega ";
	if (title.compare(0, prefix.size(), prefix) != 0)
		return false;

	size_t position = prefix.size();
	char* end = nullptr;

	long major = std::strtol(title.c_str() + position, &end, 10);
	if (!end || *end != '.')
		return false;

	position = static_cast<size_t>(end - title.c_str()) + 1;
	size_t versionEnd = title.find(' ', position);
	std::string digits = title.substr(position, versionEnd - position);

	if (digits.size() != 2 ||
		digits[0] < '0' || digits[0] > '9' ||
		digits[1] < '0' || digits[1] > '9')
	{
		return false;
	}

	int minor = digits[0] - '0';
	int patch = digits[1] - '0';
	int beta = 0;

	if (versionEnd != std::string::npos)
	{
		const std::string betaPrefix = " Beta ";
		if (title.compare(versionEnd, betaPrefix.size(), betaPrefix) != 0)
			return false;

		position = versionEnd + betaPrefix.size();
		long parsedBeta = std::strtol(title.c_str() + position, &end, 10);

		if (!end || *end != '\0' || parsedBeta <= 0)
			return false;

		beta = static_cast<int>(parsedBeta);
	}

	version.major = static_cast<int>(major);
	version.minor = minor;
	version.patch = patch;
	version.beta = beta;
	version.buildRank =
		version.major * 10000 +
		version.minor * 1000 +
		version.patch * 100 +
		(beta > 0 ? beta : 99);
	version.displayName = title;

	return true;
}

bool FindLatestRebornOmegaVersion(
	const std::string& feed,
	RebornOmegaVersionInfo& version)
{
	bool found = false;
	size_t itemStart = 0;

	while ((itemStart = feed.find("<item", itemStart)) != std::string::npos)
	{
		size_t itemEnd = feed.find("</item>", itemStart);
		if (itemEnd == std::string::npos)
			break;

		size_t titleStart = feed.find("<title>", itemStart);
		size_t titleEnd = feed.find("</title>", titleStart);
		size_t linkStart = feed.find("<link>", itemStart);
		size_t linkEnd = feed.find("</link>", linkStart);

		if (titleStart != std::string::npos &&
			titleEnd != std::string::npos &&
			titleEnd < itemEnd)
		{
			titleStart += 7;
			std::string title = feed.substr(titleStart, titleEnd - titleStart);

			RebornOmegaVersionInfo candidate;

			if (ParseRebornOmegaVersion(title, candidate) &&
				(!found || candidate.buildRank > version.buildRank))
			{
				if (linkStart != std::string::npos &&
					linkEnd != std::string::npos &&
					linkEnd < itemEnd)
				{
					linkStart += 6;
					candidate.pageUrl =
						feed.substr(linkStart, linkEnd - linkStart);
				}

				size_t guidStart = feed.find("<guid", itemStart);
				size_t guidEnd = feed.find("</guid>", guidStart);

				if (guidStart != std::string::npos &&
					guidEnd != std::string::npos &&
					guidEnd < itemEnd)
				{
					guidStart = feed.find('>', guidStart);

					if (guidStart != std::string::npos && guidStart < guidEnd)
					{
						++guidStart;
						std::string guid = feed.substr(guidStart, guidEnd - guidStart);
						const std::string guidPrefix = "downloads";

						if (guid.compare(0, guidPrefix.size(), guidPrefix) == 0)
						{
							candidate.downloadStartUrl =
								"https://www.moddb.com/downloads/start/" +
								guid.substr(guidPrefix.size());
						}
					}
				}

				version = candidate;
				found = true;
			}
		}

		itemStart = itemEnd + 7;
	}

	return found;
}

static volatile LONG s_updateCheckState = REBORN_UPDATE_IDLE;
static int s_installedBuildRank = 0;
static RebornOmegaVersionInfo s_updateCheckResult;
static volatile LONG s_updateCheckCanceled = FALSE;
static volatile LONG s_downloadPaused = FALSE;

static DWORD WINAPI RebornOmegaUpdateCheckThread(LPVOID)
{

	if (InterlockedCompareExchange(
		&s_updateCheckCanceled,
		FALSE,
		FALSE))
	{
		InterlockedExchange(
			&s_updateCheckState,
			REBORN_UPDATE_IDLE);
		return 0;
	}

	std::string feed;

	if (!DownloadRebornOmegaUpdateFeed(feed))
	{
		InterlockedExchange(
			&s_updateCheckState,
			REBORN_UPDATE_CONNECTION_FAILED);
		return 0;
	}

	RebornOmegaVersionInfo version;

	if (!FindLatestRebornOmegaVersion(feed, version))
	{
		InterlockedExchange(
			&s_updateCheckState,
			REBORN_UPDATE_DATA_INVALID);
		return 0;
	}

	if (InterlockedCompareExchange(
		&s_updateCheckCanceled,
		FALSE,
		FALSE))
	{
		InterlockedExchange(
			&s_updateCheckState,
			REBORN_UPDATE_IDLE);
		return 0;
	}

	s_updateCheckResult = version;

	InterlockedExchange(
		&s_updateCheckState,
		version.buildRank <= s_installedBuildRank
		? REBORN_UPDATE_UP_TO_DATE
		: REBORN_UPDATE_AVAILABLE);

	return 0;
}

bool StartRebornOmegaUpdateCheck(int installedBuildRank)
{
	if (InterlockedCompareExchange(
		&s_updateCheckState,
		REBORN_UPDATE_CHECKING,
		REBORN_UPDATE_IDLE) != REBORN_UPDATE_IDLE)
	{
		return false;
	}

	s_installedBuildRank = installedBuildRank;
	s_updateCheckResult = RebornOmegaVersionInfo();

	InterlockedExchange(&s_updateCheckCanceled, FALSE);

	HANDLE thread = CreateThread(
		nullptr,
		0,
		RebornOmegaUpdateCheckThread,
		nullptr,
		0,
		nullptr);

	if (!thread)
	{
		InterlockedExchange(
			&s_updateCheckState,
			REBORN_UPDATE_IDLE);
		return false;
	}

	CloseHandle(thread);
	return true;
}

RebornOmegaUpdateCheckState GetRebornOmegaUpdateCheckState()
{
	return static_cast<RebornOmegaUpdateCheckState>(
		InterlockedCompareExchange(
			&s_updateCheckState,
			REBORN_UPDATE_IDLE,
			REBORN_UPDATE_IDLE));
}

bool GetRebornOmegaUpdateCheckResult(RebornOmegaVersionInfo& version)
{
	RebornOmegaUpdateCheckState state =
		GetRebornOmegaUpdateCheckState();

	if (state != REBORN_UPDATE_UP_TO_DATE &&
		state != REBORN_UPDATE_AVAILABLE)
	{
		return false;
	}

	version = s_updateCheckResult;
	return true;
}

void FinishRebornOmegaUpdateCheck()
{
	InterlockedExchange(
		&s_updateCheckState,
		REBORN_UPDATE_IDLE);
}

void CancelRebornOmegaUpdateCheck()
{
	InterlockedExchange(
		&s_updateCheckCanceled,
		TRUE);
}

static volatile LONG s_downloadState = REBORN_DOWNLOAD_IDLE;
static volatile LONG s_downloadCanceled = FALSE;
static volatile LONG s_downloadedBytes = 0;
static volatile LONG s_totalBytes = 0;
static std::string s_downloadedInstallerPath;
static std::string s_downloadMirrorUrl;

static DWORD WINAPI RebornOmegaInstallerDownloadThread(
	LPVOID parameter)
{
	std::string* mirrorUrlParameter =
		static_cast<std::string*>(parameter);

	std::string mirrorUrl = *mirrorUrlParameter;
	delete mirrorUrlParameter;

	const std::string prefix = "https://";

	if (mirrorUrl.compare(
		0,
		prefix.size(),
		prefix) != 0)
	{
		InterlockedExchange(
			&s_downloadState,
			REBORN_DOWNLOAD_FAILED);
		return 0;
	}

	size_t hostStart = prefix.size();
	size_t pathStart =
		mirrorUrl.find('/', hostStart);

	if (pathStart == std::string::npos)
	{
		InterlockedExchange(
			&s_downloadState,
			REBORN_DOWNLOAD_FAILED);
		return 0;
	}

	std::string hostText =
		mirrorUrl.substr(
			hostStart,
			pathStart - hostStart);

	std::string pathText =
		mirrorUrl.substr(pathStart);

	std::wstring host(
		hostText.begin(),
		hostText.end());

	std::wstring path(
		pathText.begin(),
		pathText.end());

	char tempDirectory[MAX_PATH] = {};

	DWORD tempDirectoryLength =
		GetTempPathA(
			MAX_PATH,
			tempDirectory);

	if (tempDirectoryLength == 0 ||
		tempDirectoryLength >= MAX_PATH)
	{
		InterlockedExchange(
			&s_downloadState,
			REBORN_DOWNLOAD_FAILED);

		return 0;
	}

	std::string installerPath =
		std::string(tempDirectory) +
		"RebornOmegaUpdateSetup.exe";

	HINTERNET session = WinHttpOpen(
		L"Reborn Omega Updater",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS,
		0);

	HINTERNET connection = nullptr;
	HINTERNET request = nullptr;
	HANDLE file = INVALID_HANDLE_VALUE;
	bool success = false;

	if (session)
	{
		WinHttpSetTimeouts(
			session,
			5000,
			5000,
			10000,
			30000);

		connection = WinHttpConnect(
			session,
			host.c_str(),
			INTERNET_DEFAULT_HTTPS_PORT,
			0);
	}

	if (connection)
	{
		request = WinHttpOpenRequest(
			connection,
			L"GET",
			path.c_str(),
			nullptr,
			WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			WINHTTP_FLAG_SECURE);
	}

	if (request &&
		WinHttpSendRequest(
			request,
			WINHTTP_NO_ADDITIONAL_HEADERS,
			0,
			WINHTTP_NO_REQUEST_DATA,
			0,
			0,
			0) &&
		WinHttpReceiveResponse(
			request,
			nullptr))
	{
		DWORD statusCode = 0;
		DWORD statusCodeSize =
			sizeof(statusCode);

		if (WinHttpQueryHeaders(
			request,
			WINHTTP_QUERY_STATUS_CODE |
			WINHTTP_QUERY_FLAG_NUMBER,
			WINHTTP_HEADER_NAME_BY_INDEX,
			&statusCode,
			&statusCodeSize,
			WINHTTP_NO_HEADER_INDEX) &&
			statusCode >= 200 &&
			statusCode < 300)
		{
			file = CreateFileA(
				installerPath.c_str(),
				GENERIC_WRITE,
				0,
				nullptr,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				nullptr);

			if (file != INVALID_HANDLE_VALUE)
			{
				DWORD totalBytes = 0;
				DWORD totalBytesSize =
					sizeof(totalBytes);

				if (WinHttpQueryHeaders(
					request,
					WINHTTP_QUERY_CONTENT_LENGTH |
					WINHTTP_QUERY_FLAG_NUMBER,
					WINHTTP_HEADER_NAME_BY_INDEX,
					&totalBytes,
					&totalBytesSize,
					WINHTTP_NO_HEADER_INDEX))
				{
					InterlockedExchange(
						&s_totalBytes,
						static_cast<LONG>(
							totalBytes));
				}

				InterlockedExchange(
					&s_downloadState,
					REBORN_DOWNLOAD_DOWNLOADING);

				char buffer[65536];
				DWORD bytesRead = 0;
				LONG downloadedBytes = 0;

				success = true;

				do
				{
					if (InterlockedCompareExchange(
						&s_downloadPaused,
						FALSE,
						FALSE))
					{

						DEBUG_LOG((
							"Reborn updater: Worker entered paused state\n"));
						InterlockedExchange(
							&s_downloadState,
							REBORN_DOWNLOAD_PAUSED);

						while (InterlockedCompareExchange(
							&s_downloadPaused,
							FALSE,
							FALSE) &&
							!InterlockedCompareExchange(
								&s_downloadCanceled,
								FALSE,
								FALSE))
						{
							Sleep(50);
						}

						DEBUG_LOG((
							"Reborn updater: Worker left paused state paused=%ld canceled=%ld\n",
							InterlockedCompareExchange(
								&s_downloadPaused,
								FALSE,
								FALSE),
							InterlockedCompareExchange(
								&s_downloadCanceled,
								FALSE,
								FALSE)));

						if (InterlockedCompareExchange(
							&s_downloadCanceled,
							FALSE,
							FALSE))
						{
							success = false;
							break;
						}

						InterlockedExchange(
							&s_downloadState,
							REBORN_DOWNLOAD_DOWNLOADING);
					}

					if (InterlockedCompareExchange(
						&s_downloadCanceled,
						FALSE,
						FALSE))
					{
						success = false;
						break;
					}

					DEBUG_LOG((
						"Reborn updater: entering WinHttpReadData downloaded=%ld\n",
						downloadedBytes));

					if (!WinHttpReadData(
						request,
						buffer,
						sizeof(buffer),
						&bytesRead))
					{
						DWORD errorCode =
							GetLastError();

						DEBUG_LOG((
							"Reborn updater: WinHttpReadData failed error=%lu\n",
							errorCode));

						success = false;
						break;
					}

					DEBUG_LOG((
						"Reborn updater: WinHttpReadData returned bytes=%lu\n",
						bytesRead));

					if (bytesRead > 0)
					{
						DWORD bytesWritten = 0;

						if (!WriteFile(
							file,
							buffer,
							bytesRead,
							&bytesWritten,
							nullptr) ||
							bytesWritten != bytesRead)
						{
							success = false;
							break;
						}

						downloadedBytes +=
							static_cast<LONG>(
								bytesRead);

						InterlockedExchange(
							&s_downloadedBytes,
							downloadedBytes);
					}
				} while (bytesRead > 0);

				if (success &&
					totalBytes > 0 &&
					downloadedBytes != static_cast<LONG>(totalBytes))
				{
					DEBUG_LOG((
						"Reborn updater: Incomplete download downloaded=%ld total=%lu\n",
						downloadedBytes,
						totalBytes));

					success = false;
				}
			}
		}
	}

	if (file != INVALID_HANDLE_VALUE)
		CloseHandle(file);

	if (request)
		WinHttpCloseHandle(request);

	if (connection)
		WinHttpCloseHandle(connection);

	if (session)
		WinHttpCloseHandle(session);

	if (InterlockedCompareExchange(
		&s_downloadCanceled,
		FALSE,
		FALSE))
	{
		DeleteFileA(
			installerPath.c_str());

		InterlockedExchange(
			&s_downloadState,
			REBORN_DOWNLOAD_CANCELED);

		return 0;
	}

	if (!success)
	{
		DeleteFileA(
			installerPath.c_str());

		InterlockedExchange(
			&s_downloadState,
			REBORN_DOWNLOAD_FAILED);

		return 0;
	}

	s_downloadedInstallerPath =
		installerPath;

	InterlockedExchange(
		&s_downloadState,
		REBORN_DOWNLOAD_COMPLETED);

	return 0;
}

bool StartRebornOmegaInstallerDownload(
	const std::string& mirrorUrl)
{

	LONG currentState = InterlockedCompareExchange(
		&s_downloadState,
		REBORN_DOWNLOAD_IDLE,
		REBORN_DOWNLOAD_IDLE);

	if (currentState == REBORN_DOWNLOAD_CANCELED ||
		currentState == REBORN_DOWNLOAD_FAILED ||
		currentState == REBORN_DOWNLOAD_COMPLETED)
	{
		FinishRebornOmegaInstallerDownload();
	}

	if (InterlockedCompareExchange(
		&s_downloadState,
		REBORN_DOWNLOAD_STARTING,
		REBORN_DOWNLOAD_IDLE) != REBORN_DOWNLOAD_IDLE)
	{
		return false;
	}

	s_downloadMirrorUrl = mirrorUrl;

	InterlockedExchange(&s_downloadCanceled, FALSE);
	InterlockedExchange(&s_downloadPaused, FALSE);
	InterlockedExchange(&s_downloadedBytes, 0);
	InterlockedExchange(&s_totalBytes, 0);
	s_downloadedInstallerPath.clear();

	std::string* threadUrl =
		new std::string(mirrorUrl);

	HANDLE thread = CreateThread(
		nullptr,
		0,
		RebornOmegaInstallerDownloadThread,
		threadUrl,
		0,
		nullptr);

	if (!thread)
	{
		delete threadUrl;

		InterlockedExchange(
			&s_downloadState,
			REBORN_DOWNLOAD_IDLE);

		return false;
	}

	CloseHandle(thread);
	return true;
}

void CancelRebornOmegaInstallerDownload()
{
	InterlockedExchange(
		&s_downloadCanceled,
		TRUE);
}

RebornOmegaDownloadState GetRebornOmegaDownloadState()
{
	return static_cast<RebornOmegaDownloadState>(
		InterlockedCompareExchange(
			&s_downloadState,
			REBORN_DOWNLOAD_IDLE,
			REBORN_DOWNLOAD_IDLE));
}

RebornOmegaDownloadProgress
GetRebornOmegaDownloadProgress()
{
	RebornOmegaDownloadProgress progress;

	progress.downloadedBytes =
		static_cast<unsigned long>(
			InterlockedCompareExchange(
				&s_downloadedBytes,
				0,
				0));

	progress.totalBytes =
		static_cast<unsigned long>(
			InterlockedCompareExchange(
				&s_totalBytes,
				0,
				0));

	progress.percent =
		progress.totalBytes > 0
		? static_cast<int>(
			static_cast<unsigned long long>(
				progress.downloadedBytes) *
			100 /
			progress.totalBytes)
		: 0;

	return progress;
}

const std::string&
GetRebornOmegaDownloadedInstallerPath()
{
	return s_downloadedInstallerPath;
}

void FinishRebornOmegaInstallerDownload()
{
	InterlockedExchange(
		&s_downloadState,
		REBORN_DOWNLOAD_IDLE);
}

void PauseRebornOmegaInstallerDownload()
{
	if (InterlockedCompareExchange(
			&s_downloadState,
			REBORN_DOWNLOAD_DOWNLOADING,
			REBORN_DOWNLOAD_DOWNLOADING) !=
		REBORN_DOWNLOAD_DOWNLOADING)
	{
		return;
	}

	InterlockedExchange(
		&s_downloadPaused,
		TRUE);
}

void ResumeRebornOmegaInstallerDownload()
{
	LONG state = InterlockedCompareExchange(
		&s_downloadState,
		REBORN_DOWNLOAD_PAUSED,
		REBORN_DOWNLOAD_PAUSED);

	LONG paused = InterlockedCompareExchange(
		&s_downloadPaused,
		FALSE,
		FALSE);

	DEBUG_LOG((
		"Reborn updater: Resume requested state=%ld paused=%ld\n",
		state,
		paused));

	if (state != REBORN_DOWNLOAD_PAUSED)
		return;

	InterlockedExchange(
		&s_downloadPaused,
		FALSE);

	DEBUG_LOG((
		"Reborn updater: Resume flag cleared\n"));
}

bool RetryRebornOmegaInstallerDownload()
{
	if (InterlockedCompareExchange(
		&s_downloadState,
		REBORN_DOWNLOAD_FAILED,
		REBORN_DOWNLOAD_FAILED) !=
		REBORN_DOWNLOAD_FAILED)
	{
		return false;
	}

	std::string mirrorUrl =
		s_downloadMirrorUrl;

	if (mirrorUrl.empty())
		return false;

	FinishRebornOmegaInstallerDownload();

	return StartRebornOmegaInstallerDownload(
		mirrorUrl);
}

Bool GetRebornOmegaAutomaticUpdateChecksEnabled()
{
	UserPreferences preferences;

	if (!preferences.load(
		"RebornOmegaOptions\\RebornOmegaOptions.ini"))
	{
		return TRUE;
	}

	AsciiString value =
		preferences["AutomaticUpdateChecks"];

	if (value.isEmpty())
		return TRUE;

	return stricmp(value.str(), "no") != 0;
}

Bool SetRebornOmegaAutomaticUpdateChecksEnabled(
	Bool enabled)
{
	AsciiString directory =
		TheGlobalData->getPath_UserData();

	directory.concat("RebornOmegaOptions");

	CreateDirectoryA(
		directory.str(),
		nullptr);

	UserPreferences preferences;

	preferences.load(
		"RebornOmegaOptions\\RebornOmegaOptions.ini");

	preferences["AutomaticUpdateChecks"] =
		enabled ? "yes" : "no";

	return preferences.write();
}
