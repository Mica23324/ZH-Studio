#include "ZHRounded.hpp"
#include "ZHStudioTools.hpp"
#include "ZHCostModel.hpp"
#include "ZHTheme.hpp"
#include "GUI_App.hpp"
#include "GLToolbar.hpp"
#include "MainFrame.hpp"
#include "DeviceWeb/DeviceWebPage.hpp"
#include "Plater.hpp"
#include "PartPlate.hpp"
#include "Widgets/Button.hpp"
#include "Widgets/StateColor.hpp"
#include "wxExtensions.hpp"
#include "libslic3r/GCode/GCodeProcessor.hpp"
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/listctrl.h>
#include <wx/msgdlg.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/timer.h>
#include <wx/settings.h>
#include <array>
#include <chrono>
#include <cmath>
#include <functional>
#include <iomanip>
#include <locale>
#include <map>
#include <mutex>
#include <sstream>

namespace Slic3r { namespace GUI {
namespace {
double zh_clamp01(double v) { return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v); }

// Gentle fade-in so dialogs arrive with motion instead of popping.
void fade_in(wxDialog* dlg)
{
    dlg->SetTransparent(0);
    auto* timer = new wxTimer(dlg);
    auto start = std::make_shared<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
    dlg->Bind(wxEVT_TIMER, [timer, dlg, start](wxTimerEvent&) {
        const double e = std::chrono::duration<double>(std::chrono::steady_clock::now() - *start).count();
        const int alpha = static_cast<int>(zh_clamp01(e / 0.22) * 255);
        dlg->SetTransparent(alpha);
        if (alpha >= 255) { timer->Stop(); dlg->SetTransparent(255); }
    }, timer->GetId());
    timer->Start(16);
}

// Custom ZH dialogs do not always have a native default Cancel button. Handle
// Escape at the dialog level so it works from any focused child control.
void bind_modal_escape(wxDialog* dialog)
{
    dialog->Bind(wxEVT_CHAR_HOOK, [dialog](wxKeyEvent& event) {
        if (event.GetKeyCode() == WXK_ESCAPE)
            dialog->EndModal(wxID_CANCEL);
        else
            event.Skip();
    });
}

wxString text(const char* de, const char* en)
{
    const auto language = wxGetApp().app_config->get("language");
    return wxString::FromUTF8(language.rfind("de", 0) == 0 ? de : en);
}
wxFont font(int points = 10, bool bold = false)
{
    wxFont value = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    value.SetPointSize(points);
    value.SetWeight(bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL);
    return value;
}
wxColour background() { return wxColour(wxGetApp().dark_mode() ? "#101827" : "#F3F6FA"); }
wxColour card() { return wxColour(wxGetApp().dark_mode() ? "#192539" : "#FFFFFF"); }
wxColour foreground() { return wxColour(wxGetApp().dark_mode() ? "#E7EEF8" : "#172B43"); }
wxColour muted() { return wxColour(wxGetApp().dark_mode() ? "#A5B6CC" : "#51647C"); }
wxColour border() { return wxColour(wxGetApp().dark_mode() ? "#344761" : "#D6E0EC"); }

std::string number(double value)
{
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::fixed << std::setprecision(6) << value;
    return out.str();
}
double saved_rate(const char* key, double fallback, double maximum)
{
    // Saved values use a locale-independent decimal representation.
    const auto parsed = ZHStudio::parse_decimal(wxGetApp().app_config->get(key), maximum);
    return parsed ? *parsed : fallback;
}
wxStaticText* label(wxWindow* parent, const wxString& value, int size = 10, bool bold = false, bool secondary = false)
{
    auto* result = new wxStaticText(parent, wxID_ANY, value);
    result->SetFont(font(size, bold));
    result->SetForegroundColour(secondary ? muted() : foreground());
    return result;
}
Button* button(wxWindow* parent, const wxString& value, bool primary = false, wxWindowID id = wxID_ANY, bool chrome = false)
{
    auto* result = new Button(parent, value, "", wxBORDER_NONE, 0, id);
    result->SetFont(font(10, primary));
    result->SetPaddingSize(parent->FromDIP(wxSize(14, 8)));
    result->SetCornerRadius(parent->FromDIP(7));
    result->SetMinSize(parent->FromDIP(wxSize(-1, 36)));
    const wxColour normal = chrome ? wxColour("#182B42") : card();
    const wxColour hover = chrome ? wxColour("#243D58") : (wxGetApp().dark_mode() ? wxColour("#243A54") : wxColour("#E8F1FA"));
    result->SetBackgroundColor(StateColor(
        std::pair{chrome ? wxColour("#132237") : background(), (int)StateColor::Disabled},
        std::pair{primary ? ZHTheme::down() : hover, (int)StateColor::Pressed},
        std::pair{primary ? ZHTheme::hovered() : hover, (int)StateColor::Hovered},
        std::pair{primary ? ZHTheme::accent() : normal, (int)StateColor::Normal}));
    result->SetBorderColor(StateColor(
        std::pair{ZHTheme::accent(), (int)StateColor::Focused},
        std::pair{primary ? ZHTheme::accent() : (chrome ? wxColour("#304761") : border()), (int)StateColor::Normal}));
    // Dark text on Cyan/Amber, white on the darker Ocean/Violet palette.
    const wxColour on_accent = ZHTheme::index() == 0 || ZHTheme::index() == 3 ? wxColour("#081522") : wxColour("#FFFFFE");
    result->SetTextColor(StateColor(
        std::pair{chrome ? wxColour("#687E98") : muted(), (int)StateColor::Disabled},
        std::pair{primary ? on_accent : (chrome ? wxColour("#E7EEF8") : foreground()), (int)StateColor::Normal}));
    result->SetToolTip(value);
    return result;
}
void heading(wxWindow* parent, wxBoxSizer* outer, const wxString& title, const wxString& subtitle)
{
    auto* strip = new wxPanel(parent);
    strip->SetBackgroundColour(wxColour("#0C1829"));
    auto* box = new wxBoxSizer(wxVERTICAL);
    auto* mark = label(strip, "ZH-STUDIO ALPHA / WORKSPACE", 9, true);
    mark->SetForegroundColour(ZHTheme::hovered());
    box->Add(mark, 0, wxLEFT | wxRIGHT | wxTOP, parent->FromDIP(22));
    auto* name = label(strip, title, 22, true);
    name->SetForegroundColour(wxColour("#F1F5F9"));
    box->Add(name, 0, wxLEFT | wxRIGHT | wxTOP, parent->FromDIP(22));
    auto* sub = label(strip, subtitle, 10);
    sub->SetForegroundColour(wxColour("#B5C8DE"));
    sub->Wrap(parent->FromDIP(700));
    box->Add(sub, 0, wxALL, parent->FromDIP(22));
    strip->SetSizer(box);
    outer->Add(strip, 0, wxEXPAND);
}

bool save_settings(const std::map<std::string, std::string>& values, wxWindow* parent)
{
    auto* config = wxGetApp().app_config;
    std::map<std::string, std::string> old;
    for (const auto& item : values) old[item.first] = config->get(item.first);
    try {
        for (const auto& item : values) config->set(item.first, item.second);
        config->save();
        return true;
    } catch (const std::exception&) {
        // Do not leave partially changed in-memory preferences after a disk-write failure.
        for (const auto& item : old) config->set(item.first, item.second);
        wxMessageBox(text("Die Einstellungen konnten nicht gespeichert werden. Bitte freien Speicher und Schreibrechte prüfen.",
                          "Settings could not be saved. Check free disk space and write permissions."),
                     "ZH-Studio", wxOK | wxICON_ERROR, parent);
        return false;
    }
}

struct SliceSnapshot {
    bool available = false;
    bool price_available = false;
    double grams = 0.0;
    double hours = 0.0;
    double material_cost = 0.0;
    wxString plate;
};
SliceSnapshot current_slice()
{
    SliceSnapshot snapshot;
    auto* plater = wxGetApp().plater();
    if (!plater || plater->is_background_process_slicing()) return snapshot;
    auto* plate = plater->get_partplate_list().get_curr_plate();
    if (!plate || !plate->is_slice_result_valid()) return snapshot;
    auto* result = plate->get_gcode_result();
    if (!result) return snapshot;
    std::unique_lock<std::mutex> lock(result->result_mutex, std::try_to_lock);
    if (!lock.owns_lock()) return snapshot;
    const double seconds = result->print_statistics.modes[static_cast<size_t>(PrintEstimatedStatistics::ETimeMode::Normal)].time;
    if (!std::isfinite(seconds) || seconds <= 0.0 || seconds > 360000000.0) return snapshot;
    bool have_volume = false, have_all_prices = true;
    for (const auto& entry : result->print_statistics.total_volumes_per_extruder) {
        const size_t filament = entry.first;
        const double mm3 = entry.second;
        if (!std::isfinite(mm3) || mm3 < 0.0) return SliceSnapshot{};
        if (mm3 == 0.0) continue; // unused slots need no density or price
        if (filament >= result->filament_densities.size()) return SliceSnapshot{};
        const double density = result->filament_densities[filament];
        if (!std::isfinite(density) || density <= 0.0) return SliceSnapshot{};
        const double grams = mm3 * density / 1000.0;
        snapshot.grams += grams;
        have_volume = have_volume || grams > 0.0;
        if (filament < result->filament_costs.size() && std::isfinite(result->filament_costs[filament]) && result->filament_costs[filament] >= 0.0)
            snapshot.material_cost += grams / 1000.0 * result->filament_costs[filament];
        else have_all_prices = false;
    }
    if (!have_volume || !std::isfinite(snapshot.grams) || snapshot.grams > 10000000.0) return SliceSnapshot{};
    snapshot.hours = seconds / 3600.0;
    snapshot.available = true;
    snapshot.price_available = have_all_prices && std::isfinite(snapshot.material_cost) && snapshot.material_cost / snapshot.grams * 1000.0 <= 100000.0;
    snapshot.plate = wxString::FromUTF8(plate->get_plate_name());
    if (snapshot.plate.IsEmpty()) snapshot.plate = wxString::Format("%d", plate->get_index() + 1);
    return snapshot;
}

class CostDialog final : public wxDialog {
    std::array<wxTextCtrl*, 6> fields{};
    std::array<wxString, 6> names{};
    const std::array<double, 6> maxima{{10000000, 100000, 100000, 100000, 10000, 100000}};
    std::array<wxStaticText*, 3> parts{};
    wxStaticText *total = nullptr, *energy = nullptr, *source = nullptr, *error = nullptr;
    std::array<wxString, 3> part_titles{};
    Button* save = nullptr;
    SliceSnapshot sliced;
    bool manual = false, price_edited = false;

    // Plain text summary of the estimate - for notes, quotes and spreadsheets.
    wxString summary_text() const
    {
        wxString out = text("ZH-Studio · Druckkosten-Schätzung", "ZH-Studio · Print cost estimate") + "\n";
        out += text("Geschätzte Gesamtkosten: ", "Estimated total: ") + total->GetLabel() + "\n";
        for (size_t i = 0; i < parts.size(); ++i)
            out += part_titles[i] + ": " + parts[i]->GetLabel() + "\n";
        if (!energy->GetLabel().IsEmpty()) out += energy->GetLabel() + "\n";
        return out.Trim();
    }

    bool values(std::array<double, 6>& output)
    {
        wxString issue;
        for (size_t i = 0; i < fields.size(); ++i) {
            const auto parsed = ZHStudio::parse_decimal(fields[i]->GetValue().ToStdString(), maxima[i]);
            fields[i]->SetBackgroundColour(parsed ? card() : wxColour(wxGetApp().dark_mode() ? "#42232E" : "#FFF0F2"));
            fields[i]->Refresh();
            if (!parsed && issue.empty()) issue = names[i] + text(": Zahl zwischen 0 und ", ": enter a number from 0 to ") + wxString::Format("%.0f", maxima[i]);
            output[i] = parsed.value_or(0.0);
        }
        error->SetLabel(issue);
        error->Wrap(FromDIP(430));
        return issue.empty();
    }
    void update_source()
    {
        wxString value;
        if (!sliced.available) value = text("MANUELLE SCHÄTZUNG\nNoch keine gültigen Slicing-Daten. Gewicht und Dauer selbst eingeben.",
                                            "MANUAL ESTIMATE\nNo valid sliced data yet. Enter weight and duration.");
        else if (manual) value = text("MANUELL ANGEPASST\nAusgehend von Platte ", "MANUALLY ADJUSTED\nBased on plate ") + sliced.plate;
        else value = text("AUS SLICING ÜBERNOMMEN · PLATTE ", "FROM SLICING · PLATE ") + sliced.plate;
        if (sliced.available) value += (sliced.price_available && !price_edited)
            ? text("\nFilamentpreis nach Materialgewicht gemittelt; inkl. Spülung und Stützen.", "\nWeight-averaged filament price; includes purge and support.")
            : text("\nFilamentpreis aus manueller Eingabe bzw. gespeichertem Tarif.", "\nFilament price uses the entered or saved rate.");
        source->SetLabel(value);
        source->Wrap(FromDIP(430));
    }
    void calculate()
    {
        if (!total) return;
        std::array<double, 6> in{};
        const bool valid = values(in);
        save->Enable(valid);
        if (valid) {
            const auto result = ZHStudio::estimate_cost({in[0],in[1],in[2],in[3],in[4],in[5]});
            total->SetLabel(wxString::Format("%.2f EUR", result.total));
            parts[0]->SetLabel(wxString::Format("%.2f EUR", result.material));
            parts[1]->SetLabel(wxString::Format("%.2f EUR", result.energy));
            parts[2]->SetLabel(wxString::Format("%.2f EUR", result.wear));
            energy->SetLabel(wxString::Format(text("%.3f kWh geschätzte Energie", "%.3f kWh estimated energy"), result.kwh));
        } else {
            total->SetLabel(wxString::FromUTF8("—"));
            for (auto* part : parts) part->SetLabel(wxString::FromUTF8("—"));
            energy->SetLabel(text("Eingaben prüfen", "Check inputs"));
        }
        update_source();
        Layout();
    }
    void load_snapshot()
    {
        sliced = current_slice();
        manual = false;
        price_edited = false;
        fields[0]->ChangeValue(wxString::FromUTF8(number(sliced.available ? sliced.grams : 0.0)));
        fields[1]->ChangeValue(wxString::FromUTF8(number(sliced.available ? sliced.hours : 0.0)));
        const double price = sliced.price_available ? sliced.material_cost / sliced.grams * 1000.0 : saved_rate("zh_cost_filament",25,100000);
        fields[2]->ChangeValue(wxString::FromUTF8(number(price)));
        calculate();
    }
public:
    explicit CostDialog(wxWindow* parent)
        : wxDialog(parent, wxID_ANY, text("ZH-Studio · Druckkosten", "ZH-Studio · Print costs"), wxDefaultPosition,
                   wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    {
        SetFont(font());
        SetBackgroundColour(background());
        bind_modal_escape(this);
        // ZH-Studio: rounded window frame in the workspace colours.
        ZHRounded::Apply(this, 2, RGB(45, 65, 92));
        auto* outer = new wxBoxSizer(wxVERTICAL);
        heading(this, outer, text("Was kostet dein Druck?", "What does your print cost?"),
            text("Material, Energie und Maschinenkosten. Eine nachvollziehbare Schätzung.", "Material, energy and machine costs. An estimate you can understand."));
        auto* scroll = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
        scroll->SetBackgroundColour(background()); scroll->SetScrollRate(0, FromDIP(12));
        auto* columns = new wxBoxSizer(wxHORIZONTAL);
        auto* form = new wxPanel(scroll); form->SetBackgroundColour(card());
        auto* formbox = new wxBoxSizer(wxVERTICAL);
        source = label(form, "", 9, false, true);
        formbox->Add(source, 0, wxEXPAND | wxALL, FromDIP(18));
        auto* grid = new wxFlexGridSizer(2, FromDIP(12), FromDIP(16)); grid->AddGrowableCol(1);
        names = {text("Filament (g)","Filament (g)"),text("Druckdauer (h)","Print time (h)"),
            text("Filamentpreis (EUR/kg)","Filament price (EUR/kg)"),text("Mittlere Leistung (W)","Average power (W)"),
            text("Strompreis (EUR/kWh)","Electricity (EUR/kWh)"),text("Maschine (EUR/h)","Machine (EUR/h)")};
        const double defaults[] = {0,0,saved_rate("zh_cost_filament",25,100000),saved_rate("zh_cost_watts",150,100000),
            saved_rate("zh_cost_electricity",0.35,10000),saved_rate("zh_cost_wear",0.10,100000)};
        for (size_t i=0; i<fields.size(); ++i) {
            grid->Add(label(form,names[i]),0,wxALIGN_CENTER_VERTICAL);
            fields[i] = new wxTextCtrl(form,wxID_ANY,wxString::FromUTF8(number(defaults[i])),wxDefaultPosition,FromDIP(wxSize(155,30)));
            fields[i]->SetFont(font()); fields[i]->SetForegroundColour(foreground()); fields[i]->SetMaxLength(64);
            fields[i]->SetName(names[i]); fields[i]->SetToolTip(names[i] + text(" · Komma oder Punkt, keine Tausendertrennzeichen", " · Decimal comma or point; no thousands separators"));
            grid->Add(fields[i],1,wxEXPAND);
            fields[i]->Bind(wxEVT_TEXT,[this,i](wxCommandEvent&){
                if (i < 2) manual = true;
                if (i == 2) price_edited = true;
                if (save) save->SetLabel(text("Tarife speichern","Save rates"));
                calculate();
            });
        }
        formbox->Add(grid,0,wxEXPAND|wxLEFT|wxRIGHT,FromDIP(18));
        error = label(form,"",9); error->SetForegroundColour(wxColour(wxGetApp().dark_mode()?"#FDA4AF":"#B42338"));
        error->SetMinSize(FromDIP(wxSize(-1,38)));
        formbox->Add(error,0,wxEXPAND|wxALL,FromDIP(18));
        auto* reload = button(form,text("Slicing-Daten übernehmen","Use sliced data"));
        reload->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){load_snapshot();});
        formbox->Add(reload,0,wxLEFT|wxRIGHT|wxBOTTOM,FromDIP(18)); form->SetSizer(formbox);
        columns->Add(form,1,wxEXPAND|wxALL,FromDIP(16));
        auto* summary = new wxPanel(scroll); summary->SetBackgroundColour(card()); summary->SetMinSize(FromDIP(wxSize(245,-1)));
        auto* summarybox = new wxBoxSizer(wxVERTICAL);
        summarybox->Add(label(summary,text("GESCHÄTZTE GESAMTKOSTEN","ESTIMATED TOTAL"),9,true,true),0,wxALL,FromDIP(18));
        total = label(summary,wxString::FromUTF8("—"),26,true); total->SetForegroundColour(ZHTheme::accent());
        summarybox->Add(total,0,wxLEFT|wxRIGHT|wxBOTTOM,FromDIP(18));
        const wxString part_names[] = {text("Material","Material"),text("Strom","Energy"),text("Verschleiß / Maschine","Wear / machine")};
        for (int i = 0; i < 3; ++i) part_titles[i] = part_names[i];
        for (size_t i=0;i<3;++i) {
            summarybox->Add(label(summary,part_names[i],10,false,true),0,wxLEFT|wxRIGHT|wxTOP,FromDIP(18));
            parts[i] = label(summary,wxString::FromUTF8("—"),14,true);
            summarybox->Add(parts[i],0,wxLEFT|wxRIGHT|wxTOP,FromDIP(18));
        }
        energy = label(summary,"",9,false,true);
        summarybox->Add(energy,0,wxALL,FromDIP(18)); summary->SetSizer(summarybox);
        columns->Add(summary,0,wxEXPAND|wxRIGHT|wxTOP|wxBOTTOM,FromDIP(16)); scroll->SetSizer(columns);
        outer->Add(scroll,1,wxEXPAND);
        auto* note = label(this,text("Schätzung, keine Messung. EUR als Währung. Ohne Arbeit, Versand und Ausschuss.\nTarife speichern ändert keine Filamentprofile. Komma oder Punkt als Dezimaltrennzeichen.",
            "Estimate, not a measurement. Currency: EUR. Excludes labour, shipping and failed prints.\nSaving rates does not change filament profiles. Decimal comma or point is accepted."),9,false,true);
        note->Wrap(FromDIP(760)); outer->Add(note,0,wxLEFT|wxRIGHT|wxBOTTOM,FromDIP(18));
        auto* footer = new wxBoxSizer(wxHORIZONTAL);
        save = button(this,text("Tarife speichern","Save rates"));
        save->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){
            std::array<double,6> in{}; if(!values(in)) {calculate(); return;}
            const char* keys[] = {"zh_cost_filament","zh_cost_watts","zh_cost_electricity","zh_cost_wear"};
            std::map<std::string,std::string> values_to_save;
            for(size_t i=0;i<4;++i) values_to_save[keys[i]]=number(in[i+2]);
            if(save_settings(values_to_save,this)) save->SetLabel(text("Tarife gespeichert","Rates saved"));
        });
        footer->Add(save);
        auto* copy = button(this,text("Zusammenfassung kopieren","Copy summary"));
        copy->Bind(wxEVT_BUTTON,[this,copy](wxCommandEvent&){
            if (copy_text_to_clipboard(summary_text()))
                copy->SetLabel(text("Kopiert","Copied"));
        });
        footer->Add(copy,0,wxLEFT,FromDIP(8));
        footer->AddStretchSpacer();
        auto* close = button(this,text("Schließen","Close"),true,wxID_CANCEL);
        close->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){EndModal(wxID_CANCEL);}); footer->Add(close);
        outer->Add(footer,0,wxEXPAND|wxALL,FromDIP(18));
        SetSizer(outer); load_snapshot(); SetMinSize(FromDIP(wxSize(790,520))); SetSize(FromDIP(wxSize(860,710))); CentreOnParent(); fade_in(this);
    }
};

class WorkspaceDialog final : public wxDialog {
public:
    explicit WorkspaceDialog(wxWindow* parent)
        : wxDialog(parent,wxID_ANY,wxString::FromUTF8("ZH-Studio · Workspace"))
    {
        SetFont(font()); SetBackgroundColour(background());
        bind_modal_escape(this);
        ZHRounded::Apply(this, 2, RGB(45, 65, 92));
        auto* outer = new wxBoxSizer(wxVERTICAL);
        heading(this,outer,text("Dein Workspace.","Your workspace."),text("Weniger Umwege. Deine Farben. Deine Werkzeuge.","Fewer clicks. Your colours. Your tools."));
        auto* content = new wxPanel(this); content->SetBackgroundColour(card());
        auto* body = new wxBoxSizer(wxVERTICAL);
        body->Add(label(content,text("01  /  AKZENTFARBE","01  /  ACCENT PALETTE"),10,true),0,wxALL,FromDIP(18));
        auto* palettes = new wxBoxSizer(wxHORIZONTAL);
        auto selected = std::make_shared<int>(ZHTheme::index());
        const std::string saved = wxGetApp().app_config->get("zh_theme");
        for(int i=0;i<4;++i) if(saved==ZHTheme::ids[i]) *selected=i;
        auto choices = std::make_shared<std::array<Button*,4>>();
        // Live preview of the accent: the palette itself only applies on restart, so the
        // user gets to see the result before committing to it.
        auto* preview = new wxPanel(content); preview->SetBackgroundColour(background());
        auto* previewbox = new wxBoxSizer(wxVERTICAL);
        auto* preview_bar = new wxPanel(preview); preview_bar->SetBackgroundColour(ZHTheme::accent());
        preview_bar->SetMinSize(FromDIP(wxSize(-1,6)));
        auto* preview_head = label(preview,text("ZH-Studio","ZH-Studio"),12,true);
        auto* preview_btn = button(preview,text("Drucken","Print"));
        preview_btn->SetBorderColor(ZHTheme::accent());
        previewbox->Add(preview_bar,0,wxEXPAND|wxBOTTOM,FromDIP(10));
        previewbox->Add(preview_head,0,wxLEFT|wxBOTTOM,FromDIP(2));
        previewbox->Add(preview_btn,0,wxLEFT,FromDIP(2));
        preview->SetSizer(previewbox);
        auto repaint = [selected,choices,preview_bar,preview_btn]{
            for(int i=0;i<4;++i) {
                (*choices)[i]->SetBorderWidth(i==*selected?3:1);
                (*choices)[i]->SetBorderColor(i==*selected ? foreground() : border());
                (*choices)[i]->SetLabel(wxString::FromUTF8(ZHTheme::ids[i]) + (i==*selected?" *":""));
            }
            const wxColour accent(ZHTheme::normal[*selected]);
            preview_bar->SetBackgroundColour(accent);
            preview_btn->SetBorderColor(accent);
            preview_btn->SetBackgroundColor(accent);
            preview_bar->Refresh();
            preview_btn->SetTextColor(*selected==0||*selected==3 ? wxColour("#081522") : wxColour("#FFFFFE"));
        };
        for(int i=0;i<4;++i) {
            auto* swatch = button(content,wxString::FromUTF8(ZHTheme::ids[i])); (*choices)[i]=swatch;
            swatch->SetBackgroundColor(wxColour(ZHTheme::normal[i]));
            swatch->SetTextColor(i==0||i==3?wxColour("#081522"):wxColour("#FFFFFE"));
            swatch->SetMinSize(FromDIP(wxSize(104,48)));
            swatch->Bind(wxEVT_BUTTON,[selected,repaint,i](wxCommandEvent&){*selected=i;repaint();});
            palettes->Add(swatch,1,wxRIGHT,FromDIP(8));
        }
        repaint(); body->Add(palettes,0,wxEXPAND|wxLEFT|wxRIGHT,FromDIP(18));
        body->Add(label(content,text("02  /  VORSCHAU","02  /  PREVIEW"),10,true),0,wxALL,FromDIP(18));
        body->Add(preview,0,wxEXPAND|wxLEFT|wxRIGHT,FromDIP(18));
        auto* restart = label(content,text("Neue Farben werden nach einem Neustart aktiv. Deine Arbeit bleibt geöffnet.",
            "New colours apply after restarting. Your current work stays open."),9,false,true);
        restart->Wrap(FromDIP(470)); body->Add(restart,0,wxALL,FromDIP(18));
        body->Add(label(content,text("03  /  SPRACHE","03  /  LANGUAGE"),10,true),0,wxALL,FromDIP(18));
        auto* langrow = new wxBoxSizer(wxHORIZONTAL);
        auto lang = std::make_shared<std::string>(wxGetApp().app_config->get("language"));
        auto langbtns = std::make_shared<std::array<Button*,2>>();
        auto langrepaint = [lang,langbtns]{
            const bool de = lang->rfind("de",0)==0;
            (*langbtns)[0]->SetBorderWidth(de?3:1); (*langbtns)[1]->SetBorderWidth(de?1:3);
            (*langbtns)[0]->SetBorderColor(de?foreground():border()); (*langbtns)[1]->SetBorderColor(de?border():foreground());
        };
        const wxString langlabels[2]={wxString::FromUTF8("Deutsch"),wxString::FromUTF8("English")};
        const char* langcodes[2]={"de_DE","en_US"};
        for(int i=0;i<2;++i) {
            auto* b=button(content,langlabels[i]); (*langbtns)[i]=b; b->SetMinSize(FromDIP(wxSize(120,44)));
            b->Bind(wxEVT_BUTTON,[lang,langrepaint,i,langcodes](wxCommandEvent&){*lang=langcodes[i];langrepaint();});
            langrow->Add(b,0,wxRIGHT,FromDIP(8));
        }
        langrepaint(); body->Add(langrow,0,wxLEFT|wxRIGHT|wxBOTTOM,FromDIP(18));
        auto* langnote = label(content,text("Die Web-Oberfläche wechselt sofort. Die vollständige App-Sprache wird nach einem Neustart aktiv.",
            "The web interface switches immediately. The full app language applies after a restart."),9,false,true);
        langnote->Wrap(FromDIP(470)); body->Add(langnote,0,wxLEFT|wxRIGHT|wxBOTTOM,FromDIP(18));
        content->SetSizer(body);
        outer->Add(content,1,wxEXPAND|wxALL,FromDIP(18));
        auto* footer = new wxBoxSizer(wxHORIZONTAL);
        auto* cancel = button(this,text("Abbrechen","Cancel"),false,wxID_CANCEL);
        cancel->Bind(wxEVT_BUTTON,[this](wxCommandEvent&){EndModal(wxID_CANCEL);}); footer->Add(cancel);
        auto* reset = button(this,text("Zurücksetzen","Reset"));
        reset->SetToolTip(text("Akzentfarbe und Sprache auf die ZH-Studio-Standards zurücksetzen.",
                                "Restore the ZH-Studio defaults for accent colour and language."));
        reset->Bind(wxEVT_BUTTON,[selected,lang,repaint,langrepaint](wxCommandEvent&){
            *selected = 0; *lang = "en_US";
            repaint(); langrepaint();
        });
        footer->Add(reset,0,wxLEFT,FromDIP(8)); footer->AddStretchSpacer();
        auto* apply = button(this,text("Übernehmen","Apply"),true,wxID_OK);
        apply->Bind(wxEVT_BUTTON,[this,selected,lang,parent](wxCommandEvent&){
            if (*selected<0||*selected>=4) return;
            std::map<std::string,std::string> settings{{"zh_theme",ZHTheme::ids[*selected]},
                                                       {"language",*lang},{"language_user_set","1"}};
            if (!save_settings(settings,this)) return;
            if (auto* frame = dynamic_cast<MainFrame*>(parent)) {
                const wxString script = wxString::Format("if(window.ZHSetLanguage)ZHSetLanguage('%s');",
                    lang->rfind("de", 0) == 0 ? "de" : "en");
                frame->RunScript(script);
                frame->RunScriptLeft(script);
            }
            EndModal(wxID_OK);
        }); footer->Add(apply); outer->Add(footer,0,wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM,FromDIP(18));
        SetSizerAndFit(outer); CentreOnParent(); fade_in(this);
    }
};

struct ZHCommand {
    wxString title;
    wxString detail;
    wxString shortcut;
    wxString keywords;
    std::function<void()> run;
};

class CommandPalette final : public wxDialog {
    wxTextCtrl* search = nullptr;
    wxListCtrl* results = nullptr;
    wxStaticText* detail = nullptr;
    wxStaticText* empty = nullptr;
    std::vector<ZHCommand> commands;
    bool action_selected{false};

    int selected_row() const
    {
        const long item = results->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        return item == wxNOT_FOUND ? -1 : static_cast<int>(item);
    }

    void select_row(int row)
    {
        const int count = results->GetItemCount();
        if (count == 0) return;
        row = (row % count + count) % count;
        const int previous = selected_row();
        if (previous >= 0)
            results->SetItemState(previous, 0, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
        results->SetItemState(row, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED,
                              wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
        results->EnsureVisible(row);
        update_detail();
    }

    void update_detail()
    {
        const int row = selected_row();
        if (row < 0) { detail->SetLabel(wxEmptyString); return; }
        const long index = results->GetItemData(row);
        if (index < 0 || static_cast<size_t>(index) >= commands.size()) return;
        detail->SetLabel(commands[static_cast<size_t>(index)].detail);
        detail->Wrap(std::max(1, GetClientSize().x - FromDIP(48)));
    }

    void update_metrics()
    {
        const int width = std::max(FromDIP(320), GetClientSize().x - FromDIP(44));
        const int shortcut_width = std::min(FromDIP(130), width / 3);
        wxListItem column;
        column.SetMask(wxLIST_MASK_WIDTH);
        column.SetWidth(width - shortcut_width);
        results->SetColumn(0, column);
        column.SetWidth(shortcut_width);
        results->SetColumn(1, column);
        detail->Wrap(std::max(1, GetClientSize().x - FromDIP(48)));
    }

    void refresh_results()
    {
        const wxString needle = search->GetValue().Trim().Lower();
        results->Freeze();
        results->DeleteAllItems();
        for (size_t i = 0; i < commands.size(); ++i) {
            const ZHCommand& command = commands[i];
            const wxString haystack = (command.title + " " + command.detail + " " +
                                       command.shortcut + " " + command.keywords).Lower();
            if (!needle.IsEmpty() && haystack.Find(needle) == wxNOT_FOUND) continue;
            const long row = results->InsertItem(results->GetItemCount(), command.title);
            results->SetItem(row, 1, command.shortcut);
            results->SetItemData(row, static_cast<long>(i));
        }
        const bool any = results->GetItemCount() > 0;
        empty->Show(!any);
        if (any) select_row(0);
        else detail->SetLabel(wxEmptyString);
        results->Thaw();
        Layout();
    }

    void execute_row(int row)
    {
        // wxTextCtrl/wxListCtrl can report Enter through both their key handler and
        // their activation event. Close and execute only once.
        if (action_selected || row < 0 || row >= results->GetItemCount()) return;
        const long index = results->GetItemData(row);
        if (index < 0 || static_cast<size_t>(index) >= commands.size()) return;
        action_selected = true;
        chosen = commands[static_cast<size_t>(index)].run;
        EndModal(wxID_OK);
    }

public:
    std::function<void()> chosen;

    CommandPalette(wxWindow* parent, MainFrame* frame)
        : wxDialog(parent, wxID_ANY, wxString::FromUTF8("ZH-Studio · Commands"), wxDefaultPosition,
                   parent->FromDIP(wxSize(720, 560)), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    {
        SetFont(font());
        SetBackgroundColour(background());
        bind_modal_escape(this);
        ZHRounded::Apply(this, 2, RGB(45, 65, 92));
        const wxString ctrl = wxString::FromUTF8(Slic3r::GUI::shortkey_ctrl_prefix().c_str());
        auto* plater = frame->plater();
        const bool loaded = frame->is_loaded();
        const bool has_model = plater && !plater->model().objects.empty();
        const bool busy = plater && plater->is_background_process_slicing();
        auto add = [this](const wxString& title, const wxString& description, const wxString& shortcut,
                          const wxString& keywords, bool enabled, std::function<void()> action) {
            if (enabled) commands.push_back({title, description, shortcut, keywords, std::move(action)});
        };

        add(text("Startseite", "Home"), text("Zurück zum Workspace.", "Return to your workspace."), "",
            "home workspace startseite", loaded, [frame] { frame->request_select_tab(MainFrame::tpHome); });
        add(text("Vorbereiten", "Prepare"), text("Modelle und Schneideeinstellungen bearbeiten.", "Edit models and slicing settings."), "",
            "prepare editor model slicing vorbereiten", loaded, [frame] { frame->request_select_tab(MainFrame::tp3DEditor); });
        add(text("Vorschau", "Preview"), text("Geschnittene Schichten und Werkzeugwege prüfen.", "Inspect sliced layers and toolpaths."), "",
            "preview gcode layers vorschau", loaded, [frame] { frame->request_select_tab(MainFrame::tpPreview); });
        add(text("Gerät", "Device"), text("Druckerstatus und Steuerung öffnen.", "Open printer status and controls."), "",
            "device printer monitor geraet", loaded, [frame] { frame->request_select_tab(MainFrame::tpMonitor); });
        add(text("Projekt", "Project"), text("Projektinformationen und Dateien öffnen.", "Open project information and files."), "",
            "project files projekt", loaded && frame->m_project, [frame] { frame->select_tab(frame->m_project); });
        add(text("Kalibrierung", "Calibration"), text("Kalibrierungswerkzeuge öffnen.", "Open calibration tools."), "",
            "calibration kalibrierung", loaded && frame->m_calibration, [frame] { frame->select_tab(frame->m_calibration); });
        add(text("Filament-Verwaltung", "Filament Manager"), text("Materialien und Spulen verwalten.", "Manage materials and spools."), "",
            "filament material spool manager", loaded && frame->web_device(), [frame] { frame->select_tab(frame->web_device()); });
        add(text("Neues Projekt", "New Project"), text("Ein leeres Projekt starten.", "Start an empty project."), ctrl + "N",
            "new project neu projekt", loaded && frame->can_start_new_project(),
            [frame, plater] { if (frame->can_start_new_project()) plater->new_project(); });
        add(text("Projekt öffnen", "Open Project"), text("Eine 3MF- oder Modelldatei öffnen.", "Open a 3MF or model file."), ctrl + "O",
            "open project file oeffnen", loaded && frame->can_open_project(),
            [frame, plater] { if (frame->can_open_project()) plater->load_project(); });
        add(text("Modell importieren", "Import Model"), text("Ein Modell zum aktuellen Projekt hinzufügen.", "Add a model to the current project."), ctrl + "I",
            "import model stl obj 3mf", loaded && frame->can_add_models(),
            [frame, plater] { if (frame->can_add_models()) plater->add_file(); });
        add(text("Projekt speichern", "Save Project"), text("Das aktuelle Projekt speichern.", "Save the current project."), ctrl + "S",
            "save project speichern", loaded && frame->can_save(),
            [frame, plater] { if (frame->can_save()) plater->save_project(); });
        add(text("Projekt speichern unter", "Save Project As"), text("Das Projekt unter einem neuen Namen speichern.", "Save the project under a new name."), ctrl + "Shift+S",
            "save as project speichern unter", loaded && frame->can_save_as(),
            [frame, plater] { if (frame->can_save_as()) plater->save_project(true); });
        add(text("Platte schneiden", "Slice Plate"), text("Die aktuelle Platte schneiden und die Vorschau öffnen.", "Slice the current plate and open Preview."), ctrl + "R",
            "slice plate schneiden slicen", loaded && has_model && !busy && frame->can_slice_plate(),
            [frame] { frame->slice_plate(); });
        add(text("G-code exportieren", "Export G-code"), text("Das geschnittene Ergebnis exportieren.", "Export the sliced result."), ctrl + "G",
            "export gcode file exportieren", loaded && frame->can_export_gcode(),
            [frame, plater] {
                if (frame->can_export_gcode())
                    wxPostEvent(plater, SimpleEvent(EVT_GLTOOLBAR_EXPORT_SLICED_FILE));
            });
        add(text("Platte drucken", "Print Plate"), text("Den Druckdialog für die aktuelle Platte öffnen.", "Open the print dialog for the current plate."), ctrl + "Shift+G",
            "print plate drucken drucker", loaded && frame->can_print_plate(),
            [frame] { frame->print_plate(); });
        add(text("Alle Modelle anordnen", "Arrange All Models"), text("Modelle auf den verfügbaren Platten automatisch anordnen.", "Automatically arrange models on available plates."), "A",
            "arrange layout anordnen alle models", loaded && has_model && !busy && frame->can_add_models() && plater->can_do_ui_job(),
            [frame, plater] {
                if (!frame->can_add_models() || plater->model().objects.empty() ||
                    plater->is_background_process_slicing() || !plater->can_do_ui_job()) return;
                plater->set_prepare_state(Job::PREPARE_STATE_DEFAULT);
                plater->arrange();
            });
        add(text("Auswahl duplizieren", "Duplicate Selection"), text("Die ausgewählten Modelle mit einstellbarer Anzahl kopieren.", "Copy selected models with an adjustable count."), "",
            "duplicate clone copy selection kopieren duplizieren", loaded && frame->can_clone() && plater->can_do_ui_job(),
            [frame, plater] {
                if (!frame->can_clone() || !plater->can_do_ui_job()) return;
                plater->clone_selection();
            });
        add(text("Auswahl zentrieren", "Center Selection"), text("Die Auswahl mittig auf der aktuellen Platte ausrichten.", "Center the selection on the current plate."), "",
            "center centre selection zentrieren mittig", loaded && frame->can_clone() && plater->can_do_ui_job(),
            [frame, plater] {
                if (!frame->can_clone() || !plater->can_do_ui_job()) return;
                plater->center_selection();
            });
        add(text("Auswahl an X ausrichten", "Align Selection X"), text("Die Auswahl an ihrer X-Mitte ausrichten.", "Align the selection by its X center."), "",
            "align x center selection ausrichten x mittig", loaded && frame->can_clone() && plater->can_do_ui_job(),
            [frame, plater] {
                if (!frame->can_clone() || !plater->can_do_ui_job()) return;
                plater->align_selection_x_center();
            });
        add(text("Auswahl an Y ausrichten", "Align Selection Y"), text("Die Auswahl an ihrer Y-Mitte ausrichten.", "Align the selection by its Y center."), "",
            "align y center selection ausrichten y mittig", loaded && frame->can_clone() && plater->can_do_ui_job(),
            [frame, plater] {
                if (!frame->can_clone() || !plater->can_do_ui_job()) return;
                plater->align_selection_y_center();
            });
        add(text("Auswahl an Z ausrichten", "Align Selection Z"), text("Die Auswahl an ihrer Z-Mitte ausrichten.", "Align the selection by its Z center."), "",
            "align z center selection ausrichten z mittig", loaded && frame->can_clone() && plater->can_do_ui_job(),
            [frame, plater] {
                if (!frame->can_clone() || !plater->can_do_ui_job()) return;
                plater->align_selection_z_center();
            });
        add(text("Auswahl kopieren", "Copy Selection"), text("Die Auswahl in die Zwischenablage kopieren.", "Copy the selection to the clipboard."), "",
            "copy clipboard selection kopieren zwischenablage", loaded && frame->can_clone() && plater->can_copy_to_clipboard() && plater->can_do_ui_job(),
            [frame, plater] {
                if (!frame->can_clone() || !plater->can_copy_to_clipboard() || !plater->can_do_ui_job()) return;
                plater->copy_selection_to_clipboard();
            });
        add(text("Auswahl einfügen", "Paste Selection"), text("Modelle aus der Zwischenablage einfügen.", "Paste models from the clipboard."), "",
            "paste clipboard selection einfuegen zwischenablage", loaded && frame->can_add_models() && plater->can_paste_from_clipboard() && plater->can_do_ui_job(),
            [frame, plater] {
                if (!frame->can_add_models() || !plater->can_paste_from_clipboard() || !plater->can_do_ui_job()) return;
                plater->paste_from_clipboard();
            });
        add(text("Alle Objekte als STL exportieren", "Export All Objects as STL"), text("Das gesamte Projekt als eine STL-Datei exportieren.", "Export the whole project as one STL file."), "",
            "export stl mesh model objekte", loaded && frame->can_export_model(),
            [frame, plater] { if (frame->can_export_model()) plater->export_stl(); });
        add(text("Generische 3MF exportieren", "Export Generic 3MF"), text("Eine portable 3MF-Datei ohne herstellerspezifische Erweiterungen exportieren.", "Export a portable 3MF without vendor-specific extensions."), "",
            "export generic 3mf project portable", loaded && frame->can_export_model(),
            [frame, plater] { if (frame->can_export_model()) plater->export_core_3mf(); });
        add(text("Alle geschnittenen Platten exportieren", "Export All Sliced Plates"), text("Die geschnittenen Ergebnisse aller druckbereiten Platten exportieren.", "Export the sliced results of every print-ready plate."), "",
            "export all sliced plates gcode 3mf alle platten", loaded && frame->can_export_all_gcode(),
            [frame, plater] {
                if (frame->can_export_all_gcode())
                    wxPostEvent(plater, SimpleEvent(EVT_GLTOOLBAR_EXPORT_ALL_SLICED_FILE));
            });
        add(text("Preset-Bündel exportieren", "Export Preset Bundle"), text("Drucker-, Material- und Prozesseinstellungen als Dateien sichern.", "Back up printer, filament and process settings as files."), "",
            "export preset bundle config settings konfiguration sichern", loaded, [frame] { frame->export_config(); });
        add(text("Aktuelle Platte prüfen", "Validate Current Plate"), text("Bauvolumen, Material und Schneideeinstellungen prüfen.", "Check build volume, material and slicing settings."), "",
            "validate check plate pruefen bauvolumen material", loaded && has_model && !busy && frame->can_add_models() && plater->can_do_ui_job(),
            [frame, plater] {
                if (!frame->can_add_models() || plater->model().objects.empty() ||
                    plater->is_background_process_slicing() || !plater->can_do_ui_job()) return;
                bool model_fits = false;
                bool validate_error = false;
                plater->validate_current_plate(model_fits, validate_error);
                const bool valid = model_fits && !validate_error;
                wxMessageBox(valid
                    ? text("Die aktuelle Platte hat die Prüfung bestanden.", "The current plate passed validation.")
                    : text("Die aktuelle Platte ist noch nicht druckbereit. Die markierten Hinweise und Fehler prüfen.",
                           "The current plate is not print-ready. Review the highlighted warnings and errors."),
                    "ZH-Studio", wxOK | (valid ? wxICON_INFORMATION : wxICON_WARNING), frame);
            });
        add(text("Druckkosten", "Print Costs"), text("Material, Energie und Maschinenkosten schätzen.", "Estimate material, energy and machine costs."), "",
            "cost price money druckkosten kosten", loaded, [frame] { show_zh_print_costs(frame); });
        add(text("Workspace-Einstellungen", "Workspace Settings"), text("Akzentfarbe und Sprache ändern.", "Change accent palette and language."), "",
            "workspace theme color language einstellungen", loaded, [frame] { show_zh_workspace_settings(frame); });
        add(text("Anwendungseinstellungen", "Preferences"), text("Drucker, Darstellung und Systemeinstellungen öffnen.", "Open printer, appearance and system settings."), ctrl + ",",
            "preferences settings einstellungen dark mode darstellung", loaded, [] { wxGetApp().open_preferences(); });
        add(text("Kamera einpassen", "Fit Camera"), text("Die Kamera an Szene oder Auswahl anpassen.", "Fit the camera to the scene or selection."), "Z",
            "fit camera zoom kamera", loaded && frame->can_change_view(),
            [frame] { if (frame->can_change_view()) frame->view_zoom_to_fit(); });
        add(text("Ansicht von oben", "Top View"), text("Die Kamera senkrecht über die Platte bewegen.", "Move the camera directly above the plate."), ctrl + "1",
            "view top camera ansicht oben", loaded && frame->can_change_view(),
            [frame] { if (frame->can_change_view()) frame->select_view("top"); });
        add(text("Frontansicht", "Front View"), text("Die Kamera vor die Platte bewegen.", "Move the camera in front of the plate."), ctrl + "3",
            "view front camera ansicht vorne", loaded && frame->can_change_view(),
            [frame] { if (frame->can_change_view()) frame->select_view("front"); });
        add(text("Rechtsansicht", "Right View"), text("Die Kamera rechts neben die Platte bewegen.", "Move the camera to the right of the plate."), ctrl + "6",
            "view right camera ansicht rechts", loaded && frame->can_change_view(),
            [frame] { if (frame->can_change_view()) frame->select_view("right"); });
        add(text("Isometrische Ansicht", "Isometric View"), text("Die Kamera in die Standardperspektive bewegen.", "Move the camera to the default perspective."), ctrl + "7",
            "view iso isometric camera ansicht perspektive", loaded && frame->can_change_view(),
            [frame] { if (frame->can_change_view()) frame->select_view("iso"); });
        add(text("Protokoll anzeigen", "Show Log"), text("Das Diagnoseprotokoll öffnen.", "Open the diagnostic log."), "",
            "log debug diagnostik protokoll", loaded, [frame] { frame->show_log_window(); });

        auto* outer = new wxBoxSizer(wxVERTICAL);
        auto* eyebrow = label(this, "ZH-STUDIO / COMMAND", 9, true);
        eyebrow->SetForegroundColour(ZHTheme::hovered());
        outer->Add(eyebrow, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(22));
        outer->Add(label(this, text("Was möchtest du tun?", "What do you want to do?"), 20, true),
                   0, wxLEFT | wxRIGHT | wxTOP | wxBOTTOM, FromDIP(18));
        search = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, FromDIP(wxSize(-1, 42)), wxTE_PROCESS_ENTER);
        search->SetHint(text("Aktionen suchen …", "Search actions …"));
        search->SetFont(font(11));
        search->SetForegroundColour(foreground());
        search->SetBackgroundColour(card());
        outer->Add(search, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(22));
        outer->AddSpacer(FromDIP(12));
        results = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, FromDIP(wxSize(-1, 330)),
                                 wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_NO_HEADER | wxBORDER_NONE);
        results->SetFont(font(10));
        results->SetBackgroundColour(card());
        results->SetForegroundColour(foreground());
        results->InsertColumn(0, wxEmptyString, wxLIST_FORMAT_LEFT, FromDIP(510));
        results->InsertColumn(1, wxEmptyString, wxLIST_FORMAT_RIGHT, FromDIP(130));
        results->Bind(wxEVT_LIST_ITEM_SELECTED, [this](wxListEvent&) { update_detail(); });
        results->Bind(wxEVT_LIST_ITEM_ACTIVATED, [this](wxListEvent& event) { execute_row(event.GetIndex()); });
        results->Bind(wxEVT_KEY_DOWN, [this](wxKeyEvent& event) {
            const int row = selected_row();
            if (event.GetKeyCode() == WXK_RETURN) execute_row(row);
            else if (event.GetKeyCode() == WXK_DOWN) select_row(row + 1);
            else if (event.GetKeyCode() == WXK_UP) select_row(row - 1);
            else if (event.GetKeyCode() == WXK_ESCAPE) EndModal(wxID_CANCEL);
            else event.Skip();
        });
        outer->Add(results, 1, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(22));
        empty = label(this, text("Keine passende Aktion gefunden.", "No matching action found."), 10, false, true);
        empty->Hide();
        outer->Add(empty, 0, wxALL | wxALIGN_CENTER, FromDIP(18));
        detail = label(this, wxEmptyString, 9, false, true);
        detail->SetMinSize(FromDIP(wxSize(-1, 34)));
        results->Bind(wxEVT_SIZE, [this](wxSizeEvent& event) { update_metrics(); event.Skip(); });
        update_metrics();
        outer->Add(detail, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(12));
        auto* footer = label(this, text("Hoch/Runter: auswählen    Enter: ausführen    Esc: schließen",
                                       "Up/Down: select    Enter: run    Esc: close"), 9, false, true);
        outer->Add(footer, 0, wxALL, FromDIP(18));
        SetSizerAndFit(outer);
        SetMinSize(FromDIP(wxSize(640, 480)));
        CentreOnParent();
        search->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { refresh_results(); });
        search->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { execute_row(selected_row()); });
        search->Bind(wxEVT_KEY_DOWN, [this](wxKeyEvent& event) {
            const int row = selected_row();
            if (event.GetKeyCode() == WXK_DOWN) select_row(row + 1);
            else if (event.GetKeyCode() == WXK_UP) select_row(row - 1);
            else if (event.GetKeyCode() == WXK_RETURN) execute_row(row);
            else if (event.GetKeyCode() == WXK_ESCAPE) EndModal(wxID_CANCEL);
            else event.Skip();
        });
        refresh_results();
        search->SetFocus();
        fade_in(this);
    }
};

} // namespace
void show_zh_print_costs(wxWindow* parent) {CostDialog dialog(parent);dialog.ShowModal();}
void show_zh_workspace_settings(wxWindow* parent)
{
    WorkspaceDialog dialog(parent);
    dialog.ShowModal();
}
bool zh_has_visible_modal_dialog(wxWindow* parent)
{
    for (wxWindowList::compatibility_iterator node = wxTopLevelWindows.GetFirst();
         node != nullptr; node = node->GetNext()) {
        auto* dialog = dynamic_cast<wxDialog*>(node->GetData());
        if (dialog && dialog != parent && dialog->IsShown() && dialog->IsModal())
            return true;
    }
    return false;
}

bool show_zh_command_palette(wxWindow* parent)
{
    auto* frame = dynamic_cast<MainFrame*>(parent);
    if (!frame || !parent->IsShown() || zh_has_visible_modal_dialog(parent)) return false;
    wxWindow* active = wxGetActiveWindow();
    if (active && active != frame) return false;

    std::function<void()> chosen;
    {
        CommandPalette dialog(parent, frame);
        if (dialog.ShowModal() == wxID_OK)
            chosen = std::move(dialog.chosen);
    }
    if (chosen) chosen();
    return true;
}
} }
