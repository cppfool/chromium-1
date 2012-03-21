// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_SYSTEM_KEY_EVENT_LISTENER_H_
#define CHROME_BROWSER_CHROMEOS_SYSTEM_KEY_EVENT_LISTENER_H_
#pragma once

#include "base/memory/singleton.h"
#include "base/message_loop.h"

typedef union _XEvent XEvent;

namespace chromeos {

// SystemKeyEventListener listens for volume related key presses from GDK, then
// tells the AudioHandler to adjust volume accordingly.  Start by just calling
// instance() to get it going.

class SystemKeyEventListener : public MessageLoopForUI::Observer {
 public:
  class CapsLockObserver {
   public:
    virtual void OnCapsLockChange(bool enabled) = 0;
   protected:
    CapsLockObserver() {}
    virtual ~CapsLockObserver() {}
    DISALLOW_COPY_AND_ASSIGN(CapsLockObserver);
  };

  static void Initialize();
  static void Shutdown();
  // GetInstance returns NULL if not initialized or if already shutdown.
  static SystemKeyEventListener* GetInstance();

  void Stop();

  void AddCapsLockObserver(CapsLockObserver* observer);
  void RemoveCapsLockObserver(CapsLockObserver* observer);

 private:
  // Defines the delete on exit Singleton traits we like.  Best to have this
  // and const/dest private as recommended for Singletons.
  friend struct DefaultSingletonTraits<SystemKeyEventListener>;
  friend class SystemKeyEventListenerTest;

  SystemKeyEventListener();
  virtual ~SystemKeyEventListener();

  // MessageLoopForUI::Observer overrides.
  virtual base::EventStatus WillProcessEvent(
      const base::NativeEvent& event) OVERRIDE;
  virtual void DidProcessEvent(const base::NativeEvent& event) OVERRIDE;

  // Tell X we are interested in the specified key/mask combination.
  // CapsLock and Numlock are always ignored.
  void GrabKey(int32 key, uint32 mask);

  void OnCapsLock(bool enabled);

  // Returns true if the event was processed, false otherwise.
  virtual bool ProcessedXEvent(XEvent* xevent);

  int32 key_brightness_down_;
  int32 key_brightness_up_;
  int32 key_volume_mute_;
  int32 key_volume_down_;
  int32 key_volume_up_;
  int32 key_f6_;
  int32 key_f7_;
  int32 key_f8_;
  int32 key_f9_;
  int32 key_f10_;

  bool stopped_;

  unsigned int num_lock_mask_;
  bool num_lock_is_on_;
  bool caps_lock_is_on_;
  ObserverList<CapsLockObserver> caps_lock_observers_;

  // Base X ID for events from the XKB extension.
  int xkb_event_base_;

  DISALLOW_COPY_AND_ASSIGN(SystemKeyEventListener);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_SYSTEM_KEY_EVENT_LISTENER_H_
