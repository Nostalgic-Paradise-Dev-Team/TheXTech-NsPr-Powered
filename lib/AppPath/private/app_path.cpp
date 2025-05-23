/*
 * TheXTech - A platform game engine ported from old source code for VB6
 *
 * Copyright (c) 2009-2011 Andrew Spinks, original VB6 code
 * Copyright (c) 2020-2025 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <DirManager/dirman.h>
#include <Utils/files.h>
#include <IniProcessor/ini_processing.h>
#define FMT_NOEXCEPT
#include <fmt/fmt_format.h>

#include "../app_path.h"
#include "../../version.h"
#include "app_path_private.h"

#include "sdl_proxy/sdl_assert.h"

std::string AppPathManager::m_settingsPath;
std::string AppPathManager::m_gamesavesPath;
std::string AppPathManager::m_userPath;

std::string AppPathManager::m_screenshotsPath;
std::string AppPathManager::m_gifrecordingsPath;
std::string AppPathManager::m_logsPath;

std::string AppPathManager::m_customAssetsRoot;
std::string AppPathManager::m_customUserDirectory;
std::string AppPathManager::m_customGameDirName;

std::string AppPathManager::m_currentAssetPackPath;
std::string AppPathManager::m_assetPackPostfix;

bool AppPathManager::m_isPortable = false;


static void appendSlash(std::string &path)
{
#if defined(__EMSCRIPTEN__)
    // fix emscripten bug of duplicated worlds
    if(path.empty() || path.back() != '/')
        path.push_back('/');
#else
    if(!path.empty() && path.back() != '/')
        path.push_back('/');
#endif
}

void AppPathManager::addAssetsRoot(const std::string &root)
{
    m_customAssetsRoot = root;
    appendSlash(m_customAssetsRoot);
}

void AppPathManager::setUserDirectory(const std::string& root)
{
    m_customUserDirectory = root;
    appendSlash(m_customUserDirectory);
}

void AppPathManager::setGameDirName(const std::string& dirName)
{
    m_customGameDirName = dirName;
    appendSlash(m_customGameDirName);
    // Also append to front
    if(!m_customGameDirName.empty() && m_customGameDirName.front() != '/')
        m_customGameDirName.insert(0, 1, '/');
}

void AppPathManager::initAppPath()
{
    AppPathP::initDefaultPaths(m_customGameDirName.empty() ? THEXTECH_DIRECTORY_PREFIX : m_customGameDirName);

    // When user directory is redefined externally
    if(!m_customUserDirectory.empty())
    {
        m_userPath = m_customUserDirectory;
        // Assets root matches to user directory if not specified other
        initSettingsPath();
        return;
    }

    if(AppPathP::portableAvailable() && checkPortable())
        return;

    std::string userDirPath = AppPathP::userDirectory();

    if(userDirPath.empty())
        goto defaultSettingsPath;
    else
    {
        DirMan appDir(userDirPath);
        if(userDirPath != "/" && !appDir.exists() && !appDir.mkpath())
            goto defaultSettingsPath;

        m_userPath = appDir.absolutePath();
    }

    appendSlash(m_userPath);

    initSettingsPath();

    return;

defaultSettingsPath:
    m_userPath = AppPathP::appDirectory();
    initSettingsPath();

#if defined(__EMSCRIPTEN__) && !defined(DISABLE_LOGGING)
    std::printf("== Default User Path is %s\n", m_userPath.c_str());
    fflush(stdout);
#endif
}

std::string AppPathManager::userAddedAssetsRoot()
{
    return m_customAssetsRoot;
}

std::vector<std::pair<std::string, AssetsPathType>> AppPathManager::assetsSearchPath()
{
    std::vector<std::pair<std::string, AssetsPathType>> out;

    if(!m_customAssetsRoot.empty())
        out.push_back({m_customAssetsRoot, AssetsPathType::Single});

    if(!m_customUserDirectory.empty())
        out.push_back({m_customUserDirectory, AssetsPathType::Legacy});

    if(!AppPathP::appDirectory().empty())
        out.push_back({AppPathP::appDirectory(), AssetsPathType::Legacy});

    if(!m_isPortable)
    {
        if(m_customUserDirectory.empty())
            out.push_back({m_userPath, AssetsPathType::Legacy});

        if(!AppPathP::assetsRoot().empty())
            out.push_back({AppPathP::assetsRoot(), AppPathP::assetsRootType()});

#ifdef APP_PATH_HAS_EXTRA_WORLDS
        // 3DS: add assets from additional romfs packages
        for(const std::string& root : AppPathManager::worldRootDirs())
            out.push_back({root, AssetsPathType::Single});
#endif
    }

    // add slashes to all strings
    for(size_t i = 0; i < out.size(); i++)
        appendSlash(out[i].first);

    // remove any duplicates
    for(size_t i = 0; i < out.size(); i++)
    {
        for(size_t j = i + 1; j < out.size();)
        {
            if(out[i].first == out[j].first)
                out.erase(out.begin() + j);
            else
                j++;
        }
    }

    return out;
}

void AppPathManager::setCurrentAssetPack(const std::string &id, const std::string &path)
{
    m_currentAssetPackPath = path;
    appendSlash(m_currentAssetPackPath);

    if(!id.empty())
        m_assetPackPostfix = id + "/";
    else
        m_assetPackPostfix = "";

    initSettingsPath();
}

bool AppPathManager::checkPortable()
{
    std::string appPath = AppPathP::appDirectory();

    if(appPath.empty())
        return false;

    if(m_settingsPath.empty())
        m_settingsPath = appPath;

    if(m_userPath.empty())
        m_userPath = appPath;

    if(!Files::fileExists(settingsFileSTD()))
        return false;

    m_isPortable = false;

    IniProcessing checkForPort(settingsFileSTD());
    checkForPort.beginGroup("Main");
    m_isPortable = checkForPort.value("force-portable", false).toBool();
    checkForPort.endGroup();

    if(m_isPortable)
        initSettingsPath();

    return m_isPortable;
}

void AppPathManager::initSettingsPath()
{
    // Default settings path
    initString(m_settingsPath, AppPathP::settingsRoot(), m_userPath + "settings/");

    // Default settings path
    initString(m_gamesavesPath, AppPathP::gamesavesRoot(), m_settingsPath + "gamesaves/" + m_assetPackPostfix);

    // Check if need to use system-wide screenshots directory
    initString(m_screenshotsPath, AppPathP::screenshotsRoot(), m_userPath + "screenshots/" + m_assetPackPostfix);

    // Check if need to use system-wide gif recording directory
    initString(m_gifrecordingsPath, AppPathP::gifRecsRoot(), m_userPath + "gif-recordings/" + m_assetPackPostfix);

    // Check if need to use system-wide logs directory
    initString(m_logsPath, AppPathP::logsRoot(), m_userPath + "logs/");

#ifndef VITA
    // Just in case, avoid mad jokes with making name-sake file as a settings folder
    if(Files::fileExists(m_settingsPath))
        Files::deleteFile(m_settingsPath);
#endif

    // Create the settings directory
    if(!DirMan::exists(m_settingsPath))
        DirMan::mkAbsPath(m_settingsPath);

    // Also create the game saves root folder to be exist
    if(!DirMan::exists(gameSaveRootDir()))
        DirMan::mkAbsPath(gameSaveRootDir());

    // Create the user assets directory, as a hint for the user
    if(!DirMan::exists(m_userPath + "assets/"))
        DirMan::mkAbsPath(m_userPath + "assets/");

    // And create empty worlds and battle directories too, make a hint for user
    if(!DirMan::exists(userWorldsRootDir()))
        DirMan::mkAbsPath(userWorldsRootDir());
    if(!DirMan::exists(userBattleRootDir()))
        DirMan::mkAbsPath(userBattleRootDir());
}

void AppPathManager::initString(std::string& text, const std::string& inValue, const std::string& defValue)
{
    if(m_isPortable)
        text = defValue;
    else
    {
        text = inValue;
        if(text.empty())
            text = defValue;
    }
}


std::string AppPathManager::settingsFileSTD() // Writable
{
    return m_settingsPath + "thextech.ini";
}

std::string AppPathManager::settingsRoot() // Writable
{
    return m_settingsPath;
}

std::string AppPathManager::settingsControlsFileSTD() // Writable
{
    return m_settingsPath + "controls.ini";
}

std::string AppPathManager::userAppDirSTD() // Writable
{
    return m_userPath;
}

std::string AppPathManager::assetsRoot() // Readable
{
    SDL_assert_release(!m_currentAssetPackPath.empty());
    return m_currentAssetPackPath;
}

std::string AppPathManager::logsDir() // Writable
{
    return m_logsPath;
}

std::string AppPathManager::languagesDir() // Readable
{
    SDL_assert_release(!m_currentAssetPackPath.empty());
    return m_currentAssetPackPath + "languages/";
}

std::string AppPathManager::screenshotsDir() // Writable
{
    return m_screenshotsPath;
}

std::string AppPathManager::gifRecordsDir() // Writable
{
    return m_gifrecordingsPath;
}

std::string AppPathManager::gameSaveRootDir() // Writable
{
    return m_gamesavesPath;
}

std::string AppPathManager::gameplayRecordsRootDir() // Writable
{
    return m_userPath + "gameplay-records/" + m_assetPackPostfix;
}

std::string AppPathManager::userWorldsRootDir() // Readable
{
    return m_userPath + "worlds/" + m_assetPackPostfix;
}

std::string AppPathManager::userBattleRootDir() // Readable
{
    return m_userPath + "battle/" + m_assetPackPostfix;
}

void AppPathManager::install()
{
    std::string path = AppPathP::userDirectory();

    if(!path.empty())
    {
        DirMan appDir(path);
        if(!appDir.exists())
            appDir.mkpath(path);
    }
}

bool AppPathManager::userDirIsAvailable()
{
    SDL_assert_release(!m_currentAssetPackPath.empty());
    return (m_userPath != m_currentAssetPackPath);
}

void AppPathManager::syncFs()
{
    AppPathP::syncFS();
}
