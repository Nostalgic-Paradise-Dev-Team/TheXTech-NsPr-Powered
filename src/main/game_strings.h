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

#pragma once
#ifndef GAME_STRINGS_H
#define GAME_STRINGS_H

#include <string>

struct GameContent
{
    std::string msgBoxTitleInfo;
    std::string msgBoxTitleWarning;
    std::string msgBoxTitleError;

    std::string errorOpenFileFailed;
    std::string errorInvalidEnterWarp;
    std::string errorNoStartPoint;
    std::string messageScanningLevels;
    std::string formatMinutesSeconds;

#if !defined(NO_WINDOW_FOCUS_TRACKING) && !defined(THEXTECH_DISABLE_LANG_TOOLS)
    std::string screenPaused;
#endif

    std::string warpNeedStarCount;

    std::string pauseItemContinue;
    std::string pauseItemRestartLevel;
    std::string pauseItemResetCheckpoints;
    std::string pauseItemQuitTesting;
    std::string pauseItemReturnToEditor;
    std::string pauseItemPlayerSetup;
    std::string pauseItemEnterCode;
    std::string pauseItemSaveAndContinue;
    std::string pauseItemSaveAndQuit;
    std::string pauseItemQuit;


    // ConnectScreen

    std::string connectReconnectTitle;

    std::string connectPressAButton;

    std::string connectTestProfile;
    std::string connectHoldStart;
    std::string connectDisconnect;

    std::string connectDropMe;

    std::string connectForceResume;
    std::string connectDropPX;

    std::string connectWaitingForInputDevice;
    std::string connectPressSelectForControlsOptions_P1;
    std::string connectPressSelectForControlsOptions_P2;
};

extern GameContent g_gameStrings;

void initGameStrings();

#endif // GAME_STRINGS_H
