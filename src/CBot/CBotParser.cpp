/*
 * This file is part of the Colobot: Gold Edition source code
 * Copyright (C) 2001-2016, Daniel Roux, EPSITEC SA & TerranovaTeam
 * http://epsitec.ch; http://colobot.info; http://github.com/colobot
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

#include "CBot/CBotParser.h"

namespace CBot
{

bool CBotParser::IsClass(CBotToken* p)
{
    SkipAllModifiers(p);

    return IsOfType(p, ID_CLASS);
}

bool CBotParser::IsFunctionDefinition(CBotToken* p)
{
    SkipAllModifiers(p);

    SkipTokens(p, {TokenTypVar, ID_DBLDOTS});

    if (!IsOfType(p, ID_OPENPAR)) return false;

    SkipTokens(p, {TokenTypVar, ID_COMMA});

    return IsOfType(p, ID_CLOSEPAR) && IsOfType(p, ID_OPBLK);
}

bool CBotParser::IsFunctionCall(CBotToken* p)
{
    return IsOfType(p, TokenTypVar) && IsOfType(p, ID_OPENPAR);
}

ProtectionLevel CBotParser::ReadAccessModifier(CBotToken* &p)
{
    if      ( IsOfType(p, ID_PUBLIC)    ) return ProtectionLevel::Public;
    else if ( IsOfType(p, ID_PROTECTED) ) return ProtectionLevel::Protected;
    else if ( IsOfType(p, ID_PRIVATE)   ) return ProtectionLevel::Private;
    else                                  return ProtectionLevel::None;
}

bool CBotParser::RequireType(CBotToken* &p, int type, CBotCStack* stack, CBotError error)
{
    if (IsOfType(p, type)) return false;
    else
    {
        stack->SetError(error, p);
        return true;
    }
}

bool CBotParser::ValidateBlock(CBotToken* &p, CBotCStack* stack)
{
    CBotToken* openBlock = p;

    if (RequireType(p, ID_OPBLK, stack, CBotErrOpenBlock)) return false;

    bool success = TrySkipToExitBlock(p);

    if (!success)
    {
        stack->SetError(CBotErrCloseBlock, openBlock);
        return false;
    }
    else return true;
}

void CBotParser::SkipClassOrFunction(CBotToken* &p)
{
    while (p != nullptr && p->GetType() != TokenTypNone)
    {
        if (p->GetType() == ID_OPBLK) break;
        else p = p->GetNext();
    }

    p = p->GetNext(); // skip '{'

    TrySkipToExitBlock(p);
}

bool CBotParser::TrySkipToExitBlock(CBotToken* &p)
{
    int level = 1;
    while (level > 0 && p != nullptr)
    {
        int type = p->GetType();
        if (type == ID_OPBLK) level++;
        if (type == ID_CLBLK) level--;
        p = p->GetNext();
    }

    return level == 0;
}

void CBotParser::SkipAllModifiers(CBotToken* &p)
{
    SkipTokens(p, {ID_PUBLIC, ID_PROTECTED, ID_PRIVATE, ID_EXTERN, ID_SYNCHO, ID_STATIC});
}

void CBotParser::SkipTokens(CBotToken* &p, std::vector<int> tokens)
{
    unsigned int i = 0;

    while (i < tokens.size() && p != nullptr)
    {
        if (IsOfType(p, tokens[i])) i = 0;
        else i++;
    }
}

} // namespace CBot




