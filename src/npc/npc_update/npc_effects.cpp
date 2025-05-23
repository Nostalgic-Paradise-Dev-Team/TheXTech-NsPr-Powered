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

#include "sdl_proxy/sdl_stdinc.h"

#include "globals.h"
#include "npc.h"
#include "npc_traits.h"
#include "npc/npc_queues.h"
#include "config.h"
#include "collision.h"
#include "layers.h"
#include "editor.h"
#include "player.h"
#include "npc_update_priv.h"
#include "phys_env.h"

#include "main/trees.h"

static inline void NPCEffectLogic_EmergeUp(int A)
{
    if(NPC[A].Direction == 0) // Move toward the closest player
    {
        int target_plr = NPCTargetPlayer(NPC[A]);

        if(target_plr)
            NPC[A].Direction = -Player[target_plr].Direction;
    }

    NPC[A].Frame = EditorNPCFrame(NPC[A].Type, NPC[A].Direction, A);
    NPC[A].Effect2 += 1;
    NPC[A].Location.Y -= 1; // .01
    NPC[A].Location.Height += 1;

    if(NPC[A]->HeightGFX > 0)
    {
        if(NPC[A].Effect2 >= NPC[A]->HeightGFX)
        {
            NPC[A].Effect = NPCEFF_NORMAL;
            NPC[A].Effect2 = 0;
            NPC[A].Location.set_height_floor(NPC[A]->THeight);
        }
    }
    else
    {
        if(NPC[A].Effect2 >= NPC[A]->THeight)
        {
            NPC[A].Effect = NPCEFF_NORMAL;
            NPC[A].Effect2 = 0;
            NPC[A].Location.Height = NPC[A]->THeight;
        }
    }
}

static inline void NPCEffectLogic_EmergeDown(int A)
{
    if(NPC[A].Type == NPCID_LEAF_POWER)
        NPC[A].Direction = 1;
    else if(NPC[A].Direction == 0) // Move toward the closest player
    {
        int target_plr = NPCTargetPlayer(NPC[A]);

        if(target_plr)
            NPC[A].Direction = -Player[target_plr].Direction;
    }

    NPC[A].Effect2 += 1;
    NPC[A].Location.Y += 1;

    if(NPC[A].Effect2 == 32)
    {
        NPC[A].Effect = NPCEFF_NORMAL;
        NPC[A].Effect2 = 0;

        NPC[A].Location.Height = (g_config.fix_npc_emerge_size) ? NPC[A]->THeight : 32;

        for(int bCheck = 1; bCheck <= 2; bCheck++)
        {
            // if(bCheck == 1)
            // {
            //     // fBlock = FirstBlock[(NPC[A].Location.X / 32) - 1];
            //     // lBlock = LastBlock[((NPC[A].Location.X + NPC[A].Location.Width) / 32.0) + 1];
            //     blockTileGet(NPC[A].Location, fBlock, lBlock);
            // }
            // else
            // {
                   // buggy, mentioned above, should be numBlock - numTempBlock + 1 -- ds-sloth
                   // it's not a problem here because the NPC is moved out of the way of the block
                   // during the first loop, so can't collide during the second loop.
            //     fBlock = numBlock - numTempBlock;
            //     lBlock = numBlock;
            // }

            auto collBlockSentinel = (bCheck == 1)
                ? treeFLBlockQuery(NPC[A].Location, SORTMODE_COMPAT)
                : treeTempBlockQuery(NPC[A].Location, SORTMODE_LOC);

            for(BlockRef_t block : collBlockSentinel)
            {
                int B = block;

                if(!Block[B].Invis && !(BlockIsSizable[Block[B].Type] && NPC[A].Location.Y > Block[B].Location.Y) && !Block[B].Hidden)
                {
                    if(CheckCollision(NPC[A].Location, Block[B].Location))
                    {
                        NPC[A].Location.Y = Block[B].Location.Y - NPC[A].Location.Height - 0.1_n;
                        break;
                    }
                }
            }
        }
    }
}

static inline void NPCEffectLogic_Encased(int A)
{
    bool still_encased = false;

    // Note: since SMBX64, this logic doesn't check for Hidden or Active, so an encased NPC will not escape encased mode properly in Battle Mode (where killed NPCs just become inactive)
    // Note 2: NPCID_BOSS_FRAGILE does not use the encased logic, it has its own specific logic to check for nearby NPCID_BOSS_CASE
    for(int B : treeNPCQuery(NPC[A].Location, SORTMODE_NONE))
    {
        if(NPC[B].Type == NPCID_BOSS_CASE)
        {
            if(CheckCollision(NPC[A].Location, NPC[B].Location))
            {
                still_encased = true;
                break;
            }
        }
    }

    if(!still_encased)
        NPC[A].Effect = NPCEFF_NORMAL;
}

static inline void NPCEffectLogic_DropItem(int A)
{
    // modern item drop
    if(NPC[A].Effect3 != 0)
    {
        const Player_t& p = Player[NPC[A].Effect3];
        const Location_t& pLoc = p.Location;
        Location_t& nLoc = NPC[A].Location;

        // Y logic
        vScreen_t& vscreen = vScreenByPlayer(NPC[A].Effect3);

        // put above player
        num_t target_X = pLoc.X + (pLoc.Width - nLoc.Width) / 2;
        num_t target_Y = pLoc.Y + pLoc.Height - 192;

        // anticipate player movement
        if(PlayerNormal(p))
        {
            target_X += pLoc.SpeedX;
            target_Y += pLoc.SpeedY;
        }

        // never allow item to go fully offscreen
        if(target_Y < -vscreen.Y - 8)
            target_Y = -vscreen.Y - 8;

        // perform movement
        num_t delta_X = target_X - nLoc.X;
        num_t delta_Y = target_Y - nLoc.Y;

        num_t move_X = delta_X / 8;
        num_t move_Y = delta_Y / 8;

        num_t dist = num_t::dist(move_X, move_Y);

        // this magic number is 8 * sqrt(2)
        if(dist > 0 && dist < 11.31371_n)
        {
            move_X = (move_X * 11.31371_r).divided_by(dist);
            move_Y = (move_Y * 11.31371_r).divided_by(dist);
        }

        if(num_t::abs(delta_Y) < num_t::abs(move_Y) || NPC[A].Special5 <= 66)
            nLoc.Y = target_Y;
        else
            nLoc.Y += move_Y;

        if(num_t::abs(delta_X) < num_t::abs(move_X) || NPC[A].Special5 <= 66)
            nLoc.X = target_X;
        else
            nLoc.X += move_X;

        // timer logic
        if(NPC[A].Special5 <= 66)
            NPC[A].Special5 -= 1;
        else if(nLoc.X == target_X && nLoc.Y == target_Y)
            NPC[A].Special5 = 66;

        // enter SMBX mode on timer expiration
        if(NPC[A].Special5 <= 0)
        {
            NPC[A].Effect3 = 0;
            NPC[A].Special5 = 0;
        }
    }
    else
    {
        NPC[A].Location.Y += 2.2_n;

        NPC[A].Effect2 += 1;
        if(NPC[A].Effect2 == 5)
            NPC[A].Effect2 = 1;
    }
}

static inline void NPCEffectLogic_Warp(int A)
{
    // NOTE: this code previously used Effect2 to store destination position, and now it uses SpecialX/Y
    if(NPC[A].Effect3 == 1)
    {
        NPC[A].Location.Y -= 1;
        if(NPC[A].Type == NPCID_PLATFORM_S1)
            NPC[A].Location.Y -= 1;

        if(NPC[A].Location.Y + NPC[A].Location.Height <= NPC[A].SpecialY)
        {
            NPC[A].Effect = NPCEFF_NORMAL;
            NPC[A].SpecialY = 0;
            NPC[A].Effect3 = 0;
        }
    }
    else if(NPC[A].Effect3 == 3)
    {
        NPC[A].Location.Y += 1;

        if(NPC[A].Type == NPCID_PLATFORM_S1)
            NPC[A].Location.Y += 1;

        if(NPC[A].Location.Y >= NPC[A].SpecialY)
        {
            NPC[A].Effect = NPCEFF_NORMAL;
            NPC[A].SpecialY = 0;
            NPC[A].Effect3 = 0;
        }
    }
    else if(NPC[A].Effect3 == 2)
    {
        if(NPC[A].Type == NPCID_POWER_S3 || NPC[A].Type == NPCID_LIFE_S3 || NPC[A].Type == NPCID_POISON || NPC[A].Type == NPCID_POWER_S1 || NPC[A].Type == NPCID_POWER_S4 || NPC[A].Type == NPCID_LIFE_S1 || NPC[A].Type == NPCID_LIFE_S4 || NPC[A].Type == NPCID_BRUTE_SQUISHED || NPC[A].Type == NPCID_BIG_GUY)
            NPC[A].Location.X -= Physics.NPCMushroomSpeed;
        else if(NPC[A]->CanWalkOn)
            NPC[A].Location.X -= 1;
        else
            NPC[A].Location.X -= Physics.NPCWalkingSpeed;

        if(NPC[A].Location.X + NPC[A].Location.Width <= NPC[A].SpecialX)
        {
            NPC[A].Effect = NPCEFF_NORMAL;
            NPC[A].SpecialX = 0;
            NPC[A].Effect3 = 0;
        }
    }
    else if(NPC[A].Effect3 == 4)
    {
        if(NPC[A].Type == NPCID_POWER_S3 || NPC[A].Type == NPCID_LIFE_S3 || NPC[A].Type == NPCID_POISON || NPC[A].Type == NPCID_POWER_S1 || NPC[A].Type == NPCID_POWER_S4 || NPC[A].Type == NPCID_LIFE_S1 || NPC[A].Type == NPCID_LIFE_S4 || NPC[A].Type == NPCID_BRUTE_SQUISHED || NPC[A].Type == NPCID_BIG_GUY)
            NPC[A].Location.X += Physics.NPCMushroomSpeed;
        else if(NPC[A]->CanWalkOn)
            NPC[A].Location.X += 1;
        else
            NPC[A].Location.X += Physics.NPCWalkingSpeed;

        if(NPC[A].Location.X >= NPC[A].SpecialX)
        {
            NPC[A].Effect = NPCEFF_NORMAL;
            NPC[A].SpecialX = 0;
            NPC[A].Effect3 = 0;
        }
    }

    NPCFrames(A);

    if(NPC[A].Effect == NPCEFF_NORMAL && NPC[A].Type != NPCID_ITEM_BURIED)
    {
        NPC[A].Layer = LAYER_SPAWNED_NPCS;
        syncLayers_NPC(A);
    }
}

static inline void NPCEffectLogic_PetTongue(int A)
{
    NPC[A].TimeLeft = 100;
    NPC[A].Effect3 -= 1;
    if(NPC[A].Effect3 <= 0)
    {
        NPC[A].Effect = NPCEFF_NORMAL;
        NPC[A].Effect2 = 0;
        NPC[A].Effect3 = 0;
    }
}

static inline void NPCEffectLogic_PetInside(int A)
{
    NPC[A].TimeLeft = 100;
    if(Player[NPC[A].Effect2].YoshiNPC != A)
    {
        NPC[A].Effect = NPCEFF_NORMAL;
        NPC[A].Effect2 = 0;
        NPC[A].Effect3 = 0;
    }
}

static inline void NPCEffectLogic_Waiting(int A)
{
    NPC[A].Effect2 -= 1;
    if(NPC[A].Effect2 <= 0)
    {
        NPC[A].Effect = NPCEFF_NORMAL;
        NPC[A].Effect2 = 0;
        NPC[A].Effect3 = 0;
    }
}

static inline void NPCEffectLogic_Maze(int A)
{
    NPC_t& npc = NPC[A];

    NPCFrames(A);
    NPCSectionWrap(npc);
    NPCCollide(A);

    if(npc.TurnAround)
    {
        npc.Effect3 = (npc.Effect3 & 3) ^ MAZE_DIR_FLIP_BIT;
        npc.TurnAround = false;
    }

    // make balls go very slightly faster, and give them speed when they leave so they don't bounce forever
    bool is_ball = (npc.Type == NPCID_PLR_FIREBALL || npc.Type == NPCID_PLR_ICEBALL);
    bool is_bullet = (npc.Type == NPCID_BULLET && npc.CantHurt);
    bool is_player_thrown = is_ball || (npc.Type == NPCID_CHAR3_HEAVY || npc.Type == NPCID_BOMB);

    PhysEnv_Maze(npc.Location, npc.Effect2, npc.Effect3, A, 0, npc.Quicksand ? 1 : (npc.Wet ? 2 : 4) + is_player_thrown + 4 * is_bullet, {false, false, false, false});

    if(npc.Effect3 == MAZE_DIR_LEFT)
        npc.Direction = -1;
    else if(npc.Effect3 == MAZE_DIR_RIGHT)
        npc.Direction = 1;

    if(!npc.Effect2)
    {
        npc.Effect = NPCEFF_NORMAL;

        bool is_vert = (npc.Effect3 % 4 == MAZE_DIR_UP || npc.Effect3 % 4 == MAZE_DIR_DOWN);
        npc.Effect3 = 0;

        if(is_bullet)
            npc.Location.SpeedX = (npc.CantHurt ? 8 : 4) * npc.Direction;
        else if(is_vert)
        {
            if(!npc.Projectile && !npc.Wet)
            {
                npc.Projectile = true;
                npc.Effect3 = 128;
            }

            if(is_ball && npc.Special != 5)
                npc.Location.SpeedX = 1 - 4 * iRand(2) + 2 * dRand();
        }
    }
}


void NPCEffects(int A)
{
    if(NPC[A].Effect == NPCEFF_EMERGE_UP) // Bonus coming out of a block effect
        NPCEffectLogic_EmergeUp(A);
    else if(NPC[A].Effect == NPCEFF_ENCASED)
        NPCEffectLogic_Encased(A);
    else if(NPC[A].Effect == NPCEFF_DROP_ITEM) // Bonus item is falling from the players container effect
        NPCEffectLogic_DropItem(A);
    else if(NPC[A].Effect == NPCEFF_EMERGE_DOWN) // Bonus falling out of a block
        NPCEffectLogic_EmergeDown(A);
    else if(NPC[A].Effect == NPCEFF_WARP) // Warp Generator
        NPCEffectLogic_Warp(A);
    else if(NPC[A].Effect == NPCEFF_PET_TONGUE) // Grabbed by Yoshi
        NPCEffectLogic_PetTongue(A);
    else if(NPC[A].Effect == NPCEFF_PET_INSIDE) // Held by Yoshi
        NPCEffectLogic_PetInside(A);
    else if(NPC[A].Effect == NPCEFF_WAITING) // Holding Pattern
        NPCEffectLogic_Waiting(A);
    else if(NPC[A].Effect == NPCEFF_MAZE) // In a maze zone
        NPCEffectLogic_Maze(A);
}
