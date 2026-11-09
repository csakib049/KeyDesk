#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <richedit.h>
#include <windowsx.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream> 
#include <ctime>
#include <filesystem>
#include <algorithm>
#include <cwctype>
#include <cstdlib>
#include <cwchar>
#include <cstring>

namespace fs = std::filesystem;
static HINSTANCE g_instance;
static HMODULE g_richEditModule=nullptr;
static HWND g_main=nullptr, g_overlay=nullptr, g_noteTabs=nullptr, g_noteEdit=nullptr;
static HWND g_todoList=nullptr, g_todoInput=nullptr, g_clock=nullptr, g_watch=nullptr, g_timerDisplay=nullptr, g_timerStartButton=nullptr;
static HWND g_alarmInput=nullptr, g_alarmPeriod=nullptr, g_alarmMessageInput=nullptr, g_timerInput=nullptr;
static HWND g_alarmNotification=nullptr;
static HWND g_palette=nullptr,g_paletteSearch=nullptr,g_paletteList=nullptr;
static HWND g_paletteTitle=nullptr;
static bool g_paletteShortcutsOnly=false;
static HHOOK g_keyboardHook=nullptr;
static HFONT g_uiFont=nullptr, g_overlayFont=nullptr, g_overlayClockFont=nullptr, g_bigFont=nullptr, g_timerFont=nullptr, g_headingFont=nullptr;
static bool g_dark=false, g_keysVisible=true, g_watchRunning=false, g_alarmEnabled=false, g_timerRunning=false;
static bool g_overlayDragging=false;
static int g_alarmHour=8, g_alarmMinute=0, g_currentNote=0, g_overlayX=60, g_overlayY=60, g_currentPage=0, g_themeIndex=8;
static std::wstring g_alarmMessage;
static ULONGLONG g_watchStart=0, g_watchElapsed=0, g_timerEnd=0, g_timerPausedMs=0;
static POINT g_dragMouse{}, g_dragOrigin{};
static std::wstring g_keyText=L"Ready", g_recentText, g_overlayTime=L"--:-- --";
static std::vector<std::wstring> g_noteNames, g_notePaths;
static std::vector<HWND> g_pageControls[3];
static std::vector<std::vector<HWND>> g_overviewFocusGrid;
static std::vector<std::pair<HWND,RECT>> g_designRects;
static int g_clientW=1020,g_clientH=780;

enum : int {
    ID_PAGE_CLOCK=100, ID_PAGE_ALARM, ID_PAGE_TIMER, ID_TIMER_START, ID_TIMER_RESET,
    ID_WATCH_TEXT, ID_WATCH_START, ID_WATCH_RESET, ID_NOTE_NEW, ID_NOTE_OPEN,
    ID_NOTE_DELETE, ID_TODO_ADD, ID_TODO_EDIT, ID_TODO_DELETE, ID_KEY_TOGGLE,
    ID_THEME_TOGGLE, ID_ALARM_SET, ID_EXIT, ID_NAV_OVERVIEW, ID_NAV_NOTES, ID_NAV_TODO,
    ID_THEME_PICK, ID_TIMER_SET, ID_PAGE_TIMER_DISPLAY, ID_PAGE_CLOCK_LABEL, ID_PAGE_TIMER_LABEL,
    ID_WATCH_LABEL, ID_TODO_LIST=210, ID_TODO_INPUT, ID_ALARM_INPUT, ID_TIMER_INPUT, ID_NOTE_TABS, ID_NOTE_EDIT, ID_ALARM_PERIOD=216, ID_ALARM_MESSAGE,
    ID_CLOCK_TICK=1, ID_NOTE_AUTOSAVE=2,
    ID_SHORTCUTS=400, ID_NAV_PREVIOUS, ID_NAV_NEXT, ID_TAB_PREVIOUS, ID_TAB_NEXT,
    ID_PALETTE_OPEN, ID_PALETTE_SEARCH, ID_PALETTE_RESULTS, ID_NOTE_BOLD, ID_NOTE_ITALIC
};

struct ShortcutEntry {const wchar_t* category;const wchar_t* keys;WORD key;BYTE modifiers;int command;};
static const ShortcutEntry g_shortcuts[]={
    {L"Navigation",L"Ctrl + Up",VK_UP,FVIRTKEY|FCONTROL,ID_NAV_PREVIOUS},
    {L"Navigation",L"Ctrl + Down",VK_DOWN,FVIRTKEY|FCONTROL,ID_NAV_NEXT},
    {L"Navigation",L"Ctrl + Left",VK_LEFT,FVIRTKEY|FCONTROL,ID_TAB_PREVIOUS},
    {L"Navigation",L"Ctrl + Right",VK_RIGHT,FVIRTKEY|FCONTROL,ID_TAB_NEXT},
    {L"Commands",L"Ctrl + P",'P',FVIRTKEY|FCONTROL,ID_PALETTE_OPEN},
    {L"Notes",L"Ctrl + B",'B',FVIRTKEY|FCONTROL,ID_NOTE_BOLD},
    {L"Notes",L"Ctrl + I",'I',FVIRTKEY|FCONTROL,ID_NOTE_ITALIC},
    {L"Commands",L"Esc",VK_ESCAPE,0,0}
};
struct PaletteAction {const wchar_t* category;const wchar_t* title;const wchar_t* shortcut;int command;};
static const PaletteAction g_paletteActions[]={
    {L"Navigation",L"Go to Overview",L"",ID_NAV_OVERVIEW},
    {L"Navigation",L"Go to Notes",L"",ID_NAV_NOTES},
    {L"Navigation",L"Go to To-do",L"",ID_NAV_TODO},
    {L"Commands",L"Show Keyboard Shortcuts",L"",ID_SHORTCUTS},
    {L"Notes",L"Create a new note",L"",ID_NOTE_NEW},
    {L"Notes",L"Open a text note",L"",ID_NOTE_OPEN},
    {L"Notes",L"Delete current note",L"",ID_NOTE_DELETE},
    {L"Notes",L"Bold selected note text",L"Ctrl + B",ID_NOTE_BOLD},
    {L"Notes",L"Italicize selected note text",L"Ctrl + I",ID_NOTE_ITALIC},
    {L"Tasks",L"Add task",L"",ID_TODO_ADD},
    {L"Tasks",L"Update selected task",L"",ID_TODO_EDIT},
    {L"Tasks",L"Delete selected task",L"",ID_TODO_DELETE},
    {L"Timer",L"Start or pause countdown",L"",ID_TIMER_START},
    {L"Timer",L"Reset countdown",L"",ID_TIMER_RESET},
    {L"Timer",L"Start or pause stopwatch",L"",ID_WATCH_START},
    {L"Appearance",L"Toggle light/dark mode",L"",ID_THEME_TOGGLE},
    {L"Appearance",L"Toggle keystroke display",L"",ID_KEY_TOGGLE}
};
struct PaletteRow {int command;std::wstring text,search;};
static std::vector<PaletteRow> g_paletteRows;

struct Theme {const wchar_t* name;COLORREF accent;};
static const Theme g_themes[]={
    {L"Berry  #660033",RGB(102,0,51)},{L"Aqua  #00BFC1",RGB(0,191,193)},
    {L"Dark  #151515",RGB(21,21,21)},{L"Wine  #93032E",RGB(147,3,46)},
    {L"Rose  #A4133C",RGB(164,19,60)},{L"Black  #0B0B0B",RGB(11,11,11)},
    {L"Graphite  #161616",RGB(22,22,22)},{L"Orange  #FB5607",RGB(251,86,7)},
    {L"Violet  #8338EC",RGB(131,56,236)}
};
static COLORREF accentColor(){return g_themes[g_themeIndex].accent;}
static COLORREF blend(COLORREF a,COLORREF b,int amount){return RGB((GetRValue(a)*amount+GetRValue(b)*(100-amount))/100,(GetGValue(a)*amount+GetGValue(b)*(100-amount))/100,(GetBValue(a)*amount+GetBValue(b)*(100-amount))/100);}
static COLORREF backgroundColor(){return g_dark?RGB(22,23,27):blend(accentColor(),RGB(255,255,255),4);}
static COLORREF textColor(){return g_dark?RGB(242,242,246):RGB(28,28,42);}
static COLORREF surfaceColor(){return g_dark?RGB(38,39,45):RGB(255,255,255);}
static COLORREF cardColor(){return g_dark?RGB(38,39,45):blend(accentColor(),RGB(255,255,255),8);}
static std::wstring appDir(){wchar_t p[MAX_PATH]{};GetModuleFileNameW(nullptr,p,MAX_PATH);std::wstring s=p;auto i=s.find_last_of(L"\\/");return i==s.npos?L".":s.substr(0,i);}
static std::wstring dataDir(){return appDir()+L"\\KeyDeskData";}
static std::wstring notesDir(){return dataDir()+L"\\Notes";}
static std::wstring todosDir(){return dataDir()+L"\\ToDo";}
static std::wstring todoFile(){return todosDir()+L"\\todos.txt";}
static std::wstring readUtf8(const std::wstring& path);
static bool writeUtf8(const std::wstring& path,const std::wstring& value);
static std::wstring settingsFile(){
    wchar_t roaming[MAX_PATH]{};DWORD n=GetEnvironmentVariableW(L"APPDATA",roaming,MAX_PATH);
    if(n>0&&n<MAX_PATH)return std::wstring(roaming)+L"\\KeyDesk\\settings.ini";
    return dataDir()+L"\\settings.ini";
}
static bool saveSettings(){
    const std::wstring file=settingsFile();std::error_code ec;fs::create_directories(fs::path(file).parent_path(),ec);
    std::wstring data=L"[Appearance]\r\n";
    data+=L"dark="+(g_dark?std::wstring(L"1"):std::wstring(L"0"))+L"\r\n";
    data+=L"theme="+std::to_wstring(g_themeIndex)+L"\r\n";
    data+=L"keystrokes="+(g_keysVisible?std::wstring(L"1"):std::wstring(L"0"))+L"\r\n";
    return writeUtf8(file,data);
}
static void loadSettings(){
    std::wistringstream input(readUtf8(settingsFile()));std::wstring line,section;
    while(std::getline(input,line)){
        if(line.empty())continue;
        if(line.front()==L'['&&line.back()==L']'){section=line.substr(1,line.size()-2);continue;}
        auto eq=line.find(L'=');if(eq==std::wstring::npos||section!=L"Appearance")continue;
        std::wstring key=line.substr(0,eq),value=line.substr(eq+1);
        if(key==L"dark")g_dark=(value==L"1");
        else if(key==L"theme"){int n=_wtoi(value.c_str());if(n>=0&&n<(int)(sizeof(g_themes)/sizeof(g_themes[0])))g_themeIndex=n;}
        else if(key==L"keystrokes")g_keysVisible=(value==L"1");
    }
}

static std::wstring readUtf8(const std::wstring& path){
    std::ifstream f{fs::path(path),std::ios::binary}; if(!f)return L"";
    std::string bytes((std::istreambuf_iterator<char>(f)),{});
    if(bytes.size()>=3&&(unsigned char)bytes[0]==0xEF&&(unsigned char)bytes[1]==0xBB&&(unsigned char)bytes[2]==0xBF)bytes.erase(0,3);
    if(bytes.empty())return L"";
    int n=MultiByteToWideChar(CP_UTF8,0,bytes.data(),(int)bytes.size(),nullptr,0);
    UINT cp=CP_UTF8; if(!n){cp=CP_ACP;n=MultiByteToWideChar(cp,0,bytes.data(),(int)bytes.size(),nullptr,0);}
    if(!n)return L"";std::wstring out(n,L'\0');MultiByteToWideChar(cp,0,bytes.data(),(int)bytes.size(),out.data(),n);return out;
}
static bool writeUtf8(const std::wstring& path,const std::wstring& value){
    std::ofstream f{fs::path(path),std::ios::binary|std::ios::trunc};if(!f)return false;
    int n=value.empty()?0:WideCharToMultiByte(CP_UTF8,0,value.data(),(int)value.size(),nullptr,0,nullptr,nullptr);
    if(n){std::string bytes(n,'\0');WideCharToMultiByte(CP_UTF8,0,value.data(),(int)value.size(),bytes.data(),n,nullptr,nullptr);static const unsigned char bom[]={0xEF,0xBB,0xBF};f.write((const char*)bom,3);f.write(bytes.data(),n);}
    return (bool)f;
}
static HFONT uiFont(){if(!g_uiFont)g_uiFont=CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");return g_uiFont;}
static HFONT bigFont(){if(!g_bigFont)g_bigFont=CreateFontW(-46,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");return g_bigFont;}
static HFONT headingFont(){if(!g_headingFont)g_headingFont=CreateFontW(-28,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");return g_headingFont;}
static HWND control(HWND parent,const wchar_t* cls,const wchar_t* title,DWORD style,int x,int y,int w,int h,int id,int page=-1){
    HWND c=CreateWindowExW(0,cls,title,WS_CHILD|WS_VISIBLE|style,x,y,w,h,parent,(HMENU)(INT_PTR)id,g_instance,nullptr);
    if(c){SendMessageW(c,WM_SETFONT,(WPARAM)uiFont(),TRUE);g_designRects.push_back({c,RECT{x,y,x+w,y+h}});if(page>=0)g_pageControls[page].push_back(c);}return c;
}
static void layoutDashboard(HWND h){RECT c{};GetClientRect(h,&c);g_clientW=std::max(1,(int)(c.right-c.left));g_clientH=std::max(1,(int)(c.bottom-c.top));double sx=g_clientW/1020.0,sy=g_clientH/780.0;for(const auto& entry:g_designRects){const RECT&r=entry.second;SetWindowPos(entry.first,nullptr,(int)(r.left*sx),(int)(r.top*sy),std::max(1,(int)((r.right-r.left)*sx)),std::max(1,(int)((r.bottom-r.top)*sy)),SWP_NOZORDER|SWP_NOACTIVATE);}auto fit=[&](HWND control,HFONT& font,int base){if(!control)return;RECT r{};GetClientRect(control,&r);int size=std::max(12,(int)(base*std::min(sx,sy)));HDC dc=GetDC(control);std::wstring value=control==g_clock?L"12:59:59 PM":L"999:59:59.9";HFONT chosen=nullptr;while(size>=12){HFONT candidate=CreateFontW(-size,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");HGDIOBJ old=SelectObject(dc,candidate);SIZE ext{};GetTextExtentPoint32W(dc,value.c_str(),(int)value.size(),&ext);SelectObject(dc,old);if(ext.cx<=r.right-r.left-12){chosen=candidate;break;}DeleteObject(candidate);--size;}ReleaseDC(control,dc);if(chosen){HFONT previous=font;font=chosen;SendMessageW(control,WM_SETFONT,(WPARAM)font,TRUE);if(previous)DeleteObject(previous);}};fit(g_clock,g_bigFont,46);fit(g_timerDisplay,g_timerFont,46);}
static void ensureFolders(){std::error_code ec;fs::create_directories(fs::path(notesDir()),ec);fs::create_directories(fs::path(todosDir()),ec);}

static std::wstring noteText(){int n=GetWindowTextLengthW(g_noteEdit);std::wstring s(n+1,L'\0');GetWindowTextW(g_noteEdit,s.data(),n+1);s.resize(n);return s;}
static std::wstring noteFormatPath(const std::wstring& path){
    std::error_code ec;std::wstring folder=dataDir()+L"\\NoteFormatting";fs::create_directories(fs::path(folder),ec);SetFileAttributesW(folder.c_str(),FILE_ATTRIBUTE_HIDDEN);
    unsigned long long hash=1469598103934665603ull;for(wchar_t c:path){hash^=(unsigned long long)towlower(c);hash*=1099511628211ull;}
    wchar_t name[32]{};swprintf_s(name,L"%016llX.rtf",hash);return folder+L"\\"+name;
}
struct RichOut {std::string data;};
struct RichIn {const std::string* data;size_t offset;};
static DWORD CALLBACK richOutCallback(DWORD_PTR cookie,LPBYTE buffer,LONG count,LONG* written){auto* out=(RichOut*)cookie;out->data.append((const char*)buffer,(size_t)count);*written=count;return 0;}
static DWORD CALLBACK richInCallback(DWORD_PTR cookie,LPBYTE buffer,LONG count,LONG* read){auto* in=(RichIn*)cookie;size_t remain=in->data->size()-in->offset;size_t n=std::min((size_t)count,remain);if(n)memcpy(buffer,in->data->data()+in->offset,n);in->offset+=n;*read=(LONG)n;return 0;}
static std::string streamNoteRtf(){RichOut out;EDITSTREAM stream{};stream.dwCookie=(DWORD_PTR)&out;stream.pfnCallback=richOutCallback;SendMessageW(g_noteEdit,EM_STREAMOUT,SF_RTF,(LPARAM)&stream);return out.data;}
static void saveCurrentNote(){
    if(g_currentNote<0||g_currentNote>=(int)g_notePaths.size()||!g_noteEdit)return;
    writeUtf8(g_notePaths[g_currentNote],noteText());std::string rtf=streamNoteRtf();std::wstring sidecar=noteFormatPath(g_notePaths[g_currentNote]);
    std::ofstream f{fs::path(sidecar),std::ios::binary|std::ios::trunc};if(f&&!rtf.empty())f.write(rtf.data(),(std::streamsize)rtf.size());
}
static void loadNote(int index){
    if(index<0||index>=(int)g_notePaths.size())return;g_currentNote=index;std::wstring plain=readUtf8(g_notePaths[index]);std::wstring sidecar=noteFormatPath(g_notePaths[index]);std::ifstream f{fs::path(sidecar),std::ios::binary};
    if(f){std::string rtf((std::istreambuf_iterator<char>(f)),{});RichIn input{&rtf,0};EDITSTREAM stream{};stream.dwCookie=(DWORD_PTR)&input;stream.pfnCallback=richInCallback;SendMessageW(g_noteEdit,EM_STREAMIN,SF_RTF,(LPARAM)&stream);if(noteText()!=plain){SetWindowTextW(g_noteEdit,plain.c_str());std::error_code ec;fs::remove(fs::path(sidecar),ec);}}
    else SetWindowTextW(g_noteEdit,plain.c_str());
}
static void toggleNoteStyle(DWORD mask,DWORD effect){
    if(GetFocus()!=g_noteEdit){SetFocus(g_noteEdit);}
    LONG start=0,end=0;SendMessageW(g_noteEdit,EM_GETSEL,(WPARAM)&start,(LPARAM)&end);if(start==end)return;
    CHARFORMAT2W format{};format.cbSize=sizeof(format);SendMessageW(g_noteEdit,EM_GETCHARFORMAT,SCF_SELECTION,(LPARAM)&format);format.dwMask=mask;format.dwEffects=(format.dwEffects&effect)?0:effect;SendMessageW(g_noteEdit,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&format);saveCurrentNote();
}
static void addNoteTab(int index){TCITEMW t{};t.mask=TCIF_TEXT;t.pszText=(LPWSTR)g_noteNames[index].c_str();TabCtrl_InsertItem(g_noteTabs,index,&t);}
static void refreshNoteName(int index){TCITEMW t{};t.mask=TCIF_TEXT;t.pszText=(LPWSTR)g_noteNames[index].c_str();TabCtrl_SetItem(g_noteTabs,index,&t);}
static void loadNotes(){
    g_noteNames.clear();g_notePaths.clear();std::error_code ec;
    for(const auto& e:fs::directory_iterator(fs::path(notesDir()),ec)){
        if(ec)break;if(!e.is_regular_file())continue;std::wstring ext=e.path().extension().wstring();
        if(_wcsicmp(ext.c_str(),L".txt")==0){g_notePaths.push_back(e.path().wstring());g_noteNames.push_back(e.path().filename().wstring());}
    }
    std::vector<size_t> order(g_noteNames.size());for(size_t i=0;i<order.size();++i)order[i]=i;
    std::sort(order.begin(),order.end(),[](size_t a,size_t b){return _wcsicmp(g_noteNames[a].c_str(),g_noteNames[b].c_str())<0;});
    std::vector<std::wstring> names,paths;for(size_t i:order){names.push_back(g_noteNames[i]);paths.push_back(g_notePaths[i]);}g_noteNames.swap(names);g_notePaths.swap(paths);
    if(g_noteNames.empty()){std::wstring p=notesDir()+L"\\Untitled.txt";writeUtf8(p,L"");g_noteNames.push_back(L"Untitled.txt");g_notePaths.push_back(p);}
    for(int i=0;i<(int)g_noteNames.size();++i)addNoteTab(i);loadNote(0);
}
static int findNotePath(const std::wstring& path){for(int i=0;i<(int)g_notePaths.size();++i)if(CompareStringOrdinal(g_notePaths[i].c_str(),-1,path.c_str(),-1,TRUE)==CSTR_EQUAL)return i;return -1;}
static void newNote(){
    saveCurrentNote();
    wchar_t path[MAX_PATH]=L"New note.txt";OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=g_main;
    std::wstring initial=notesDir();o.lpstrFilter=L"Text files (*.txt)\0*.txt\0\0";o.lpstrFile=path;o.nMaxFile=MAX_PATH;o.lpstrInitialDir=initial.c_str();o.lpstrDefExt=L"txt";o.lpstrTitle=L"Create a note";o.Flags=OFN_PATHMUSTEXIST;
    if(!GetSaveFileNameW(&o))return;std::wstring full=fs::path(path).wstring();if(fs::path(full).extension().empty())full+=L".txt";
    std::wstring parent=fs::path(full).parent_path().wstring();if(CompareStringOrdinal(parent.c_str(),-1,initial.c_str(),-1,TRUE)!=CSTR_EQUAL){MessageBoxW(g_main,L"Please save new notes inside KeyDeskData\\Notes.",L"KeyDesk",MB_OK|MB_ICONINFORMATION);return;}
    int found=findNotePath(full);if(found>=0){TabCtrl_SetCurSel(g_noteTabs,found);loadNote(found);return;}
    if(fs::exists(fs::path(full))){MessageBoxW(g_main,L"A file with that name already exists. Use Open note to edit it.",L"KeyDesk",MB_OK|MB_ICONINFORMATION);return;}
    writeUtf8(full,L"");int i=(int)g_noteNames.size();g_noteNames.push_back(fs::path(full).filename().wstring());g_notePaths.push_back(full);addNoteTab(i);TabCtrl_SetCurSel(g_noteTabs,i);loadNote(i);
}
static void openNote(){
    saveCurrentNote();wchar_t path[MAX_PATH]=L"";OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=g_main;
    std::wstring initial=notesDir();o.lpstrFilter=L"Text files (*.txt)\0*.txt\0All files (*.*)\0*.*\0\0";o.lpstrFile=path;o.nMaxFile=MAX_PATH;o.lpstrInitialDir=initial.c_str();o.lpstrTitle=L"Open a text note";o.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
    if(!GetOpenFileNameW(&o))return;std::wstring source=fs::path(path).wstring(),dest=notesDir()+L"\\"+fs::path(source).filename().wstring();
    std::error_code ec;fs::path srcPath(source),destPath(dest);
    if(CompareStringOrdinal(srcPath.parent_path().wstring().c_str(),-1,fs::path(notesDir()).wstring().c_str(),-1,TRUE)!=CSTR_EQUAL){
        if(fs::exists(destPath,ec)){int n=1;std::wstring stem=srcPath.stem().wstring(),ext=srcPath.extension().wstring();do{dest=notesDir()+L"\\"+stem+L" ("+std::to_wstring(n++)+L")"+ext;destPath=fs::path(dest);}while(fs::exists(destPath,ec));}
        fs::copy_file(srcPath,destPath,fs::copy_options::none,ec);if(ec){MessageBoxW(g_main,L"Could not copy that file into KeyDeskData\\Notes.",L"KeyDesk",MB_OK|MB_ICONERROR);return;}
    }
    int found=findNotePath(dest);if(found>=0){TabCtrl_SetCurSel(g_noteTabs,found);loadNote(found);return;}
    int i=(int)g_noteNames.size();g_notePaths.push_back(dest);g_noteNames.push_back(destPath.filename().wstring());addNoteTab(i);TabCtrl_SetCurSel(g_noteTabs,i);loadNote(i);
}
static void deleteNote(){
    if(g_currentNote<0||g_currentNote>=(int)g_notePaths.size())return;
    if(MessageBoxW(g_main,(L"Delete '"+g_noteNames[g_currentNote]+L"'? This cannot be undone.").c_str(),L"Delete note",MB_YESNO|MB_ICONWARNING)!=IDYES)return;
    std::error_code ec;fs::remove(fs::path(noteFormatPath(g_notePaths[g_currentNote])),ec);ec.clear();fs::remove(fs::path(g_notePaths[g_currentNote]),ec);int old=g_currentNote;TabCtrl_DeleteItem(g_noteTabs,old);g_noteNames.erase(g_noteNames.begin()+old);g_notePaths.erase(g_notePaths.begin()+old);
    if(g_noteNames.empty()){std::wstring p=notesDir()+L"\\Untitled.txt";writeUtf8(p,L"");g_noteNames.push_back(L"Untitled.txt");g_notePaths.push_back(p);addNoteTab(0);old=0;}else if(old>=(int)g_noteNames.size())old=(int)g_noteNames.size()-1;
    TabCtrl_SetCurSel(g_noteTabs,old);loadNote(old);
}

static std::wstring todoLineText(int index){wchar_t b[2048]{};SendMessageW(g_todoList,LB_GETTEXT,index,(LPARAM)b);return b;}
static bool todoDone(int index){return SendMessageW(g_todoList,LB_GETITEMDATA,index,0)!=0;}
static void saveTodos(){std::wstring data;int n=(int)SendMessageW(g_todoList,LB_GETCOUNT,0,0);for(int i=0;i<n;++i)data+=(todoDone(i)?L"1\t":L"0\t")+todoLineText(i)+L"\n";writeUtf8(todoFile(),data);}
static void addTodoItem(const std::wstring& text,bool done=false){if(text.empty())return;LRESULT i=SendMessageW(g_todoList,LB_ADDSTRING,0,(LPARAM)text.c_str());if(i!=LB_ERR&&i!=LB_ERRSPACE)SendMessageW(g_todoList,LB_SETITEMDATA,i,done?1:0);}
static void loadTodos(){std::wistringstream in(readUtf8(todoFile()));std::wstring line;while(std::getline(in,line)){if(line.size()<2||line[1]!=L'\t')continue;addTodoItem(line.substr(2),line[0]==L'1');}}
static void addTodo(){int n=GetWindowTextLengthW(g_todoInput);std::wstring text(n+1,L'\0');GetWindowTextW(g_todoInput,text.data(),n+1);text.resize(n);if(text.empty())return;addTodoItem(text);SetWindowTextW(g_todoInput,L"");saveTodos();InvalidateRect(g_todoList,nullptr,TRUE);}
static void editTodo(){int i=(int)SendMessageW(g_todoList,LB_GETCURSEL,0,0);if(i==LB_ERR)return;int n=GetWindowTextLengthW(g_todoInput);std::wstring text(n+1,L'\0');GetWindowTextW(g_todoInput,text.data(),n+1);text.resize(n);if(text.empty())return;bool done=todoDone(i);SendMessageW(g_todoList,LB_DELETESTRING,i,0);SendMessageW(g_todoList,LB_INSERTSTRING,i,(LPARAM)text.c_str());SendMessageW(g_todoList,LB_SETITEMDATA,i,done?1:0);SendMessageW(g_todoList,LB_SETCURSEL,i,0);SetWindowTextW(g_todoInput,L"");saveTodos();InvalidateRect(g_todoList,nullptr,TRUE);}
static void deleteTodo(){int i=(int)SendMessageW(g_todoList,LB_GETCURSEL,0,0);if(i==LB_ERR)return;SendMessageW(g_todoList,LB_DELETESTRING,i,0);SetWindowTextW(g_todoInput,L"");saveTodos();InvalidateRect(g_todoList,nullptr,TRUE);}

static std::wstring keyName(DWORD vk){
    switch(vk){case VK_SPACE:return L"␣";case VK_RETURN:return L"Enter";case VK_BACK:return L"⮨";case VK_TAB:return L"Tab";case VK_ESCAPE:return L"Esc";case VK_SHIFT:return L"Shift";case VK_CONTROL:return L"Ctrl";case VK_MENU:return L"Alt";case VK_LWIN:case VK_RWIN:return L"Win";case VK_CAPITAL:return L"Caps";case VK_LEFT:return L"🢠";case VK_RIGHT:return L"🢣";case VK_UP:return L"⇧";case VK_DOWN:return L"⇩";default:break;}
    if(vk>=VK_F1&&vk<=VK_F24)return L"F"+std::to_wstring(vk-VK_F1+1);wchar_t b[64]{};LONG code=(LONG)(MapVirtualKeyW(vk,MAPVK_VK_TO_VSC)<<16);if(GetKeyNameTextW(code,b,64))return b;return L"";
}
static void setKeyText(const std::wstring& text){g_keyText=text.substr(0,5);InvalidateRect(g_overlay,nullptr,TRUE);}
static void showPressedKey(DWORD vk,const KBDLLHOOKSTRUCT* key){
    if(vk==VK_SHIFT||vk==VK_CONTROL||vk==VK_MENU||vk==VK_LWIN||vk==VK_RWIN){setKeyText(keyName(vk));return;}
    if(vk==VK_SPACE){g_recentText+=keyName(vk);if(g_recentText.size()>5)g_recentText.erase(0,g_recentText.size()-5);setKeyText(g_recentText);return;}
    BYTE state[256]{};GetKeyboardState(state);
    for(int k:{VK_SHIFT,VK_LSHIFT,VK_RSHIFT,VK_CONTROL,VK_LCONTROL,VK_RCONTROL,VK_MENU,VK_LMENU,VK_RMENU})if(GetAsyncKeyState(k)&0x8000)state[k]|=0x80;
    state[VK_CAPITAL]=(BYTE)((GetKeyState(VK_CAPITAL)&1)?1:0);if(vk<256)state[vk]|=0x80;
    wchar_t chars[8]{};HKL layout=GetKeyboardLayout(GetWindowThreadProcessId(GetForegroundWindow(),nullptr));int count=ToUnicodeEx(vk,key->scanCode,state,chars,8,0,layout);
    if(count>0){std::wstring printable;for(int i=0;i<count;++i)if(iswprint(chars[i]))printable.push_back(chars[i]);if(!printable.empty()){g_recentText+=printable;if(g_recentText.size()>5)g_recentText.erase(0,g_recentText.size()-5);setKeyText(g_recentText);return;}}
    std::wstring name=keyName(vk);if(!name.empty())setKeyText(name);
}
static LRESULT CALLBACK keyboardProc(int code,WPARAM wp,LPARAM lp){if(code==HC_ACTION&&(wp==WM_KEYDOWN||wp==WM_SYSKEYDOWN))showPressedKey(((KBDLLHOOKSTRUCT*)lp)->vkCode,(KBDLLHOOKSTRUCT*)lp);return CallNextHookEx(g_keyboardHook,code,wp,lp);}

static void showDashboardAtOverlay(){
    if(!g_main||!g_overlay)return;RECT r{};GetWindowRect(g_overlay,&r);HMONITOR mon=MonitorFromPoint(POINT{r.left,r.top},MONITOR_DEFAULTTONEAREST);MONITORINFO mi{sizeof(mi)};GetMonitorInfoW(mon,&mi);
    int x=std::clamp(r.left,mi.rcWork.left,std::max(mi.rcWork.left,mi.rcWork.right-780));int y=std::clamp(r.top,mi.rcWork.top,std::max(mi.rcWork.top,mi.rcWork.bottom-570));
    SetWindowPos(g_main,HWND_TOP,x,y,780,570,SWP_SHOWWINDOW);ShowWindow(g_main,SW_RESTORE);SetForegroundWindow(g_main);ShowWindow(g_overlay,SW_HIDE);
}
static void saveOverlayPosition(){std::wofstream f{fs::path(dataDir()+L"\\overlay-position.txt")};f<<g_overlayX<<L" "<<g_overlayY;}
static void restoreOverlayPosition(){std::wifstream f{fs::path(dataDir()+L"\\overlay-position.txt")};if(f)f>>g_overlayX>>g_overlayY;}
static LRESULT CALLBACK overlayProc(HWND h,UINT m,WPARAM w,LPARAM l){
    switch(m){
    case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);RECT r{};GetClientRect(h,&r);HBRUSH brush=CreateSolidBrush(RGB(32,32,34));HPEN pen=CreatePen(PS_SOLID,1,RGB(64,64,66));HGDIOBJ ob=SelectObject(dc,brush),op=SelectObject(dc,pen);RoundRect(dc,r.left,r.top,r.right,r.bottom,38,38);SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(brush);DeleteObject(pen);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(248,248,248));if(!g_overlayFont)g_overlayFont=CreateFontW(-23,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");if(!g_overlayClockFont)g_overlayClockFont=CreateFontW(-12,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");RECT keyRect{r.left+4,r.top+3,r.right-4,r.bottom-21};HGDIOBJ old=SelectObject(dc,g_overlayFont);DrawTextW(dc,g_keyText.c_str(),-1,&keyRect,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);SelectObject(dc,g_overlayClockFont);RECT timeRect{r.left+4,r.bottom-22,r.right-4,r.bottom-2};SetTextColor(dc,RGB(205,205,210));DrawTextW(dc,g_overlayTime.c_str(),-1,&timeRect,DT_CENTER|DT_VCENTER|DT_SINGLELINE);SelectObject(dc,old);EndPaint(h,&ps);return 0;}
    case WM_LBUTTONDOWN:{SetCapture(h);g_overlayDragging=true;g_dragMouse={GET_X_LPARAM(l),GET_Y_LPARAM(l)};RECT r{};GetWindowRect(h,&r);g_dragOrigin={r.left,r.top};return 0;}
    case WM_MOUSEMOVE:if(g_overlayDragging&&(w&MK_LBUTTON)){POINT p{};GetCursorPos(&p);int dx=p.x-(g_dragOrigin.x+g_dragMouse.x),dy=p.y-(g_dragOrigin.y+g_dragMouse.y);if(abs(dx)>2||abs(dy)>2){g_overlayX=g_dragOrigin.x+dx;g_overlayY=g_dragOrigin.y+dy;SetWindowPos(h,HWND_TOPMOST,g_overlayX,g_overlayY,190,70,SWP_NOACTIVATE);}}return 0;
    case WM_LBUTTONUP:if(g_overlayDragging){POINT p{};GetCursorPos(&p);bool clicked=(abs(p.x-(g_dragOrigin.x+g_dragMouse.x))<=3&&abs(p.y-(g_dragOrigin.y+g_dragMouse.y))<=3);g_overlayDragging=false;ReleaseCapture();if(clicked)showDashboardAtOverlay();else saveOverlayPosition();}return 0;
    case WM_ERASEBKGND:return 1;
    }return DefWindowProcW(h,m,w,l);
}
static void makeOverlay(){WNDCLASSW c{};c.lpfnWndProc=overlayProc;c.hInstance=g_instance;c.lpszClassName=L"KeyDeskOverlay";c.hCursor=LoadCursor(nullptr,IDC_HAND);c.hbrBackground=nullptr;RegisterClassW(&c);restoreOverlayPosition();g_overlay=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW|WS_EX_LAYERED,c.lpszClassName,L"KeyDesk",WS_POPUP,g_overlayX,g_overlayY,190,70,nullptr,nullptr,g_instance,nullptr);if(g_overlay){HRGN pill=CreateRoundRectRgn(0,0,190,70,70,70);SetWindowRgn(g_overlay,pill,TRUE);SetLayeredWindowAttributes(g_overlay,0,232,LWA_ALPHA);if(g_keysVisible)ShowWindow(g_overlay,SW_SHOWNOACTIVATE);}}

static void setTheme(){EnumChildWindows(g_main,[](HWND c,LPARAM){InvalidateRect(c,nullptr,TRUE);return TRUE;},0);InvalidateRect(g_main,nullptr,TRUE);InvalidateRect(g_todoList,nullptr,TRUE);}
static void addPageControls(int page){for(HWND h:g_pageControls[page])ShowWindow(h,SW_SHOW);}
static void hidePageControls(int page){for(HWND h:g_pageControls[page])ShowWindow(h,SW_HIDE);}
static void selectPage(int page){if(page<0||page>2)return;g_currentPage=page;for(int i=0;i<3;++i)hidePageControls(i);addPageControls(page);InvalidateRect(g_main,nullptr,TRUE);}
static void cycleSection(int delta){int page=(g_currentPage+delta+3)%3;selectPage(page);}
static void cyclePageOrNoteTab(int delta){
    if(g_currentPage==1&&g_noteTabs){int count=TabCtrl_GetItemCount(g_noteTabs);if(count>1){int current=TabCtrl_GetCurSel(g_noteTabs);int next=(current+delta+count)%count;TabCtrl_SetCurSel(g_noteTabs,next);saveCurrentNote();loadNote(next);InvalidateRect(g_noteTabs,nullptr,TRUE);return;}}
    cycleSection(delta);
}
static void navigateOverviewGrid(int dx,int dy){
    if(g_overviewFocusGrid.empty())return;int row=-1,col=-1;
    for(int r=0;r<(int)g_overviewFocusGrid.size();++r)for(int c=0;c<(int)g_overviewFocusGrid[r].size();++c)if(GetFocus()==g_overviewFocusGrid[r][c]){row=r;col=c;}
    if(row<0){if(dy<0){row=(int)g_overviewFocusGrid.size()-1;col=0;}else{row=0;col=dx>0?(int)g_overviewFocusGrid[0].size()-1:0;}}
    else if(dx){col=std::clamp(col+dx,0,(int)g_overviewFocusGrid[row].size()-1);}
    else if(dy){row=std::clamp(row+dy,0,(int)g_overviewFocusGrid.size()-1);col=std::min(col,(int)g_overviewFocusGrid[row].size()-1);}
    HWND target=g_overviewFocusGrid[row][col];if(target&&IsWindowVisible(target))SetFocus(target);
}
static bool focusIsTextControl(){
    HWND focus=GetFocus();if(!focus)return false;wchar_t cls[64]{};GetClassNameW(focus,cls,64);
    return wcscmp(cls,L"Edit")==0||wcscmp(cls,L"ListBox")==0||wcscmp(cls,L"ComboLBox")==0||wcsncmp(cls,L"RichEdit",8)==0;
}
static const wchar_t* shortcutDescription(int command){switch(command){case ID_NAV_PREVIOUS:return L"Go to previous section or Overview row";case ID_NAV_NEXT:return L"Go to next section or Overview row";case ID_TAB_PREVIOUS:return L"Previous page, note tab, or Overview control";case ID_TAB_NEXT:return L"Next page, note tab, or Overview control";case ID_PALETTE_OPEN:return L"Open the Command Palette";case ID_NOTE_BOLD:return L"Bold selected note text";case ID_NOTE_ITALIC:return L"Italicize selected note text";default:return L"Close the palette or dismiss the active popup";}}
static std::wstring lowerText(std::wstring s){std::transform(s.begin(),s.end(),s.begin(),[](wchar_t c){return (wchar_t)towlower(c);});return s;}
static bool containsText(const std::wstring& value,const std::wstring& query){return query.empty()||lowerText(value).find(lowerText(query))!=std::wstring::npos;}
static void rebuildPalette(){
    if(!g_paletteList)return;SendMessageW(g_paletteList,LB_RESETCONTENT,0,0);g_paletteRows.clear();
    int n=GetWindowTextLengthW(g_paletteSearch);std::wstring query(n+1,L'\0');GetWindowTextW(g_paletteSearch,query.data(),n+1);query.resize(n);
    if(g_paletteShortcutsOnly){
        for(const auto& s:g_shortcuts){std::wstring label=std::wstring(s.category)+L"    "+s.keys+L"    "+shortcutDescription(s.command);if(containsText(label,query)){SendMessageW(g_paletteList,LB_ADDSTRING,0,(LPARAM)label.c_str());g_paletteRows.push_back({0,label,lowerText(label)});}}
    }else{
        for(const auto& a:g_paletteActions){std::wstring label=std::wstring(a.category)+L"    "+a.title;for(const auto& s:g_shortcuts)if(s.command==a.command){label+=L"    ";label+=s.keys;break;}if(containsText(label,query)){SendMessageW(g_paletteList,LB_ADDSTRING,0,(LPARAM)label.c_str());g_paletteRows.push_back({a.command,label,lowerText(label)});}}
        for(const auto& s:g_shortcuts){std::wstring label=std::wstring(s.category)+L"    "+s.keys+L"    "+shortcutDescription(s.command);if(s.command&&containsText(label,query)){SendMessageW(g_paletteList,LB_ADDSTRING,0,(LPARAM)label.c_str());g_paletteRows.push_back({s.command,label,lowerText(label)});}}
    }
    if(SendMessageW(g_paletteList,LB_GETCOUNT,0,0)>0)SendMessageW(g_paletteList,LB_SETCURSEL,0,0);
}
static void invokePaletteCommand(int command){
    switch(command){
        case ID_NOTE_NEW:case ID_NOTE_OPEN:case ID_NOTE_DELETE:case ID_NOTE_BOLD:case ID_NOTE_ITALIC:selectPage(1);break;
        case ID_TODO_ADD:case ID_TODO_EDIT:case ID_TODO_DELETE:selectPage(2);break;
        case ID_TIMER_START:case ID_TIMER_RESET:case ID_WATCH_START:case ID_THEME_TOGGLE:case ID_KEY_TOGGLE:selectPage(0);break;
        default:break;
    }
    if(command==ID_TODO_ADD){SetFocus(g_todoInput);return;}
    SendMessageW(g_main,WM_COMMAND,MAKEWPARAM(command,0),0);
}
static void closePalette(){if(g_palette)DestroyWindow(g_palette);g_palette=nullptr;g_paletteSearch=nullptr;g_paletteList=nullptr;g_paletteTitle=nullptr;if(g_main&&IsWindow(g_main))SetForegroundWindow(g_main);}
static LRESULT CALLBACK paletteProc(HWND h,UINT m,WPARAM w,LPARAM l){
    switch(m){
        case WM_CREATE:{g_paletteTitle=CreateWindowExW(0,L"STATIC",g_paletteShortcutsOnly?L"Keyboard Shortcuts":L"Command Palette",WS_CHILD|WS_VISIBLE,20,16,460,30,h,nullptr,g_instance,nullptr);SendMessageW(g_paletteTitle,WM_SETFONT,(WPARAM)uiFont(),TRUE);g_paletteSearch=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,20,54,460,34,h,(HMENU)(INT_PTR)ID_PALETTE_SEARCH,g_instance,nullptr);SendMessageW(g_paletteSearch,WM_SETFONT,(WPARAM)uiFont(),TRUE);g_paletteList=CreateWindowExW(WS_EX_CLIENTEDGE,L"LISTBOX",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_VSCROLL|LBS_NOTIFY|LBS_NOINTEGRALHEIGHT,20,100,460,330,h,(HMENU)(INT_PTR)ID_PALETTE_RESULTS,g_instance,nullptr);SendMessageW(g_paletteList,WM_SETFONT,(WPARAM)uiFont(),TRUE);rebuildPalette();SetFocus(g_paletteSearch);return 0;}
        case WM_COMMAND:if(LOWORD(w)==ID_PALETTE_SEARCH&&HIWORD(w)==EN_CHANGE){rebuildPalette();return 0;}if((LOWORD(w)==ID_PALETTE_RESULTS&&HIWORD(w)==LBN_DBLCLK)||LOWORD(w)==IDOK){int row=(int)SendMessageW(g_paletteList,LB_GETCURSEL,0,0);if(row!=LB_ERR&&row<(int)g_paletteRows.size()){int command=g_paletteRows[row].command;closePalette();if(command)invokePaletteCommand(command);}return 0;}if(LOWORD(w)==IDCANCEL){closePalette();return 0;}break;
        case WM_CLOSE:closePalette();return 0;
        case WM_ERASEBKGND:{RECT r{};GetClientRect(h,&r);HBRUSH b=CreateSolidBrush(backgroundColor());FillRect((HDC)w,&r,b);DeleteObject(b);return 1;}
    }return DefWindowProcW(h,m,w,l);
}
static void openPalette(bool shortcutsOnly){
    if(g_palette){g_paletteShortcutsOnly=shortcutsOnly;SetWindowTextW(g_paletteTitle,shortcutsOnly?L"Keyboard Shortcuts":L"Command Palette");SetWindowTextW(g_paletteSearch,L"");rebuildPalette();ShowWindow(g_palette,SW_SHOW);SetForegroundWindow(g_palette);SetFocus(g_paletteSearch);return;}
    g_paletteShortcutsOnly=shortcutsOnly;WNDCLASSW c{};c.lpfnWndProc=paletteProc;c.hInstance=g_instance;c.lpszClassName=L"KeyDeskCommandPalette";c.hCursor=LoadCursor(nullptr,IDC_ARROW);c.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassW(&c);
    RECT r{};GetWindowRect(g_main,&r);g_palette=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_TOPMOST,c.lpszClassName,shortcutsOnly?L"Keyboard Shortcuts":L"Command Palette",WS_POPUP|WS_CAPTION|WS_SYSMENU,r.left+(r.right-r.left-520)/2,r.top+70,520,490,g_main,nullptr,g_instance,nullptr);if(g_palette){ShowWindow(g_palette,SW_SHOW);SetForegroundWindow(g_palette);SetFocus(g_paletteSearch);}
}
static void makeControls(HWND h){
    HWND title=control(h,L"STATIC",L"KeyDesk",SS_LEFT,108,24,380,42,0);SendMessageW(title,WM_SETFONT,(WPARAM)headingFont(),TRUE);
    control(h,L"STATIC",L"Keyboard & Timer Tool",SS_LEFT,110,68,360,30,0);
    control(h,L"BUTTON",L"Exit",BS_OWNERDRAW,922,22,66,34,ID_EXIT);
    control(h,L"BUTTON",L"Keyboard shortcuts",BS_OWNERDRAW,770,22,142,34,ID_SHORTCUTS);
    control(h,L"BUTTON",L"⌂  Overview",BS_OWNERDRAW,32,116,172,46,ID_NAV_OVERVIEW);
    control(h,L"BUTTON",L"▤  Notes",BS_OWNERDRAW,208,116,156,46,ID_NAV_NOTES);
    control(h,L"BUTTON",L"☑  To-do",BS_OWNERDRAW,368,116,156,46,ID_NAV_TODO);

    g_clock=control(h,L"STATIC",L"",SS_CENTER|SS_OWNERDRAW,145,211,310,70,ID_PAGE_CLOCK,0);SendMessageW(g_clock,WM_SETFONT,(WPARAM)bigFont(),TRUE);
    control(h,L"STATIC",L"Current Time",SS_CENTER,180,283,240,28,ID_PAGE_CLOCK_LABEL,0);
    g_timerDisplay=control(h,L"STATIC",L"00:00:00.0",SS_CENTER|SS_OWNERDRAW,622,211,310,70,ID_PAGE_TIMER_DISPLAY,0);SendMessageW(g_timerDisplay,WM_SETFONT,(WPARAM)bigFont(),TRUE);
    control(h,L"STATIC",L"Countdown Timer",SS_CENTER,662,283,230,28,ID_PAGE_TIMER_LABEL,0);
    g_timerStartButton=control(h,L"BUTTON",L"Start timer",BS_OWNERDRAW,624,350,164,42,ID_TIMER_START,0);
    control(h,L"BUTTON",L"Reset",BS_OWNERDRAW,802,350,126,42,ID_TIMER_RESET,0);
    control(h,L"STATIC",L"Alarm time",SS_LEFT,64,433,142,30,ID_PAGE_ALARM,0);
    g_alarmInput=control(h,L"EDIT",L"08:00",WS_BORDER|ES_AUTOHSCROLL,220,426,120,38,ID_ALARM_INPUT,0);
    g_alarmPeriod=control(h,WC_COMBOBOXW,L"",CBS_DROPDOWNLIST|WS_VSCROLL,352,426,96,150,ID_ALARM_PERIOD,0);SendMessageW(g_alarmPeriod,CB_ADDSTRING,0,(LPARAM)L"AM");SendMessageW(g_alarmPeriod,CB_ADDSTRING,0,(LPARAM)L"PM");SendMessageW(g_alarmPeriod,CB_SETCURSEL,0,0);
    control(h,L"BUTTON",L"Set alarm",BS_OWNERDRAW,470,426,150,38,ID_ALARM_SET,0);
    control(h,L"STATIC",L"Alarm message",SS_LEFT,64,485,142,30,0,0);
    g_alarmMessageInput=control(h,L"EDIT",L"",WS_BORDER|ES_AUTOHSCROLL,220,478,400,38,ID_ALARM_MESSAGE,0);
    control(h,L"STATIC",L"Timer length (minutes)",SS_LEFT,64,540,226,30,ID_PAGE_TIMER,0);
    g_timerInput=control(h,L"EDIT",L"5",WS_BORDER|ES_AUTOHSCROLL,302,533,172,38,ID_TIMER_INPUT,0);
    control(h,L"BUTTON",L"Set countdown",BS_OWNERDRAW,510,533,150,38,ID_TIMER_SET,0);
        control(h,L"BUTTON",L"Keystrokes: On",BS_OWNERDRAW,38,610,232,46,ID_KEY_TOGGLE,0);
        control(h,L"STATIC",L"Accent theme",SS_LEFT,300,619,116,28,0,0);
        HWND themes=control(h,WC_COMBOBOXW,L"",CBS_DROPDOWNLIST|WS_VSCROLL,420,611,290,180,ID_THEME_PICK,0);
    for(const auto& t:g_themes)SendMessageW(themes,CB_ADDSTRING,0,(LPARAM)t.name);SendMessageW(themes,CB_SETCURSEL,g_themeIndex,0);
        control(h,L"BUTTON",L"Switch to dark theme",BS_OWNERDRAW,730,610,250,46,ID_THEME_TOGGLE,0);
        g_watch=control(h,L"STATIC",L"Stopwatch   00:00:00.0",SS_LEFT|SS_OWNERDRAW,42,688,360,42,ID_WATCH_TEXT,0);
        control(h,L"BUTTON",L"Start / Pause",BS_OWNERDRAW,690,685,148,40,ID_WATCH_START,0);control(h,L"BUTTON",L"Reset",BS_OWNERDRAW,854,685,124,40,ID_WATCH_RESET,0);

    g_noteTabs=control(h,WC_TABCONTROLW,L"",0,30,182,940,34,ID_NOTE_TABS,1);
    g_noteEdit=control(h,L"RICHEDIT50W",L"",WS_BORDER|ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN|WS_VSCROLL|ES_NOHIDESEL,30,228,940,392,ID_NOTE_EDIT,1);
    control(h,L"BUTTON",L"New note",BS_OWNERDRAW,30,650,140,40,ID_NOTE_NEW,1);control(h,L"BUTTON",L"Open note",BS_OWNERDRAW,184,650,140,40,ID_NOTE_OPEN,1);control(h,L"BUTTON",L"Delete note",BS_OWNERDRAW,338,650,140,40,ID_NOTE_DELETE,1);control(h,L"STATIC",L"Notes save automatically as UTF-8 .txt files",SS_LEFT,510,657,430,30,0,1);
    g_todoList=control(h,L"LISTBOX",L"",WS_BORDER|WS_VSCROLL|LBS_NOTIFY|LBS_OWNERDRAWFIXED|LBS_HASSTRINGS,30,188,940,398,ID_TODO_LIST,2);SendMessageW(g_todoList,LB_SETITEMHEIGHT,0,42);
    g_todoInput=control(h,L"EDIT",L"",WS_BORDER|ES_AUTOHSCROLL,30,612,390,38,ID_TODO_INPUT,2);control(h,L"BUTTON",L"Add",BS_OWNERDRAW,438,612,112,38,ID_TODO_ADD,2);control(h,L"BUTTON",L"Update selected",BS_OWNERDRAW,568,612,180,38,ID_TODO_EDIT,2);control(h,L"BUTTON",L"Delete selected",BS_OWNERDRAW,766,612,204,38,ID_TODO_DELETE,2);control(h,L"STATIC",L"Select a task to edit it. Double-click to check or uncheck.",SS_LEFT,30,672,600,28,0,2);
    g_overviewFocusGrid={
        {GetDlgItem(h,ID_TIMER_START),GetDlgItem(h,ID_TIMER_RESET)},
        {GetDlgItem(h,ID_ALARM_INPUT),GetDlgItem(h,ID_ALARM_PERIOD),GetDlgItem(h,ID_ALARM_MESSAGE),GetDlgItem(h,ID_ALARM_SET)},
        {GetDlgItem(h,ID_TIMER_INPUT),GetDlgItem(h,ID_TIMER_SET)},
        {GetDlgItem(h,ID_KEY_TOGGLE),GetDlgItem(h,ID_THEME_PICK),GetDlgItem(h,ID_THEME_TOGGLE)},
        {GetDlgItem(h,ID_WATCH_START),GetDlgItem(h,ID_WATCH_RESET)}
    };
}

static void showAlarmNotification();
static void updateClock(){time_t now=time(nullptr);tm t{};tm* lt=localtime(&now);if(lt)t=*lt;int hour=t.tm_hour%12;if(!hour)hour=12;wchar_t b[64]{};swprintf_s(b,L"%d:%02d:%02d %ls",hour,t.tm_min,t.tm_sec,t.tm_hour>=12?L"PM":L"AM");SetWindowTextW(g_clock,b);wchar_t overlayTime[32]{};swprintf_s(overlayTime,L"%d:%02d:%02d %ls",hour,t.tm_min,t.tm_sec,t.tm_hour>=12?L"PM":L"AM");if(g_overlayTime!=overlayTime){g_overlayTime=overlayTime;if(g_overlay)InvalidateRect(g_overlay,nullptr,TRUE);}
    ULONGLONG ms=g_watchElapsed+(g_watchRunning?GetTickCount64()-g_watchStart:0);swprintf_s(b,L"Stopwatch   %02llu:%02llu:%02llu.%d",ms/3600000,(ms/60000)%60,(ms/1000)%60,(int)((ms/100)%10));SetWindowTextW(g_watch,b);
    if(g_alarmEnabled&&t.tm_hour==g_alarmHour&&t.tm_min==g_alarmMinute){g_alarmEnabled=false;showAlarmNotification();}
    ULONGLONG nowTicks=GetTickCount64();ULONGLONG left=g_timerRunning?(g_timerEnd>nowTicks?g_timerEnd-nowTicks:0):g_timerPausedMs;
    swprintf_s(b,L"%llu:%02llu:%02llu.%d",left/3600000,(left/60000)%60,(left/1000)%60,(int)((left/100)%10));SetWindowTextW(g_timerDisplay,b);
    if(g_timerRunning&&left==0){g_timerRunning=false;g_timerPausedMs=0;SetWindowTextW(g_timerStartButton,L"Start timer");MessageBeep(MB_ICONEXCLAMATION);MessageBoxW(g_main,L"Timer finished.",L"KeyDesk",MB_OK|MB_ICONINFORMATION);}
}
static bool readTimerMinutes(ULONGLONG& ms){wchar_t s[32]{};GetWindowTextW(g_timerInput,s,32);int mins=_wtoi(s);if(mins<=0||mins>=100000)return false;ms=(ULONGLONG)mins*60000;return true;}
static LRESULT CALLBACK alarmNotificationProc(HWND h,UINT m,WPARAM w,LPARAM l){
    switch(m){
        case WM_CREATE:{std::wstring message=g_alarmMessage.empty()?L"Your KeyDesk alarm is ringing.":g_alarmMessage;CreateWindowExW(0,L"STATIC",L"Alarm time reached",WS_CHILD|WS_VISIBLE,22,16,305,30,h,nullptr,g_instance,nullptr);CreateWindowExW(0,L"STATIC",message.c_str(),WS_CHILD|WS_VISIBLE|SS_LEFT,22,48,305,55,h,nullptr,g_instance,nullptr);CreateWindowExW(0,L"BUTTON",L"Dismiss",WS_CHILD|WS_VISIBLE|WS_TABSTOP,228,112,100,32,h,(HMENU)(INT_PTR)1,g_instance,nullptr);SetTimer(h,1,1800,nullptr);MessageBeep(MB_ICONEXCLAMATION);return 0;}
        case WM_TIMER:MessageBeep(MB_ICONEXCLAMATION);return 0;
        case WM_COMMAND:if(LOWORD(w)==1){DestroyWindow(h);return 0;}break;
        case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);RECT r{};GetClientRect(h,&r);HBRUSH b=CreateSolidBrush(g_dark?RGB(38,39,45):RGB(255,255,255));FillRect(dc,&r,b);DeleteObject(b);HPEN p=CreatePen(PS_SOLID,2,accentColor());HGDIOBJ old=SelectObject(dc,p);HGDIOBJ oldBrush=SelectObject(dc,GetStockObject(NULL_BRUSH));RoundRect(dc,0,0,r.right,r.bottom,18,18);SelectObject(dc,old);SelectObject(dc,oldBrush);DeleteObject(p);EndPaint(h,&ps);return 0;}
        case WM_CTLCOLORSTATIC:{HDC dc=(HDC)w;SetTextColor(dc,g_dark?RGB(245,245,248):RGB(32,32,42));SetBkMode(dc,TRANSPARENT);return (LRESULT)GetStockObject(HOLLOW_BRUSH);}
        case WM_CLOSE:DestroyWindow(h);return 0;
        case WM_DESTROY:KillTimer(h,1);if(g_alarmNotification==h)g_alarmNotification=nullptr;return 0;
    }return DefWindowProcW(h,m,w,l);
}
static void showAlarmNotification(){
    if(g_alarmNotification){SetForegroundWindow(g_alarmNotification);return;}
    static bool registered=false;if(!registered){WNDCLASSW c{};c.lpfnWndProc=alarmNotificationProc;c.hInstance=g_instance;c.lpszClassName=L"KeyDeskAlarmNotification";c.hCursor=LoadCursor(nullptr,IDC_ARROW);c.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassW(&c);registered=true;}
    HMONITOR mon=MonitorFromWindow(g_main,MONITOR_DEFAULTTONEAREST);MONITORINFO mi{sizeof(mi)};GetMonitorInfoW(mon,&mi);int x=(int)mi.rcWork.right-370,y=(int)mi.rcWork.bottom-160;
    g_alarmNotification=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW,L"KeyDeskAlarmNotification",L"KeyDesk alarm",WS_POPUP,x,y,350,160,g_main,nullptr,g_instance,nullptr);if(g_alarmNotification){ShowWindow(g_alarmNotification,SW_SHOWNOACTIVATE);UpdateWindow(g_alarmNotification);}
}
static void drawRoundRect(HDC dc,RECT r,COLORREF fill,COLORREF edge,int radius=18){HBRUSH b=CreateSolidBrush(fill);HPEN p=CreatePen(PS_SOLID,1,edge);HGDIOBJ ob=SelectObject(dc,b),op=SelectObject(dc,p);RoundRect(dc,r.left,r.top,r.right,r.bottom,radius,radius);SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(b);DeleteObject(p);}
static bool accentNeedsDarkText(){return GetRValue(accentColor())*299+GetGValue(accentColor())*587+GetBValue(accentColor())*114>150000;}
static bool primaryButton(int id){return (id==ID_NAV_OVERVIEW&&g_currentPage==0)||(id==ID_NAV_NOTES&&g_currentPage==1)||(id==ID_NAV_TODO&&g_currentPage==2)||id==ID_TIMER_START||id==ID_NOTE_NEW||id==ID_TODO_ADD;}
static void drawButton(DRAWITEMSTRUCT* d){int id=GetDlgCtrlID(d->hwndItem);bool primary=primaryButton(id),pressed=(d->itemState&ODS_SELECTED)!=0;COLORREF fill=primary?accentColor():surfaceColor();if(pressed)fill=blend(fill,RGB(0,0,0),12);COLORREF edge=primary?accentColor():blend(accentColor(),surfaceColor(),38);drawRoundRect(d->hDC,d->rcItem,fill,edge,14);wchar_t label[128]{};GetWindowTextW(d->hwndItem,label,128);SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,primary?(accentNeedsDarkText()?RGB(24,24,30):RGB(255,255,255)):accentColor());SelectObject(d->hDC,uiFont());RECT r=d->rcItem;InflateRect(&r,-8,-2);DrawTextW(d->hDC,label,-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);if(d->itemState&ODS_FOCUS)DrawFocusRect(d->hDC,&d->rcItem);}
static void paintDashboard(HWND h){PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);int savedMap=SaveDC(dc);SetMapMode(dc,MM_ANISOTROPIC);SetWindowExtEx(dc,1020,780,nullptr);SetViewportExtEx(dc,g_clientW,g_clientH,nullptr);RECT client{0,0,1020,780};HBRUSH bg=CreateSolidBrush(backgroundColor());FillRect(dc,&client,bg);DeleteObject(bg);
    RECT banner{0,0,client.right,166};HBRUSH hb=CreateSolidBrush(g_dark?RGB(28,29,35):blend(accentColor(),RGB(255,255,255),10));FillRect(dc,&banner,hb);DeleteObject(hb);
    int saved=SaveDC(dc);IntersectClipRect(dc,0,0,client.right,166);HBRUSH wave=CreateSolidBrush(g_dark?blend(accentColor(),RGB(28,29,35),36):blend(accentColor(),RGB(255,255,255),24));HPEN none=(HPEN)GetStockObject(NULL_PEN);HGDIOBJ ob=SelectObject(dc,wave),op=SelectObject(dc,none);Ellipse(dc,client.right-330,-265,client.right+250,315);Ellipse(dc,client.right-210,-210,client.right+160,210);SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(wave);RestoreDC(dc,saved);
    drawRoundRect(dc,RECT{34,30,92,88},accentColor(),accentColor(),14);HBRUSH key=CreateSolidBrush(accentNeedsDarkText()?RGB(35,35,45):RGB(255,255,255));HGDIOBJ old=SelectObject(dc,key);for(int row=0;row<3;++row)for(int col=0;col<4;++col){RECT k{43+col*11,39+row*11,50+col*11,46+row*11};RoundRect(dc,k.left,k.top,k.right,k.bottom,2,2);}RECT space{50,73,76,79};RoundRect(dc,space.left,space.top,space.right,space.bottom,2,2);SelectObject(dc,old);DeleteObject(key);
    if(g_currentPage==0){drawRoundRect(dc,RECT{30,176,492,338},cardColor(),blend(accentColor(),surfaceColor(),23));drawRoundRect(dc,RECT{508,176,970,338},cardColor(),blend(accentColor(),surfaceColor(),23));drawRoundRect(dc,RECT{30,412,970,590},cardColor(),blend(accentColor(),surfaceColor(),23));drawRoundRect(dc,RECT{30,600,970,666},cardColor(),blend(accentColor(),surfaceColor(),23));drawRoundRect(dc,RECT{30,675,970,760},cardColor(),blend(accentColor(),surfaceColor(),23));
        HBRUSH iconBg=CreateSolidBrush(blend(accentColor(),surfaceColor(),15));HGDIOBJ oldBrush=SelectObject(dc,iconBg);HPEN iconPen=CreatePen(PS_SOLID,4,accentColor());HGDIOBJ oldPen=SelectObject(dc,iconPen);Ellipse(dc,50,218,130,298);Ellipse(dc,66,234,114,282);MoveToEx(dc,90,258,nullptr);LineTo(dc,90,244);MoveToEx(dc,90,258,nullptr);LineTo(dc,103,258);Ellipse(dc,528,218,608,298);Ellipse(dc,544,238,592,286);MoveToEx(dc,568,262,nullptr);LineTo(dc,568,248);MoveToEx(dc,568,262,nullptr);LineTo(dc,581,262);MoveToEx(dc,562,232,nullptr);LineTo(dc,574,232);MoveToEx(dc,568,232,nullptr);LineTo(dc,568,238);SelectObject(dc,oldBrush);SelectObject(dc,oldPen);DeleteObject(iconBg);DeleteObject(iconPen);
        HPEN divider=CreatePen(PS_SOLID,1,blend(accentColor(),surfaceColor(),18));HGDIOBJ op=SelectObject(dc,divider);MoveToEx(dc,56,485,nullptr);LineTo(dc,944,485);SelectObject(dc,op);DeleteObject(divider);}
    else if(g_currentPage==1)drawRoundRect(dc,RECT{24,174,976,715},cardColor(),blend(accentColor(),surfaceColor(),23));
    else drawRoundRect(dc,RECT{24,174,976,715},cardColor(),blend(accentColor(),surfaceColor(),23));
    RestoreDC(dc,savedMap);EndPaint(h,&ps);
}
static void toggleTodoCheck(){int i=(int)SendMessageW(g_todoList,LB_GETCURSEL,0,0);if(i==LB_ERR)return;SendMessageW(g_todoList,LB_SETITEMDATA,i,todoDone(i)?0:1);saveTodos();InvalidateRect(g_todoList,nullptr,TRUE);}
static LRESULT CALLBACK mainProc(HWND h,UINT m,WPARAM w,LPARAM l){
    switch(m){
    case WM_CREATE:
        g_main=h;ensureFolders();loadSettings();g_richEditModule=LoadLibraryW(L"Msftedit.dll");if(!g_richEditModule){MessageBoxW(h,L"Windows Rich Edit could not be loaded, so note formatting is unavailable.",L"KeyDesk",MB_OK|MB_ICONERROR);return -1;}makeControls(h);SetWindowTextW(GetDlgItem(h,ID_KEY_TOGGLE),g_keysVisible?L"Keystrokes: On":L"Keystrokes: Off");SetWindowTextW(GetDlgItem(h,ID_THEME_TOGGLE),g_dark?L"Switch to light theme":L"Switch to dark theme");layoutDashboard(h);loadNotes();loadTodos();makeOverlay();g_keyboardHook=SetWindowsHookExW(WH_KEYBOARD_LL,keyboardProc,g_instance,0);SetTimer(h,ID_CLOCK_TICK,100,nullptr);g_timerPausedMs=300000;selectPage(0);updateClock();return 0;
    case WM_TIMER:if(w==ID_CLOCK_TICK)updateClock();else if(w==ID_NOTE_AUTOSAVE){KillTimer(h,ID_NOTE_AUTOSAVE);saveCurrentNote();}return 0;
    case WM_NOTIFY:{NMHDR* n=(NMHDR*)l;if(n->idFrom==ID_NOTE_TABS&&n->code==TCN_SELCHANGE){saveCurrentNote();loadNote(TabCtrl_GetCurSel(g_noteTabs));}return 0;}
    case WM_COMMAND:{int id=LOWORD(w),event=HIWORD(w);switch(id){
        case ID_EXIT:saveCurrentNote();saveTodos();DestroyWindow(g_main);return 0;
        case ID_NAV_OVERVIEW:selectPage(0);return 0;case ID_NAV_NOTES:selectPage(1);return 0;case ID_NAV_TODO:selectPage(2);return 0;
        case ID_SHORTCUTS:openPalette(true);return 0;
        case ID_NAV_PREVIOUS:if(g_currentPage==0)navigateOverviewGrid(0,-1);else cycleSection(-1);return 0;
        case ID_NAV_NEXT:if(g_currentPage==0)navigateOverviewGrid(0,1);else cycleSection(1);return 0;
        case ID_TAB_PREVIOUS:if(g_currentPage==0)navigateOverviewGrid(-1,0);else cyclePageOrNoteTab(-1);return 0;
        case ID_TAB_NEXT:if(g_currentPage==0)navigateOverviewGrid(1,0);else cyclePageOrNoteTab(1);return 0;
        case ID_PALETTE_OPEN:openPalette(false);return 0;
        case ID_WATCH_START:if(g_watchRunning){g_watchElapsed+=GetTickCount64()-g_watchStart;g_watchRunning=false;}else{g_watchStart=GetTickCount64();g_watchRunning=true;}return 0;
        case ID_WATCH_RESET:g_watchRunning=false;g_watchElapsed=0;updateClock();return 0;
        case ID_ALARM_SET:{wchar_t s[32]{};GetWindowTextW(g_alarmInput,s,32);int hh=0,mm=0,period=(int)SendMessageW(g_alarmPeriod,CB_GETCURSEL,0,0);if(swscanf_s(s,L"%d:%d",&hh,&mm)==2&&hh>=1&&hh<=12&&mm>=0&&mm<60&&period>=0){g_alarmHour=(hh%12)+(period==1?12:0);g_alarmMinute=mm;int length=GetWindowTextLengthW(g_alarmMessageInput);g_alarmMessage.assign(length+1,L'\0');GetWindowTextW(g_alarmMessageInput,g_alarmMessage.data(),length+1);g_alarmMessage.resize(length);g_alarmEnabled=true;MessageBoxW(h,(L"Alarm set for "+std::wstring(s)+(period==1?L" PM":L" AM")+L". Keep KeyDesk running.").c_str(),L"KeyDesk",MB_OK);}else MessageBoxW(h,L"Enter a time from 1:00 to 12:59 and choose AM or PM.",L"KeyDesk",MB_OK|MB_ICONWARNING);return 0;}
        case ID_TIMER_START:if(g_timerRunning){g_timerPausedMs=g_timerEnd>GetTickCount64()?g_timerEnd-GetTickCount64():0;g_timerRunning=false;SetWindowTextW(g_timerStartButton,L"Start timer");}else{if(!g_timerPausedMs&&!readTimerMinutes(g_timerPausedMs)){MessageBoxW(h,L"Enter a timer duration in minutes first.",L"KeyDesk",MB_OK|MB_ICONWARNING);return 0;}g_timerEnd=GetTickCount64()+g_timerPausedMs;g_timerRunning=true;SetWindowTextW(g_timerStartButton,L"Pause timer");}updateClock();return 0;
        case ID_TIMER_RESET:g_timerRunning=false;g_timerPausedMs=0;SetWindowTextW(g_timerStartButton,L"Start timer");updateClock();return 0;
        case ID_TIMER_SET:if(readTimerMinutes(g_timerPausedMs)){g_timerRunning=false;SetWindowTextW(g_timerStartButton,L"Start timer");updateClock();}else MessageBoxW(h,L"Enter a positive duration in minutes.",L"KeyDesk",MB_OK|MB_ICONWARNING);return 0;
        case ID_KEY_TOGGLE:g_keysVisible=!g_keysVisible;ShowWindow(g_overlay,g_keysVisible?SW_SHOWNOACTIVATE:SW_HIDE);SetWindowTextW(GetDlgItem(h,ID_KEY_TOGGLE),g_keysVisible?L"Keystrokes: On":L"Keystrokes: Off");saveSettings();return 0;
        case ID_THEME_TOGGLE:g_dark=!g_dark;setTheme();SetWindowTextW(GetDlgItem(h,ID_THEME_TOGGLE),g_dark?L"Switch to light theme":L"Switch to dark theme");saveSettings();return 0;
        case ID_THEME_PICK:if(event==CBN_SELCHANGE){int selected=(int)SendMessageW((HWND)l,CB_GETCURSEL,0,0);if(selected>=0&&selected<(int)(sizeof(g_themes)/sizeof(g_themes[0]))){g_themeIndex=selected;setTheme();saveSettings();}}return 0;
        case ID_NOTE_NEW:newNote();return 0;case ID_NOTE_OPEN:openNote();return 0;case ID_NOTE_DELETE:deleteNote();return 0;
        case ID_NOTE_BOLD:toggleNoteStyle(CFM_BOLD,CFE_BOLD);return 0;case ID_NOTE_ITALIC:toggleNoteStyle(CFM_ITALIC,CFE_ITALIC);return 0;
        case ID_NOTE_EDIT:if(event==EN_CHANGE)SetTimer(h,ID_NOTE_AUTOSAVE,450,nullptr);return 0;
        case ID_TODO_ADD:addTodo();return 0;case ID_TODO_EDIT:editTodo();return 0;case ID_TODO_DELETE:deleteTodo();return 0;
        case ID_TODO_LIST:if(event==LBN_SELCHANGE){int i=(int)SendMessageW(g_todoList,LB_GETCURSEL,0,0);if(i!=LB_ERR)SetWindowTextW(g_todoInput,todoLineText(i).c_str());}else if(event==LBN_DBLCLK)toggleTodoCheck();return 0;
    }break;}
    case WM_DRAWITEM:{DRAWITEMSTRUCT* d=(DRAWITEMSTRUCT*)l;int id=GetDlgCtrlID(d->hwndItem);if(id==ID_PAGE_CLOCK||id==ID_PAGE_TIMER_DISPLAY||id==ID_WATCH_TEXT){HBRUSH b=CreateSolidBrush(cardColor());FillRect(d->hDC,&d->rcItem,b);DeleteObject(b);SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,textColor());HFONT font=(HFONT)SendMessageW(d->hwndItem,WM_GETFONT,0,0);HGDIOBJ old=SelectObject(d->hDC,font?font:uiFont());wchar_t label[128]{};GetWindowTextW(d->hwndItem,label,128);RECT r=d->rcItem;if(id==ID_WATCH_TEXT)InflateRect(&r,-4,0);DrawTextW(d->hDC,label,-1,&r,(id==ID_WATCH_TEXT?DT_LEFT:DT_CENTER)|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);SelectObject(d->hDC,old);return TRUE;}if(w==ID_TODO_LIST){if(d->itemID==(UINT)-1)return TRUE;bool selected=(d->itemState&ODS_SELECTED)!=0,done=todoDone((int)d->itemID);COLORREF back=selected?(g_dark?blend(accentColor(),RGB(25,25,30),28):blend(accentColor(),RGB(255,255,255),15)):surfaceColor();HBRUSH b=CreateSolidBrush(back);FillRect(d->hDC,&d->rcItem,b);DeleteObject(b);SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,textColor());RECT r=d->rcItem;r.left+=16;r.right-=10;std::wstring label=(done?L"☑  ":L"□  ")+todoLineText((int)d->itemID);if(done)SetTextColor(d->hDC,g_dark?RGB(150,155,160):RGB(130,135,140));DrawTextW(d->hDC,label.c_str(),-1,&r,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);if(d->itemState&ODS_FOCUS)DrawFocusRect(d->hDC,&d->rcItem);return TRUE;}drawButton(d);return TRUE;}
    case WM_PAINT:paintDashboard(h);return 0;
    case WM_LBUTTONDOWN:SetFocus(h);return 0;
    case WM_ERASEBKGND:{RECT r{};GetClientRect(h,&r);HBRUSH b=CreateSolidBrush(backgroundColor());FillRect((HDC)w,&r,b);DeleteObject(b);return 1;}
    case WM_CTLCOLORSTATIC:{HDC dc=(HDC)w;SetTextColor(dc,textColor());SetBkMode(dc,TRANSPARENT);return (LRESULT)GetStockObject(HOLLOW_BRUSH);}
    case WM_CTLCOLOREDIT:case WM_CTLCOLORLISTBOX:{HDC dc=(HDC)w;SetTextColor(dc,textColor());SetBkColor(dc,surfaceColor());static HBRUSH brush=nullptr;if(brush)DeleteObject(brush);brush=CreateSolidBrush(surfaceColor());return (LRESULT)brush;}
    case WM_SIZE:if(w!=SIZE_MINIMIZED){layoutDashboard(h);InvalidateRect(h,nullptr,TRUE);}if(g_overlay){if(w==SIZE_MINIMIZED){if(g_keysVisible)ShowWindow(g_overlay,SW_SHOWNOACTIVATE);}else ShowWindow(g_overlay,SW_HIDE);}return 0;
    case WM_CLOSE:saveCurrentNote();saveTodos();saveSettings();if(g_keysVisible){ShowWindow(g_main,SW_HIDE);ShowWindow(g_overlay,SW_SHOWNOACTIVATE);}else ShowWindow(g_main,SW_MINIMIZE);return 0;
    case WM_DESTROY:saveCurrentNote();saveTodos();saveOverlayPosition();saveSettings();if(g_alarmNotification)DestroyWindow(g_alarmNotification);if(g_keyboardHook)UnhookWindowsHookEx(g_keyboardHook);if(g_overlay)DestroyWindow(g_overlay);if(g_uiFont)DeleteObject(g_uiFont);if(g_overlayFont)DeleteObject(g_overlayFont);if(g_overlayClockFont)DeleteObject(g_overlayClockFont);if(g_bigFont)DeleteObject(g_bigFont);if(g_timerFont)DeleteObject(g_timerFont);if(g_headingFont)DeleteObject(g_headingFont);PostQuitMessage(0);return 0;
    }return DefWindowProcW(h,m,w,l);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR,int){
    g_instance=instance;INITCOMMONCONTROLSEX cc{sizeof(cc),ICC_TAB_CLASSES};InitCommonControlsEx(&cc);WNDCLASSW wc{};wc.lpfnWndProc=mainProc;wc.hInstance=instance;wc.lpszClassName=L"KeyDeskMainWindow";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassW(&wc);
    HMONITOR mon=MonitorFromPoint(POINT{250,180},MONITOR_DEFAULTTONEAREST);MONITORINFO mi{sizeof(mi)};GetMonitorInfoW(mon,&mi);int workLeft=(int)mi.rcWork.left,workTop=(int)mi.rcWork.top,workRight=(int)mi.rcWork.right,workBottom=(int)mi.rcWork.bottom;int winW=std::min(1020,workRight-workLeft),winH=std::min(780,workBottom-workTop);int x=std::clamp(250,workLeft,std::max(workLeft,workRight-winW)),y=std::clamp(180,workTop,std::max(workTop,workBottom-winH));
    g_main=CreateWindowW(wc.lpszClassName,L"KeyDesk",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,x,y,winW,winH,nullptr,nullptr,instance,nullptr);if(!g_main)return 1;UpdateWindow(g_main);
    std::vector<ACCEL> entries;for(const auto& s:g_shortcuts)if(s.command)entries.push_back(ACCEL{s.modifiers,s.key,(WORD)s.command});HACCEL shortcuts=CreateAcceleratorTableW(entries.data(),(int)entries.size());
    MSG msg;while(GetMessageW(&msg,nullptr,0,0)>0){
        if(g_palette&&msg.message==WM_KEYDOWN&&msg.wParam==VK_ESCAPE){SendMessageW(g_palette,WM_CLOSE,0,0);continue;}
        if(g_palette&&msg.message==WM_KEYDOWN&&msg.wParam==VK_DOWN&&GetFocus()==g_paletteSearch){SetFocus(g_paletteList);if(SendMessageW(g_paletteList,LB_GETCURSEL,0,0)==LB_ERR)SendMessageW(g_paletteList,LB_SETCURSEL,0,0);continue;}
        if(g_palette&&msg.message==WM_KEYDOWN&&msg.wParam==VK_RETURN){SendMessageW(g_palette,WM_COMMAND,MAKEWPARAM(IDOK,0),0);continue;}
        if(g_palette&&IsDialogMessageW(g_palette,&msg))continue;
        HWND foreground=GetForegroundWindow();DWORD process=0;if(foreground)GetWindowThreadProcessId(foreground,&process);
        bool openPaletteChord=msg.message==WM_KEYDOWN&&msg.wParam=='P'&&(GetKeyState(VK_CONTROL)&0x8000)!=0;
        bool noteFormatChord=msg.message==WM_KEYDOWN&&GetFocus()==g_noteEdit&&(GetKeyState(VK_CONTROL)&0x8000)&&(msg.wParam=='B'||msg.wParam=='I');
        bool allowShortcut=!focusIsTextControl()||(!g_palette&&openPaletteChord)||noteFormatChord;
        if(process==GetCurrentProcessId()&&shortcuts&&allowShortcut&&TranslateAcceleratorW(g_main,shortcuts,&msg))continue;
        TranslateMessage(&msg);DispatchMessageW(&msg);
    }
    if(shortcuts)DestroyAcceleratorTable(shortcuts);return (int)msg.wParam;
}
