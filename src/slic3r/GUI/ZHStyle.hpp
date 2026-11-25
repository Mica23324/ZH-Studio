#pragma once

#include <wx/checkbox.h>
#include <wx/colour.h>
#include <wx/combobox.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/tglbtn.h>
#include <wx/window.h>

#include "ZHTheme.hpp"

#include <algorithm>

// ZH-Studio surface palette for the native slicer workspace. The values mirror the
// web tokens (resources/web/homepage3/css/zh-studio.css) so the embedded web panels
// and the native controls share one look. Header only on purpose: the GUI sources are
// collected by a glob that is only re-scanned on a full reconfigure.
namespace ZHStyle {
    inline wxColour surface()        { return wxColour("#0B1220"); }
    inline wxColour card()           { return wxColour("#111C2D"); }
    inline wxColour elev()           { return wxColour("#162237"); }
    inline wxColour elev_hover()     { return wxColour("#1B2A43"); }
    inline wxColour border()         { return wxColour("#22324A"); }
    inline wxColour border_strong()  { return wxColour("#2D415C"); }
    inline wxColour text()           { return wxColour("#E7EEF8"); }
    inline wxColour muted()          { return wxColour("#93A6BF"); }
    inline wxColour faint()          { return wxColour("#6D84A0"); }

    inline wxColour workspace_surface(bool dark) { return dark ? surface() : wxColour("#F3F6FA"); }
    inline wxColour panel(bool dark)              { return dark ? card() : wxColour("#FFFFFF"); }
    inline wxColour raised(bool dark)             { return dark ? elev() : wxColour("#F8FAFC"); }
    inline wxColour outline(bool dark)            { return dark ? border() : wxColour("#E2E8F0"); }
    inline wxColour primary_text(bool dark)       { return dark ? text() : wxColour("#1E293B"); }
    inline wxColour secondary_text(bool dark)     { return dark ? muted() : wxColour("#64748B"); }
    inline wxColour accent_fill(bool dark)        { return dark ? ZHTheme::dark_tint() : ZHTheme::tint(); }
    inline wxColour accent_text(bool dark)        { return dark ? wxColour("#F8FAFC") : ZHTheme::accent(); }
    inline wxColour success_fill(bool dark)       { return dark ? wxColour("#10B981") : wxColour("#059669"); }
    inline wxColour warning_fill(bool dark)       { return dark ? wxColour("#F59E0B") : wxColour("#D97706"); }
    inline wxColour danger_fill(bool dark)        { return dark ? wxColour("#EF4444") : wxColour("#DC2626"); }
    inline wxColour success_tint(bool dark)       { return dark ? wxColour("#064E3B") : wxColour("#ECFDF5"); }
    inline wxColour warning_tint(bool dark)       { return dark ? wxColour("#451A03") : wxColour("#FFFBEB"); }
    inline wxColour danger_tint(bool dark)        { return dark ? wxColour("#450A0A") : wxColour("#FEF2F2"); }

    inline bool is_field(wxWindow* w)
    {
        return dynamic_cast<wxChoice*>(w) || dynamic_cast<wxComboBox*>(w) ||
               dynamic_cast<wxSpinCtrl*>(w) || dynamic_cast<wxTextCtrl*>(w) ||
               dynamic_cast<wxToggleButton*>(w) || dynamic_cast<wxCheckBox*>(w);
    }

    // A label that carries a heading weight is treated as a section title.
    inline bool is_heading(wxStaticText* label)
    {
        const wxFont font = label->GetFont();
        return font.GetWeight() == wxFONTWEIGHT_BOLD || font.GetPointSize() >= 11.0f;
    }

    // Restyles a single input control.
    inline void ApplyField(wxWindow* field, bool dark)
    {
        if (!field) return;

        // The surface colours belong to the dark workspace; in light mode the native
        // control colours are kept.
        if (dark) {
            if (dynamic_cast<wxTextCtrl*>(field) || dynamic_cast<wxSpinCtrl*>(field)) {
                field->SetBackgroundColour(elev());
                field->SetForegroundColour(text());
            } else if (dynamic_cast<wxChoice*>(field) || dynamic_cast<wxComboBox*>(field)) {
                field->SetForegroundColour(text());
            } else if (dynamic_cast<wxToggleButton*>(field) || dynamic_cast<wxCheckBox*>(field)) {
                field->SetForegroundColour(text());
            }
        }

        // The upstream settings are typeset very small; the ZH look uses a readable size.
        wxFont font = field->GetFont();
        if (font.GetPointSize() > 0 && font.GetPointSize() < 9.5f) {
            font.SetPointSize(9.5f);
            field->SetFont(font);
        }
    }

    // Style for the section headings of the settings sidebar.
    inline void ApplyHeading(wxWindow* label, const wxColour& accent)
    {
        if (!label) return;
        wxFont font = label->GetFont();
        font.SetPointSize(std::max(font.GetPointSize(), 10));
        font.SetWeight(wxFONTWEIGHT_BOLD);
        label->SetFont(font);
        label->SetForegroundColour(accent);
    }

    // Restyles the controls of an option group: labels get the ZH hierarchy, fields
    // get the ZH surface colours. Safe to call on any sizer.
    inline void ApplyOptionSizer(wxSizer* sizer, const wxColour& accent, bool dark)
    {
        if (!sizer) return;

        for (size_t i = 0; i < sizer->GetChildren().GetCount(); ++i) {
            wxSizerItemList::Node* node = sizer->GetChildren().Item(i);
            if (!node) continue;
            wxSizerItem* item = node->GetData();
            if (!item) continue;

            if (wxSizer* child_sizer = item->GetSizer()) {
                ApplyOptionSizer(child_sizer, accent, dark);
                continue;
            }

            wxWindow* window = item->GetWindow();
            if (!window) continue;

            if (auto* label = dynamic_cast<wxStaticText*>(window)) {
                if (is_heading(label)) {
                    ApplyHeading(label, accent);
                } else {
                    if (dark) label->SetForegroundColour(muted());
                    wxFont font = label->GetFont();
                    if (font.GetPointSize() > 0 && font.GetPointSize() < 9.5f) {
                        font.SetPointSize(9.5f);
                        label->SetFont(font);
                    }
                }
                continue;
            }

            if (is_field(window)) {
                ApplyField(window, dark);
                // Spin controls and combo boxes own their edit control as a child.
                for (wxWindow* child : window->GetChildren())
                    if (is_field(child)) ApplyField(child, dark);
                continue;
            }

            if (auto* box = dynamic_cast<wxStaticBox*>(window)) {
                if (dark) {
                    box->SetBackgroundColour(card());
                    box->SetForegroundColour(border_strong());
                }
                wxFont font = box->GetFont();
                font.SetPointSize(std::max(font.GetPointSize(), 10));
                font.SetWeight(wxFONTWEIGHT_BOLD);
                box->SetFont(font);
                continue;
            }

            if (auto* panel = dynamic_cast<wxPanel*>(window)) {
                if (dark) panel->SetBackgroundColour(card());
                ApplyOptionSizer(panel->GetSizer(), accent, dark);
            }
        }
    }
}
