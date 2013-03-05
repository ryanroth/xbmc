/*
*      Copyright (C) 2007-2013 Team XBMC
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
*  Parts of this file are Copyright (C) 2009 Stephen Kitt <steve@sk2.org>
*/

#include "LinuxJoystick.h"
#include "utils/log.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/joystick.h>
#include <sstream>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

/* The following values come from include/input.h in the kernel
   source; the small variant is used up to version 2.6.27, the large
   one from 2.6.28 onwards. We need to handle both values because the
   kernel doesn't; it only expects one of the values, and we need to
   determine which one at run-time. */
#define KEY_MAX_LARGE 0x2FF
#define KEY_MAX_SMALL 0x1FF

/* Axis map size. */
#define AXMAP_SIZE (ABS_MAX + 1)

/* Button map size. */
#define BTNMAP_SIZE (KEY_MAX_LARGE - BTN_MISC + 1)


/* The following values come from include/joystick.h in the kernel source. */
#define JSIOCSBTNMAP_LARGE _IOW('j', 0x33, __u16[KEY_MAX_LARGE - BTN_MISC + 1])
#define JSIOCSBTNMAP_SMALL _IOW('j', 0x33, __u16[KEY_MAX_SMALL - BTN_MISC + 1])
#define JSIOCGBTNMAP_LARGE _IOR('j', 0x34, __u16[KEY_MAX_LARGE - BTN_MISC + 1])
#define JSIOCGBTNMAP_SMALL _IOR('j', 0x34, __u16[KEY_MAX_SMALL - BTN_MISC + 1])

using namespace std;

static const char *axis_names[ABS_MAX + 1] =
{
"X",     "Y",     "Z",     "Rx",    "Ry",    "Rz",    "Throttle", "Rudder",
"Wheel", "Gas",   "Brake", "?",     "?",     "?",     "?",        "?",
"Hat0X", "Hat0Y", "Hat1X", "Hat1Y", "Hat2X", "Hat2Y", "Hat3X",    "Hat3Y",
"?",     "?",     "?",      "?",    "?",     "?",     "?",
};

static const char *button_names[KEY_MAX - BTN_MISC + 1] =
{
"Btn0",     "Btn1",     "Btn2",      "Btn3",    "Btn4",     "Btn5",       "Btn6",    "Btn7",     "Btn8",     "Btn9",     "?",         "?",        "?",       "?",         "?",         "?",
"LeftBtn",  "RightBtn", "MiddleBtn", "SideBtn", "ExtraBtn", "ForwardBtn", "BackBtn", "TaskBtn",  "?",        "?",        "?",         "?",        "?",       "?",         "?",         "?",
"Trigger",  "ThumbBtn", "ThumbBtn2", "TopBtn",  "TopBtn2",  "PinkieBtn",  "BaseBtn", "BaseBtn2", "BaseBtn3", "BaseBtn4", "BaseBtn5",  "BaseBtn6", "BtnDead",
"BtnA",     "BtnB",     "BtnC",      "BtnX",    "BtnY",     "BtnZ",       "BtnTL",   "BtnTR",    "BtnTL2",   "BtnTR2",   "BtnSelect", "BtnStart", "BtnMode", "BtnThumbL", "BtnThumbR", "?",
"?",        "?",        "?",         "?",       "?",        "?",          "?",       "?",        "?",        "?",        "?",         "?",        "?",       "?",         "?",         "?",
"WheelBtn", "Gear up",
};

CLinuxJoystick::CLinuxJoystick(int fd, unsigned int id, const char *name, unsigned char buttons, unsigned char axes)
  : m_state(), m_fd(fd)
{
  m_state.id          = id;
  m_state.name        = name;
  m_state.buttonCount = std::min(m_state.buttonCount, (unsigned int)buttons);
  m_state.hatCount    = 0; // TODO: Translate hat buttons into hats
  m_state.axisCount   = std::min(m_state.axisCount, (unsigned int)axes);
}

CLinuxJoystick::~CLinuxJoystick()
{
  close(m_fd);
}

/* static */
void CLinuxJoystick::Initialize(JoystickArray &joysticks)
{
  string inputDir("/dev/input");
  DIR *pd = opendir(inputDir.c_str());
  if (pd == NULL)
  {
    CLog::Log(LOGERROR, /* __FUNCTION__ */ "CLinuxJoystick::Initialize: can't open /dev/input (errno=%d)", errno);
    return;
  }

  dirent *pDirent;
  while ((pDirent = readdir(pd))!= NULL)
  {
    if (strncmp(pDirent->d_name, "js", 2) == 0)
    {
      // Found a joystick device
      string filename(inputDir + "/" + pDirent->d_name);
      CLog::Log(LOGINFO, /* __FUNCTION__ */ "CLinuxJoystick::Initialize: opening joystick %s", filename.c_str());

      int fd = open(filename.c_str(), O_RDONLY);
      if (fd < 0)
      {
        CLog::Log(LOGERROR, /* __FUNCTION__ */ "CLinuxJoystick::Initialize: can't open %s (errno=%d)", filename.c_str(), errno);
        continue;
      }

      unsigned char axes = 2;
      unsigned char buttons = 2;
      int version = 0x000800;
      char name[128] = "Unknown";

      if (ioctl(fd, JSIOCGVERSION, &version) < 0 ||
          ioctl(fd, JSIOCGAXES, &axes) < 0 ||
          ioctl(fd, JSIOCGBUTTONS, &buttons) < 0 ||
          ioctl(fd, JSIOCGNAME(128), name) < 0)
      {
        CLog::Log(LOGERROR, /* __FUNCTION__ */ "CLinuxJoystick::Initialize: failed ioctl() (errno=%d)", errno);
        continue;
      }

      // We don't support the old (0.x) interface
      if (version < 0x010000)
      {
        CLog::Log(LOGERROR, /* __FUNCTION__ */ "CLinuxJoystick::Initialize: old (0.x) interface is not supported (version=%08x)", version);
        continue;
      }

      uint16_t btnmap[BTNMAP_SIZE];
      uint8_t axmap[AXMAP_SIZE];

      GetButtonMap(fd, btnmap);
      GetAxisMap(fd, axmap);

      CLog::Log(LOGINFO, /* __FUNCTION__ */ "CLinuxJoystick::Initialize: driver version is %d.%d.%d",
          version >> 16, (version >> 8) & 0xff, version & 0xff);

      /* Determine whether the button map is usable. */
      int btnmapok = 1;
      for (int i = 0; btnmapok && i < buttons; i++)
      {
        if (btnmap[i] < BTN_MISC || btnmap[i] > KEY_MAX)
        {
          btnmapok = 0;
          break;
        }
      }

      if (!btnmapok)
      {
        /* btnmap out of range for names. Don't print any. */
        CLog::Log(LOGINFO, /* __FUNCTION__ */ "CLinuxJoystick::Initialize: XBMC is not fully compatible with your kernel. Unable to retrieve button map!");
        CLog::Log(LOGINFO, "CLinuxJoystick::Initialize: Joystick \"%s\" has %d buttons and %d axes", name, buttons, axes);
      }
      else
      {
        ostringstream strButtons;
        for (int i = 0; i < buttons; i++)
        {
          strButtons << button_names[btnmap[i] - BTN_MISC];
          if (i < buttons - 1)
            strButtons << ", ";
        }
        CLog::Log(LOGINFO, "Buttons: %s", strButtons.str().c_str());

        ostringstream strAxes;
        for (int i = 0; i < axes; i++)
        {
          strAxes << axis_names[axmap[i]];
          if (i < axes - 1)
            strAxes << ", ";
        }
        CLog::Log(LOGINFO, "Axes: %s", strAxes.str().c_str());
      }

      // Got enough information, time to move on to the next joystick
      joysticks.push_back(boost::shared_ptr<IJoystick>(new CLinuxJoystick(fd,
          joysticks.size(), name, buttons, axes)));
    }
  }
  (void)closedir(pd);
}

/* static */
int CLinuxJoystick::DetermineIoctl(int fd, int *ioctls, int *ioctl_used, void *argp)
{
  int retval = 0;

  /* Try each ioctl in turn. */
  for (int i = 0; ioctls[i]; i++)
  {
    if ((retval = ioctl(fd, ioctls[i], argp)) >= 0)
    {
      /* The ioctl did something. */
      *ioctl_used = ioctls[i];
      return retval;
    }
    else if (errno != -EINVAL)
    {
      /* Some other error occurred. */
      return retval;
     }
  }
  return retval;
}

/**
 * Retrieves the current button map in the given array, which must contain at
 * least BTNMAP_SIZE elements. Returns the result of the ioctl(): negative in
 * case of an error, 0 otherwise for kernels up to 2.6.30, the length of the
 * array actually copied for later kernels.
 */
int CLinuxJoystick::GetButtonMap(int fd, uint16_t *btnmap)
{
  static int jsiocsbtnmap = 0;
  int ioctls[] = { JSIOCGBTNMAP, JSIOCGBTNMAP_LARGE, JSIOCGBTNMAP_SMALL, 0 };

  if (jsiocsbtnmap != 0)
  {
    /* We already know which ioctl to use. */
    return ioctl(fd, jsiocsbtnmap, btnmap);
  }
  else
  {
    return DetermineIoctl(fd, ioctls, &jsiocsbtnmap, btnmap);
  }
}

/**
 * Retrieves the current axis map in the given array, which must contain at
 * least AXMAP_SIZE elements. Returns the result of the ioctl(): negative in
 * case of an error, 0 otherwise for kernels up to 2.6.30, the length of the
 * array actually copied for later kernels.
 */
int CLinuxJoystick::GetAxisMap(int fd, uint8_t *axmap)
{
  return ioctl(fd, JSIOCGAXMAP, axmap);
}

/* static */
void CLinuxJoystick::DeInitialize(JoystickArray &joysticks)
{
}

void CLinuxJoystick::Update()
{
}
