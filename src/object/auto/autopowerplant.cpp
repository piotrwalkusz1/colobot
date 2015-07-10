/*
 * This file is part of the Colobot: Gold Edition source code
 * Copyright (C) 2001-2014, Daniel Roux, EPSITEC SA & TerranovaTeam
 * http://epsiteс.ch; http://colobot.info; http://github.com/colobot
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://gnu.org/licenses
 */


#include "object/auto/autopowerplant.h"

#include "graphics/engine/terrain.h"

#include "math/geometry.h"

#include "object/object_manager.h"
#include "object/interface/transportable_object.h"
#include "object/level/parserline.h"
#include "object/level/parserparam.h"

#include "ui/interface.h"
#include "ui/gauge.h"
#include "ui/window.h"

#include <stdio.h>
#include <string.h>


const float POWERPLANT_POWER    =  0.4f;    // Necessary energy for a battery
const float POWERPLANT_DELAY    = 12.0f;    // processing time




// Object's constructor.

CAutoPowerPlant::CAutoPowerPlant(CObject* object) : CAuto(object)
{
    m_partiSphere = -1;
    Init();
}

// Object's destructor.

CAutoPowerPlant::~CAutoPowerPlant()
{
}


// Destroys the object.

void CAutoPowerPlant::DeleteObject(bool all)
{
    if ( m_partiSphere != -1 )
    {
        m_particle->DeleteParticle(m_partiSphere);
        m_partiSphere = -1;
    }

    if ( !all )
    {
        CObject* cargo = SearchMetal();
        if ( cargo != nullptr )
        {
            CObjectManager::GetInstancePointer()->DeleteObject(cargo);
        }

        cargo = SearchPower();
        if ( cargo != nullptr )
        {
            CObjectManager::GetInstancePointer()->DeleteObject(cargo);
        }
    }

    CAuto::DeleteObject(all);
}


// Initialize the object.

void CAutoPowerPlant::Init()
{
    m_time = 0.0f;
    m_timeVirus = 0.0f;
    m_lastUpdateTime = 0.0f;
    m_lastParticle = 0.0f;

    m_phase    = AENP_WAIT;  // waiting ...
    m_progress = 0.0f;
    m_speed    = 1.0f/2.0f;

    CAuto::Init();
}


// Management of an event.

bool CAutoPowerPlant::EventProcess(const Event &event)
{
    CObject*    cargo;
    Math::Vector    pos, ppos, speed;
    Math::Point     dim, c, p;
    Gfx::TerrainRes  res;
    float       big;
    bool        bGO;

    CAuto::EventProcess(event);

    if ( m_engine->GetPause() )  return true;
    if ( event.type != EVENT_FRAME )  return true;

    m_progress += event.rTime*m_speed;
    m_timeVirus -= event.rTime;

    if ( m_object->GetVirusMode() )  // contaminated by a virus?
    {
        if ( m_timeVirus <= 0.0f )
        {
            m_timeVirus = 0.1f+Math::Rand()*0.3f;

            if ( m_lastParticle+m_engine->ParticleAdapt(0.05f) <= m_time )
            {
                m_lastParticle = m_time;
                pos = m_object->GetPosition(0);
                pos.y += 10.0f;
                speed.x = (Math::Rand()-0.5f)*10.0f;
                speed.z = (Math::Rand()-0.5f)*10.0f;
                speed.y = -7.0f;
                dim.x = Math::Rand()*0.5f+0.5f;
                dim.y = dim.x;
                m_particle->CreateParticle(pos, speed, dim, Gfx::PARTIFIREZ, 1.0f, 0.0f, 0.0f);
            }
        }
        return true;
    }

    UpdateInterface(event.rTime);
    EventProgress(event.rTime);

    big = m_object->GetEnergy();

    res = m_terrain->GetResource(m_object->GetPosition(0));
    if ( res == Gfx::TR_POWER )
    {
        big += event.rTime*0.01f;  // recharges the big pile
    }

    if ( m_phase == AENP_WAIT )
    {
        if ( m_progress >= 1.0f )
        {
            bGO = false;
            cargo = SearchMetal();  // transform metal?
            if ( cargo != 0 )
            {
                if ( cargo->GetType() == OBJECT_METAL )
                {
                    if ( big > POWERPLANT_POWER )  bGO = true;
                }
                else
                {
                    if ( !SearchVehicle() )  bGO = true;
                }
            }

            if ( bGO )
            {
                if ( cargo->GetType() == OBJECT_METAL )
                {
                    cargo->SetLock(true);  // usable metal
                    CreatePower();  // creates the battery
                }

                SetBusy(true);
                InitProgressTotal(POWERPLANT_DELAY);
                CAuto::UpdateInterface();

                pos = m_object->GetPosition(0);
                pos.y += 4.0f;
                speed = Math::Vector(0.0f, 0.0f, 0.0f);
                dim.x = 3.0f;
                dim.y = dim.x;
                m_partiSphere = m_particle->CreateParticle(pos, speed, dim, Gfx::PARTISPHERE1, POWERPLANT_DELAY, 0.0f, 0.0f);

                m_phase    = AENP_CREATE;
                m_progress = 0.0f;
                m_speed    = 1.0f/POWERPLANT_DELAY;
            }
            else
            {
                if ( rand()%3 == 0 && big > 0.01f )
                {
                    m_phase    = AENP_BLITZ;
                    m_progress = 0.0f;
                    m_speed    = 1.0f/Math::Rand()*1.0f+1.0f;
                }
                else
                {
                    m_phase    = AENP_WAIT;  // still waiting ...
                    m_progress = 0.0f;
                    m_speed    = 1.0f/2.0f;
                }
            }
        }
    }

    if ( m_phase == AENP_BLITZ )
    {
        if ( m_progress < 1.0f && big > 0.01f )
        {
            if ( m_lastParticle+m_engine->ParticleAdapt(0.05f) <= m_time )
            {
                m_lastParticle = m_time;
                pos = m_object->GetPosition(0);
                pos.y += 10.0f;
                speed.x = (Math::Rand()-0.5f)*1.0f;
                speed.z = (Math::Rand()-0.5f)*1.0f;
                speed.y = -7.0f;
                dim.x = Math::Rand()*0.5f+0.5f;
                dim.y = dim.x;
                m_particle->CreateParticle(pos, speed, dim, Gfx::PARTIFIREZ, 1.0f, 0.0f, 0.0f);
            }
        }
        else
        {
            m_phase    = AENP_WAIT;  // still waiting ...
            m_progress = 0.0f;
            m_speed    = 1.0f/2.0f;
        }
    }

    if ( m_phase == AENP_CREATE )
    {
        if ( m_progress < 1.0f )
        {
            cargo = SearchMetal();
            if ( cargo != 0 )
            {
                if ( cargo->GetType() == OBJECT_METAL )
                {
                    big -= event.rTime/POWERPLANT_DELAY*POWERPLANT_POWER;
                }
                else
                {
                    big += event.rTime/POWERPLANT_DELAY*0.25f;
                }
                cargo->SetZoom(0, 1.0f-m_progress);
            }

            cargo = SearchPower();
            if ( cargo != 0 )
            {
                cargo->SetZoom(0, m_progress);
            }

            if ( m_lastParticle+m_engine->ParticleAdapt(0.10f) <= m_time )
            {
                m_lastParticle = m_time;

                pos = m_object->GetPosition(0);
                c.x = pos.x;
                c.y = pos.z;
                p.x = c.x;
                p.y = c.y+2.0f;
                p = Math::RotatePoint(c, Math::Rand()*Math::PI*2.0f, p);
                pos.x = p.x;
                pos.z = p.y;
                pos.y += 2.5f+Math::Rand()*3.0f;
                speed = Math::Vector(0.0f, 0.0f, 0.0f);
                dim.x = Math::Rand()*2.0f+1.0f;
                dim.y = dim.x;
                m_particle->CreateParticle(pos, speed, dim, Gfx::PARTIGLINT, 1.0f, 0.0f, 0.0f);

                pos = m_object->GetPosition(0);
                pos.y += 3.0f;
                speed.x = (Math::Rand()-0.5f)*30.0f;
                speed.z = (Math::Rand()-0.5f)*30.0f;
                speed.y = Math::Rand()*20.0f+10.0f;
                dim.x = Math::Rand()*0.4f+0.4f;
                dim.y = dim.x;
                m_particle->CreateTrack(pos, speed, dim, Gfx::PARTITRACK2, 2.0f, 50.0f, 1.2f, 1.2f);

                pos = m_object->GetPosition(0);
                pos.y += 10.0f;
                speed.x = (Math::Rand()-0.5f)*1.5f;
                speed.z = (Math::Rand()-0.5f)*1.5f;
                speed.y = -6.0f;
                dim.x = Math::Rand()*1.0f+1.0f;
                dim.y = dim.x;
                m_particle->CreateParticle(pos, speed, dim, Gfx::PARTIFIREZ, 1.0f, 0.0f, 0.0f);

                m_sound->Play(SOUND_ENERGY, m_object->GetPosition(0),
                              1.0f, 1.0f+Math::Rand()*1.5f);
            }
        }
        else
        {
            cargo = SearchMetal();
            if ( cargo != nullptr )
            {
                m_object->SetPower(nullptr);
                CObjectManager::GetInstancePointer()->DeleteObject(cargo);
            }

            cargo = SearchPower();
            if ( cargo != nullptr )
            {
                cargo->SetZoom(0, 1.0f);
                cargo->SetLock(false);  // usable battery
                dynamic_cast<CTransportableObject*>(cargo)->SetTransporter(m_object);
                cargo->SetPosition(0, Math::Vector(0.0f, 3.0f, 0.0f));
                m_object->SetPower(cargo);

                m_main->DisplayError(INFO_ENERGY, m_object);
            }

            SetBusy(false);
            CAuto::UpdateInterface();

            m_phase    = AENP_SMOKE;
            m_progress = 0.0f;
            m_speed    = 1.0f/5.0f;
        }
    }

    if ( m_phase == AENP_SMOKE )
    {
        if ( m_progress < 1.0f )
        {
            if ( m_lastParticle+m_engine->ParticleAdapt(0.05f) <= m_time )
            {
                m_lastParticle = m_time;

                pos = m_object->GetPosition(0);
                pos.y += 17.0f;
                pos.x += (Math::Rand()-0.5f)*3.0f;
                pos.z += (Math::Rand()-0.5f)*3.0f;
                speed.x = 0.0f;
                speed.z = 0.0f;
                speed.y = 6.0f+Math::Rand()*6.0f;
                dim.x = Math::Rand()*1.5f+1.0f;
                dim.y = dim.x;
                m_particle->CreateParticle(pos, speed, dim, Gfx::PARTISMOKE3, 4.0f);
            }
        }
        else
        {
            m_phase    = AENP_WAIT;
            m_progress = 0.0f;
            m_speed    = 1.0f/2.0f;
        }
    }

    if ( big < 0.0f )  big = 0.0f;
    if ( big > 1.0f )  big = 1.0f;
    m_object->SetEnergy(big);  // shift the big pile

    return true;
}


// Seeking the metal object.

CObject* CAutoPowerPlant::SearchMetal()
{
    CObject*    pObj;
    ObjectType  type;

    pObj = m_object->GetPower();
    if ( pObj == 0 )  return 0;

    type = pObj->GetType();
    if ( type == OBJECT_METAL  ||
         type == OBJECT_SCRAP1 ||
         type == OBJECT_SCRAP2 ||
         type == OBJECT_SCRAP3 )  return pObj;

    return 0;
}

// Search if a vehicle is too close.

bool CAutoPowerPlant::SearchVehicle()
{
    Math::Vector cPos = m_object->GetPosition(0);

    for (CObject* obj : CObjectManager::GetInstancePointer()->GetAllObjects())
    {
        ObjectType type = obj->GetType();
        if ( type != OBJECT_HUMAN    &&
             type != OBJECT_MOBILEfa &&
             type != OBJECT_MOBILEta &&
             type != OBJECT_MOBILEwa &&
             type != OBJECT_MOBILEia &&
             type != OBJECT_MOBILEfc &&
             type != OBJECT_MOBILEtc &&
             type != OBJECT_MOBILEwc &&
             type != OBJECT_MOBILEic &&
             type != OBJECT_MOBILEfi &&
             type != OBJECT_MOBILEti &&
             type != OBJECT_MOBILEwi &&
             type != OBJECT_MOBILEii &&
             type != OBJECT_MOBILEfs &&
             type != OBJECT_MOBILEts &&
             type != OBJECT_MOBILEws &&
             type != OBJECT_MOBILEis &&
             type != OBJECT_MOBILErt &&
             type != OBJECT_MOBILErc &&
             type != OBJECT_MOBILErr &&
             type != OBJECT_MOBILErs &&
             type != OBJECT_MOBILEsa &&
             type != OBJECT_MOBILEtg &&
             type != OBJECT_MOBILEft &&
             type != OBJECT_MOBILEtt &&
             type != OBJECT_MOBILEwt &&
             type != OBJECT_MOBILEit &&
             type != OBJECT_MOBILEdr &&
             type != OBJECT_MOTHER   &&
             type != OBJECT_ANT      &&
             type != OBJECT_SPIDER   &&
             type != OBJECT_BEE      &&
             type != OBJECT_WORM     )  continue;

        if (obj->GetCrashSphereCount() == 0) continue;

        auto crashSphere = obj->GetFirstCrashSphere();
        if (Math::DistanceToSphere(cPos, crashSphere.sphere) < 10.0f)
            return true;
    }

    return false;
}

// Create a cell.

void CAutoPowerPlant::CreatePower()
{
    Math::Vector pos = m_object->GetPosition(0);
    float angle = m_object->GetAngleY(0);
    float powerLevel = 1.0f;
    CObject* power = CObjectManager::GetInstancePointer()->CreateObject(pos, angle, OBJECT_POWER, powerLevel);
    power->SetLock(true);  // battery not yet usable

    pos = power->GetPosition(0);
    pos.y += 3.0f;
    power->SetPosition(0, pos);
}

// Seeking the battery during manufacture.

CObject* CAutoPowerPlant::SearchPower()
{
    Math::Vector cPos = m_object->GetPosition(0);

    for (CObject* obj : CObjectManager::GetInstancePointer()->GetAllObjects())
    {
        if ( !obj->GetLock() )  continue;

        ObjectType  type = obj->GetType();
        if ( type != OBJECT_POWER )  continue;

        Math::Vector oPos = obj->GetPosition(0);
        if ( oPos.x == cPos.x &&
             oPos.z == cPos.z )
        {
            return obj;
        }
    }

    return nullptr;
}


// Returns an error due the state of the automation.

Error CAutoPowerPlant::GetError()
{
    CObject*    pObj;
    ObjectType  type;
    Gfx::TerrainRes  res;

    if ( m_object->GetVirusMode() )
    {
        return ERR_BAT_VIRUS;
    }

    if ( m_phase != AENP_WAIT  &&
         m_phase != AENP_BLITZ )  return ERR_OK;

    res = m_terrain->GetResource(m_object->GetPosition(0));
    if ( res != Gfx::TR_POWER )  return ERR_ENERGY_NULL;

    if ( m_object->GetEnergy() < POWERPLANT_POWER )  return ERR_ENERGY_LOW;

    pObj = m_object->GetPower();
    if ( pObj == 0 )  return ERR_ENERGY_EMPTY;
    type = pObj->GetType();
    if ( type == OBJECT_POWER )  return ERR_OK;
    if ( type != OBJECT_METAL  &&
         type != OBJECT_SCRAP1 &&
         type != OBJECT_SCRAP2 &&
         type != OBJECT_SCRAP3 )  return ERR_ENERGY_BAD;

    return ERR_OK;
}


// Creates all the interface when the object is selected.

bool CAutoPowerPlant::CreateInterface(bool bSelect)
{
    Ui::CWindow*    pw;
    Math::Point     pos, ddim;
    float       ox, oy, sx, sy;

    CAuto::CreateInterface(bSelect);

    if ( !bSelect )  return true;

    pw = static_cast< Ui::CWindow* >(m_interface->SearchControl(EVENT_WINDOW0));
    if ( pw == 0 )  return false;

    ox = 3.0f/640.0f;
    oy = 3.0f/480.0f;
    sx = 33.0f/640.0f;
    sy = 33.0f/480.0f;

    pos.x = ox+sx*14.5f;
    pos.y = oy+sy*0;
    ddim.x = 14.0f/640.0f;
    ddim.y = 66.0f/480.0f;
    pw->CreateGauge(pos, ddim, 0, EVENT_OBJECT_GENERGY);

    pos.x = ox+sx*0.0f;
    pos.y = oy+sy*0;
    ddim.x = 66.0f/640.0f;
    ddim.y = 66.0f/480.0f;
    pw->CreateGroup(pos, ddim, 108, EVENT_OBJECT_TYPE);

    return true;
}

// Updates the state of all buttons on the interface,
// following the time that elapses ...

void CAutoPowerPlant::UpdateInterface(float rTime)
{
    Ui::CWindow*    pw;
    Ui::CGauge*     pg;

    CAuto::UpdateInterface(rTime);

    if ( m_time < m_lastUpdateTime+0.1f )  return;
    m_lastUpdateTime = m_time;

    if ( !m_object->GetSelect() )  return;

    pw = static_cast< Ui::CWindow* >(m_interface->SearchControl(EVENT_WINDOW0));
    if ( pw == 0 )  return;

    pg = static_cast< Ui::CGauge* >(pw->SearchControl(EVENT_OBJECT_GENERGY));
    if ( pg != 0 )
    {
        pg->SetLevel(m_object->GetEnergy());
    }
}


// Saves all parameters of the controller.

bool CAutoPowerPlant::Write(CLevelParserLine* line)
{
    if ( m_phase == AENP_STOP ||
         m_phase == AENP_WAIT )  return false;

    line->AddParam("aExist", CLevelParserParamUPtr{new CLevelParserParam(true)});
    CAuto::Write(line);
    line->AddParam("aPhase", CLevelParserParamUPtr{new CLevelParserParam(static_cast<int>(m_phase))});
    line->AddParam("aProgress", CLevelParserParamUPtr{new CLevelParserParam(m_progress)});
    line->AddParam("aSpeed", CLevelParserParamUPtr{new CLevelParserParam(m_speed)});

    return true;
}

// Restores all parameters of the controller.

bool CAutoPowerPlant::Read(CLevelParserLine* line)
{
    if ( !line->GetParam("aExist")->AsBool(false) )  return false;

    CAuto::Read(line);
    m_phase = static_cast< AutoPowerPlantPhase >(line->GetParam("aPhase")->AsInt(AENP_WAIT));
    m_progress = line->GetParam("aProgress")->AsFloat(0.0f);
    m_speed = line->GetParam("aSpeed")->AsFloat(1.0f);

    m_lastUpdateTime = 0.0f;
    m_lastParticle = 0.0f;

    return true;
}

