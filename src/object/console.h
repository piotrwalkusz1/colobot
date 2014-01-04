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

/**
 * \file object/console.h
 * \brief Cheat console
 */
#pragma once

#include "app/pausemanager.h"

#include "common/global.h"
#include "common/singleton.h"
#include "common/event.h"

#include <string>
#include <vector>
#include <map>

namespace Ui {
    class CInterface;
}

enum ConsoleMode {
    CONSOLE_GAME,
    CONSOLE_MENU
};

enum ConsoleVariableType {
    VARTYPE_NULL = 0,
    VARTYPE_STRING,
    VARTYPE_INT,
    VARTYPE_LONG,
    VARTYPE_DOUBLE,
    VARTYPE_FLOAT,
    VARTYPE_BOOL
};

struct ConsoleVariable {
    ConsoleVariableType type;
    void* value;
};

class CConsole : public CSingleton<CConsole>
{
public:
    CConsole();
    ~CConsole();
    
    bool ProcessEvent(const Event &event);
    void SetMode(ConsoleMode mode);
    
    void Create();
    void Show();
    void Hide();
    void Toggle();
    bool IsVisible();
    
    void AddFunction(std::string name, Error (*func)(std::vector<std::string> params));
    void AddVariable(std::string name, std::string* var);
    void AddVariable(std::string name, int* var);
    void AddVariable(std::string name, long* var);
    void AddVariable(std::string name, double* var);
    void AddVariable(std::string name, float* var);
    void AddVariable(std::string name, bool* var);
    void AddAlias(std::string name, std::string cmd);
    
    ConsoleVariable GetVariable(std::string name);
    
private:
    void ProcessCommand(std::string input, bool first=true);
    
private:
    static Error toggle(std::vector<std::string> params);
    static Error list(std::vector<std::string> params);
    static Error alias(std::vector<std::string> params);
    static Error bit_or(std::vector<std::string> params);
    static Error bit_and(std::vector<std::string> params);
    static Error bit_clear(std::vector<std::string> params);
    
private:
    Ui::CInterface* m_interface;
    CPauseManager*  m_pause;
    
    bool m_visible;
    ConsoleMode m_mode;
    
    std::map<std::string, Error (*)(std::vector<std::string> params)> m_functions;
    std::map<std::string, ConsoleVariable> m_variables;
    std::map<std::string, std::string> m_aliases;
};

