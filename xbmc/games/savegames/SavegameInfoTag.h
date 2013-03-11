#pragma once
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

#include "utils/IDeserializable.h"
#include "utils/ISerializable.h"
#include "utils/StdString.h"

class CDateTime;

class CSavegameInfoTag : public ISerializable, public IDeserializable
{
public:
  CSavegameInfoTag() { Reset(); }
  void Reset();
  CSavegameInfoTag(const CSavegameInfoTag &other) { *this = other; }
  const CSavegameInfoTag& operator=(const CSavegameInfoTag &rhs);
  
  virtual void Serialize(CVariant& value) const;
  virtual void Deserialize(const CVariant& value);
  
  const CStdString &GetPath() const { return m_path; }
  void SetPath(const CStdString &path) { m_path = path; }

  const int GetDatabaseId() const { return m_databaseId; }
  void SetDatabaseId(int databaseId) { m_databaseId = databaseId; }

  const unsigned int GetSize() const { return m_size; } // excluding header
  void SetSize(unsigned int size) { m_size = size; }

  const CStdString &GetRomPath() const { return m_romPath; }
  void SetRomPath(const CStdString &romPath) { m_romPath = romPath; }

  const CStdString &GetRomCRC() const { return m_romCRC; }
  void SetRomCRC(const CStdString &romCRC) { m_romCRC = romCRC; }
  
  const CStdString &GetGameClient() const { return m_gameClient; }
  void SetGameClient(const CStdString &gameClient) { m_gameClient = gameClient; }
  
  const uint64_t GetPlaytimeFrames() const { return m_playtimeFrames; }
  void SetPlaytimeFrames(uint64_t playtimeFrames) { m_playtimeFrames = playtimeFrames; }

  const double GetPlaytimeWallClock() const { return m_playtimeWallClock; } // seconds
  void SetPlaytimeWallClock(double playtimeWallClock) { m_playtimeWallClock = playtimeWallClock; } // seconds

private:
  CStdString   m_path;
  int          m_databaseId;
  unsigned int m_size; // excluding header
  CStdString   m_romPath;
  CStdString   m_romCRC;
  CStdString   m_gameClient;
  uint64_t     m_playtimeFrames;
  double       m_playtimeWallClock; // seconds
};
