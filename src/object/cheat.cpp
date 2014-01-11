// * This file is part of the COLOBOT source code
// * Copyright (C) 2001-2008, Daniel ROUX & EPSITEC SA, www.epsitec.ch
// * Copyright (C) 2012, Polish Portal of Colobot (PPC)
// *
// * This program is free software: you can redistribute it and/or modify
// * it under the terms of the GNU General Public License as published by
// * the Free Software Foundation, either version 3 of the License, or
// * (at your option) any later version.
// *
// * This program is distributed in the hope that it will be useful,
// * but WITHOUT ANY WARRANTY; without even the implied warranty of
// * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// * GNU General Public License for more details.
// *
// * You should have received a copy of the GNU General Public License
// * along with this program. If not, see  http://www.gnu.org/licenses/.

#include "object/cheat.h"

#include "app/app.h"

#include "common/logger.h"

#include "object/robotmain.h"

#include "graphics/engine/engine.h"
#include "graphics/engine/terrain.h"

#include <bitset>

#include <boost/lexical_cast.hpp>

template<> CCheat* CSingleton<CCheat>::m_instance = nullptr;

CCheat::CCheat()
{
    CConsole* console = CConsole::GetInstancePointer();
    console->AddFunction("test", test);
    console->AddFunction("add_event", add_event);
    console->AddAlias("update_interface", "add_event "+ToString(EVENT_UPDINTERFACE));
    
    console->AddAlias("winmission", "add_event "+ToString(EVENT_WIN));
    console->AddAlias("lostmission", "add_event "+ToString(EVENT_LOST));
    
    console->AddVariable("build", &g_build);
    console->AddVariable("researchDone", &g_researchDone);
    console->AddVariable("researchEnable", &g_researchEnable);
    console->AddAlias("fly", "bit_or researchDone "+ToBitmask(RESEARCH_FLY)+"&update_interface");
    console->AddAlias("allresearch", "researchDone = -1&update_interface");
    console->AddAlias("allbuildings", "build = -1&update_interface");
    console->AddAlias("all", "allresearch&allbuildings");
    
    m_trainerPilot = false;
    console->AddVariable("trainerPilot", &m_trainerPilot);
    console->AddAlias("trainerpilot", "toggle trainerPilot");
    
    m_selectInsect = false;
    console->AddVariable("selectInsect", &m_selectInsect);
    console->AddAlias("selectinsect", "toggle selectInsect");
    
    console->AddAlias("nolimit", "maxFlyingHeight = 280");
    
    console->AddVariableSetFunction("speed", speed);
    console->AddAlias("speed4", "speed = 4");
    console->AddAlias("speed8", "speed = 8");
    console->AddAlias("crazy", "speed = 1000");
}

CCheat::~CCheat()
{
}

std::string CCheat::ToBitmask(long i)
{
    std::bitset<sizeof(long)> x(i);
    return x.to_string();
}

std::string CCheat::ToString(EventType event)
{
    return boost::lexical_cast<std::string>(static_cast<int>(event));
}

Error CCheat::test(std::vector<std::string> params)
{
    CLogger::GetInstancePointer()->Debug("Testing, testing, testing...\n");
    return ERR_OK;
}

Error CCheat::add_event(std::vector<std::string> params)
{
    if(params.size() < 1) {
        CLogger::GetInstancePointer()->Error("Usage: add_event [event_id]\n");
        return ERR_CMD;
    }

    CApplication::GetInstancePointer()->GetEventQueue()->AddEvent(Event(static_cast<EventType>(boost::lexical_cast<int>(params[0]))));
    return ERR_OK;
}

Error CCheat::speed(ConsoleVariable var, std::string params)
{
    float value = boost::lexical_cast<float>(params);
    CRobotMain::GetInstancePointer()->SetSpeed(value);
    return ERR_OK;
}

/*if (m_phase == PHASE_SIMUL)
{

    if (strcmp(cmd, "controller") == 0)
    {
        if (m_controller != nullptr)
        {
            // Don't use SelectObject because it checks if the object is selectable
            if (m_camera->GetType() == Gfx::CAM_TYPE_VISIT)
                StopDisplayVisit();

            CObject* prev = DeselectAll();
            if (prev != nullptr && prev != m_controller)
               m_controller->AddDeselList(prev);

            SelectOneObject(m_controller, true);
            m_short->UpdateShortcuts();
        }
        return;
    }

    if (strcmp(cmd, "photo1") == 0)
    {
        m_freePhoto = !m_freePhoto;
        if (m_freePhoto)
        {
            m_camera->SetType(Gfx::CAM_TYPE_FREE);
            ChangePause(PAUSE_PHOTO);
        }
        else
        {
            m_camera->SetType(Gfx::CAM_TYPE_BACK);
            ChangePause(PAUSE_NONE);
        }
        return;
    }

    if (strcmp(cmd, "photo2") == 0)
    {
        m_freePhoto = !m_freePhoto;
        if (m_freePhoto)
        {
            m_camera->SetType(Gfx::CAM_TYPE_FREE);
            ChangePause(PAUSE_PHOTO);
            DeselectAll();  // removes the control buttons
            m_map->ShowMap(false);
            m_displayText->HideText(true);
        }
        else
        {
            m_camera->SetType(Gfx::CAM_TYPE_BACK);
            ChangePause(PAUSE_NONE);
            m_map->ShowMap(m_mapShow);
            m_displayText->HideText(false);
        }
        return;
    }

    if (strcmp(cmd, "noclip") == 0)
    {
        CObject* object = GetSelect();
        if (object != nullptr)
            object->SetClip(false);
        return;
    }

    if (strcmp(cmd, "clip") == 0)
    {
        CObject* object = GetSelect();
        if (object != nullptr)
            object->SetClip(true);
        return;
    }

    if (strcmp(cmd, "addhusky") == 0)
    {
        CObject* object = GetSelect();
        if (object != nullptr)
            object->SetMagnifyDamage(object->GetMagnifyDamage()*0.1f);
        return;
    }

    if (strcmp(cmd, "addfreezer") == 0)
    {
        CObject* object = GetSelect();
        if (object != nullptr)
            object->SetRange(object->GetRange()*10.0f);
        return;
    }

    if (strcmp(cmd, "\155\157\157") == 0)
    {
        // VGhpcyBpcyBlYXN0ZXItZWdnIGFuZCBzbyBpdCBzaG91bGQgYmUgb2JmdXNjYXRlZCEgRG8gbm90
        // IGNsZWFuLXVwIHRoaXMgY29kZSEK
        GetLogger()->Info(" _________________________\n");
        GetLogger()->Info("< \x50\x6F\x6C\x73\x6B\x69 \x50\x6F\x72\x74\x61\x6C C\x6F\x6C\x6F\x62\x6F\x74\x61! \x3E\n");
        GetLogger()->Info(" -------------------------\n");
        GetLogger()->Info("        \x5C\x20\x20\x20\x5E\x5F\x5F\x5E\n");
        GetLogger()->Info("        \x20\x5C\x20\x20\x28\x6F\x6F\x29\x5C\x5F\x5F\x5F\x5F\x5F\x5F\x5F\n");
        GetLogger()->Info("            \x28\x5F\x5F\x29\x5C   \x20\x20\x20\x20\x29\x5C\x2F\x5C\n");
        GetLogger()->Info("            \x20\x20\x20\x20\x7C|\x2D\x2D\x2D\x2D\x77\x20\x7C\n");
        GetLogger()->Info("          \x20\x20    \x7C\x7C\x20\x20\x20\x20 ||\n");
    }

    if (strcmp(cmd, "fullpower") == 0)
    {
        CObject* object = GetSelect();
        if (object != nullptr)
        {
            CObject* power = object->GetPower();
            if (power != nullptr)
                power->SetEnergy(1.0f);

            object->SetShield(1.0f);
            CPhysics* physics = object->GetPhysics();
            if (physics != nullptr)
                physics->SetReactorRange(1.0f);
        }
        return;
    }

    if (strcmp(cmd, "fullenergy") == 0)
    {
        CObject* object = GetSelect();
        if (object != nullptr)
        {
            CObject* power = object->GetPower();
            if (power != nullptr)
                power->SetEnergy(1.0f);
        }
        return;
    }

    if (strcmp(cmd, "fullshield") == 0)
    {
        CObject* object = GetSelect();
        if (object != nullptr)
            object->SetShield(1.0f);
        return;
    }

    if (strcmp(cmd, "fullrange") == 0)
    {
        CObject* object = GetSelect();
        if (object != nullptr)
        {
            CPhysics* physics = object->GetPhysics();
            if (physics != nullptr)
                physics->SetReactorRange(1.0f);
        }
        return;
    }
}

if (strcmp(cmd, "debugmode") == 0)
{
    if (m_app->IsDebugModeActive(DEBUG_ALL))
    {
        m_app->SetDebugModeActive(DEBUG_ALL, false);
    }
    else
    {
        m_app->SetDebugModeActive(DEBUG_ALL, true);
    }
    return;
}

if (strcmp(cmd, "showstat") == 0)
{
    m_engine->SetShowStats(!m_engine->GetShowStats());
    return;
}

if (strcmp(cmd, "invshadow") == 0)
{
    m_engine->SetShadow(!m_engine->GetShadow());
    return;
}

if (strcmp(cmd, "invdirty") == 0)
{
    m_engine->SetDirty(!m_engine->GetDirty());
    return;
}

if (strcmp(cmd, "invfog") == 0)
{
    m_engine->SetFog(!m_engine->GetFog());
    return;
}

if (strcmp(cmd, "invlens") == 0)
{
    m_engine->SetLensMode(!m_engine->GetLensMode());
    return;
}

if (strcmp(cmd, "invwater") == 0)
{
    m_engine->SetWaterMode(!m_engine->GetWaterMode());
    return;
}

if (strcmp(cmd, "invsky") == 0)
{
    m_engine->SetSkyMode(!m_engine->GetSkyMode());
    return;
}

if (strcmp(cmd, "invplanet") == 0)
{
    m_engine->SetPlanetMode(!m_engine->GetPlanetMode());
    return;
}

if (strcmp(cmd, "showpos") == 0)
{
    m_showPos = !m_showPos;
    return;
}

if (strcmp(cmd, "showsoluce") == 0)
{
    m_showSoluce = !m_showSoluce;
    m_dialog->ShowSoluceUpdate();
    return;
}

/ * TODO: #if _TEEN
if (strcmp(cmd, "allteens") == 0)
#else* /
if (strcmp(cmd, "allmission") == 0)
{
    m_showAll = !m_showAll;
    m_dialog->AllMissionUpdate();
    return;
}

if (strcmp(cmd, "invradar") == 0)
{
    m_cheatRadar = !m_cheatRadar;
    return;
}

if (m_phase == PHASE_SIMUL)
    m_displayText->DisplayError(ERR_CMD, Math::Vector(0.0f,0.0f,0.0f));*/
