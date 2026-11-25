'use strict';
const assert = require('node:assert/strict');
const vm = require('node:vm');
const fs = require('node:fs');
const path = require('node:path');
const root = path.resolve(__dirname, '..');
const brand = fs.readFileSync(path.join(root,'resources/web/homepage3/js/zh-studio.js'),'utf8');
let checks = 0;
function environment(palette, native, opts={}) {
    const styles={}, attributes={}, sent=[], docListeners={}; let writes=0, intervalFn=null;
    const store={};
    if(opts.store) Object.assign(store,opts.store);
    const note={textContent:'',setAttribute:()=>{},classList:{toggle:()=>{}}};
    const search=(opts.lang===null?'':'?lang='+(opts.lang||'de_DE'))+'&zh_theme='+encodeURIComponent(palette)+(opts.query||'');
    const context={URLSearchParams, navigator:{language:'de-DE', userAgent:opts.ua||''},
        window:{location:{search:search},
            setInterval:fn=>{intervalFn=fn;return 1;},
            localStorage:{getItem:k=>store[k]||null,setItem:(k,v)=>{store[k]=v;}}},
        document:{documentElement:{style:{setProperty:(k,v)=>{writes++;styles[k]=v;}},setAttribute:(k,v)=>attributes[k]=v},
            addEventListener:(name,fn)=>{docListeners[name]=fn;},getElementById:()=>note,querySelectorAll:()=>[]},
        SendWXMessage:value=>sent.push(JSON.parse(value))};
    if(native)context.window.wx={postMessage:()=>{}};
    vm.createContext(context); vm.runInContext(brand,context);
    return {context,styles,attributes,sent,note,store,docListeners,
        runInterval:()=>{if(intervalFn)intervalFn();}, writes:()=>writes,
        setUA:ua=>{context.navigator.userAgent=ua;}};
}
for(const [id,expected] of [['cyan','#0ea5e9'],['ocean','#2563eb'],['violet','#7c3aed'],['amber','#d97706'],['constructor','#0ea5e9'],['__proto__','#0ea5e9'],['unknown','#0ea5e9']]){
    assert.equal(environment(id,false).styles['--zh-accent'],expected);checks++;
}
const darkEnv=environment('cyan',false,{ua:'BBL-Slicer/v2.0 (dark) BBL-Language/de'});
assert.equal(darkEnv.attributes['data-zh-mode'],'dark');checks++;
assert.equal(darkEnv.styles['--zh-soft'],'#152e43');checks++;
assert.equal(darkEnv.styles['--zh-link'],'#38bdf8');checks++;
const lightEnv=environment('cyan',false,{ua:'BBL-Slicer/v2.0 (light) BBL-Language/de'});
assert.equal(lightEnv.attributes['data-zh-mode'],'light');checks++;
assert.equal(lightEnv.styles['--zh-soft'],'#dff0fc');checks++;
assert.equal(lightEnv.styles['--zh-link'],'#0369a1');checks++;
assert.equal(environment('cyan',false,{ua:'(dark)',query:'&zh_mode=light'}).attributes['data-zh-mode'],'light');checks++;
const oceanLight=environment('ocean',false,{ua:'(light)'});
assert.equal(oceanLight.styles['--zh-on-accent'],'#ffffff');checks++;
assert.equal(oceanLight.styles['--zh-on-accent-icon'],'none');checks++;
assert.equal(environment('cyan',false,{ua:'(light)'}).styles['--zh-on-accent-icon'],'brightness(0)');checks++;
const live=environment('violet',false,{ua:'(dark)'});
const writesBefore=live.writes();
live.runInterval();assert.equal(live.writes(),writesBefore);checks++;
live.setUA('(light)');live.runInterval();
assert.equal(live.attributes['data-zh-mode'],'light');checks++;
assert.equal(live.styles['--zh-soft'],'#ece7fd');checks++;
live.runInterval();assert.equal(live.attributes['data-zh-mode'],'light');checks++;
const native=environment('cyan',true);
native.context.window.ZHCommand('zh_print_costs');native.context.window.ZHCommand('zh_workspace');native.context.window.ZHCommand('zh_commands');
assert.equal(native.sent.length,3);checks++;
assert.equal(native.sent[2].command,'zh_commands');checks++;
native.context.window.ZHCommand('homepage_logout');assert.equal(native.sent.length,3);checks++;
const standalone=environment('cyan',false);standalone.context.window.ZHCommand('zh_print_costs');
assert.match(standalone.note.textContent,/Desktop-App/);checks++;
const langEnv=environment('cyan',true);
langEnv.context.window.ZHSetLanguage('en');
assert.equal(langEnv.store.zh_lang,'en');checks++;
assert.equal(langEnv.attributes.lang,'en');checks++;
assert.equal(langEnv.sent[langEnv.sent.length-1].command,'zh_set_language');checks++;
assert.equal(langEnv.sent[langEnv.sent.length-1].lang,'en_US');checks++;
langEnv.context.window.ZHToggleLang();
assert.equal(langEnv.store.zh_lang,'de');checks++;
langEnv.context.window.ZHSetLanguage('xx');assert.equal(langEnv.store.zh_lang,'de');checks++;
// The app language is remembered for pages that open without a language parameter.
assert.equal(langEnv.store.zh_lang_app,'de');checks++;
assert.equal(langEnv.context.window.ZHLang(),'de');checks++;
const innerPage=environment('cyan',false,{lang:null,store:{zh_lang_app:'de'}});
assert.equal(innerPage.context.window.ZHLang(),'de');checks++;
const manualChoice=environment('cyan',false,{lang:null,store:{zh_lang:'en',zh_lang_app:'de'}});
assert.equal(manualChoice.context.window.ZHLang(),'en');checks++;
const englishApp=environment('cyan',false,{lang:'en_US'});
assert.equal(englishApp.context.window.ZHLang(),'en');checks++;
assert.equal(englishApp.store.zh_lang_app,'en');checks++;
// The web language follows the language the app runs in: a choice from an older
// session must not keep the web UI in a different language than the native UI.
const staleChoice=environment('cyan',false,{ua:'BBL-Slicer/v2.0 (dark) BBL-Language/en',store:{zh_lang:'de',zh_lang_for:'de'}});
assert.equal(staleChoice.context.window.ZHLang(),'en');checks++;
const sameAppChoice=environment('cyan',false,{ua:'BBL-Slicer/v2.0 (dark) BBL-Language/en',store:{zh_lang:'de',zh_lang_for:'en'}});
assert.equal(sameAppChoice.context.window.ZHLang(),'de');checks++;
const uaWins=environment('cyan',false,{ua:'BBL-Slicer/v2.0 (dark) BBL-Language/en',store:{zh_lang_app:'de'}});
assert.equal(uaWins.context.window.ZHLang(),'en');checks++;
const toggleFor=environment('cyan',true,{ua:'BBL-Slicer/v2.0 (dark) BBL-Language/en'});
toggleFor.context.window.ZHSetLanguage('de');
assert.equal(toggleFor.store.zh_lang_for,'en');checks++;

const home=fs.readFileSync(path.join(root,'resources/web/homepage3/js/home.js'),'utf8');
const input={};let searches=0,prevented=0;
const context={document:{getElementById:()=>input},window:{},$:()=>({prop:()=>{}}),
    TranslatePage:()=>{},GetCurrentTextByKey:()=>'',console};
vm.createContext(context);vm.runInContext(home,context);
for(const name of ['SendMsg_GetRecentFile','SendMsg_GetStaffPick','SendMsg_GetMakerlabList','SendMsg_GetPrintHistory'])context[name]=()=>{};
context.OnSearchOnline=()=>searches++;
context.OnHomeInit();
input.onkeydown({key:'a',preventDefault:()=>prevented++});assert.equal(searches,0);assert.equal(prevented,0);checks+=2;
input.onkeydown({key:'Enter',isComposing:true,preventDefault:()=>prevented++});assert.equal(searches,0);checks++;
input.onkeydown({key:'Enter',isComposing:false,preventDefault:()=>prevented++});assert.equal(searches,1);assert.equal(prevented,1);checks+=2;
const handlers=new Map(); let hidden=0;
context.$=selector=>({
    off(name){handlers.delete(String(selector)+':'+name);return this;},
    on(name,handler){const key=String(selector)+':'+name;assert(!handlers.has(key),'handler duplicated');handlers.set(key,handler);return this;},
    hide(){hidden++;},width(){return 100;},height(){return 100;},attr(){return 'model.3mf';}
});
context.Set_RecentFile_MouseRightBtn_Event();context.Set_RecentFile_MouseRightBtn_Event();
assert.equal(handlers.size,4);checks++;
const dismiss=handlers.get(String(context.document)+':click.zhRecentMenu');
dismiss({target:{id:'outside',parentNode:null}});assert.equal(hidden,1);checks++;
dismiss({target:{id:'recnet_context_menu',parentNode:null}});assert.equal(hidden,1);checks++;
function recentEnvironment() {
    function element(text='') {
        const classes=new Set(), attrs={}, listeners={};
        return {textContent:text,value:'',style:{},attrs,listeners,
            classList:{contains:k=>classes.has(k),add:k=>classes.add(k),
                toggle(k,on){if(on)classes.add(k);else classes.delete(k);}},
            setAttribute:(k,v)=>{attrs[k]=v;},getAttribute:k=>attrs[k],
            addEventListener:(k,fn)=>{listeners[k]=fn;}};
    }
    const makeItem=name=>{const e=element();e.name=name;e.querySelector=()=>({textContent:name});return e;};
    const items=Array.from({length:8},(_,i)=>makeItem('Project '+i));
    const list=element(), field=element(), sort=element(), sortLabel=element(), hint=element(), count=element(), commandShortcut=element('Ctrl K');
    list.items=items;list.querySelectorAll=()=>list.items;
    sort.querySelector=()=>sortLabel;
    const docListeners={}, observers=[], sent=[];
    const nodes={MiniFileList:list,'zh-recent-filter-input':field,'zh-recent-sort':sort,
        'zh-recent-hint':hint,'zh-recent-count':count,'zh-command-shortcut':commandShortcut};
    const ctx={URLSearchParams,navigator:{language:'en',userAgent:'BBL-Language/en',platform:'MacIntel'},
        MutationObserver:class {constructor(fn){observers.push(fn);}observe(){}},
        window:{location:{search:'?lang=en_US',pathname:'/web/homepage3/home.html'},
            localStorage:{getItem:()=>null,setItem:()=>{}},setInterval:()=>{},wx:{postMessage:()=>{}}},
        SendWXMessage:value=>sent.push(JSON.parse(value)),
        document:{documentElement:{style:{setProperty:()=>{}},setAttribute:()=>{}},
            getElementById:id=>nodes[id]||null,querySelectorAll:()=>[],
            addEventListener:(name,fn)=>{docListeners[name]=fn;}}};
    vm.createContext(ctx);vm.runInContext(brand,ctx);docListeners.DOMContentLoaded();
    return {ctx,list,field,sort,sortLabel,hint,count,commandShortcut,docListeners,sent,observers,makeItem};
}
const recent=recentEnvironment();
assert.equal(recent.commandShortcut.textContent,'⌘ K');checks++;
let shortcutPrevented=0;
recent.ctx.ZHCommand=recent.ctx.window.ZHCommand;
recent.docListeners.keydown({key:'k',metaKey:true,ctrlKey:false,altKey:false,target:{},preventDefault:()=>shortcutPrevented++});
assert.equal(recent.sent.length,1);checks++;
assert.equal(recent.sent[0].command,'zh_commands');checks++;
assert.equal(shortcutPrevented,1);checks++;
let focusedShortcutPrevented=0;
recent.docListeners.keydown({key:'K',metaKey:false,ctrlKey:true,altKey:false,target:{tagName:'INPUT'},preventDefault:()=>focusedShortcutPrevented++});
assert.equal(recent.sent.length,2);checks++;
assert.equal(recent.sent[1].command,'zh_commands');checks++;
assert.equal(focusedShortcutPrevented,1);checks++;
recent.docListeners.keydown({key:'k',metaKey:false,ctrlKey:false,altKey:false,target:{tagName:'INPUT'},preventDefault:()=>{throw new Error('plain typing consumed');}});
assert.equal(recent.sent.length,2);checks++;
let altShortcutPrevented=0;
recent.docListeners.keydown({key:'k',metaKey:true,ctrlKey:false,altKey:true,target:{},preventDefault:()=>altShortcutPrevented++});
assert.equal(recent.sent.length,2);checks++;
assert.equal(altShortcutPrevented,0);checks++;
const visualOrder=()=>recent.list.items.filter(e=>!e.classList.contains('zh-filtered-out'))
    .slice().sort((a,b)=>Number(a.style.order)-Number(b.style.order)).map(e=>e.name);
assert.equal(recent.count.textContent,'8');checks++;
recent.field.value='Project 7';recent.field.listeners.input();
assert.deepEqual(visualOrder(),['Project 7']);checks++;
recent.field.value='';recent.field.listeners.input();recent.sort.listeners.click();
assert.deepEqual(visualOrder(),Array.from({length:8},(_,i)=>'Project '+(7-i)));checks++;
recent.field.value='Project';recent.field.listeners.input();
assert.equal(visualOrder()[0],'Project 7');checks++;
recent.sort.listeners.click();assert.equal(visualOrder()[0],'Project 0');checks++;
recent.sort.listeners.click();recent.ctx.window.ZHSetLanguage('de');
assert.equal(recent.sortLabel.textContent,'Älteste zuerst');checks++;
recent.ctx.window.ZHSetLanguage('en');assert.equal(recent.sortLabel.textContent,'Oldest first');checks++;
recent.field.value='No match';recent.field.listeners.input();
assert.equal(visualOrder().length,0);checks++;
assert(recent.list.classList.contains('zh-empty'));checks++;
assert.match(recent.list.attrs['data-empty'],/No matching projects/);checks++;
recent.list.items=[recent.makeItem('New project'),recent.makeItem('No match result')];
recent.observers[0]();assert.deepEqual(visualOrder(),['No match result']);checks++;
assert.equal(recent.count.textContent,'2');checks++;
assert.deepEqual(recent.list.items.map(e=>e.name),['New project','No match result']);checks++;
const deckCss=fs.readFileSync(path.join(root,'resources/web/homepage3/css/zh-studio.css'),'utf8');
assert.match(deckCss,/\.FileItem\.zh-filtered-out\{display:none!important\}/);checks++;
assert(!deckCss.includes('var(--border)'));checks++;
assert(!deckCss.includes('mix-blend-mode'));checks++;
assert(!deckCss.includes('backdrop-filter'));checks++;
assert.match(deckCss,/\.zh-emblem-float\{[^}]*position:relative[^}]*z-index:2[^}]*animation:zh-float/);checks++;
const catalogSource=fs.readFileSync(path.join(root,'resources/web/data/text.js'),'utf8');
const catalogContext={};vm.createContext(catalogContext);vm.runInContext(catalogSource,catalogContext);
const brandedKeys=['t1','t2','t83','t85','wk2','wk4','wk6','wk8','wk10','wk12','wk16'];
for(const entries of Object.values(catalogContext.LangText)){
    for(const key of brandedKeys){
        if(typeof entries[key]==='string')assert(!entries[key].includes('Bambu Studio'),`${key} still uses the old app name`);checks++;
    }
}
const homepageSource=fs.readFileSync(path.join(root,'resources/web/homepage3/home.html'),'utf8');
const helpBoard=homepageSource.slice(homepageSource.indexOf('id="WikiGuideBoard"'),homepageSource.indexOf('id="zh-footer"'));
assert(!helpBoard.includes('Bambu Studio'));checks++;
assert.match(homepageSource,/Independent community fork of Bambu Studio/);checks++;
console.log(`ZH web regression tests: ${checks} checks passed (isolated DOM/bridge test doubles).`);
