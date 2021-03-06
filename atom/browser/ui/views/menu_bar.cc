// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/views/menu_bar.h"

#if defined(USE_X11)
#include "gtk/gtk.h"
#endif

#include "atom/browser/ui/views/menu_delegate.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/models/menu_model.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/menu_button.h"
#include "ui/views/layout/box_layout.h"

#if defined(OS_WIN)
#include "ui/gfx/color_utils.h"
#elif defined(USE_X11)
#include "chrome/browser/ui/libgtk2ui/owned_widget_gtk2.h"
#include "chrome/browser/ui/libgtk2ui/skia_utils_gtk2.h"
#endif

namespace atom {

namespace {

const char kViewClassName[] = "AtomMenuBar";

// Default color of the menu bar.
const SkColor kDefaultColor = SkColorSetARGB(255, 233, 233, 233);

// Filter out the "&" in menu label.
base::string16 FilterMenuButtonLabel(const base::string16& label) {
  base::string16 out;
  base::RemoveChars(label, base::ASCIIToUTF16("&").c_str(), &out);
  return out;
}

#if defined(USE_X11)
void GetMenuBarColor(SkColor* enabled, SkColor* disabled, SkColor* highlight,
                     SkColor* hover, SkColor* background) {
  libgtk2ui::OwnedWidgetGtk fake_menu_bar;
  fake_menu_bar.Own(gtk_menu_bar_new());

  GtkStyle* style = gtk_rc_get_style(fake_menu_bar.get());
  *enabled = libgtk2ui::GdkColorToSkColor(style->text[GTK_STATE_NORMAL]);
  *disabled = libgtk2ui::GdkColorToSkColor(style->text[GTK_STATE_INSENSITIVE]);
  *highlight = libgtk2ui::GdkColorToSkColor(style->text[GTK_STATE_SELECTED]);
  *hover = libgtk2ui::GdkColorToSkColor(style->text[GTK_STATE_PRELIGHT]);
  *background = libgtk2ui::GdkColorToSkColor(style->bg[GTK_STATE_NORMAL]);
}
#endif

}  // namespace

MenuBar::MenuBar()
    : background_color_(kDefaultColor),
      menu_model_(NULL) {
#if defined(OS_WIN)
  background_color_ = color_utils::GetSysSkColor(COLOR_MENUBAR);
#elif defined(USE_X11)
  GetMenuBarColor(&enabled_color_, &disabled_color_, &highlight_color_,
                  &hover_color_, &background_color_);
#endif

  set_background(views::Background::CreateSolidBackground(background_color_));
  SetLayoutManager(new views::BoxLayout(
      views::BoxLayout::kHorizontal, 0, 0, 0));
}

MenuBar::~MenuBar() {
}

void MenuBar::SetMenu(ui::MenuModel* model) {
  menu_model_ = model;
  RemoveAllChildViews(true);

  for (int i = 0; i < model->GetItemCount(); ++i) {
    views::MenuButton* button = new views::MenuButton(
        this, FilterMenuButtonLabel(model->GetLabelAt(i)), this, false);
    button->set_tag(i);

#if defined(USE_X11)
    button->SetEnabledColor(enabled_color_);
    button->SetDisabledColor(disabled_color_);
    button->SetHighlightColor(highlight_color_);
    button->SetHoverColor(hover_color_);
#endif

    AddChildView(button);
  }
}

int MenuBar::GetItemCount() const {
  return menu_model_->GetItemCount();
}

bool MenuBar::GetMenuButtonFromScreenPoint(const gfx::Point& point,
                                           ui::MenuModel** menu_model,
                                           views::MenuButton** button) {
  gfx::Point location(point);
  views::View::ConvertPointFromScreen(this, &location);

  if (location.x() < 0 || location.x() >= width() || location.y() < 0 ||
      location.y() >= height())
    return false;

  for (int i = 0; i < child_count(); ++i) {
    views::View* view = child_at(i);
    if (view->bounds().Contains(location) &&
        (menu_model_->GetTypeAt(i) == ui::MenuModel::TYPE_SUBMENU)) {
      *menu_model = menu_model_->GetSubmenuModelAt(i);
      *button = static_cast<views::MenuButton*>(view);
      return true;
    }
  }

  return false;
}

const char* MenuBar::GetClassName() const {
  return kViewClassName;
}

void MenuBar::ButtonPressed(views::Button* sender, const ui::Event& event) {
}

void MenuBar::OnMenuButtonClicked(views::View* source,
                                  const gfx::Point& point) {
  if (!menu_model_)
    return;

  views::MenuButton* button = static_cast<views::MenuButton*>(source);
  int id = button->tag();
  ui::MenuModel::ItemType type = menu_model_->GetTypeAt(id);
  if (type != ui::MenuModel::TYPE_SUBMENU)
    return;

  menu_delegate_.reset(new MenuDelegate(this));
  menu_delegate_->RunMenu(menu_model_->GetSubmenuModelAt(id), button);
}

}  // namespace atom
