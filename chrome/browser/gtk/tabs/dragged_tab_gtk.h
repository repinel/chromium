// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GTK_TABS_DRAGGED_TAB_GTK_H_
#define CHROME_BROWSER_GTK_TABS_DRAGGED_TAB_GTK_H_

#include <gtk/gtk.h>

#include "app/slide_animation.h"
#include "base/gfx/point.h"
#include "base/gfx/rect.h"
#include "base/gfx/size.h"
#include "base/scoped_ptr.h"
#include "base/task.h"
#include "chrome/common/owned_widget_gtk.h"

class ChromeCanvas;
class TabContents;
class TabRendererGtk;

class DraggedTabGtk : public AnimationDelegate {
 public:
  DraggedTabGtk(TabContents* datasource,
                const gfx::Point& mouse_tab_offset,
                const gfx::Size& contents_size);
  virtual ~DraggedTabGtk();

  // Moves the DraggedTabView to the appropriate location given the mouse
  // pointer at |screen_point|.
  void MoveTo(const gfx::Point& screen_point);

  // Notifies the DraggedTabView that it has become attached to a TabStrip.
  void Attach(int selected_width);

  // Notifies the DraggedTabView that it should update itself.
  void Update();

  // Animates the DraggedTabView to the specified bounds, then calls back to
  // |callback|.
  typedef Callback0::Type AnimateToBoundsCallback;
  void AnimateToBounds(const gfx::Rect& bounds,
                       AnimateToBoundsCallback* callback);

  // Returns the size of the DraggedTabView. Used when attaching to a TabStrip
  // to determine where to place the Tab in the attached TabStrip.
  gfx::Size attached_tab_size() const { return attached_tab_size_; }

 private:
  // Overridden from AnimationDelegate:
  virtual void AnimationProgressed(const Animation* animation);
  virtual void AnimationEnded(const Animation* animation);
  virtual void AnimationCanceled(const Animation* animation);

  // Gets the preferred size of the dragged tab.
  virtual gfx::Size GetPreferredSize();

  // Resizes the container to fit the content for the current attachment mode.
  void ResizeContainer();

  // Utility for scaling a size by the current scaling factor.
  int ScaleValue(int value);

  // Sets the color map of the container window to allow the window to be
  // transparent.
  void SetContainerColorMap();

  // expose-event handler that redraws the dragged tab.
  static gboolean OnExpose(GtkWidget* widget, GdkEventExpose* event,
                           DraggedTabGtk* dragged_tab);

  // The window that contains the dragged tab or tab contents.
  GtkWidget* container_;

  // The renderer that paints the dragged tab.
  scoped_ptr<TabRendererGtk> renderer_;

  // True if the view is currently attached to a TabStrip. Controls rendering
  // and sizing modes.
  bool attached_;

  // The unscaled offset of the mouse from the top left of the dragged Tab.
  // This is used to maintain an appropriate offset for the mouse pointer when
  // dragging scaled and unscaled representations, and also to calculate the
  // position of detached windows.
  gfx::Point mouse_tab_offset_;

  // The desired width of the TabRenderer when the DraggedTabView is attached
  // to a TabStrip.
  gfx::Size attached_tab_size_;

  // The dimensions of the TabContents being dragged.
  gfx::Size contents_size_;

  // The animation used to slide the attached view to its final location.
  SlideAnimation close_animation_;

  // A callback notified when the animation is complete.
  scoped_ptr<Callback0::Type> animation_callback_;

  // The start and end bounds of the animation sequence.
  gfx::Rect animation_start_bounds_;
  gfx::Rect animation_end_bounds_;

  DISALLOW_COPY_AND_ASSIGN(DraggedTabGtk);
};

#endif  // CHROME_BROWSER_GTK_TABS_DRAGGED_TAB_GTK_H_
