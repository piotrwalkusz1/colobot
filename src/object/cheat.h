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
 * \file object/cheat.h
 * \brief Cheats for cheat console
 */
#pragma once

#include "app/app.h"

#include "common/singleton.h"

#include "object/console.h"

#include <string>
#include <vector>

class CCheat : public CSingleton<CCheat>
{
public:
    CCheat();
    ~CCheat();
    
private:
    std::string ToBitmask(long i);
    std::string ToString(EventType event);
    
private:
    static Error test(std::vector<std::string> params);
    static Error add_event(std::vector<std::string> params);
    static Error speed(ConsoleVariable var, std::string params);

public:
    bool m_trainerPilot;
    bool m_selectInsect;
};

