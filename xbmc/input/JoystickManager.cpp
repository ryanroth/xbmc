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
 */

#include "system.h" // for HAS_SDL_JOYSTICK
#if defined(HAS_SDL_JOYSTICK)

#include "JoystickManager.h"
#include "Application.h"
#include "ButtonTranslator.h"
#include "utils/log.h"
#include "MouseStat.h"

// Include joystick APIs
#if defined(TARGET_WINDOWS)
#include "input/windows/WINJoystickXInput.h"
#include "input/windows/WINJoystickDX.h"
#elif defined(TARGET_LINUX)
#include "input/linux/LinuxJoystick.h"
#if defined(HAS_SDL_JOYSTICK)
#include "input/linux/LinuxJoystickSDL.h"
#endif
#endif // TARGET_LINUX


#define OCCURED(time) ((time) <= XbmcThreads::SystemClockMillis())
#define JOY_ARRAY_LENGTH(x) (sizeof((x)) / sizeof((x)[0]))
#define ABS(X) ((X) >= 0 ? (X) : (-(X)))

#define ACTION_FIRST_DELAY      500 // ms
#define ACTION_REPEAT_DELAY     100 // ms
// Axis must be pushed past this for digital action repeats
#define AXIS_DIGITAL_DEADZONE  0.5f


CJoystickManager &CJoystickManager::Get()
{
  static CJoystickManager joystickManager;
  return joystickManager;
}

void CJoystickManager::Initialize()
{
  if (!IsEnabled())
    return;
  
  // Initialize joystick APIs
#if defined(TARGET_WINDOWS)
  CJoystickXInput::Initialize(m_joysticks);
  CJoystickDX::Initialize(m_joysticks);
#elif defined(TARGET_LINUX)
  CLinuxJoystick::Initialize(m_joysticks);
#if defined(HAS_SDL_JOYSTICK)
  CLinuxJoystickSDL::Initialize(m_joysticks);
#endif
#endif // TARGET_LINUX

  while (m_joysticks.size() > JOY_ARRAY_LENGTH(m_states))
    m_joysticks.pop_back();

  for (unsigned int i = 0; i < JOY_ARRAY_LENGTH(m_states); i++)
    m_states[i].Reset();
}

void CJoystickManager::DeInitialize()
{
  // De-initialize joystick APIs
#if defined(TARGET_WINDOWS)
  CJoystickXInput::DeInitialize(m_joysticks);
  CJoystickDX::DeInitialize(m_joysticks);
#elif defined(TARGET_LINUX)
  CLinuxJoystick::DeInitialize(m_joysticks);
#if defined(HAS_SDL_JOYSTICK)
  CLinuxJoystickSDL::DeInitialize(m_joysticks);
#endif
#endif // TARGET_LINUX

  for (unsigned int i = 0; i < JOY_ARRAY_LENGTH(m_states); i++)
    m_states[i].Reset();
}

void CJoystickManager::Update()
{
  if (!IsEnabled())
    return;

  for (JoystickArray::iterator it = m_joysticks.begin(); it != m_joysticks.end(); it++)
    (*it)->Update();
  ProcessStateChanges();
}

void CJoystickManager::ProcessStateChanges()
{
  for (unsigned int joyID = 0; joyID < m_joysticks.size(); joyID++)
  {
    ProcessButtonPresses(m_states[joyID], m_joysticks[joyID]->GetState(), joyID);
    ProcessHatPresses(m_states[joyID], m_joysticks[joyID]->GetState(), joyID);
    ProcessAxisMotion(m_states[joyID], m_joysticks[joyID]->GetState(), joyID);
  }

  // If tracking an action and the time has elapsed, execute the action now
  if (m_actionTracker.actionID && OCCURED(m_actionTracker.targetTime))
  {
    CAction action(m_actionTracker.actionID, 1.0f, 0.0f, m_actionTracker.name);
    g_application.ExecuteInputAction(action);
    m_actionTracker.Track(action); // Update the timer
  }

  // Reset the wakeup check, so that the check will be performed next button press also
  ResetWakeup();
}

void CJoystickManager::ProcessButtonPresses(SJoystick &oldState, const SJoystick &newState, unsigned int joyID)
{
  for (unsigned int i = 0; i < newState.buttonCount; i++)
  {
    if (oldState.buttons[i] == newState.buttons[i])
      continue;
    oldState.buttons[i] = newState.buttons[i];

    CLog::Log(LOGDEBUG, "Joystick %d button %d %s", joyID, i + 1, newState.buttons[i] ? "pressed" : "unpressed");

    // Check to see if an action is registered for the button first
    int        actionID;
    CStdString actionName;
    bool       fullrange;
    // Button ID is i + 1
    if (!CButtonTranslator::GetInstance().TranslateJoystickString(g_application.GetActiveWindowID(),
      newState.name.c_str(), i + 1, JACTIVE_BUTTON, actionID, actionName, fullrange))
    {
      CLog::Log(LOGDEBUG, "-> Joystick %d button %d no registered action", joyID, i + 1);
      continue;
    }
    g_Mouse.SetActive(false);

    // Ignore all button presses during this ProcessStateChanges() if we woke
    // up the screensaver (but always send joypad unpresses)
    if (!Wakeup() && newState.buttons[i])
    {
      CAction action(actionID, 1.0f, 0.0f, actionName);
      g_application.ExecuteInputAction(action);
      // Track the button press for deferred repeated execution
      m_actionTracker.Track(action);
    }
    else if (!newState.buttons[i])
    {
      m_actionTracker.Reset(); // If a button was released, reset the tracker
    }
  }
}

void CJoystickManager::ProcessHatPresses(SJoystick &oldState, const SJoystick &newState, unsigned int joyID)
{
  for (unsigned int i = 0; i < newState.hatCount; i++)
  {
    SHat &oldHat = oldState.hats[i];
    const SHat &newHat = newState.hats[i];
    if (oldHat == newHat)
      continue;

    CLog::Log(LOGDEBUG, "Joystick %d hat %d new direction: %s", joyID, i + 1, newHat.GetDirection());

    // Up, right, down, left
    for (unsigned int j = 0; j < 4; j++)
    {
      if (oldHat[j] == newHat[j])
        continue;
      oldHat[j] = newHat[j];

      int        actionID;
      CStdString actionName;
      bool       fullrange;
      // Up is (1 << 0), right (1 << 1), down (1 << 2), left (1 << 3). Hat ID is i + 1
      int buttonID = (1 << j) << 16 | (i + 1);
      if (!buttonID || !CButtonTranslator::GetInstance().TranslateJoystickString(g_application.GetActiveWindowID(),
        newState.name.c_str(), buttonID, JACTIVE_HAT, actionID, actionName, fullrange))
      {
        static const char *dir[] = {"UP", "RIGHT", "DOWN", "LEFT"};
        CLog::Log(LOGDEBUG, "-> Joystick %d hat %d direction %s no registered action", joyID, i + 1, dir[j]);
        continue;
      }
      g_Mouse.SetActive(false);

      // Ignore all button presses during this ProcessStateChanges() if we woke
      // up the screensaver (but always send joypad unpresses)
      if (!Wakeup() && newHat[j])
      {
        CAction action(actionID, 1.0f, 0.0f, actionName);
        g_application.ExecuteInputAction(action);
        // Track the hat press for deferred repeated execution
        m_actionTracker.Track(action);
      }
      else if (!newHat[j])
      {
        // If a hat was released, reset the tracker
        m_actionTracker.Reset();
      }
    }
  }
}

void CJoystickManager::ProcessAxisMotion(SJoystick &oldState, const SJoystick &newState, unsigned int joyID)
{
  for (unsigned int i = 0; i < newState.axisCount; i++)
  {
    // Absolute magnitude
    float absAxis = ABS(newState.axes[i]);

    // Only send one "centered" message
    if (absAxis < 0.01f)
    {
      if (ABS(oldState.axes[i]) < 0.01f)
      {
        // The values might not have been exactly equal, so make them
        oldState.axes[i] = newState.axes[i];
        continue;
      }
      CLog::Log(LOGDEBUG, "Joystick %d axis %d centered", joyID, i + 1);
    }
    // Note: don't overwrite oldState until we know whether the action is analog or digital

    int        actionID;
    CStdString actionName;
    bool       fullrange;
    // Axis ID is i + 1, and negative if newState.axes[i] < 0
    if (!CButtonTranslator::GetInstance().TranslateJoystickString(g_application.GetActiveWindowID(),
      newState.name.c_str(), newState.axes[i] >= 0.0f ? (i + 1) : -(int)(i + 1), JACTIVE_AXIS, actionID,
      actionName, fullrange))
    {
      continue;
    }
    g_Mouse.SetActive(false);

    // Use newState.axes[i] as the second about so subscribers can recover the original value
    CAction action(actionID, fullrange ? (newState.axes[i] + 1.0f) / 2.0f : absAxis, newState.axes[i], actionName);

    // For digital event, we treat action repeats like buttons and hats
    if (!CButtonTranslator::IsAnalog(actionID))
    {
      // NOW we overwrite old action and continue if no change in digital states
      bool bContinue = !((ABS(oldState.axes[i]) >= AXIS_DIGITAL_DEADZONE) ^ (absAxis >= AXIS_DIGITAL_DEADZONE));
      oldState.axes[i] = newState.axes[i];
      if (bContinue)
        continue;

      if (absAxis >= 0.01f) // Because we already sent a "centered" message
        CLog::Log(LOGDEBUG, "Joystick %d axis %d %s", joyID, i + 1,
          absAxis >= AXIS_DIGITAL_DEADZONE ? "activated" : "deactivated (but not centered)");

      if (!Wakeup() && absAxis >= AXIS_DIGITAL_DEADZONE)
      {
        g_application.ExecuteInputAction(action);
        m_actionTracker.Track(action);
      }
      else if (absAxis < AXIS_DIGITAL_DEADZONE)
      {
        m_actionTracker.Reset();
      }
    }
    else // CButtonTranslator::IsAnalog(actionID)
    {
      // We don't log about analog actions because they are sent every frame
      oldState.axes[i] = newState.axes[i];

      if (Wakeup())
        continue;

      if (newState.axes[i] != 0.0f)
        g_application.ExecuteInputAction(action);

      // The presence of analog actions disables others from being tracked
      m_actionTracker.Reset();
    }
  }
}

void CJoystickManager::ActionTracker::Track(const CAction &action)
{
  if (actionID != action.GetID())
  {
    // A new button was pressed, send the action and start tracking it
    actionID   = action.GetID();
    name       = action.GetName();
    targetTime = XbmcThreads::SystemClockMillis() + ACTION_FIRST_DELAY;
  }
  else if (OCCURED(targetTime))
  {
    // Same button was pressed, send the action if the delay has elapsed
    targetTime = XbmcThreads::SystemClockMillis() + ACTION_REPEAT_DELAY;
  }
}

bool CJoystickManager::Wakeup()
{
  static bool bWokenUp = false;

  // Refresh bWokenUp after every call to ResetWakeup() (which sets m_bWakeupChecked to false)
  if (!m_bWakeupChecked)
  {
    m_bWakeupChecked = true;

    // Reset the timers and check to see if we have woken the application
    g_application.ResetSystemIdleTimer();
    g_application.ResetScreenSaver();
    bWokenUp = g_application.WakeUpScreenSaverAndDPMS();
  }
  return bWokenUp;
}

void CJoystickManager::SetEnabled(bool enabled /* = true */)
{
  if (enabled && !m_bEnabled)
  {
    m_bEnabled = true;
    Initialize();
  }
  else if (!enabled && m_bEnabled)
  {
    DeInitialize();
    m_bEnabled = false;
  }
}

#endif // defined(HAS_SDL_JOYSTICK)
