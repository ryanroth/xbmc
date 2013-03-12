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

#include <stdint.h>
#include <vector>

class CDateTime;

class CSavegame : public ISerializable, public IDeserializable
{
public:
  CSavegame() { Reset(); }
  CSavegame(const CVariant &object);
  void Reset();
  CSavegame(const CSavegame &other) { *this = other; }
  const CSavegame& operator=(const CSavegame &rhs);
  
  bool Read(std::vector<uint8_t> &data);
  void Write(const std::vector<uint8_t> &data) const;

  virtual void Serialize(CVariant& value) const;
  virtual void Deserialize(const CVariant& value);
  
  // Serializable information

  /**
   * Path to savegame is derived from game client and game CRC. Returns empty
   * if these are unknown.
   */
  const CStdString &GetPath() const { VerifyPath(); return m_path; }

  int GetDatabaseId() const { return m_databaseId; }
  void SetDatabaseId(int databaseId) { m_databaseId = databaseId; }
  bool IsDatabaseObject() const { return m_databaseId != -1; }

  size_t GetSize() const { return m_size; } // excluding header
  void SetSize(size_t size) { m_size = size; }

  const CStdString &GetGamePath() const { return m_gamePath; }
  void SetGamePath(const CStdString &gamePath) { m_gamePath = gamePath; }

  const CStdString &GetGameClient() const { return m_gameClient; }
  void SetGameClient(const CStdString &gameClient) { m_gameClient = gameClient; }
  
  const CStdString &GetGameCRC() const { return m_gameCRC; }
  void SetGameCRC(const CStdString &gameCRC) { m_gameCRC = gameCRC; }
  
  uint64_t GetPlaytimeFrames() const { return m_playtimeFrames; }
  void SetPlaytimeFrames(uint64_t playtimeFrames) { m_playtimeFrames = playtimeFrames; }

  double GetPlaytimeWallClock() const { return m_playtimeWallClock; } // seconds
  void SetPlaytimeWallClock(double playtimeWallClock) { m_playtimeWallClock = playtimeWallClock; } // seconds

private:
  /**
   * Generates and stores the path to the savegame. Return false if the path
   * remains invalid.
   */
  bool VerifyPath() const;

  static const unsigned int VERSION = 1;

  mutable CStdString m_path;
  
  int        m_databaseId;
  size_t     m_size; // excluding XBMC header
  CStdString m_gamePath;
  CStdString m_gameClient;
  CStdString m_gameCRC;
  uint64_t   m_playtimeFrames;
  double     m_playtimeWallClock; // seconds
};
