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

#include "renderop_bitmap.h"
#include "core/render.h"
#include "globals.h"
#include "lunaimgbox.h"


RenderBitmapOp::RenderBitmapOp() : RenderOp()
{
    static_assert(sizeof(RenderBitmapOp) <= c_rAllocChunkSize,
                  "Size of RenderBitmapOp class must be smaller than c_rAllocChunkSize");
}

void RenderBitmapOp::Draw(Renderer *renderer)
{
    if(!direct_img || (direct_img->getH() == 0) || (direct_img->getW() == 0))
        return;

    int screenX = this->x;
    int screenY = this->y;

    if(sceneCoords)
    {
        screenX -= vScreen[renderer->GetCameraIdx()].CameraAddX();
        screenY -= vScreen[renderer->GetCameraIdx()].CameraAddY();
    }
    else
    {
        Render::TranslateScreenCoords(screenX, screenY, this->sw, this->sh);
    }

    // Get integer values as current rendering backends prefer that
    int x = screenX;
    int y = screenY;
    int sx = this->sx;
    int sy = this->sy;
    int width = this->sw;
    int height = this->sh;

    // Trim height/width if necessary
    if(direct_img->getW() < width + sx)
        width = direct_img->getW() - sx;

    if(direct_img->getH() < height + sy)
        height = direct_img->getH() - sy;

    // Don't render if no size
    if((width <= 0) || (height <= 0))
        return;

    XRender::renderTextureBasic(x, y, width, height, direct_img->m_image, sx, sy, color);
}
