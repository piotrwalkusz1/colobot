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
#include "app/pausemanager.h"

#include "common/logger.h"

#include "object/robotmain.h"

#include "graphics/engine/camera.h"
#include "graphics/engine/engine.h"
#include "graphics/engine/terrain.h"

#include "ui/displaytext.h"
#include "ui/mainmap.h"

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
    
    m_freePhoto = false;
    console->AddFunction("photo", photo);
    console->AddAlias("photo1", "photo false");
    console->AddAlias("photo2", "photo true");
    
    m_cheatRadar = false;
    console->AddVariable("cheatRadar", &m_cheatRadar);
    console->AddAlias("invradar", "toggle cheatRadar");
    
    console->AddAlias("showstat", "toggle showStats");
    console->AddAlias("invshadow", "toggle shadows");
    console->AddAlias("invdirty", "toggle dirty");
    console->AddAlias("invfog", "toggle fog");
    console->AddAlias("invlens", "toggle lens");
    console->AddAlias("invwater", "toggle water");
    console->AddAlias("invsky", "toggle sky");
    console->AddAlias("invplanet", "toggle planet");
    
    console->AddAlias("noclip", "currentObject.clip = false");
    console->AddAlias("clip", "currentObject.clip = true");
    
    console->AddAlias("fullenergy", "currentObject.energyCell.power = 1.0");
    console->AddAlias("fullshield", "currentObject.shield = 1.0");
    console->AddAlias("fullrange", "currentObject.range = 1.0"); //TODO!
    console->AddAlias("fullpower", "fullenergy&fullshield&fullrange");
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

Error CCheat::photo(std::vector<std::string> params)
{
    if(params.size() < 1) {
        CLogger::GetInstancePointer()->Error("Usage: photo [removeControls]\n");
        return ERR_CMD;
    }
    
    bool removeControls;
    if(params[0] == "true") removeControls = true;
    else if(params[0] == "false") removeControls = false;
    else {
        CLogger::GetInstancePointer()->Error("Usage: photo [removeControls]\n");
        CLogger::GetInstancePointer()->Error("removeControls must be true/false\n");
        return ERR_CMD;
    }
    
    CCheat* cheat = CCheat::GetInstancePointer();
    CRobotMain* robotmain = CRobotMain::GetInstancePointer();
    CPauseManager* pause = CPauseManager::GetInstancePointer();
    
    cheat->m_freePhoto = !cheat->m_freePhoto;
    if (cheat->m_freePhoto)
    {
        robotmain->GetCamera()->SetType(Gfx::CAM_TYPE_FREE);
        pause->SetPause(PAUSE_PHOTO);
        if(removeControls) {
            robotmain->DeselectAll();  // removes the control buttons
            robotmain->GetMap()->ShowMap(false);
            robotmain->GetDisplayText()->HideText(true);
        }
    }
    else
    {
        robotmain->GetCamera()->SetType(Gfx::CAM_TYPE_BACK);
        pause->ClearPause();
        if(removeControls) {
            robotmain->GetMap()->ShowMap(robotmain->GetShowMap());
            robotmain->GetDisplayText()->HideText(false);
        }
    }
    
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

if (m_phase == PHASE_SIMUL)
    m_displayText->DisplayError(ERR_CMD, Math::Vector(0.0f,0.0f,0.0f));*/
