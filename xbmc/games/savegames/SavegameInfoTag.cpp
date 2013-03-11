/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "SavegameInfoTag.h"
#include "utils/Variant.h"

void CSavegameInfoTag::Reset()
{
  m_path.clear();
  m_databaseId = -1;
  m_size = 0;
  m_romPath.clear();
  m_romCRC.clear();
  m_gameClient.clear();
  m_playtimeFrames = 0;
  m_playtimeWallClock = 0.0;
}

CSavegameInfoTag::CSavegameInfoTag(const CVariant &object)
{
  Deserialize(object);
}

const CSavegameInfoTag& CSavegameInfoTag::operator=(const CSavegameInfoTag& rhs)
{
  if (this != &rhs)
  {
    m_path              = rhs.m_path;
    m_databaseId        = rhs.m_databaseId;
    m_size              = rhs.m_size;
    m_romPath           = rhs.m_romPath;
    m_romCRC            = rhs.m_romCRC;
    m_gameClient        = rhs.m_gameClient;
    m_playtimeFrames    = rhs.m_playtimeFrames;
    m_playtimeWallClock = rhs.m_playtimeWallClock;
  }
  return *this;
}

void CSavegameInfoTag::Serialize(CVariant& value) const
{
  value["path"]              = m_path;
  value["databaseid"]        = m_databaseId;
  value["size"]              = m_size;
  value["rompath"]           = m_romPath;
  value["romcrc"]            = m_romCRC;
  value["gameclient"]        = m_gameClient;
  value["playtimeframes"]    = m_playtimeFrames;
  value["playtimewallclock"] = m_playtimeWallClock;
}

void CSavegameInfoTag::Deserialize(const CVariant& value)
{
  m_path              = value["path"].asString();
  m_databaseId        = value["databaseid"].asInteger();
  m_size              = value["size"].asUnsignedInteger();
  m_romPath           = value["rompath"].asString();
  m_romCRC            = value["romcrc"].asString();
  m_gameClient        = value["gameclient"].asString();
  m_playtimeFrames    = value["playtimeframes"].asUnsignedInteger();
  m_playtimeWallClock = value["playtimewallclock"].asDouble();
}
