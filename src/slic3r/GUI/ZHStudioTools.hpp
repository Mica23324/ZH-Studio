#pragma once
class wxWindow;
class wxPanel;
namespace Slic3r { namespace GUI {
class MainFrame;
// Native tools; no web service or Bambu network plugin required.
void show_zh_print_costs(wxWindow* parent);
void show_zh_workspace_settings(wxWindow* parent);
bool show_zh_command_palette(wxWindow* parent);
bool zh_has_visible_modal_dialog(wxWindow* parent);
} }
