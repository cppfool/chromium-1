// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/scoped_ptr.h"
#include "chrome/browser/prefs/session_startup_pref.h"
#include "chrome/browser/protector/base_setting_change.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace protector {

namespace {

const char kStartupUrl[] = "http://example.com/";

}  // namespace

class PrefsBackupInvalidChangeTest : public testing::Test {
 protected:
  TestingProfile profile_;
};

// Test that correct default values are applied by Init.
TEST_F(PrefsBackupInvalidChangeTest, Defaults) {
  SessionStartupPref startup_pref(SessionStartupPref::URLS);
  startup_pref.urls.push_back(GURL(kStartupUrl));
  SessionStartupPref::SetStartupPref(&profile_, startup_pref);

  scoped_ptr<BaseSettingChange> change(CreatePrefsBackupInvalidChange());
  change->Init(&profile_);

  SessionStartupPref new_startup_pref =
      SessionStartupPref::GetStartupPref(&profile_);
  // Startup type should be reset to default.
  EXPECT_EQ(SessionStartupPref::GetDefaultStartupType(), new_startup_pref.type);
  // Startup URLs should be left unchanged.
  EXPECT_EQ(1UL, new_startup_pref.urls.size());
  EXPECT_EQ(std::string(kStartupUrl), new_startup_pref.urls[0].spec());
}

// Test that "restore last session" setting is not changed by Init.
TEST_F(PrefsBackupInvalidChangeTest, DefaultsWithSessionRestore) {
  SessionStartupPref startup_pref(SessionStartupPref::LAST);
  startup_pref.urls.push_back(GURL(kStartupUrl));
  SessionStartupPref::SetStartupPref(&profile_, startup_pref);

  scoped_ptr<BaseSettingChange> change(CreatePrefsBackupInvalidChange());
  change->Init(&profile_);

  SessionStartupPref new_startup_pref =
      SessionStartupPref::GetStartupPref(&profile_);
  // Both startup type and startup URLs should be left unchanged.
  EXPECT_EQ(SessionStartupPref::LAST, new_startup_pref.type);
  EXPECT_EQ(1UL, new_startup_pref.urls.size());
  EXPECT_EQ(std::string(kStartupUrl), new_startup_pref.urls[0].spec());
}

}  // namespace protector
