// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/plugins/ppapi/ppb_gpu_blacklist_private_impl.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "ppapi/c/private/ppb_gpu_blacklist_private.h"
#include "webkit/plugins/plugin_switches.h"

namespace webkit {
namespace ppapi {

namespace {

bool IsGpuBlacklisted() {
  CommandLine* command_line = CommandLine::ForCurrentProcess();
  CHECK(command_line);
  return command_line->HasSwitch(switches::kDisablePepper3dForUntrustedUse);
}

}  // namespace

const PPB_GpuBlacklist_Private ppb_gpu_blacklist = {
  &IsGpuBlacklisted,
};

// static
const PPB_GpuBlacklist_Private* PPB_GpuBlacklist_Private_Impl::GetInterface() {
  return &ppb_gpu_blacklist;
}

}  // namespace ppapi
}  // namespace webkit

