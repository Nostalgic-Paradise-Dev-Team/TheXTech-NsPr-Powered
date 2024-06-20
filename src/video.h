/*
 * TheXTech - A platform game engine ported from old source code for VB6
 *
 * Copyright (c) 2009-2011 Andrew Spinks, original VB6 code
 * Copyright (c) 2020-2024 Vitaly Novichkov <admin@wohlnet.ru>
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
#ifndef VIDEO_H
#define VIDEO_H

#include <string>
#include <unordered_map>

struct VideoSettings_t
{
    enum ScaleDownTextures
    {
        SCALE_DOWN_NONE = 0,
        SCALE_DOWN_SAFE = 1,
        SCALE_DOWN_ALL = 2,
    };

    enum RenderMode_t
    {
        RENDER_ACCELERATED_VSYNC_DEPRECATED = -1,
        RENDER_SOFTWARE = 0,
        RENDER_ACCELERATED_AUTO,
        RENDER_ACCELERATED_SDL,
        RENDER_ACCELERATED_OPENGL,
        RENDER_ACCELERATED_OPENGL_ES,
        RENDER_ACCELERATED_OPENGL_LEGACY,
        RENDER_ACCELERATED_OPENGL_ES_LEGACY,
        RENDER_END
    };

    enum BatteryStatus_t
    {
        BATTERY_STATUS_OFF = 0,
        BATTERY_STATUS_FULLSCREEN_WHEN_LOW,
        BATTERY_STATUS_ANY_WHEN_LOW,
        BATTERY_STATUS_FULLSCREEN_ON,
        BATTERY_STATUS_ALWAYS_ON,
    };

    enum ScaleModes
    {
        SCALE_DYNAMIC_INTEGER = -3,
        SCALE_DYNAMIC_NEAREST = -2,
        SCALE_DYNAMIC_LINEAR = -1,
        SCALE_FIXED_05X = 0,
        SCALE_FIXED_1X = 1,
        SCALE_FIXED_2X = 2,
    };

    //! Render mode
    int    render_mode = RENDER_ACCELERATED_AUTO;
    //! The currently running render mode
    int    render_mode_obtained = RENDER_ACCELERATED_AUTO;
    //! Attempt to enable vSync
    bool   render_vsync = false;
    //! Render scaling mode
    int    scale_mode = SCALE_DYNAMIC_NEAREST;
    //! Device battery status indicator
    int    show_battery_status = BATTERY_STATUS_OFF;
    //! Allow game to work when window is not active
    bool   background_work = false;
    //! Allow background input handling for game controllers
    bool   allowBgControllerInput = false;
    //! Enable frameskip
    bool   enable_frameskip = true;
    //! Show FPS counter
    bool   show_fps = false;
    //! 2x scale down all textures to reduce the memory usage
    int    scale_down_textures = SCALE_DOWN_SAFE;
};

static const std::unordered_map<int, std::string> ScaleMode_strings =
{
    {VideoSettings_t::SCALE_DYNAMIC_INTEGER, "integer"},
    {VideoSettings_t::SCALE_DYNAMIC_NEAREST, "nearest"},
    {VideoSettings_t::SCALE_DYNAMIC_LINEAR, "linear"},
    {VideoSettings_t::SCALE_FIXED_05X, "0.5x"},
    {VideoSettings_t::SCALE_FIXED_1X, "1x"},
    {VideoSettings_t::SCALE_FIXED_2X, "2x"},
};

#endif // VIDEO_H
