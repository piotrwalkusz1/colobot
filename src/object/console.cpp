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

#include "object/console.h"

#include "common/logger.h"

#include "math/geometry.h"

#include "ui/interface.h"

#include <bitset>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>


template<> CConsole* CSingleton<CConsole>::m_instance = nullptr;


CConsole::CConsole()
{
    m_interface = CRobotMain::GetInstancePointer()->GetInterface();
    m_pause = CPauseManager::GetInstancePointer();

    m_visible = false;
    
    // Built-in functions
    AddFunction("toggle", toggle);
    AddFunction("list", list);
    AddFunction("alias", alias);
    AddFunction("bit_or", bit_or);
    AddFunction("bit_and", bit_and);
    AddFunction("bit_clear", bit_clear);
}

CConsole::~CConsole()
{
    m_interface = nullptr;
    m_pause = nullptr;
}

bool CConsole::ProcessEvent(const Event &event)
{
    if (/*!m_movie->IsExist()   &&
        !m_movieLock && !m_editLock &&*/
        !m_pause->GetPause() &&
        event.type == EVENT_KEY_DOWN &&
        event.key.key == KEY(PAUSE))  // Pause ?
    {
        Toggle();
        return false;
    }
    
    if (event.type == EVENT_KEY_DOWN &&
        event.key.key == KEY(RETURN) && m_visible)
    {
        char cmd[128];
        Ui::CEdit* pe = static_cast<Ui::CEdit*>(m_interface->SearchControl(EVENT_CMD));
        if (pe == nullptr) return false;
        pe->GetText(cmd, 128);
        pe->SetText("");
        pe->ClearState(Ui::STATE_VISIBLE);
        m_pause->ClearPause();
        ProcessCommand(cmd);
        m_visible = false;
        return false;
    }
    
    return true;
}

void CConsole::SetMode(ConsoleMode mode)
{
    m_mode = mode;
}

void CConsole::Create()
{
    Math::Point pos, dim;

    if(m_mode == CONSOLE_GAME) {
        pos.x =  20.0f/640.0f;
        pos.y = 106.0f/480.0f;
    } else {
        pos.x =  50.0f/640.0f;
        pos.y = 452.0f/480.0f;
    }
    dim.x = (320.0f-pos.x)/640.0f;
    dim.y =  18.0f/480.0f;
    Ui::CEdit* pe = static_cast<Ui::CEdit*>(m_interface->CreateEdit(pos, dim, 0, EVENT_CMD));
    if (pe == nullptr) return;
    pe->ClearState(Ui::STATE_VISIBLE);
    m_visible = false;
}

bool CConsole::IsVisible()
{
    return m_visible;
}

void CConsole::Show()
{
    Ui::CEdit* pe = static_cast<Ui::CEdit*>(m_interface->SearchControl(EVENT_CMD));
    if (pe == nullptr) return;
    pe->SetState(Ui::STATE_VISIBLE);
    pe->SetFocus(true);
    m_pause->SetPause(PAUSE_CHEAT);

    m_visible = true;
}

void CConsole::Hide()
{
    Ui::CEdit* pe = static_cast<Ui::CEdit*>(m_interface->SearchControl(EVENT_CMD));
    if (pe == nullptr) return;
    pe->SetText("");
    pe->ClearState(Ui::STATE_VISIBLE);
    m_pause->ClearPause();

    m_visible = false;
}

void CConsole::Toggle()
{
    if(m_visible)
        Hide();
    else
        Show();
}

void CConsole::AddFunction(std::string name, Error (*func)(std::vector<std::string> params))
{
    m_functions[name] = func;
    CLogger::GetInstancePointer()->Debug("Console: Added function \"%s\"\n", name.c_str());
}

void CConsole::AddVariable(std::string name, ConsoleVariableType type, void* value, Error (*set_func)(ConsoleVariable, std::string))
{
    ConsoleVariable var;
    var.type = type;
    var.value = value;
    var.set = set_func;
    if(m_variables.find(name) == m_variables.end()) {
        CLogger::GetInstancePointer()->Debug("Console: Added variable \"%s\" (%s)\n", name.c_str(), GetVariableTypeAsString(type).c_str());
    } else {
        CLogger::GetInstancePointer()->Debug("Console: Updated variable \"%s\" (%s)\n", name.c_str(), GetVariableTypeAsString(type).c_str());
    }
    m_variables[name] = var;
}

void CConsole::AddVariable(std::string name, Error (*set_func)(ConsoleVariable, std::string))
{
    AddVariable(name, VARTYPE_NULL, nullptr, set_func);
}

void CConsole::AddVariable(std::string name, std::string* value, Error (*set_func)(ConsoleVariable, std::string))
{
    AddVariable(name, VARTYPE_STRING, value, set_func);
}

void CConsole::AddVariable(std::string name, int* value, Error (*set_func)(ConsoleVariable, std::string))
{
    AddVariable(name, VARTYPE_INT, value, set_func);
}

void CConsole::AddVariable(std::string name, long* value, Error (*set_func)(ConsoleVariable, std::string))
{
    AddVariable(name, VARTYPE_LONG, value, set_func);
}

void CConsole::AddVariable(std::string name, double* value, Error (*set_func)(ConsoleVariable, std::string))
{
    AddVariable(name, VARTYPE_DOUBLE, value, set_func);
}

void CConsole::AddVariable(std::string name, float* value, Error (*set_func)(ConsoleVariable, std::string))
{
    AddVariable(name, VARTYPE_FLOAT, value, set_func);
}

void CConsole::AddVariable(std::string name, bool* value, Error (*set_func)(ConsoleVariable, std::string))
{
    AddVariable(name, VARTYPE_BOOL, value, set_func);
}

void CConsole::AddVariableSetFunction(std::string name, Error (*set_func)(ConsoleVariable, std::string))
{
    m_variables[name].set = set_func;
}

void CConsole::AddAlias(std::string name, std::string code)
{
    m_aliases[name] = code;
    CLogger::GetInstancePointer()->Debug("Console: Added alias \"%s\" = \"%s\"\n", name.c_str(), code.c_str());
}

std::string CConsole::GetVariableTypeAsString(ConsoleVariableType type)
{
    switch(type) {
        case VARTYPE_STRING: return "string";
        case VARTYPE_INT:    return "int";
        case VARTYPE_LONG:   return "long";
        case VARTYPE_DOUBLE: return "double";
        case VARTYPE_FLOAT:  return "float";
        case VARTYPE_BOOL:   return "bool";
        default:
        case VARTYPE_NULL:   return "null";
    }
}

ConsoleVariable CConsole::GetVariable(std::string name)
{
    for(auto& it : m_variables) {
        if(name == it.first) {
            return it.second;
        }
    }
    
    ConsoleVariable var;
    var.type = VARTYPE_NULL;
    return var;
}

void CConsole::ProcessCommand(std::string input, bool first)
{
    if(first)
        CLogger::GetInstancePointer()->Info("Console: Command \"%s\"\n", input.c_str());
    else
        CLogger::GetInstancePointer()->Debug("Console: Command redirected to \"%s\"\n", input.c_str());
        
    std::vector<std::string> commands;
    boost::split(commands, input, boost::is_any_of("&"), boost::token_compress_on);
    if(commands.size() > 1) {
        for(auto& cmd : commands) {
            ProcessCommand(cmd, false);
        }
        return;
    }

    std::vector<std::string> command;
    boost::split(command, commands[0], boost::is_any_of(" "), boost::token_compress_on);
    
    for(auto& it : m_functions) {
        if(command[0] == it.first) {
            command.erase(command.begin());
            it.second(command);
            return;
        }
    }
    
    if(command.size() >= 3 && command[1] == "=") {
        ConsoleVariable var = GetVariable(command[0]);
        command.erase(command.begin());
        command.erase(command.begin());
        if(var.set == nullptr) {
            switch(var.type) {
                case VARTYPE_NULL:   CLogger::GetInstancePointer()->Error("Error in console command: tried to assign to NULL\n"); return;
                case VARTYPE_STRING: *(static_cast<std::string*>(var.value)) = boost::algorithm::join(command, " "); return;
                case VARTYPE_INT:    *(static_cast<int*>(var.value))         = boost::lexical_cast<int>(command[0]); return;
                case VARTYPE_LONG:   *(static_cast<long*>(var.value))        = boost::lexical_cast<long>(command[0]); return;
                case VARTYPE_DOUBLE: *(static_cast<double*>(var.value))      = boost::lexical_cast<double>(command[0]); return;
                case VARTYPE_FLOAT:  *(static_cast<float*>(var.value))       = boost::lexical_cast<float>(command[0]); return;
                case VARTYPE_BOOL:
                    if(command[0] == "true")
                        *(static_cast<bool*>(var.value)) = true;
                    else if(command[0] == "false")
                        *(static_cast<bool*>(var.value)) = false;
                    else
                        CLogger::GetInstancePointer()->Error("Error in console command: unable to interpret \"%s\" as boolean\n", command[0].c_str());
                    return;
                default: CLogger::GetInstancePointer()->Error("Error in console command: unknown variable type\n"); return;
            }
        } else {
            var.set(var, boost::algorithm::join(command, " "));
        }
    }
    
    for(auto& it : m_variables) {
        if(command[0] == it.first) {
            CLogger* log = CLogger::GetInstancePointer();
            std::string val;
            switch(it.second.type) {
                case VARTYPE_STRING: log->Info("%s = %s\n", it.first.c_str(), (*(static_cast<std::string*>(it.second.value))).c_str()); return;
                case VARTYPE_INT:    log->Info("%s = %d\n", it.first.c_str(), *(static_cast<int*>(it.second.value))); return;
                case VARTYPE_LONG:   log->Info("%s = %d\n", it.first.c_str(), *(static_cast<long*>(it.second.value))); return;
                case VARTYPE_DOUBLE: log->Info("%s = %f\n", it.first.c_str(), *(static_cast<double*>(it.second.value))); return;
                case VARTYPE_FLOAT:  log->Info("%s = %f\n", it.first.c_str(), *(static_cast<float*>(it.second.value))); return;
                case VARTYPE_BOOL:
                    if(*(static_cast<bool*>(it.second.value))) val = "true";
                    else val = "false";
                    log->Info("%s = %s\n", it.first.c_str(), val.c_str());
                    return;
                default:
                case VARTYPE_NULL:   log->Info("%s = (null)", it.first.c_str()); return;
            }
        }
    }
    
    for(auto& it : m_aliases) {
        if(command[0] == it.first) {
            ProcessCommand(it.second, false);
            return;
        }
    }
}

Error CConsole::toggle(std::vector<std::string> params)
{
    ConsoleVariable var = CConsole::GetInstancePointer()->GetVariable(params[0]);
    if(var.type != VARTYPE_BOOL) {
        CLogger::GetInstancePointer()->Error("Error in console command: You can use \"toggle\" only on boolean values\n");
        return ERR_GENERIC;
    }
    *(static_cast<bool*>(var.value)) = ! *(static_cast<bool*>(var.value));
    
    return ERR_OK;
}

Error CConsole::list(std::vector<std::string> params)
{
    if(params.size() < 1) {
        CLogger::GetInstancePointer()->Error("Usage: list [functions|variables|aliases]\n");
        return ERR_CMD;
    }
    
    CConsole* console = CConsole::GetInstancePointer();
    
    if(params[0] == "functions") {
        CLogger::GetInstancePointer()->Info("Available functions:\n");
        for(auto& it : console->m_functions) {
            CLogger::GetInstancePointer()->Info("%s\n", it.first.c_str());
        }
    }
    
    if(params[0] == "variables") {
        CLogger::GetInstancePointer()->Info("Available variables:\n");
        for(auto& it : console->m_variables) {
            CLogger::GetInstancePointer()->Info("%s (%s)\n", it.first.c_str(), GetVariableTypeAsString(it.second.type).c_str());
        }
    }
    
    if(params[0] == "aliases") {
        CLogger::GetInstancePointer()->Info("Available aliases:\n");
        for(auto& it : console->m_aliases) {
            CLogger::GetInstancePointer()->Info("%s - %s\n", it.first.c_str(), it.second.c_str());
        }
    }
    
    return ERR_OK;
}

Error CConsole::alias(std::vector<std::string> params)
{
    if(params.size() < 1) {
        CLogger::GetInstancePointer()->Error("Usage: alias [name] [command]\n");
        return ERR_CMD;
    }
    
    std::string name = params[0];
    params.erase(params.begin());
    std::string command = boost::algorithm::join(params, " ");
    
    CConsole::GetInstancePointer()->AddAlias(name, command);

    return ERR_OK;
}

Error CConsole::bit_or(std::vector<std::string> params)
{
    if(params.size() < 2) {
        CLogger::GetInstancePointer()->Error("Usage: bit_or [variable] [bitmask]\n");
        return ERR_CMD;
    }
    
    ConsoleVariable var = CConsole::GetInstancePointer()->GetVariable(params[0]);
    if(var.type != VARTYPE_INT && var.type != VARTYPE_LONG) {
        CLogger::GetInstancePointer()->Error("Wrong type of variable - must be int or long\n");
        return ERR_CMD;
    }
    
    std::bitset<sizeof(long)> x(params[1]);
    long bitmask = x.to_ulong();
    
    if(var.type == VARTYPE_INT)
        *(static_cast<int*>(var.value)) |= bitmask;
    else
        *(static_cast<long*>(var.value)) |= bitmask;
    
    return ERR_OK;
}

Error CConsole::bit_and(std::vector<std::string> params)
{
    if(params.size() < 2) {
        CLogger::GetInstancePointer()->Error("Usage: bit_and [variable] [bitmask]\n");
        return ERR_CMD;
    }
    
    ConsoleVariable var = CConsole::GetInstancePointer()->GetVariable(params[0]);
    if(var.type != VARTYPE_INT && var.type != VARTYPE_LONG) {
        CLogger::GetInstancePointer()->Error("Wrong type of variable - must be int or long\n");
        return ERR_CMD;
    }
    
    std::bitset<sizeof(long)> x(params[1]);
    long bitmask = x.to_ulong();
    
    if(var.type == VARTYPE_INT)
        *(static_cast<int*>(var.value)) &= bitmask;
    else
        *(static_cast<long*>(var.value)) &= bitmask;
    
    return ERR_OK;
}

Error CConsole::bit_clear(std::vector<std::string> params)
{
    if(params.size() < 2) {
        CLogger::GetInstancePointer()->Error("Usage: bit_clear [variable] [bitmask]\n");
        return ERR_CMD;
    }
    
    ConsoleVariable var = CConsole::GetInstancePointer()->GetVariable(params[0]);
    if(var.type != VARTYPE_INT && var.type != VARTYPE_LONG) {
        CLogger::GetInstancePointer()->Error("Wrong type of variable - must be int or long\n");
        return ERR_CMD;
    }
    
    std::bitset<sizeof(long)> x(params[1]);
    long bitmask = x.to_ulong();
    
    if(var.type == VARTYPE_INT)
        *(static_cast<int*>(var.value)) &= (~bitmask);
    else
        *(static_cast<long*>(var.value)) &= (~bitmask);
    return ERR_OK;
}
