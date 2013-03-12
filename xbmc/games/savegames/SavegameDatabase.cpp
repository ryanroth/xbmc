/*
 *      Copyright (C) 2012 Team XBMC
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

#include "SavegameDatabase.h"
#include "Savegame.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "FileItem.h"

//#include <vector>
//#include <set>
//#include <queue>

using namespace std;

CSavegameDatabase::CSavegameDatabase() : CDynamicDatabase("savegame")
{
  BeginDeclarations();
  DeclareIndex("path", "VARCHAR(512)");
  DeclareIndex("gamepath", "VARCHAR(512)");
  DeclareIndex("gamecrc", "CHAR(8)");
  DeclareOneToMany("gameclient", "CHAR(8)");
}

bool CSavegameDatabase::Open()
{
  return CDynamicDatabase::Open(g_advancedSettings.m_databaseSavegames);
}

bool CSavegameDatabase::CreateTables()
{
  try
  {
    BeginTransaction();
    if (!CDynamicDatabase::CreateTables())
      return false;

    return true;
  }
  catch (dbiplus::DbErrors&)
  {
    CLog::Log(LOGERROR, "SavegameDatabase: unable to create tables (error %i)", (int)GetLastError());
    RollbackTransaction();
  }
  return false;
}

bool CSavegameDatabase::UpdateOldVersion(int version)
{
  if (version < 1)
  {
    BeginDeclarations();
    DeclareIndex("file", "VARCHAR(128)");
    DeclareOneToMany("path", "VARCHAR(512)");
    DeclareOneToMany("folder", "VARCHAR(64)");
    DeclareOneToMany("year", "INTEGER");
    DeclareOneToMany("camera", "VARCHAR(72)");
  }
  return true;
}

bool CSavegameDatabase::Exists(const CVariant &object, int &idObject)
{
  if (!IsValid(object))
    return false;
  CStdString strSQL = PrepareSQL(
    "SELECT savegame.idsavegame "
    "FROM savegame "
    "WHERE path='%s'",
    object["path"].asString().c_str()
  );
  if (m_pDS->query(strSQL.c_str()))
  {
    bool bFound = false;
    if (m_pDS->num_rows() != 0)
    {
      idObject = m_pDS->fv(0).get_asInt();
      bFound = true;
    }
    m_pDS->close();
    return bFound;
  }
  return false;
}

bool CSavegameDatabase::IsValid(const CVariant &object) const
{
  return !object["path"].asString().empty();
}

CFileItem* CSavegameDatabase::CreateFileItem(const CVariant &object, int id) const
{
  CSavegame p(object);
  CFileItem *item = new CFileItem(p.GetGamePath(), false);
  item->SetPath(p.GetPath());
  item->m_dwSize = p.GetSize();
  item->m_bIsFolder = false;
  return item;
}
