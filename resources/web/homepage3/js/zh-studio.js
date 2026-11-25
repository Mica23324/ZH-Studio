'use strict';
// ZH-Studio workspace behaviour: theme (light/dark + palette), language (DE/EN),
// scroll-reveal, ripple and toast animations. Upstream IDs and bridge stay intact.
(function () {
    var query = new URLSearchParams(window.location.search);
    var palettes = {
        cyan: {accent:'#0ea5e9', hover:'#38bdf8', softDark:'#152e43', softLight:'#dff0fc',
               onAccent:'#04212f', linkDark:'#38bdf8', linkLight:'#0369a1', logoRotate:'0deg'},
        ocean: {accent:'#2563eb', hover:'#3b82f6', softDark:'#172b4d', softLight:'#e2eafc',
                onAccent:'#ffffff', linkDark:'#7dabfb', linkLight:'#1d4ed8', logoRotate:'22deg'},
        violet: {accent:'#7c3aed', hover:'#8b5cf6', softDark:'#2d2148', softLight:'#ece7fd',
                 onAccent:'#ffffff', linkDark:'#b69bfa', linkLight:'#6d28d9', logoRotate:'63deg'},
        amber: {accent:'#d97706', hover:'#f59e0b', softDark:'#3b2c16', softLight:'#fbeed2',
                onAccent:'#2a1a03', linkDark:'#fbbf24', linkLight:'#a15c07', logoRotate:'193deg'}
    };
    var queryPalette = query.get('zh_theme');
    var paletteId = Object.prototype.hasOwnProperty.call(palettes, queryPalette) ? queryPalette : 'cyan';
    var palette = palettes[paletteId];

    function currentMode() {
        var forced = query.get('zh_mode');
        if (forced === 'light' || forced === 'dark') return forced;
        var agent = navigator.userAgent || '';
        if (agent.indexOf('dark') >= 0) return 'dark';
        if (agent.indexOf('light') >= 0) return 'light';
        return 'dark';
    }
    var appliedMode = null;
    function applyTheme() {
        var mode = currentMode();
        if (mode === appliedMode) return;
        appliedMode = mode;
        var light = mode === 'light';
        var root = document.documentElement;
        root.setAttribute('data-zh-palette', paletteId);
        root.setAttribute('data-zh-mode', mode);
        root.style.setProperty('--zh-accent', palette.accent);
        root.style.setProperty('--zh-hover', palette.hover);
        root.style.setProperty('--zh-soft', light ? palette.softLight : palette.softDark);
        root.style.setProperty('--zh-on-accent', palette.onAccent);
        root.style.setProperty('--zh-on-accent-icon', palette.onAccent === '#ffffff' ? 'none' : 'brightness(0)');
        root.style.setProperty('--zh-link', light ? palette.linkLight : palette.linkDark);
        root.style.setProperty('--zh-logo-rotate', palette.logoRotate);
    }
    applyTheme();
    if (typeof window.setInterval === 'function') window.setInterval(applyTheme, 1000);

    // ---- Language (DE / EN) ----
    function storedLang() {
        try { return window.localStorage.getItem('zh_lang'); } catch (e) { return null; }
    }
    function storedAppLang() {
        try { return window.localStorage.getItem('zh_lang_app'); } catch (e) { return null; }
    }
    function storedLangFor() {
        try { return window.localStorage.getItem('zh_lang_for'); } catch (e) { return null; }
    }
    function langFromCode(code) {
        return String(code || '').toLowerCase().indexOf('de') === 0 ? 'de' : 'en';
    }
    // The language the native app is running in. It travels with the user agent
    // (BBL-Language/<code>) and with the ?lang= parameter of the page.
    function appLang() {
        var ua = navigator.userAgent || '';
        var match = /BBL-Language\/([A-Za-z_-]+)/.exec(ua);
        if (match) return langFromCode(match[1]);
        var param = query.get('lang');
        if (param) return langFromCode(param);
        return null;
    }
    function currentLang() {
        var app = appLang();
        // An explicit choice by the user wins - but only as long as it belongs to the
        // language the app currently runs in. A choice from an older session must not
        // override the app, otherwise the web UI and the native UI drift apart.
        var chosen = storedLang();
        if (chosen === 'de' || chosen === 'en') {
            var chosenFor = storedLangFor();
            if (app) {
                if (chosenFor === app) return chosen;
            } else if (!chosenFor) {
                return chosen;
            }
        }
        if (app) {
            try { window.localStorage.setItem('zh_lang_app', app); } catch (e) {}
            return app;
        }
        // The native app passes its UI language along; remember it for pages
        // that are opened without a language parameter (the setup wizard steps).
        var param = query.get('lang');
        if (param) {
            var resolved = langFromCode(param);
            try { window.localStorage.setItem('zh_lang_app', resolved); } catch (e) {}
            return resolved;
        }
        var stored = storedAppLang();
        if (stored === 'de' || stored === 'en') return stored;
        return langFromCode(navigator.language || 'en');
    }
    var lang = currentLang();
    var refreshRecent = null;
    function applyLang() {
        var nodes = document.querySelectorAll('[data-zh-de]');
        for (var i = 0; i < nodes.length; i++) {
            nodes[i].textContent = nodes[i].getAttribute(lang === 'de' ? 'data-zh-de' : 'data-zh-en');
        }
        document.documentElement.setAttribute('lang', lang);
        var filterField = document.getElementById('zh-recent-filter-input');
        if (filterField) {
            filterField.placeholder = lang === 'de' ? 'Projekte filtern' : 'Filter projects';
            filterField.setAttribute('aria-label', filterField.placeholder);
        }
        var toggles = document.querySelectorAll('[data-zh-langtoggle]');
        for (var t = 0; t < toggles.length; t++) toggles[t].textContent = lang === 'de' ? 'EN' : 'DE';
        var files = document.getElementById('MiniFileList');
        if (files) files.setAttribute('data-empty', lang === 'de'
            ? 'Dein Workspace ist bereit. Öffne ein Projekt oder starte etwas Neues.'
            : 'Your workspace is ready. Open a project or create something new.');
        if (refreshRecent) refreshRecent();
    }
    window.ZHSetLanguage = function (next) {
        if (next !== 'de' && next !== 'en') return;
        lang = next;
        try {
            window.localStorage.setItem('zh_lang', next);
            // Remember the app language this choice was made for.
            var app = appLang();
            window.localStorage.setItem('zh_lang_for', app || next);
        } catch (e) {}
        applyLang();
        // The setup wizard renders most labels from the shared translation table, so it
        // has to be re-run as well - otherwise only the ZH texts would switch.
        if (typeof TranslatePage === 'function') {
            try { TranslatePage(next === 'de' ? 'de_DE' : 'en_US'); } catch (e) {}
        }
        if (window.wx && typeof window.wx.postMessage === 'function' && typeof SendWXMessage === 'function') {
            SendWXMessage(JSON.stringify({ sequence_id: Date.now(), command: 'zh_set_language', lang: next === 'de' ? 'de_DE' : 'en_US' }));
        }
    };
    window.ZHToggleLang = function () { window.ZHSetLanguage(lang === 'de' ? 'en' : 'de'); };
    // Active language, used by the native side and by the tests.
    window.ZHLang = function () { return lang; };

    // ---- Toast ----
    window.ZHToast = function (message) {
        var el = document.getElementById('zh-toast');
        if (!el) {
            el = document.createElement('div');
            el.id = 'zh-toast';
            el.setAttribute('role', 'status');
            document.body.appendChild(el);
        }
        el.textContent = message;
        el.classList.add('zh-show');
        if (window.setTimeout) window.setTimeout(function () { el.classList.remove('zh-show'); }, 2600);
    };

    document.addEventListener('DOMContentLoaded', function () {
        applyLang();
        var commandShortcut = document.getElementById('zh-command-shortcut');
        if (commandShortcut && /Mac|iPhone|iPad/i.test(navigator.platform || navigator.userAgent || '')) {
            commandShortcut.textContent = '⌘ K';
        }
        // The setup wizard links its steps with plain hrefs; carry the language over
        // so every step stays in the language the app is running in.
        var langParam = query.get('lang');
        if (langParam) {
            var links = document.querySelectorAll('a[href]');
            for (var l = 0; l < links.length; l++) {
                var href = links[l].getAttribute('href') || '';
                if (!href || href.charAt(0) === '#' || /^[a-z][a-z0-9+.-]*:/i.test(href)) continue;
                if (href.indexOf('lang=') >= 0) continue;
                links[l].setAttribute('href', href + (href.indexOf('?') >= 0 ? '&' : '?') + 'lang=' + encodeURIComponent(langParam));
            }
        }
        var toggles = document.querySelectorAll('[data-zh-langtoggle]');
        for (var t = 0; t < toggles.length; t++) {
            toggles[t].addEventListener('click', function () { window.ZHToggleLang(); });
        }
        // Every setup step gets the same switch, so the language can be changed at any
        // point of the wizard and not only on the welcome page.
        if (!toggles.length && /\/guide\//i.test(window.location.pathname)) {
            var injected = document.createElement('button');
            injected.type = 'button';
            injected.className = 'zh-langtoggle';
            injected.setAttribute('data-zh-langtoggle', '');
            injected.setAttribute('aria-label', 'Switch language / Sprache wechseln');
            injected.textContent = lang === 'de' ? 'EN' : 'DE';
            injected.addEventListener('click', function () { window.ZHToggleLang(); });
            (document.body || document.documentElement).appendChild(injected);
        }
        var files = document.getElementById('MiniFileList');
        if (files) {
            var countEl = document.getElementById('zh-recent-count');
            var updateEmpty = function () {
                var items = files.querySelectorAll('.FileItem');
                files.classList.toggle('zh-empty', !items.length);
                if (countEl) countEl.textContent = String(items.length);
            };
            updateEmpty();
            new MutationObserver(function () { updateEmpty(); applyRecent(); }).observe(files, {childList:true});
        }
        // The palette chip mirrors the active accent so the deck always shows its colour.
        var paletteEl = document.getElementById('zh-palette-name');
        if (paletteEl) paletteEl.textContent = paletteId.charAt(0).toUpperCase() + paletteId.slice(1);
        // Scroll-reveal for sections.
        if (typeof IntersectionObserver === 'function') {
            var io = new IntersectionObserver(function (entries) {
                entries.forEach(function (en) {
                    if (en.isIntersecting) { en.target.classList.add('zh-in'); io.unobserve(en.target); }
                });
            }, {threshold: 0.12});
            document.querySelectorAll('.zh-reveal').forEach(function (n) { io.observe(n); });
        } else {
            document.querySelectorAll('.zh-reveal').forEach(function (n) { n.classList.add('zh-in'); });
        }
        // Ripple feedback on primary tiles / buttons.
        document.addEventListener('click', function (ev) {
            var target = ev.target.closest ? ev.target.closest('.zh-tool, .Menu_Btn') : null;
            if (!target) return;
            var rect = target.getBoundingClientRect();
            var r = document.createElement('span');
            r.className = 'zh-ripple';
            var size = Math.max(rect.width, rect.height);
            r.style.width = r.style.height = size + 'px';
            r.style.left = (ev.clientX - rect.left - size / 2) + 'px';
            r.style.top = (ev.clientY - rect.top - size / 2) + 'px';
            target.appendChild(r);
            if (window.setTimeout) window.setTimeout(function () { r.remove(); }, 650);
        });
        // ---- Recent projects: filter and sort ----
        var filterField = document.getElementById('zh-recent-filter-input');
        var sortOldestFirst = false;
        var sortBtn = document.getElementById('zh-recent-sort');
        var applyRecent = function () {
            if (!files) return;
            var needle = (filterField && filterField.value ? filterField.value : '').trim().toLowerCase();
            var items = files.querySelectorAll('.FileItem');
            var shown = 0;
            for (var i = 0; i < items.length; i++) {
                var nameEl = items[i].querySelector('.FileName');
                var name = nameEl ? (nameEl.textContent || '') : '';
                var match = !needle || name.toLowerCase().indexOf(needle) >= 0;
                items[i].classList.toggle('zh-filtered-out', !match);
                // CSS order leaves the host's canonical DOM order and observer untouched.
                items[i].style.order = String(sortOldestFirst ? items.length - i : i);
                if (match) shown++;
            }
            files.classList.toggle('zh-empty', !shown);
            files.setAttribute('data-empty', needle
                ? (lang === 'de' ? 'Keine passenden Projekte. Versuche einen anderen Suchbegriff.' : 'No matching projects. Try another search.')
                : (lang === 'de' ? 'Dein Workspace ist bereit. Öffne ein Projekt oder starte etwas Neues.' : 'Your workspace is ready. Open a project or create something new.'));
            var hintEl = document.getElementById('zh-recent-hint');
            if (hintEl) hintEl.textContent = needle
                ? (lang === 'de' ? shown + ' von ' + items.length + ' Projekten' : shown + ' of ' + items.length + ' projects')
                : '';
            if (sortBtn) {
                var label = sortBtn.querySelector('span');
                if (label) label.textContent = sortOldestFirst
                    ? (lang === 'de' ? 'Älteste zuerst' : 'Oldest first')
                    : (lang === 'de' ? 'Neueste zuerst' : 'Newest first');
            }
        };
        refreshRecent = applyRecent;
        applyRecent();
        if (filterField) {
            filterField.addEventListener('input', applyRecent);
            filterField.addEventListener('keydown', function (ev) { if (ev.key === 'Enter') ev.preventDefault(); });
        }
        if (sortBtn) {
            sortBtn.addEventListener('click', function () {
                sortOldestFirst = !sortOldestFirst;
                applyRecent();
            });
        }

        // ---- Keyboard shortcuts for the workspace ----
        document.addEventListener('keydown', function (ev) {
            var target = ev.target || {};
            var tag = (target.tagName || '').toLowerCase();
            var typing = tag === 'input' || tag === 'textarea' || tag === 'select' || target.isContentEditable;
            if (ev.key === 'Escape') {
                if (filterField && filterField.value) { filterField.value = ''; applyRecent(); ev.preventDefault(); }
                return;
            }
            var commandModifier = ev.ctrlKey || ev.metaKey;
            // Keep the command palette available while a workspace field is focused.
            // Plain typing and Alt-modified shortcuts must retain their normal behavior.
            if (!ev.altKey && commandModifier && (ev.key === 'k' || ev.key === 'K')) {
                ZHCommand('zh_commands');
                ev.preventDefault();
                return;
            }
            if (typing || ev.altKey) return;
            if (ev.key === '/') {
                var search = document.getElementById('HotModel_Search_Input');
                if (search) { search.focus(); ev.preventDefault(); }
                return;
            }
            if (commandModifier && (ev.key === '1' || ev.key === '2' || ev.key === '3')) {
                var tiles = document.querySelectorAll('.zh-tool');
                var tile = tiles[parseInt(ev.key, 10) - 1];
                if (tile) { tile.click(); ev.preventDefault(); }
            }
        });

        // Park the ambient loops while the workspace is not the visible tab.
        var root = document.documentElement;
        var setParked = function (parked) {
            if (root.classList) root.classList.toggle('zh-paused', parked);
        };
        document.addEventListener('visibilitychange', function () { setParked(!!document.hidden); });
        if (document.hidden) setParked(true);
        // Off-screen hero: the atmosphere can stop while the user works further down.
        var heroBox = document.getElementById('zh-hero');
        if (heroBox && typeof IntersectionObserver === 'function') {
            var heroObserver = new IntersectionObserver(function (entries) {
                entries.forEach(function (en) {
                    if (heroBox.classList) heroBox.classList.toggle('zh-idle', !en.isIntersecting);
                });
            }, {threshold: 0});
            heroObserver.observe(heroBox);
        }

        // Subtle pointer parallax on the hero emblem — depth without distraction.
        var hero = document.getElementById('zh-hero');
        var emblem = document.getElementById('zh-emblem');
        var reduce = window.matchMedia && window.matchMedia('(prefers-reduced-motion: reduce)').matches;
        if (hero && emblem && !reduce && window.requestAnimationFrame) {
            var px = 0, py = 0, tx = 0, ty = 0, raf = 0;
            var tick = function () {
                raf = 0;
                px += (tx - px) * 0.08; py += (ty - py) * 0.08;
                emblem.style.transform = 'translate3d(' + px.toFixed(2) + 'px,' + py.toFixed(2) + 'px,0)';
                if (Math.abs(tx - px) > 0.1 || Math.abs(ty - py) > 0.1) raf = window.requestAnimationFrame(tick);
            };
            hero.addEventListener('pointermove', function (ev) {
                var rect = hero.getBoundingClientRect();
                tx = ((ev.clientX - rect.left) / rect.width - 0.5) * 18;
                ty = ((ev.clientY - rect.top) / rect.height - 0.5) * 14;
                if (!raf) raf = window.requestAnimationFrame(tick);
            });
            hero.addEventListener('pointerleave', function () {
                tx = 0; ty = 0; if (!raf) raf = window.requestAnimationFrame(tick);
            });
        }
    });

    // Commands remain inside the local-homepage bridge. Remote MakerWorld/MakerLab
    // documents are NOT added to the native allowlist for these commands.
    window.ZHCommand = function (command) {
        if (command !== 'zh_print_costs' && command !== 'zh_workspace' && command !== 'zh_commands') return;
        if (!window.wx || typeof window.wx.postMessage !== 'function') {
            var note = document.getElementById('zh-native-note');
            if (note) note.textContent = lang === 'de' ? 'Diese Funktion öffnet sich in der ZH-Studio Desktop-App.' : 'This tool opens inside the ZH-Studio desktop app.';
            return;
        }
        SendWXMessage(JSON.stringify({ sequence_id: Date.now(), command: command }));
    };
}());