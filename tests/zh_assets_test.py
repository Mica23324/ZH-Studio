"""Validate ZH-Studio UI assets and source-level UX contracts."""
from pathlib import Path
from html.parser import HTMLParser
import xml.etree.ElementTree as ET
import plistlib
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
class Page(HTMLParser):
    def __init__(self):
        super().__init__(); self.ids=[]; self.links=[]; self.has_csp=False
    def handle_starttag(self, tag, attrs):
        attrs=dict(attrs)
        if 'id' in attrs: self.ids.append(attrs['id'])
        for key in ('src','href'):
            if key in attrs: self.links.append(attrs[key])
        if tag=='meta' and attrs.get('http-equiv')=='Content-Security-Policy': self.has_csp=True

for name in ('home.html','left.html'):
    path=ROOT/'resources/web/homepage3'/name
    page=Page(); page.feed(path.read_text(encoding='utf-8'))
    assert page.has_csp, f'{name}: CSP must stay enabled'
    duplicates = {value for value in page.ids if page.ids.count(value) > 1}
    assert not duplicates, f'{name}: duplicate IDs {sorted(duplicates)}'
    for link in page.links:
        if re.match(r'^(https?:|data:|#|javascript:)',link): continue
        local=link.split('?',1)[0].split('#',1)[0]
        assert (path.parent/local).exists(),f'{name}: missing {local}'
    print(f'{name}: IDs, CSP and {len(page.links)} resource links checked')
for name in ('ZH-Studio.svg','splash_logo.svg','BambuStudio_about.svg','topbar_logo.svg'):
    ET.parse(ROOT/'resources/images'/name)
    print(f'{name}: valid SVG XML')
for path in (ROOT/'resources/profiles',ROOT/'resources/printers'):
    assert path.exists()

palette_source=(ROOT/'src/slic3r/GUI/ZHStudioTools.cpp').read_text(encoding='utf-8')
webview_source=(ROOT/'src/slic3r/GUI/Widgets/WebView.cpp').read_text(encoding='utf-8')
frame_source=(ROOT/'src/slic3r/GUI/MainFrame.cpp').read_text(encoding='utf-8')
plater_source=(ROOT/'src/slic3r/GUI/Plater.cpp').read_text(encoding='utf-8')
topbar_source=(ROOT/'src/slic3r/GUI/BBLTopbar.cpp').read_text(encoding='utf-8')
splash_source=(ROOT/'src/slic3r/GUI/ZHSplash.cpp').read_text(encoding='utf-8')
splash_header=(ROOT/'src/slic3r/GUI/ZHSplash.hpp').read_text(encoding='utf-8')
startup_source=(ROOT/'src/slic3r/GUI/GUI_App.cpp').read_text(encoding='utf-8')
notebook_source=(ROOT/'src/slic3r/GUI/Notebook.cpp').read_text(encoding='utf-8')
staticbox_source=(ROOT/'src/slic3r/GUI/Widgets/StaticBox.cpp').read_text(encoding='utf-8')
staticbox_header=(ROOT/'src/slic3r/GUI/Widgets/StaticBox.hpp').read_text(encoding='utf-8')
progress_source=(ROOT/'src/slic3r/GUI/Widgets/ProgressBar.cpp').read_text(encoding='utf-8')
progress_header=(ROOT/'src/slic3r/GUI/Widgets/ProgressBar.hpp').read_text(encoding='utf-8')
libslic3r_cmake=(ROOT/'src/libslic3r/CMakeLists.txt').read_text(encoding='utf-8')
guide_source=(ROOT/'src/slic3r/GUI/WebGuideDialog.cpp').read_text(encoding='utf-8')
guide_header=(ROOT/'src/slic3r/GUI/WebGuideDialog.hpp').read_text(encoding='utf-8')
config_source=(ROOT/'src/libslic3r/Config.cpp').read_text(encoding='utf-8')
network_policy_source=(ROOT/'src/libslic3r/NetworkPolicy.hpp').read_text(encoding='utf-8')
print_config_source=(ROOT/'src/libslic3r/PrintConfig.cpp').read_text(encoding='utf-8')
app_config_source=(ROOT/'src/libslic3r/AppConfig.cpp').read_text(encoding='utf-8')
http_source=(ROOT/'src/slic3r/Utils/Http.cpp').read_text(encoding='utf-8')
network_agent_source=(ROOT/'src/slic3r/Utils/NetworkAgent.cpp').read_text(encoding='utf-8')
webview_cmake_source=(ROOT/'src/slic3r/CMakeLists.txt').read_text(encoding='utf-8')
wx_cmake_source=(ROOT/'deps/wxWidgets/wxWidgets.cmake').read_text(encoding='utf-8')
wx_patch_source=(ROOT/'deps/wxWidgets/0002-webview2-browser-arguments.patch').read_text(encoding='utf-8')
wx_patch_script=(ROOT/'deps/wxWidgets/apply_webview2_patch.cmake').read_text(encoding='utf-8')
device_server_source=(ROOT/'src/slic3r/GUI/DeviceWeb/DeviceHttpServer.cpp').read_text(encoding='utf-8')
tcp_console_source=(ROOT/'src/slic3r/Utils/TCPConsole.cpp').read_text(encoding='utf-8')
bonjour_source=(ROOT/'src/slic3r/Utils/Bonjour.cpp').read_text(encoding='utf-8')
gui_init_source=(ROOT/'src/slic3r/GUI/GUI_Init.cpp').read_text(encoding='utf-8')
gui_init_header=(ROOT/'src/slic3r/GUI/GUI_Init.hpp').read_text(encoding='utf-8')
mesh_boolean_source=(ROOT/'src/libslic3r/MeshBoolean.cpp').read_text(encoding='utf-8')
post_processor_source=(ROOT/'src/libslic3r/GCode/PostProcessor.cpp').read_text(encoding='utf-8')
home_source=(ROOT/'resources/web/homepage3/home.html').read_text(encoding='utf-8')
web_source=(ROOT/'resources/web/homepage3/js/zh-studio.js').read_text(encoding='utf-8')
assert 'static const double zero = 0.0;\n        return zero;' in config_source, 'Const float accessor must not return a reference to a temporary'
assert 'for (uint32_t n = 0; n < numConnComps; ++n)' in mesh_boolean_source, 'Connected-component index must match the MCUT count type'
assert 'snprintf(num_str, sizeof(num_str), "%zu", line_number);' in post_processor_source, 'G-code line numbers must use the size_t printf format'
assert 'cli_offline.load(std::memory_order_acquire) ||' in network_policy_source and 'config_offline.load(std::memory_order_acquire)' in network_policy_source, 'Offline sources must combine with OR semantics'
assert 'store(false' not in network_policy_source and network_policy_source.count('store(true, std::memory_order_release)') == 2, 'Offline policy must be monotonic within a process'
assert 'this->add("offline", coBool)' in print_config_source and 'new ConfigOptionBool(false)' in print_config_source.split('this->add("offline", coBool)',1)[1].split('/*def',1)[0], 'CLI offline must be a real boolean option defaulting to false'
assert 'set_bool("offline_mode", false)' in app_config_source, 'Persisted offline mode must default to false'
assert re.search(r'bool\s+offline \{ false \};',gui_init_header) and 'params.offline = m_config.has("offline") && m_config.opt_bool("offline");' in (ROOT/'src/BambuStudio.cpp').read_text(encoding='utf-8'), 'CLI offline must reach GUI startup explicitly'
assert gui_init_source.index('NetworkPolicy::enable_cli_offline();') < gui_init_source.index('new GUI::GUI_App()'), 'CLI network policy must be active before GUI construction'
http_perform=http_source.split('void Http::priv::http_perform()',1)[1].split('CURLcode res = ::curl_easy_perform(curl);',1)[0]
assert http_perform.index('if (NetworkPolicy::is_offline())') < http_perform.index('::curl_easy_setopt'), 'Offline guard must precede every libcurl operation'
assert 'errorfn(std::string{}, "Network access is disabled by offline mode", 0);' in http_perform and 'return;' in http_perform, 'Offline HTTP must report exactly one status-0 error and return'
init_config=startup_source.split('void GUI_App::init_app_config()',1)[1].split('void GUI_App::update_log_sink_region()',1)[0]
assert init_config.index('app_config->load();') < init_config.index('NetworkPolicy::enable_config_offline();') < init_config.index('s_get_log_enc_opts()'), 'Persisted offline policy must be loaded before remote logging setup'
assert 'if (NetworkPolicy::is_offline())\n        enc_options.enc_key_url.clear();' in startup_source, 'Offline logging must keep the bundled encryption key'
assert '#include "DeviceWeb/DeviceWebPage.hpp"' in palette_source, 'Filament Manager navigation needs the complete page type'
assert re.search(r'if \(show_zh_command_palette\(this\)\) return;\s*\n\s*evt\.Skip\(\);',frame_source), 'Unconsumed command shortcuts must propagate'
frame_hook=frame_source.split('this->Bind(wxEVT_CHAR_HOOK',1)[1]
assert frame_hook.index('if (zh_has_visible_modal_dialog(this))') < frame_hook.index('const int key_code = evt.GetKeyCode();'), 'Application shortcuts must not run behind a visible modal dialog'
assert 'bool zh_has_visible_modal_dialog(wxWindow* parent);' in (ROOT/'src/slic3r/GUI/ZHStudioTools.hpp').read_text(encoding='utf-8') and 'zh_has_visible_modal_dialog(parent)' in palette_source, 'Palette and main-frame shortcuts must share the modal guard'
assert 'wxTopLevelWindows.GetFirst()' in palette_source and 'dialog->IsModal()' in palette_source, 'Palette launcher must reject visible modal dialogs portably'
assert 'if (active && active != frame) return false;' in palette_source, 'Palette launcher must allow a null macOS active window but reject foreign focus'
assert 'show_zh_command_palette(m_frame)' in topbar_source and 'shortkey_ctrl_prefix()' in topbar_source, 'Topbar must expose the platform-aware command palette'
assert 'Commands · Ctrl+K' not in topbar_source, 'Topbar shortcut label must not hard-code Control'
assert "search->SetFocus();" in palette_source, 'Command palette must start with search focused'
assert all(field in palette_source for field in ('command.title + " " + command.detail','command.shortcut + " " + command.keywords')), 'Command palette search fields regressed'
assert 'chosen = std::move(dialog.chosen);' in palette_source and palette_source.index('if (chosen) chosen();') > palette_source.index('CommandPalette dialog(parent, frame);'), 'Palette must be destroyed before its action can recreate the GUI'
assert 'parent->FromDIP(wxSize(720, 560))' in palette_source, 'Palette DPI conversion must use the constructed parent'
assert all(f'frame->select_tab(frame->{page});' in palette_source for page in ('m_project', 'm_calibration', 'web_device()')), 'Optional pages must be selected by page pointer'
assert all(f'frame->{guard}()' in palette_source for guard in ('can_start_new_project', 'can_open_project', 'can_add_models', 'can_export_gcode', 'can_slice_plate', 'can_print_plate', 'can_clone', 'can_export_model', 'can_export_all_gcode')), 'Palette actions must reuse the main-frame state guards'
assert 'if (frame->can_export_gcode())' in palette_source, 'Export must be revalidated before execution'
assert '[frame] { frame->slice_plate(); }' in palette_source and 'if (prepare_slice(false))' in frame_source, 'Slice Plate must reuse the authoritative preflight'
assert 'check_ams_status(all_plates)' in frame_source and 'prepare_slice(all_plates)' in frame_source, 'Slice preflight must check AMS before posting the slice event'
assert '[frame] { frame->print_plate(); }' in palette_source and 'm_plater->apply_background_progress();' in frame_source and 'm_print_enable = can_print_plate();' in frame_source, 'Print Plate must refresh and revalidate before printing'
assert '!m_plater->get_preview_canvas3D()->is_all_plates_selected()' in frame_source, 'Single-plate printing must reject all-plates selection'
assert 'plater->set_prepare_state(Job::PREPARE_STATE_DEFAULT);' in palette_source and 'plater->arrange();' in palette_source, 'Arrange All must use the default all-plates job'
assert all(f'plater->{method}();' in palette_source for method in ('clone_selection', 'center_selection', 'align_selection_x_center', 'align_selection_y_center', 'align_selection_z_center', 'copy_selection_to_clipboard', 'paste_from_clipboard', 'export_stl', 'export_core_3mf')), 'Selection and portable-export quick actions regressed'
assert 'plater->can_copy_to_clipboard()' in palette_source and 'plater->can_paste_from_clipboard()' in palette_source, 'Clipboard quick actions must use the Plater clipboard guards'
assert 'EVT_GLTOOLBAR_EXPORT_ALL_SLICED_FILE' in palette_source and 'if (frame->can_export_all_gcode())' in palette_source, 'Export-all must revalidate and reuse the established export event'
assert 'frame->export_config();' in palette_source, 'Preset export must reuse the existing application flow'
assert 'return path.empty() || path == "<cancel>";' in plater_source and plater_source.count('if (export_file_cancelled(path))') == 3, 'Cancelled STL, 3MF and toolpath OBJ exports must not write the dialog sentinel'
assert 'copy_text_to_clipboard(summary_text())' in palette_source and 'wxTheClipboard->SetData' not in palette_source, 'Cost summary must report clipboard success only when data was accepted'
assert 'loaded && has_model && !busy && frame->can_add_models() && plater->can_do_ui_job()' in palette_source, 'Validation must be hidden when its execution guard rejects the mode'
assert 'plater->validate_current_plate(model_fits, validate_error);' in palette_source, 'Plate validation must use the existing validator'
assert 'wxGetApp().open_preferences();' in palette_source, 'Preferences must use the application-owned modal flow'
assert 'frame->can_change_view()' in palette_source and 'm_plater->get_current_canvas3D()' in frame_source, 'Camera commands must target the active canvas'
assert all(f'frame->select_view("{view}");' in palette_source for view in ('top', 'front', 'right', 'iso')), 'Standard camera views regressed'
assert all(f'ctrl + "{key}"' in palette_source for key in ('1', '3', '6', '7')), 'Camera shortcuts must show the platform modifier'
assert 'results->Bind(wxEVT_SIZE' in palette_source and 'column.SetWidth(width - shortcut_width);' in palette_source, 'Palette columns must fit the resized dialog'
assert 'if (action_selected || row < 0 || row >= results->GetItemCount()) return;' in palette_source and palette_source.index('action_selected = true;') < palette_source.index('EndModal(wxID_OK);', palette_source.index('void execute_row')), 'Palette activation must be idempotent across key and command events'
assert 'id="zh-command-launcher"' in home_source and "ZHCommand('zh_commands')" in home_source, 'Homepage must expose the command palette'
assert 'GetName() != "Home"' in webview_source and 'EnsureHomepageKeyboardShortcuts();' in webview_source, 'Only the homepage WebView may relinquish browser accelerators'
assert 'put_AreBrowserAcceleratorKeysEnabled(FALSE)' in webview_source and 'GetNativeBackend()' in webview_source, 'Homepage shortcuts must reach JavaScript after the asynchronous WebView2 backend is ready'
assert 'if (SUCCEEDED(settings3->put_AreBrowserAcceleratorKeysEnabled(FALSE)))\n            m_homepageKeyboardConfigured = true;' in webview_source, 'Homepage keyboard setup must retry after a transient WebView2 failure'
assert palette_source.count('bind_modal_escape(this);') == 3, 'All custom ZH dialogs must close on Escape from any focused child'
assert 'id="zh-command-shortcut"' in home_source and 'ev.metaKey' in web_source and 'commandModifier' in web_source, 'Homepage command shortcut must support Command and Control'
assert "command !== 'zh_commands'" in web_source and "ZHCommand('zh_commands')" in web_source, 'Homepage bridge must allow only the explicit command launcher'
assert 'command_str == "zh_commands"' in startup_source and 'show_zh_command_palette(mainframe)' in startup_source, 'Native homepage bridge must open the command palette'
assert 'NetworkPolicy::is_offline() && !is_offline_local_web_command(command_str)' in startup_source, 'Offline mode must gate homepage bridge commands'
assert 'reply.put("result", "offline")' in startup_source and 'pt::write_json(reply_stream, reply, false);' in startup_source, 'Blocked homepage commands must answer so the page can complete its request'
web_dialog_source=(ROOT/'src/slic3r/GUI/WebViewDialog.cpp').read_text(encoding='utf-8')
assert 'const std::string response_js = json(response).dump();' in web_dialog_source and "window.postMessage('%s')" not in web_dialog_source, 'Bridge responses must be serialized as JavaScript string literals'
offline_web_commands=startup_source.split('bool is_offline_local_web_command',1)[1].split('} // namespace',1)[0]
for local_command in ('get_login_info','get_recent_projects','get_web_shortcut','homepage_newproject','homepage_openproject',
                      'homepage_open_recentfile','homepage_delete_recentfile','homepage_delete_all_recentfile',
                      'homepage_explore_recentfile','zh_print_costs','zh_workspace','zh_commands','zh_set_language'):
    assert f'"{local_command}"' in offline_web_commands, f'Offline homepage command must stay available: {local_command}'
for online_command in ('homepage_login_or_register','homepage_modeldepot','request_project_download','open_project',
                        'begin_network_plugin_download','search_wiki','get_academy_list','modelmall_model_open'):
    assert f'"{online_command}"' not in offline_web_commands, f'Online homepage command must be blocked offline: {online_command}'
offline_url_policy=webview_source.split('bool is_offline_local_url',1)[1].split('/// Checks whether a file URL',1)[0]
assert all(scheme in offline_url_policy for scheme in ('"wxfs:"','"bbl:"','"memory:"','"data:"','"blob:"','"about:blank"','"file:"')), 'Offline URL policy must retain all process-local schemes'
assert 'host != "localhost"' in offline_url_policy and 'path.StartsWith("//")' in offline_url_policy and r'path.StartsWith("\\\\")' in offline_url_policy, 'Offline file URLs must reject remote authorities and UNC paths'
assert 'if (path.StartsWith("/"))' not in offline_url_policy, 'Local file:/// URLs must not be mistaken for UNC paths'
assert 'NetworkPolicy::is_offline() && !is_offline_local_url(url)' in webview_source.split('bool local_target_missing',1)[1].split('} // namespace',1)[0], 'Offline startup must not probe remote file URLs'
offline_setup=webview_source.split('bool EnsureOfflinePolicy()',1)[1].split('void FlushOfflineTarget()',1)[0]
assert 'ComPtr<ICoreWebView2_22> webView22;' in offline_setup and 'IID_PPV_ARGS(&webView22)' in offline_setup, 'Offline WebView2 setup must require the SDK interface that exposes all-frame filters'
assert 'AddWebResourceRequestedFilterWithRequestSourceKinds(' in offline_setup and 'COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL' in offline_setup and 'COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS_ALL' in offline_setup, 'Offline filter must cover every frame and resource context'
assert 'const auto fallback_response = probe_response;' in offline_setup and '[fallback_response]' in offline_setup, 'Offline callback must own a validated fallback response without retaining the WebView'
assert offline_setup.count('return deny();') >= 2 and 'args->put_Response(fallback_response.Get())' in offline_setup, 'Unknown offline requests must fail closed with a synthetic response'
assert 'CreateWebResourceResponse(' in offline_setup and 'nullptr, 403, L"Blocked by offline mode"' in offline_setup, 'Remote WebView resources must receive a synthetic 403 before network access'
assert 'remove_WebResourceRequested(m_offlineResourceToken)' in webview_source and 'RemoveWebResourceRequestedFilterWithRequestSourceKinds(' in webview_source, 'Offline WebView teardown must unregister its handler and filter'
assert 'edgeView->SetOfflineTarget(url2);' in webview_source and 'offline ? wxEmptyString : url2' in webview_source, 'Offline initial navigation must be deferred until the resource firewall is installed'
assert 'thiz->EnsureOfflinePolicy();\n            thiz->FlushOfflineTarget();' in webview_source and 'wxWebViewEdge::LoadURL(m_offlineTarget);' in webview_source, 'Backend polling must install the firewall before loading the queued local target'
assert 'm_offlineTarget = url;\n            m_offlineTargetDispatched = false;' in webview_source and 'm_offlineTargetDispatched = true;\n            m_offlinePendingUrl.clear();' in webview_source, 'Queued offline navigations must preserve the latest target without duplicate dispatch'
for argument in ('--disable-background-networking','--disable-component-update','--disable-default-apps','--disable-sync','--disable-domain-reliability','--no-first-run','--no-default-browser-check','--proxy-server=direct://','--host-resolver-rules='):
    assert argument in webview_source, f'Offline WebView2 browser argument missing: {argument}'
assert webview_source.index('wxUnsetEnv("WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS");') < webview_source.index('#if !BBL_RELEASE_TO_PUBLIC'), 'Offline mode must suppress the internal remote-debugging listener'
assert 'NetworkPolicy::is_offline() && !is_offline_local_url(e.GetURL())' in webview_source and 'e.Veto();' in webview_source, 'Offline top-level and frame navigations must be vetoed'
assert 'NetworkPolicy::is_offline() && !is_offline_local_url(url2)' in webview_source, 'The common offline loader must reject remote URLs'
sdk_include=webview_cmake_source.index('target_include_directories(libslic3r_gui BEFORE PRIVATE', webview_cmake_source.index('if (WIN32)'))
fallback_include=webview_cmake_source.index('${CMAKE_CURRENT_SOURCE_DIR}/../../deps/WebView2/include', sdk_include)
assert sdk_include < fallback_include and '_toolchain/webview2-sdk/native/include' in webview_cmake_source[sdk_include:fallback_include], 'The project-local WebView2 SDK must precede the older dependency SDK'
assert 'SetAdditionalBrowserArguments' in wx_patch_source and 'm_additionalBrowserArguments' in wx_patch_source and 'put_AdditionalBrowserArguments(browserArguments.wc_str())' in wx_patch_source, 'Reproducible wxWidgets patch must merge custom browser arguments with the user agent'
assert '0002-webview2-browser-arguments.patch' in wx_cmake_source and 'apply_webview2_patch.cmake' in wx_cmake_source, 'Dependency configuration must apply the reproducible WebView2 extension'
assert 'apply --reverse --check' in wx_patch_script and 'already applied' in wx_patch_script, 'Dependency patch application must be idempotent'
agent_library=network_agent_source.split('std::string NetworkAgent::get_libpath_in_current_directory',1)[1].split('int NetworkAgent::initialize_network_module',1)[0]
assert 'GetModuleFileNameW(NULL' in agent_library and r'find_last_of(L"/\\")' in agent_library and 'MultiByteToWideChar(CP_UTF8' in agent_library and 'WideCharToMultiByte(CP_UTF8' in agent_library, 'Network plug-in fallback must resolve the actual executable directory with Unicode-safe conversions'
assert 'bambu-studio.exe' not in agent_library and 'executable(32768)' in agent_library, 'Network plug-in fallback must not assume the old executable name or a fixed short path'
agent_init=network_agent_source.split('int NetworkAgent::initialize_network_module',1)[1].split('boost::filesystem::path data_dir_path',1)[0]
assert agent_init.index('NetworkPolicy::is_offline()') < agent_init.index('data_dir()'), 'Offline network-agent initialization must return before DLL, certificate or profile work'
rating=network_agent_source.split('int NetworkAgent::put_model_mall_rating(',1)[1].split('int NetworkAgent::get_oss_config',1)[0]
assert 'network_agent && put_model_mall_rating_url_ptr' in rating and 'get_model_publish_url_ptr' not in rating, 'Model rating must guard the function pointer it actually calls'
plugin_download=startup_source.split('int GUI_App::download_plugin',1)[1].split('int result = 0;',1)[0]
assert plugin_download.index('NetworkPolicy::is_offline()') < plugin_download.index('show_dialog('), 'Offline plug-in downloads must fail before creating any HTTP request'
assert 'pro_fn(InstallStatusDownloadFailed, 0, false)' not in startup_source and startup_source.count('bool canceled = false;') >= 2, 'Plug-in progress callbacks take a bool reference; offline paths must pass an lvalue'
device_start=device_server_source.split('void DeviceHttpServer::start()',1)[1].split('void DeviceHttpServer::stop()',1)[0]
assert device_start.index('NetworkPolicy::is_offline()') < device_start.index('acceptor_.open'), 'Offline device HTTP server must not bind a listener'
tcp_queue=tcp_console_source.split('bool TCPConsole::run_queue()',1)[1].split('try {',1)[0]
assert 'operation_aborted' in tcp_queue and tcp_queue.index('NetworkPolicy::is_offline()') < tcp_queue.index('return false;'), 'Offline printer TCP connections must abort before DNS resolution'
bonjour_lookup=bonjour_source.split('void Bonjour::priv::lookup_perform()',1)[1].split('service_dn =',1)[0]
assert bonjour_lookup.index('NetworkPolicy::is_offline()') < bonjour_lookup.index('completefn()'), 'Offline Bonjour lookup must complete without creating a socket'
assert '"language_user_set","1"' in palette_source and 'frame->RunScript(script);' in palette_source and 'frame->RunScriptLeft(script);' in palette_source, 'Workspace language changes must persist and update both web panes'
assert 'wxBitmap m_frame;' in splash_header and 'if (!m_frame.IsOk() || m_frame.GetSize() != sz)' in splash_source, 'Splash must reuse its offscreen bitmap'
assert 'wxFontInfo info(pt);' in splash_source and 'pt * dpi' not in splash_source, 'Splash point fonts must not be DPI-scaled twice'
assert 'CreateFontAtDPI(value, font_dpi, c)' in splash_source and '96.0 * dpi' in splash_source, 'Splash text must use the target monitor DPI, not the memory DC DPI'
assert all(f'static const wxFont font = make_font({size}, {bold});' in splash_source for size,bold in ((10,'true'),(30,'true'),(15,'true'),(10,'false'))), 'Splash fonts must be reused instead of allocated every frame'
assert 'm_motion_enabled(ZHMotion::enabled())' in splash_source and 'if (!m_motion_enabled)' in splash_source, 'Splash must honor reduced-motion settings'
assert 'if (m_motion_enabled && !m_timer.Start(33))' in splash_source and 'using a static splash' in splash_source, 'Splash timer failure must fall back to a dismissable static frame'
assert 'if (!m_motion_enabled) {' in splash_source.split('void ZHSplashScreen::PumpAnimation',1)[1].split('void ZHSplashScreen::BeginOutro',1)[0], 'Reduced-motion startup must not sleep through animation pumps'
assert 'ScheduleOutroAfterPaint(wxWindow* target, int delay_ms)' in splash_header and 'wxWeakRef<ZHSplashScreen> weak_splash(this);' in splash_source and 'painted->exchange(true)' in splash_source, 'Splash handover must wait for a completed target paint without retaining a raw pointer'
assert 'm_handover_deadline' in splash_header and 'std::max(delay_ms, 0) + 1000' in splash_source and 'now >= m_handover_deadline' in splash_source, 'A target that never paints must not strand the splash'
startup_active=re.sub(r'/\*.*?\*/','',startup_source,flags=re.S)
startup_active=re.sub(r'//[^\n]*','',startup_active)
assert re.findall(r'^\s*scrn->PumpAnimation\((\d+)\);',startup_active,flags=re.M)==['520','80'], 'Unexpected blocking splash pump added'
handover_pos=startup_source.index('scrn->ScheduleOutroAfterPaint(mainframe, 120);')
assert startup_source.rfind('if (scrn) {',0,handover_pos) >= 0 and handover_pos < startup_source.index('mainframe->Show(true);'), 'Splash handover must be armed before the first MainFrame paint'
assert 'CallAfter([weak_splash = scrn]' not in startup_source, 'Splash handover must not race a generic queued callback against the first frame paint'
splash_setup=startup_source.split('scrn = new ZHSplashScreen(splashscreen_pos);',1)[1].split('scrn->PumpAnimation(520);',1)[0]
assert 'wxYield();\n        if (scrn)' in splash_setup and 'if (scrn) {\n            scrn->Raise();' in splash_setup and 'if (scrn)\n            scrn->PumpAnimation(520);' in startup_source, 'Re-entrant startup yield must recheck the splash before every later dereference'
browser_helper=(ROOT/'src/slic3r/GUI/ZHBrowser.hpp').read_text(encoding='utf-8')
media_source=(ROOT/'src/slic3r/GUI/wxMediaCtrl2.cpp').read_text(encoding='utf-8')
assert 'if (NetworkPolicy::is_offline())\n        return false;' in browser_helper and '::wxLaunchDefaultBrowser(url, flags)' in browser_helper, 'External browser helper must fail closed offline and avoid recursion'
for path in (ROOT/'src').rglob('*.cpp'):
    source=path.read_text(encoding='utf-8')
    assert 'wxLaunchDefaultBrowser(' not in source, f'{path}: browser launch bypasses offline policy'
assert 'return Slic3r::GUI::zh_open_external_browser(url, flags);' in startup_source, 'Browser warning flow must use the offline-aware helper'
assert 'NetworkPolicy::is_offline()' in frame_source and '[Offline]' in frame_source, 'Offline mode must be visible in native window titles'
media_load=media_source.split('void wxMediaCtrl2::Load(wxURI url)',1)[1].split('bool wxMediaCtrl2::Load',1)[0]
assert media_load.index('NetworkPolicy::is_offline()') < media_load.index('wxMediaCtrl::Load(url)'), 'Offline media must be rejected before the backend starts loading'
assert 'm_callback_alive->store(false, std::memory_order_release);' in guide_source, 'Guide destruction must invalidate queued callbacks before joining the loader'
assert 'std::shared_ptr<std::atomic_bool> m_callback_alive' in guide_header, 'Guide callbacks need a thread-safe lifetime token'
assert guide_source.count('alive->load(std::memory_order_acquire)') == 6, 'Every GuideFrame callback that uses this must check its lifetime token'
assert 'CallAfter([this, strJS]' not in guide_source and 'CallAfter([this,strJS]' not in guide_source and 'CallAfter([this]' not in guide_source, 'GuideFrame must not queue unguarded raw-this callbacks'
assert 'm_animation_enabled = ZHMotion::enabled();' in notebook_source, 'Motion preference must be cached per animation'
assert 'tab_text(false, dark)' in notebook_source, 'Tab text must use the cached theme mode'
assert 'RefreshRect(marker_rect(old_marker_x, old_marker_width).Union(marker_rect(m_marker_x, m_marker_width)), false)' in notebook_source, 'Notebook marker repaint must stay localized'
insert_page = notebook_source.split('bool ButtonsListCtrl::InsertPage', 1)[1].split('bool ButtonsListCtrl::RemovePage', 1)[0]
set_page_text = notebook_source.split('void ButtonsListCtrl::SetPageText', 1)[1].split('void ButtonsListCtrl::SetPageToolTip', 1)[0]
assert 'm_sizer->Layout();\n    m_marker_valid = false;' in insert_page, 'Inserted tabs must invalidate marker geometry'
assert 'm_sizer->Layout();\n    m_marker_valid = false;' in set_page_text, 'Renamed tabs must invalidate marker geometry'
assert 'int page_idx = m_tabpanel->FindPage(panel);\n    if (page_idx == wxNOT_FOUND)\n        return;' in frame_source, 'Missing pages must not select the last tab'
assert 'Bind(wxEVT_TIMER, &StaticBox::on_motion_timer' in staticbox_source and 'm_motion_timer.SetOwner(this);' in staticbox_source, 'StaticBox must own one reusable frame timer'
assert 'ZHMotion::blend(m_motion_from_bg, m_motion_to_bg, eased)' in staticbox_source and 'ZHMotion::ease_out(progress)' in staticbox_source, 'StaticBox state colours must be eased instead of snapping'
assert 'if (ZHMotion::enabled()) {\n            begin_motion(' in staticbox_source, 'StaticBox animation must respect reduced-motion settings'
assert 'if (!m_motion_timer.Start(kMotionFrameMs)) {' in staticbox_source and 'm_motion_animating = false;' in staticbox_source, 'Timer failure must fall back to the final colour instead of leaving an intermediate frame'
assert 'wxTimer      m_motion_timer;' in staticbox_header and 'bool         m_motion_animating' in staticbox_header, 'StaticBox animation state must be per-widget'
assert all(target in staticbox_source for target in ('m_motion_to_bg != target_bg', 'm_motion_to_bg2 != target_bg2', 'm_motion_to_border != target_border')), 'Colour changes must invalidate animation targets even when hover/focus state is unchanged'
assert 'double ProgressBar::animated_step()' in progress_source and 'ZHMotion::ease_out(' in progress_source, 'ProgressBar must ease its filled width'
assert 'm_step = std::min(step, m_max);\n    animate_to(m_step);' in progress_source, 'Animated progress must not grow beyond the configured maximum'
assert 'static_cast<double>(m_step) : animated_step()' in progress_source, 'Disabled progress bars must snap to the reported value'
assert 'wxString::Format("%d", m_step)' in progress_source and 'text + wxString("%")' in progress_source, 'ProgressBar text must keep showing the exact target value'
assert 'wxTimer   m_anim_timer;' in progress_header and 'double    m_display_step' in progress_header, 'ProgressBar animation state must be per-widget'
assert 'if (NOT DEFINED ZH_BUILD_TIME)' in libslic3r_cmake and 'CACHE STRING "Stable build timestamp" FORCE' in libslic3r_cmake, 'Build time must be cached so the precompiled header is not invalidated on every configure'
assert 'string(TIMESTAMP COMPILE_TIME' not in libslic3r_cmake, 'A per-configure timestamp would force a full rebuild every time'
print('Native command palette, splash and tab animation contracts checked')

def plist_strings(path):
    values = {}
    entries = ET.parse(path).getroot().find('dict')
    key = None
    for entry in entries:
        if entry.tag == 'key':
            key = entry.text
        elif entry.tag == 'string' and key is not None:
            values[key] = entry.text or ''
            key = None
    return values

bundle_template = plist_strings(ROOT/'cmake/modules/MacOSXBundleInfo.plist.in')
custom_template = plist_strings(ROOT/'src/platform/osx/Info.plist.in')
assert bundle_template['CFBundleIdentifier'] == '${MACOSX_BUNDLE_GUI_IDENTIFIER}', 'Bundle identifier must come from the target'
assert custom_template['CFBundleIdentifier'] == 'com.zhstudio.app', 'Custom bundle identifier regressed'
assert custom_template['CFBundleShortVersionString'] == '@ZH_MACOS_BUNDLE_SHORT_VERSION@', 'Bundle version must use the normalized public version'
assert custom_template['CFBundleVersion'] == '@ZH_MACOS_BUNDLE_VERSION@', 'Bundle build version must use a valid three-part version'
mac_cmake = (ROOT/'src/CMakeLists.txt').read_text(encoding='utf-8')
version_source = (ROOT/'version.inc').read_text(encoding='utf-8')
version_match = re.search(r'set\(SLIC3R_VERSION "([0-9.]+)"\)', version_source)
assert version_match, 'Project version must be numeric'
normalized_version = '.'.join(str(int(part)) for part in version_match.group(1).split('.')[:3])
assert len(normalized_version.split('.')) == 3, 'macOS public version must have three components'
assert 'list(SUBLIST _zh_macos_normalized_parts 0 3 _zh_macos_public_version)' in mac_cmake
assert 'MACOSX_BUNDLE_SHORT_VERSION_STRING "${ZH_MACOS_BUNDLE_SHORT_VERSION}"' in mac_cmake
assert 'MACOSX_BUNDLE_BUNDLE_VERSION "${ZH_MACOS_BUNDLE_VERSION}"' in mac_cmake
root_cmake = (ROOT/'CMakeLists.txt').read_text(encoding='utf-8')
mac_script = (ROOT/'BuildMac.sh').read_text(encoding='utf-8')
assert 'OUTPUT_NAME "${SLIC3R_APP_KEY}"' in mac_cmake
assert 'MACOSX_BUNDLE_GUI_IDENTIFIER "com.zhstudio.app"' in mac_cmake
assert '${SLIC3R_APP_KEY}.app/Contents/Resources' in root_cmake
assert 'BambuStudio.app' not in mac_cmake + root_cmake + mac_script, 'Old macOS bundle path remains'
assert 'DWARF/ZH-Studio' in mac_script, 'Universal dSYM path regressed'
assert 'Contents/MacOS/ZH-Studio' in mac_script, 'macOS universal binary path regressed'
bundle_source = (ROOT/'cmake/modules/MacOSXBundleInfo.plist.in').read_text(encoding='utf-8')
obj_block = bundle_source.split('<string>obj</string>', 1)[1].split('</dict>', 1)[0]
assert obj_block.count('<string>OBJ</string>') == 2, 'OBJ document type must not be labelled as STL'
assert 'ZH-Studio_dep_mac_' in mac_script, 'macOS dependency package name regressed'
custom_source = (ROOT/'src/platform/osx/Info.plist.in').read_text(encoding='utf-8')
assert '<string>bambustudioopen</string>' in custom_source, 'Compatibility URL scheme must remain registered'
cmake_lines = mac_cmake.splitlines()
version_start = cmake_lines.index('# Apple bundle versions accept at most three numeric components.') + 1
version_end = next(i for i in range(version_start, len(cmake_lines)) if cmake_lines[i].startswith('configure_file('))
version_script = '\n'.join(cmake_lines[version_start:version_end])
with tempfile.TemporaryDirectory(prefix='zh-plist-test-') as temporary:
    temporary = Path(temporary)
    check = temporary/'check.cmake'
    check.write_text('set(SLIC3R_VERSION "02.09.01.70")\nset(SLIC3R_APP_KEY "ZH-Studio")\n' +
                     version_script + '\nconfigure_file("${ROOT}/src/platform/osx/Info.plist.in" "${OUT}/Info.plist" @ONLY)\n',
                     encoding='utf-8')
    subprocess.run([str(ROOT/'_toolchain/cmake/bin/cmake.exe'), f'-DROOT={ROOT.as_posix()}',
                    f'-DOUT={temporary.as_posix()}', '-P', str(check)], check=True)
    configured = plistlib.loads((temporary/'Info.plist').read_bytes())
    assert configured['CFBundleShortVersionString'] == normalized_version
    assert configured['CFBundleVersion'] == normalized_version
    assert configured['CFBundleExecutable'] == 'ZH-Studio'
print('macOS bundle identity and compatibility contracts checked')
print('ZH asset validation passed')
