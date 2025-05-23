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

#include "globals.h"
#include "player.h"
#include "player/player_update_priv.h"
#include "config.h"
#include "layers.h"
#include "npc.h"
#include "npc_traits.h"
#include "phys_env.h"
#include "npc/npc_queues.h"

void PlayerThrowItemMaze(const Player_t& p, Location_t& loc, uint8_t& maze_status)
{
    loc.SpeedX = 0;
    loc.SpeedY = 0;

    int direction = p.MazeZoneStatus % 4;

    if(direction == MAZE_DIR_UP || direction == MAZE_DIR_DOWN)
        loc.X = p.Location.X + (p.Location.Width - loc.Width) / 2;
    else
        loc.Y = p.Location.Y + (p.Location.Height - loc.Height) / 2;

    if((direction == MAZE_DIR_UP && !p.Controls.Down) || (direction == MAZE_DIR_DOWN && p.Controls.Up))
    {
        maze_status = MAZE_DIR_UP;
        loc.SpeedY = -6;
        loc.Y = p.Location.Y - loc.Height - 24;
    }
    else if(direction == MAZE_DIR_UP || direction == MAZE_DIR_DOWN)
    {
        maze_status = MAZE_DIR_DOWN;
        loc.SpeedY = 6;
        loc.Y = p.Location.Y + p.Location.Height + 24;
    }
    else if(p.Direction <= 0)
    {
        maze_status = MAZE_DIR_LEFT;
        loc.SpeedX = -6;
        loc.X = p.Location.X - loc.Width - 24;
    }
    else
    {
        maze_status = MAZE_DIR_RIGHT;
        loc.SpeedX = 6;
        loc.X = p.Location.X + p.Location.Width + 24;
    }
}

void PlayerThrownNpcMazeCheck(const Player_t& p, NPC_t& npc)
{
    if(!p.CurMazeZone)
        return;

    if(npc->NoClipping && npc.Type != NPCID_BULLET)
        return;

    npc.Effect = NPCEFF_MAZE;
    npc.Effect2 = p.CurMazeZone;
    PlayerThrowItemMaze(p, npc.Location, npc.Effect3);
}

void PlayerPoundLogic(int A)
{
    if(Player[A].Location.SpeedY != 0 && Player[A].StandingOnNPC == 0 && Player[A].Slope == 0)
    {
        if(Player[A].Mount == 3 && Player[A].MountType == 6) // Purple Yoshi Pound
        {
            bool groundPoundByAltRun = !ForcedControls && g_config.pound_by_alt_run;
            bool poundKeyPressed = groundPoundByAltRun ? Player[A].Controls.AltRun : Player[A].Controls.Down;
            bool poundKeyRelease = groundPoundByAltRun ? Player[A].AltRunRelease   : Player[A].DuckRelease;

            if(poundKeyPressed && poundKeyRelease && Player[A].CanPound)
            {
                Player[A].GroundPound = true;
                Player[A].GroundPound2 = true;
                if(Player[A].Location.SpeedY < 0)
                    Player[A].Location.SpeedY = 0;
            }
        }
    }
    else
        Player[A].CanPound = false;

    if(Player[A].GroundPound)
    {
        if(!Player[A].CanPound && Player[A].Location.SpeedY < 0)
            Player[A].GroundPound = false;

        bool groundPoundByAltRun = !ForcedControls && g_config.pound_by_alt_run;
        if(groundPoundByAltRun)
            Player[A].Controls.AltRun = true;
        else
            Player[A].Controls.Down = true;

        Player[A].CanJump = false;
        Player[A].Controls.Left = false;
        Player[A].Controls.Up = false;
        Player[A].Controls.Right = false;
        Player[A].Controls.Jump = true;
        Player[A].Location.SpeedX = Player[A].Location.SpeedX * 0.95_r;
        Player[A].RunRelease = false;
        Player[A].CanFly = false;
        Player[A].FlyCount = 0;
        Player[A].CanFly2 = false;
        Player[A].Location.SpeedY += 1;
        Player[A].CanPound = false;
        Player[A].Jump = 0;
    }
    else
    {
        // allow pounding again
        if(Player[A].Location.SpeedY < -5 && ((Player[A].Jump < 15 && Player[A].Jump != 0) || Player[A].CanFly))
            Player[A].CanPound = true;

        // rebound from hitting the ground
        if(Player[A].GroundPound2)
        {
            Player[A].Location.SpeedY = -4;
            Player[A].StandingOnNPC = 0;
            Player[A].GroundPound2 = false;
        }
    }
}

void PlayerShootChar5Beam(int A)
{
    // TODO: State-dependent moment

    Player[A].FireBallCD2 = 40;
    if(Player[A].State == 6)
        Player[A].FireBallCD2 = 25;

    if(Player[A].State == 6)
        PlaySoundSpatial(SFX_HeroSwordBeam, Player[A].Location);
    else if(Player[A].State == 7 || Player[A].State == PLR_STATE_POLAR)
        PlaySoundSpatial(SFX_HeroIce, Player[A].Location);
    else
        PlaySoundSpatial(SFX_HeroFireRod, Player[A].Location);

    numNPCs++;

    if(ShadowMode)
        NPC[numNPCs].Shadow = true;

    NPC[numNPCs].Type = NPCID_PLR_FIREBALL;

    if(Player[A].State == 7 || Player[A].State == PLR_STATE_POLAR)
        NPC[numNPCs].Type = NPCID_PLR_ICEBALL;

    if(Player[A].State == 6)
        NPC[numNPCs].Type = NPCID_SWORDBEAM;

    NPC[numNPCs].Projectile = true;
    NPC[numNPCs].Location.Height = NPC[numNPCs]->THeight;
    NPC[numNPCs].Location.Width = NPC[numNPCs]->TWidth;
    NPC[numNPCs].Location.X = Player[A].Location.X + Player[A].Location.Width / 2 + (40 * Player[A].Direction) - 8;

    if(!Player[A].Duck)
    {
        NPC[numNPCs].Location.Y = Player[A].Location.Y + 5;
        if(Player[A].State == 6)
            NPC[numNPCs].Location.Y += 7;
    }
    else
    {
        NPC[numNPCs].Location.Y = Player[A].Location.Y + 18;
        if(Player[A].State == 6)
            NPC[numNPCs].Location.Y += 4;
    }


    NPC[numNPCs].Active = true;
    NPC[numNPCs].TimeLeft = 100;
    NPC[numNPCs].Location.SpeedY = 20;
    NPC[numNPCs].CantHurt = 100;
    NPC[numNPCs].CantHurtPlayer = A;
    NPC[numNPCs].Special = Player[A].Character;

    if(NPC[numNPCs].Type == NPCID_PLR_FIREBALL)
        NPC[numNPCs].Frame = 16;

    // kill item if it is inside a wall next frame
    NPC[numNPCs].WallDeath = 5;
    NPC[numNPCs].Location.SpeedY = 0;
    NPC[numNPCs].Location.SpeedX = 5 * Player[A].Direction + (Player[A].Location.SpeedX / 3);

    if(Player[A].State == 6)
        NPC[numNPCs].Location.SpeedX = 9 * Player[A].Direction + (Player[A].Location.SpeedX / 3);

    if(Player[A].StandingOnNPC != 0)
        NPC[numNPCs].Location.Y += -Player[A].Location.SpeedY;

    if(Player[A].State != 6)
        PlayerThrownNpcMazeCheck(Player[A], NPC[numNPCs]);

    syncLayers_NPC(numNPCs);
    CheckSectionNPC(numNPCs);
}

void PlayerThrowBomb(int A)
{
    Player_t& p = Player[A];

    p.Bombs -= 1;

    numNPCs++;
    NPC[numNPCs].Active = true;
    NPC[numNPCs].TimeLeft = Physics.NPCTimeOffScreen;
    NPC[numNPCs].Section = p.Section;
    NPC[numNPCs].Type = NPCID_BOMB;
    NPC[numNPCs].Location.Width = NPC[numNPCs]->TWidth;
    NPC[numNPCs].Location.Height = NPC[numNPCs]->THeight;
    NPC[numNPCs].CantHurtPlayer = A;
    NPC[numNPCs].CantHurt = 1000;

    if(p.Duck && (p.Location.SpeedY == 0 || p.Slope > 0 || p.StandingOnNPC != 0))
    {
        NPC[numNPCs].Location.X = p.Location.X + (p.Location.Width - NPC[numNPCs].Location.Width) / 2;
        NPC[numNPCs].Location.Y = p.Location.Y + p.Location.Height - NPC[numNPCs].Location.Height;
        NPC[numNPCs].Location.SpeedX = 0;
        NPC[numNPCs].Location.SpeedY = 0;
        PlaySoundSpatial(SFX_Grab, p.Location);
    }
    else
    {
        NPC[numNPCs].Location.X = p.Location.X + (p.Location.Width - NPC[numNPCs].Location.Width) / 2;
        NPC[numNPCs].Location.Y = p.Location.Y;
        NPC[numNPCs].Location.SpeedX = 5 * p.Direction;
        NPC[numNPCs].Location.SpeedY = -6;
        NPC[numNPCs].Projectile = true;

        if(p.Location.SpeedY == 0 || p.Slope > 0 || p.StandingOnNPC != 0)
            p.SwordPoke = -10;

        PlaySoundSpatial(SFX_Throw, p.Location);
    }

    PlayerThrownNpcMazeCheck(Player[A], NPC[numNPCs]);

    syncLayers_NPC(numNPCs);
}

bool PlayerChar4HeavyOut(const int A)
{
    if(Player[A].Character != 4)
        return false;

    for(int B : NPCQueues::Active.no_change)
    {
        if(NPC[B].Active)
        {
            if(NPC[B].Type == NPCID_CHAR4_HEAVY)
            {
                if(NPC[B].Special5 == A)
                    return true;
            }
        }
    }

    return false;
}

void PlayerThrowHeavy(const int A)
{
    auto &p = Player[A];

    if(p.RunRelease && PlayerChar4HeavyOut(A))
        return;

    if(numNPCs >= maxNPCs - 100)
        return;

    p.FrameCount = 110;
    p.FireBallCD = 25;

    numNPCs++;
    NPC[numNPCs].Type = NPCID_PLR_HEAVY;
    if(ShadowMode)
        NPC[numNPCs].Shadow = true;

    if(p.Character == 3)
    {
        p.FireBallCD = 45;
        NPC[numNPCs].Type = NPCID_CHAR3_HEAVY;

        if(p.Controls.AltRun && p.Mount == 0)
        {
            NPC[numNPCs].HoldingPlayer = A;
            p.HoldingNPC = numNPCs;
            PlaySoundSpatial(SFX_Grab2, p.Location);
        }
        else
            PlaySoundSpatial(SFX_Throw, p.Location);
    }
    else if(p.Character == 4)
    {

        p.FireBallCD = 0;
        if(FlameThrower)
            p.FireBallCD = 40;
        NPC[numNPCs].Type = NPCID_CHAR4_HEAVY;
        NPC[numNPCs].Special5 = A;
        NPC[numNPCs].Special4 = p.Direction; // Special6 in SMBX 1.3
        PlaySoundSpatial(SFX_Throw, p.Location);
    }
    else
        PlaySoundSpatial(playerHammerSFX, p.Location);

    NPC[numNPCs].Projectile = true;
    NPC[numNPCs].Location.Height = NPC[numNPCs]->THeight;
    NPC[numNPCs].Location.Width = NPC[numNPCs]->TWidth;
    NPC[numNPCs].Location.X = p.Location.X + Physics.PlayerGrabSpotX[p.Character][p.State] * p.Direction;
    NPC[numNPCs].Location.Y = p.Location.Y + Physics.PlayerGrabSpotY[p.Character][p.State];
    NPC[numNPCs].Active = true;
    NPC[numNPCs].TimeLeft = 100;
    NPC[numNPCs].Location.SpeedY = 20;
    NPC[numNPCs].CantHurt = 100;
    NPC[numNPCs].CantHurtPlayer = A;

    if(p.Controls.Up)
    {
        NPC[numNPCs].Location.SpeedX = 2 * p.Direction + p.Location.SpeedX * 0.9_r;

        if(p.StandingOnNPC == 0)
            NPC[numNPCs].Location.SpeedY = -8 + p.Location.SpeedY * 0.3_r;
        else
            NPC[numNPCs].Location.SpeedY = -8 + NPC[p.StandingOnNPC].Location.SpeedY * 0.3_r;

        NPC[numNPCs].Location.Y -= 24;
        NPC[numNPCs].Location.X += -6 * p.Direction;

        if(p.Character == 3)
        {
            NPC[numNPCs].Location.SpeedY += 1;
            NPC[numNPCs].Location.SpeedX = NPC[numNPCs].Location.SpeedX * 1.5_rb;
        }
        else if(p.Character == 4)
        {
            NPC[numNPCs].Location.SpeedY = -8;
            NPC[numNPCs].Location.SpeedX = 12 * p.Direction + p.Location.SpeedX;
        }
    }
    else
    {
        NPC[numNPCs].Location.SpeedX = 4 * p.Direction + p.Location.SpeedX * 0.9_r;

        if(p.StandingOnNPC == 0)
            NPC[numNPCs].Location.SpeedY = -5 + p.Location.SpeedY * 0.3_r;
        else
            NPC[numNPCs].Location.SpeedY = -5 + NPC[p.StandingOnNPC].Location.SpeedY * 0.3_r;

        if(p.Character == 3)
            NPC[numNPCs].Location.SpeedY += 1;
        else if(p.Character == 4)
        {
            NPC[numNPCs].Location.SpeedY = -5;
            NPC[numNPCs].Location.SpeedX = 10 * p.Direction + p.Location.SpeedX;
            NPC[numNPCs].Location.Y -= 12;
        }
    }

    if(p.Character == 4)
        NPC[numNPCs].Location.X = p.Location.X + (p.Location.Width - NPC[numNPCs].Location.Width) / 2;

    PlayerThrownNpcMazeCheck(p, NPC[numNPCs]);

    syncLayers_NPC(numNPCs);
    CheckSectionNPC(numNPCs);
}

void PlayerThrowBall(const int A)
{
    // TODO: State-dependent moment
    Player_t& p = Player[A];

    if(p.SpinJump)
        p.SpinFireDir = p.Direction;

    if(numNPCs >= maxNPCs - 100)
        return;

    numNPCs++;
    NPC[numNPCs].Type = NPCID_PLR_FIREBALL;

    bool throw_ice = (p.State == PLR_STATE_ICE || p.State == PLR_STATE_POLAR);

    if(throw_ice)
        NPC[numNPCs].Type = NPCID_PLR_ICEBALL;

    if(ShadowMode)
        NPC[numNPCs].Shadow = true;

    NPC[numNPCs].Projectile = true;
    NPC[numNPCs].Location.Height = NPC[numNPCs]->THeight;
    NPC[numNPCs].Location.Width = NPC[numNPCs]->TWidth;
    NPC[numNPCs].Location.X = p.Location.X + Physics.PlayerGrabSpotX[p.Character][p.State] * p.Direction + 4;
    NPC[numNPCs].Location.Y = p.Location.Y + Physics.PlayerGrabSpotY[p.Character][p.State];
    NPC[numNPCs].Active = true;
    NPC[numNPCs].TimeLeft = 100;
    NPC[numNPCs].CantHurt = 100;
    NPC[numNPCs].CantHurtPlayer = A;
    NPC[numNPCs].Special = p.Character;

    if(throw_ice)
        NPC[numNPCs].Special = 1;

    if((p.Character == 3 || p.Character == 4) && p.Mount == 0 && p.Controls.AltRun) // peach holds fireballs
    {
        p.HoldingNPC = numNPCs;
        NPC[numNPCs].HoldingPlayer = A;
    }

    if(NPC[numNPCs].Special == 2)
        NPC[numNPCs].Frame = 4;
    if(NPC[numNPCs].Special == 3)
        NPC[numNPCs].Frame = 8;
    if(NPC[numNPCs].Special == 4)
        NPC[numNPCs].Frame = 12;

    p.FireBallCD = 30;
    if(p.Character == 2)
        p.FireBallCD = 35;
    if(p.Character == 3)
        p.FireBallCD = 40;
    if(p.Character == 4)
        p.FireBallCD = 25;

    if(p.State == PLR_STATE_POLAR && p.Slippy)
        p.FireBallCD -= 5;

    bool throw_up = p.Controls.Up;
    int throw_dir = p.Direction;

    NPC[numNPCs].Location.SpeedX = 5 * throw_dir + (p.Location.SpeedX) / 3.5_ri;

    if(throw_ice)
    {
        PlaySoundSpatial(SFX_Iceball, p.Location);

        NPC[numNPCs].Location.SpeedY = (throw_up) ? -8 : 5;
        NPC[numNPCs].Location.SpeedX = NPC[numNPCs].Location.SpeedX * 0.8_r;
    }
    else
    {
        PlaySoundSpatial(SFX_Fireball, p.Location);

        NPC[numNPCs].Location.SpeedY = (throw_up) ? -6 : 20;

        if(NPC[numNPCs].Special == 2)
            NPC[numNPCs].Location.SpeedX = NPC[numNPCs].Location.SpeedX * 0.85_r;
    }

    if(throw_up)
    {
        if(p.StandingOnNPC != 0)
            NPC[numNPCs].Location.SpeedY += NPC[p.StandingOnNPC].Location.SpeedY / 10;
        else
            NPC[numNPCs].Location.SpeedY += p.Location.SpeedY / 10;

        NPC[numNPCs].Location.SpeedX = NPC[numNPCs].Location.SpeedX * 0.9_r;
    }

    if(FlameThrower)
    {
        NPC[numNPCs].Location.SpeedX = NPC[numNPCs].Location.SpeedX * 1.5_rb;
        NPC[numNPCs].Location.SpeedY = NPC[numNPCs].Location.SpeedY * 1.5_rb;
    }

    if(p.StandingOnNPC != 0)
        NPC[numNPCs].Location.SpeedX = 5 * p.Direction + (p.Location.SpeedX + NPC[p.StandingOnNPC].Location.SpeedX) / 3.5_ri;

    // special speed and animation code for polar swimming
    if(p.AquaticSwim)
    {
        // player animation code
        p.FireBallCD -= 10;

        int plr_frame = 16;

        if(p.Controls.Left || p.Controls.Right)
        {
            // use left/right frame
        }
        else if((p.Controls.Down && !p.Controls.Up) || Player[A].Frame == 19 || Player[A].Frame == 20 || Player[A].Frame == 21)
            plr_frame = 19;
        else if(p.Controls.Up || Player[A].Frame == 40 || Player[A].Frame == 41 || Player[A].Frame == 42)
        {
            throw_up = true;
            plr_frame = 40;
        }

        if(!p.SwimCount)
        {
            p.Frame = plr_frame;
            p.FrameCount = 60;
        }

        // center NPC if player is moving up/down
        if(plr_frame != 16)
        {
            NPC[numNPCs].Location.X = p.Location.X + (p.Location.Width - NPC[numNPCs].Location.Width) / 2;
            NPC[numNPCs].Location.Y += (plr_frame == 19) ? 8 : -8;
            NPC[numNPCs].Direction = throw_dir;
            throw_dir = 0;
        }

        // don't slow NPC down in its first frame of processing
        NPC[numNPCs].Wet = 2;

        // reset NPC speed
        NPC[numNPCs].Location.SpeedX = 4 * throw_dir;
        NPC[numNPCs].Location.SpeedY = (throw_dir) ? 2 : 3;

        // special logic for throwing upwards during Polar Swim
        if(throw_up)
        {
            NPC[numNPCs].Special4 = 1; // low gravity and no speed cap
            NPC[numNPCs].Special5 = 3; // prevent bouncing
            NPC[numNPCs].Location.SpeedY = -NPC[numNPCs].Location.SpeedY;
        }

        // add player momentum
        NPC[numNPCs].Location.SpeedX += p.Location.SpeedX;
        NPC[numNPCs].Location.SpeedY += p.Location.SpeedY;
    }
    else if(!p.SpinJump)
        p.FrameCount = 110;

    PlayerThrownNpcMazeCheck(p, NPC[numNPCs]);

    syncLayers_NPC(numNPCs);
    CheckSectionNPC(numNPCs);
}

void PowerUps(const int A)
{
    auto &p = Player[A];
    //int B = 0;

    if(p.Fairy)
    {
        p.SwordPoke = 0;
        p.FireBallCD = 0;
        p.FireBallCD2 = 0;
        p.TailCount = 0;
        return;
    }

    // this condition was moved into PlayerThrowHeavy
    // if(p.State == 6 && p.Character == 4 && p.Controls.Run && p.RunRelease)
    //     BoomOut = PlayerChar4HeavyOut(A);

    // Run-triggered actions; many of the conditions were combined here, so please search for the distinctive comments to find the corresponding code in legacy source
    if(!p.Slide && p.Vine == 0 && p.Character != 5 && (!p.Duck || p.AquaticSwim) && p.Mount <= 1 && p.HoldingNPC <= 0)
    {
        // Hammer Throw Code
        if(p.State == 6)
        {
            if(p.FireBallCD <= 0)
            {
                if(p.Controls.Run && !p.SpinJump /* && !BoomOut*/)
                {
                    if(p.RunRelease || p.SpinJump || FlameThrower)
                        PlayerThrowHeavy(A);
                }
            }
        }
        // Fire Mario / Luigi code ---- FIRE FLOWER ACTION BALLS OF DOOM
        // State-dependent moment!
        else if(p.State == 3 || p.State == 7 || p.State == PLR_STATE_POLAR)
        {
            if(p.FireBallCD <= 0)
            {
                if((p.Controls.Run && !p.SpinJump) || (p.SpinJump && p.Direction != p.SpinFireDir))
                {
                    if(p.RunRelease || p.SpinJump || FlameThrower)
                        PlayerThrowBall(A);
                }
            }
        }
        // RacoonMario
        else if(p.State == 4 || p.State == 5)
        {
            // NOTE: when PowerUps is called, HoldingNPC < 0 if and only if (Player[A].Mount != 0 || Player[A].Stoned || Player[A].Fairy), and Effect is always PLREFF_NORMAL
            if(p.HoldingNPC == 0 /* && p.Mount != 2 && !p.Stoned && p.Effect == PLREFF_NORMAL */)
            {
                if((p.Controls.Run && p.RunRelease) || p.SpinJump)
                {
                    if(p.TailCount == 0 || p.TailCount >= 12)
                    {
                        p.TailCount = 1;

                        if(!p.SpinJump)
                            PlaySoundSpatial(SFX_Whip, p.Location);
                    }
                }
            }
        }
    }

    if(p.TailCount > 0)
    {
        p.TailCount += 1;
        if(p.TailCount == 25)
            p.TailCount = 0;

        if(p.TailCount % 7 == 0 || (p.SpinJump && (p.TailCount % 2) == 0))
            TailSwipe(A, true);
        else
            TailSwipe(A);

        if(p.HoldingNPC > 0)
            p.TailCount = 0;
    }


    // link stab
    if(p.Character == 5 && p.Vine == 0 && p.Mount == 0 && !p.Stoned && p.FireBallCD == 0)
        PlayerChar5StabLogic(A);


    // cooldown timer
    p.FireBallCD2 -= 1;
    if(p.FireBallCD2 < 0)
        p.FireBallCD2 = 0;

    if(!(p.Character == 3 && NPC[p.HoldingNPC].Type == NPCID_PLR_FIREBALL))
    {
        p.FireBallCD -= 1;
        if(FlameThrower)
            p.FireBallCD -= 3;
        if(p.FireBallCD < 0)
            p.FireBallCD = 0;
    }
}

void Tanooki(const int A)
{
    auto &p = Player[A];

    if(p.Fairy)
       return;

    // tanooki
    if(p.Stoned && p.Controls.Down && p.StandingOnNPC == 0)
    {
        p.Location.SpeedX = p.Location.SpeedX * 0.8_r;
        if(p.Location.SpeedX >= -0.5_n && p.Location.SpeedX <= 0.5_n)
            p.Location.SpeedX = 0;
        if(p.Location.SpeedY < 8)
            p.Location.SpeedY += 0.25_n;
    }

    if(p.StonedCD == 0)
    {
        // If .Mount = 0 And .State = 5 And .Controls.Run = True And .Controls.Down = True Then
        if(p.Mount == 0 && p.State == 5 && p.Controls.AltRun && p.Bombs == 0)
        {
            if(!p.Stoned)
                p.Effect = PLREFF_STONE;
        }
        else if(p.Stoned)
            p.Effect = PLREFF_STONE;
    }
    else
        p.StonedCD -= 1;

    if(p.Stoned)
    {
        p.StonedTime += 1;
        if(p.StonedTime >= 240)
        {
            p.Effect = PLREFF_STONE;
            p.StonedCD = 60;
        }
        else if(p.StonedTime >= 180)
        {
            p.Immune += 1;
            if(p.Immune % 3 == 0)
                p.Immune2 = !p.Immune2;
        }
    }
}
