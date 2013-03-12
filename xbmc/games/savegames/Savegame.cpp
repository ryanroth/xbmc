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
#include "filesystem/File.h"
#include "settings/Settings.h"
#include "utils/Crc32.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

using namespace XFILE;

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

bool CSavegame::Read(std::vector<uint8_t> &data)
{
  if (!VerifyPath())
    return false;

  CFile file;
  if (file.Open(m_path) && file.GetLength() > 0)
  {
    data.resize((size_t)file.GetLength());
    file.Read(data.data(), data.size());
    return true;
  }
  return false;
}

bool CSavegame::Write(std::vector<uint8_t> &data) const
{
  if (!VerifyPath())
    return false;

  CFile file;
  if (file.OpenForWrite(m_path))
    return file.Write(data.data(), data.size()) == data.size();
  return false;
}

bool CSavegame::VerifyPath() const
{
  if (!m_path.empty())
    return true;
  if (!m_gameClient.empty() && !m_gameCRC.empty())
  {
    CStdString hash;
    hash.Format("%s/%s.savestate", m_gameClient.c_str(), m_gameCRC.c_str());
    URIUtils::AddFileToFolder(g_settings.GetSavegamesFolder(), hash, m_path);
    return true;
  }
  return false;
}

void CSavegame::SetGameCRCFromFile(const CStdString &filename)
{
  std::vector<char> buffer;
  CFile file;
  int64_t length;
  if (file.Open(filename) && (length = file.GetLength()) > 0)
  {
    buffer.resize((size_t)length);
    file.Read(buffer.data(), length);
    SetGameCRCFromFile(buffer.data(), buffer.size());
  }
}

void CSavegame::SetGameCRCFromFile(const char *data, size_t length)
{
  Crc32 crc;
  crc.Compute(data, length);
  m_gameCRC.Format("%08x", (unsigned __int32)crc);
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
  m_databaseId        = (int)value["databaseid"].asInteger();
  m_size              = (size_t)value["size"].asUnsignedInteger();
  m_gamePath          = value["gamepath"].asString();
  m_gameClient        = value["gameclient"].asString();
  m_gameCRC           = value["gamecrc"].asString();
  m_playtimeFrames    = value["playtimeframes"].asUnsignedInteger();
  m_playtimeWallClock = value["playtimewallclock"].asDouble();
}
