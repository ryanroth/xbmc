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

#include "Savegame.h"
#include "settings/Settings.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

void CSavegame::Reset()
{
  m_path.clear();
  m_databaseId = -1;
  m_size = 0;
  m_gamePath.clear();
  m_gameClient.clear();
  m_gameCRC.clear();
  m_playtimeFrames = 0;
  m_playtimeWallClock = 0.0;
}

CSavegame::CSavegame(const CVariant &object)
{
  Deserialize(object);
}

const CSavegame& CSavegame::operator=(const CSavegame& rhs)
{
  if (this != &rhs)
  {
    m_path              = rhs.m_path;
    m_databaseId        = rhs.m_databaseId;
    m_size              = rhs.m_size;
    m_gamePath          = rhs.m_gamePath;
    m_gameClient        = rhs.m_gameClient;
    m_gameCRC           = rhs.m_gameCRC;
    m_playtimeFrames    = rhs.m_playtimeFrames;
    m_playtimeWallClock = rhs.m_playtimeWallClock;
  }
  return *this;
}

size_t CSavegame::Read(uint8_t **data)
{
  if (!VerifyPath())
    return false;

  if (m_size)
  {
    *data = new uint8_t[m_size];
    if (*data)
    {
      // Read
    }
    return m_size;
  }
  return 0;
}

void CSavegame::Write(const uint8_t *data, size_t size) const
{
  if (!VerifyPath())
    return;

}

bool CSavegame::VerifyPath() const
{
  if (!m_path.empty())
    return true;
  if (!m_gameClient.empty() && !m_gameCRC.empty())
  {
    CStdString hash;
    hash.Format("%s/%c/%s.savestate", m_gameClient.c_str(), m_gameCRC[0], m_gameCRC.c_str());
    URIUtils::AddFileToFolder(g_settings.GetSavegamesFolder(), hash, m_path);
    return true;
  }
  return false;
}

void CSavegame::Serialize(CVariant& value) const
{
  VerifyPath();
  value["path"]              = m_path;
  value["databaseid"]        = m_databaseId;
  value["size"]              = m_size;
  value["gamepath"]          = m_gamePath;
  value["gameclient"]        = m_gameClient;
  value["gamecrc"]           = m_gameCRC;
  value["playtimeframes"]    = m_playtimeFrames;
  value["playtimewallclock"] = m_playtimeWallClock;
}

void CSavegame::Deserialize(const CVariant& value)
{
  m_path              = value["path"].asString();
  m_databaseId        = value["databaseid"].asInteger();
  m_size              = value["size"].asUnsignedInteger();
  m_gamePath          = value["gamepath"].asString();
  m_gameClient        = value["gameclient"].asString();
  m_gameCRC           = value["gamecrc"].asString();
  m_playtimeFrames    = value["playtimeframes"].asUnsignedInteger();
  m_playtimeWallClock = value["playtimewallclock"].asDouble();
}
