// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_SHELF_LAYOUT_MANAGER_H_
#define ASH_WM_SHELF_LAYOUT_MANAGER_H_
#pragma once

#include "ash/ash_export.h"
#include "ash/launcher/launcher.h"
#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/timer.h"
#include "ui/aura/layout_manager.h"
#include "ui/gfx/insets.h"
#include "ui/gfx/rect.h"

namespace views {
class Widget;
}

namespace ash {
namespace internal {

// ShelfLayoutManager is the layout manager responsible for the launcher and
// status widgets. The launcher is given the total available width and told the
// width of the status area. This allows the launcher to draw the background and
// layout to the status area.
// To respond to bounds changes in the status area StatusAreaLayoutManager works
// closely with ShelfLayoutManager.
class ASH_EXPORT ShelfLayoutManager : public aura::LayoutManager {
 public:
  enum VisibilityState {
    // Completely visible.
    VISIBLE,

    // A couple of pixels are reserved at the bottom for the shelf.
    AUTO_HIDE,

    // Nothing is shown. Used for fullscreen windows.
    HIDDEN,
  };

  enum AutoHideState {
    AUTO_HIDE_SHOWN,
    AUTO_HIDE_HIDDEN,
  };

  // We reserve a small area at the bottom of the workspace area to ensure that
  // the bottom-of-window resize handle can be hit.
  // TODO(jamescook): Some day we may want the workspace area to be an even
  // multiple of the size of the grid (currently 8 pixels), which will require
  // removing this and finding a way for hover and click events to pass through
  // the invisible parts of the launcher.
  static const int kWorkspaceAreaBottomInset;

  // Height of the shelf when auto-hidden.
  static const int kAutoHideHeight;

  explicit ShelfLayoutManager(views::Widget* status);
  virtual ~ShelfLayoutManager();

  views::Widget* launcher_widget() {
    return launcher_ ? launcher_->widget() : NULL;
  }
  const views::Widget* launcher_widget() const {
    return launcher_ ? launcher_->widget() : NULL;
  }
  views::Widget* status() { return status_; }

  bool in_layout() const { return in_layout_; }

  // See description above field.
  int shelf_height() const { return shelf_height_; }

  // Returns the bounds the specified window should be when maximized.
  gfx::Rect GetMaximizedWindowBounds(aura::Window* window) const;
  gfx::Rect GetUnmaximizedWorkAreaBounds(aura::Window* window) const;

  // The launcher is typically created after the layout manager.
  void SetLauncher(Launcher* launcher);

  // Stops any animations and sets the bounds of the launcher and status
  // widgets.
  void LayoutShelf();

  // Sets the visibility of the shelf to |state|.
  void SetState(VisibilityState visibility_state);
  VisibilityState visibility_state() const { return state_.visibility_state; }
  AutoHideState auto_hide_state() const { return state_.auto_hide_state; }

  // Forces the visibility to |forced_visibility_state|. Any calls to SetState
  // are ignored after this until ClearForcedState is called.
  // TODO: clean this up!
  void SetForcedState(VisibilityState forced_visibility_state);
  void ClearForcedState();

  // Invoked by the shelf/launcher when the auto-hide state may have changed.
  void UpdateAutoHideState();

  // Sets whether any windows overlap the shelf. If a window overlaps the shelf
  // the shelf renders slightly differently.
  void SetWindowOverlapsShelf(bool value);

  // Overridden from aura::LayoutManager:
  virtual void OnWindowResized() OVERRIDE;
  virtual void OnWindowAddedToLayout(aura::Window* child) OVERRIDE;
  virtual void OnWillRemoveWindowFromLayout(aura::Window* child) OVERRIDE;
  virtual void OnChildWindowVisibilityChanged(aura::Window* child,
                                              bool visible) OVERRIDE;
  virtual void SetChildBounds(aura::Window* child,
                              const gfx::Rect& requested_bounds) OVERRIDE;

 private:
  class AutoHideEventFilter;

  struct TargetBounds {
    TargetBounds() : opacity(0.0f) {}

    float opacity;
    gfx::Rect launcher_bounds;
    gfx::Rect status_bounds;
    gfx::Insets work_area_insets;
  };

  struct State {
    State() : visibility_state(VISIBLE), auto_hide_state(AUTO_HIDE_HIDDEN) {}

    // Returns true if the two states are considered equal. As
    // |auto_hide_state| only matters if |visibility_state| is |AUTO_HIDE|,
    // Equals() ignores the |auto_hide_state| as appropriate.
    bool Equals(const State& other) const {
      return other.visibility_state == visibility_state &&
          (visibility_state != AUTO_HIDE ||
           other.auto_hide_state == auto_hide_state);
    }

    VisibilityState visibility_state;
    AutoHideState auto_hide_state;
  };

  // Stops any animations.
  void StopAnimating();

  // Calculates the target bounds assuming visibility of |visible|.
  void CalculateTargetBounds(const State& state,
                             TargetBounds* target_bounds) const;

  // Updates the background of the shelf.
  void UpdateShelfBackground(BackgroundAnimator::ChangeType type);

  // Returns whether the launcher should draw a background.
  bool GetLauncherPaintsBackground() const;

  // Updates the auto hide state immediately.
  void UpdateAutoHideStateNow();

  // Returns the AutoHideState. This value is determined from the launcher and
  // tray.
  AutoHideState CalculateAutoHideState(VisibilityState visibility_state) const;

  // True when inside LayoutShelf method. Used to prevent calling LayoutShelf
  // again from SetChildBounds().
  bool in_layout_;

  // Current state.
  State state_;

  // Variables to keep track of the visibility state when we need to force the
  // visibility to a particular value.
  VisibilityState forced_visibility_state_;
  VisibilityState normal_visibility_state_;
  bool is_visibility_state_forced_;

  // Height of the shelf (max of launcher and status).
  int shelf_height_;

  Launcher* launcher_;
  views::Widget* status_;

  // Do any windows overlap the shelf? This is maintained by WorkspaceManager.
  bool window_overlaps_shelf_;

  base::OneShotTimer<ShelfLayoutManager> auto_hide_timer_;

  // EventFilter used to detect when user moves the mouse over the launcher to
  // trigger showing the launcher.
  scoped_ptr<AutoHideEventFilter> event_filter_;

  DISALLOW_COPY_AND_ASSIGN(ShelfLayoutManager);
};

}  // namespace internal
}  // namespace ash

#endif  // ASH_WM_SHELF_LAYOUT_MANAGER_H_
