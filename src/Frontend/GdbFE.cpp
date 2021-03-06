#include "Frontend/GdbFE.h"
#include "Frontend/ImGuiFileBrowser.h"
#include "Frontend/TextEditor.h"
#include "LuaLayer.h"
#include "ProcessIO.h"
#include "UtilityMacros.h"
#include "imgui.h"
#include "lua.hpp"
#include <sstream>
#include <stdlib.h>

extern const char _binary__tmp_prog_luac_start;
extern const char _binary__tmp_prog_luac_end;

struct LStream
{
    const char* m_Data;
    size_t      m_DataSz;
};
static LStream s_stream_data;

static int
LuaUpdate(void);

static void
LoadResources(const LoadSettings* lset);

static int
SaveAndExit(void);

#ifdef __cplusplus
extern "C"
{
#endif

    void InitFrontend(const LoadSettings* lset) { LoadResources(lset); }
    int  DrawFrontend(void) { return LuaUpdate(); }
    int  CloseFrontend(void) { return SaveAndExit(); }

#ifdef __cplusplus
}
#endif

//------------------------------------------------------------------------------

#define MAX_RESP_SZ 1024 * 8
static char s_output_text[MAX_RESP_SZ];

static TextEditor s_editor;
static FileInfo   s_finfo;

static imgui_addons::ImGuiFileBrowser s_FileDlg;

//------------------------------------------------------------------------------

static void
GetGdbResponse(char buff[] = nullptr, int32_t buff_sz = -1)
{
    buff_sz = buff_sz > 0 ? buff_sz : MAX_RESP_SZ;
    if (buff == nullptr) {
        buff = s_output_text;
    }

    GdbMsg resp = GdbOutput();
    if (resp.m_MsgSz && (resp.m_MsgSz < (uint32_t)buff_sz)) {
        memcpy(buff, resp.m_Msg, resp.m_MsgSz);
        buff[resp.m_MsgSz] = 0;
    } else {
        // TODO : process error when reading gdb reply
    }
}

//------------------------------------------------------------------------------

struct LuaRefs
{
    int32_t m_GlobalRef;
    int32_t m_FuncRef;
};

static LuaRefs s_app_init;
static LuaRefs s_app_upd;
static LuaRefs s_app_exit;

static int
SetEditorTheme(lua_State* L);

static int
OpenFileDialog(lua_State* L);

static int
SendToGdb(lua_State* L);

static int
ReadFromGdb(lua_State* L);

static int
SetEditorFile(lua_State* L);

static int
SetEditorFileLineNum(lua_State* L);

static int
GetEditorFileLineNum(lua_State* L);

static int
SetEditorBkPts(lua_State* L);

static int
ShowTextEditor(lua_State* L);

static void
AddCFunc(lua_State* L, const char* name, lua_CFunction func)
{
    lua_pushcfunction(L, func);
    lua_setglobal(L, name);
}

static const char*
LuaDataStreamer(lua_State* L, void* data, size_t* sz)
{
    LStream* stream_d = (LStream*)data;
    *sz               = stream_d->m_DataSz;
    return stream_d->m_Data; // &_binary__tmp_prog_luac_start
}

static void
LoadResources(const LoadSettings* lset)
{
    s_finfo.m_Contents     = (char*)WmMalloc(lset->m_MaxFileSz);
    s_finfo.m_ContentMaxSz = lset->m_MaxFileSz;

    UNUSED_VAR(s_stream_data);
    UNUSED_VAR(LuaDataStreamer);

    char app[512] = { 0 };
    snprintf(app, sizeof(app), "%s/.gdbvkgui/prog.luac", getenv("HOME"));
    ParseLuaFile(app);

    int32_t init_g_ref, init_f_ref;
    init_f_ref = GetLuaMethodReference("GdbApp", "Init", &init_g_ref);
    s_app_init.m_GlobalRef = init_g_ref;
    s_app_init.m_FuncRef   = init_f_ref;

    int32_t upd_g_ref, upd_f_ref;
    upd_f_ref = GetLuaMethodReference("GdbApp", "Update", &upd_g_ref);
    s_app_upd.m_GlobalRef = upd_g_ref;
    s_app_upd.m_FuncRef   = upd_f_ref;

    int32_t exit_g_ref, exit_f_ref;
    exit_f_ref = GetLuaMethodReference("GdbApp", "OnExit", &exit_g_ref);
    s_app_exit.m_GlobalRef = exit_g_ref;
    s_app_exit.m_FuncRef   = exit_f_ref;

    // hook up C-functions
    lua_State* lstate = GetLuaState();

    AddCFunc(lstate, "SetEditorTheme", SetEditorTheme);
    AddCFunc(lstate, "FileDialog", OpenFileDialog);
    AddCFunc(lstate, "SendToGdb", SendToGdb);
    AddCFunc(lstate, "ReadFromGdb", ReadFromGdb);
    AddCFunc(lstate, "SetEditorFile", SetEditorFile);
    AddCFunc(lstate, "SetEditorFileLineNum", SetEditorFileLineNum);
    AddCFunc(lstate, "GetEditorFileLineNum", GetEditorFileLineNum);
    AddCFunc(lstate, "SetEditorBkPts", SetEditorBkPts);
    AddCFunc(lstate, "ShowTextEditor", ShowTextEditor);

    // initialize any neccessary lua state
    if (EnterLuaCallback(s_app_init.m_GlobalRef, s_app_init.m_FuncRef)) {
        ExitLuaCallback();
    }
}

static int
LuaUpdate(void)
{
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
    ImGuiIO& io = ImGui::GetIO();

    if (EnterLuaCallback(s_app_upd.m_GlobalRef, s_app_upd.m_FuncRef)) {
        // push arguments
        lua_State* lstate = GetLuaState();

        lua_pushinteger(lstate, io.DisplaySize.x);
        lua_setfield(lstate, -2, "win_width");

        lua_pushinteger(lstate, io.DisplaySize.y);
        lua_setfield(lstate, -2, "win_height");

        ExitLuaCallback();
    }

    return 0;
}

static int
SaveAndExit(void)
{
    if (EnterLuaCallback(s_app_exit.m_GlobalRef, s_app_exit.m_FuncRef)) {
        // push arguments
        // lua_State* lstate = GetLuaState();

        ExitLuaCallback();
    }
    return 0;
}

static int
SetEditorTheme(lua_State* L)
{
    uint32_t idx = (int32_t)luaL_checkinteger(L, 1);
    switch (idx) {
        case 1:
            s_editor.SetPalette(TextEditor::GetDarkPalette());
            break;
        case 2:
            s_editor.SetPalette(TextEditor::GetLightPalette());
            break;
        case 3:
            s_editor.SetPalette(TextEditor::GetRetroBluePalette());
            break;
        default:
            s_editor.SetPalette(TextEditor::GetDarkPalette());
            break;
    }
    return 0;
}

static int
OpenFileDialog(lua_State* L)
{
    const char* name = (const char*)luaL_checkstring(L, 1);
    Vec4        sz   = { 0 };
    ReadFBufferFromLua(sz.raw, 2, 2);

    if (s_FileDlg.showFileDialog(
          name, imgui_addons::ImGuiFileBrowser::DialogMode::OPEN, ImVec2(sz))) {

        lua_pushstring(L, s_FileDlg.selected_path.c_str());
    } else {
        lua_pushnil(L);
    }

    return 1;
}

static int
SendToGdb(lua_State* L)
{
    const char* cmd = (const char*)luaL_checkstring(L, 1);

    bool sent = SendCommand("%s", cmd);
    lua_pushboolean(L, sent);

    return 1;
}

static int
ReadFromGdb(lua_State* L)
{
    GetGdbResponse();

    lua_pushstring(L, s_output_text);

    return 1;
}

static int
SetEditorFile(lua_State* L)
{
    const char* fname    = (const char*)luaL_checkstring(L, 1);
    uint32_t    line_num = (uint32_t)luaL_checkinteger(L, 2);
    // uint32_t    clmn_num = (uint32_t)luaL_checkinteger(L, 3);

    bool success = false;

    if (ReadFile(fname, &s_finfo)) {
        s_editor.SetText(s_finfo.m_Contents);
        s_editor.SetReadOnly(true);

        line_num = CLAMP(line_num - 1, 0, (uint32_t)s_editor.GetTotalLines());

        // set cursor to 0,0 then to actual location
        // this should ensure that the text follows gdb
        TextEditor::Coordinates cpos(0, 0);
        s_editor.SetCursorPosition(cpos);
        cpos.mLine = line_num;
        s_editor.SetCursorPosition(cpos);

        success = true;
    }
    lua_pushboolean(L, success);
    return 1;
}

static int
SetEditorFileLineNum(lua_State* L)
{
    uint32_t line_num = (uint32_t)luaL_checkinteger(L, 1);
    uint32_t clmn_num = (uint32_t)luaL_checkinteger(L, 2);

    line_num = CLAMP(line_num - 1, 0, (uint32_t)s_editor.GetTotalLines());

    TextEditor::Coordinates cpos(line_num, clmn_num);
    s_editor.SetCursorPosition(cpos);

    return 0;
}

static int
GetEditorFileLineNum(lua_State* L)
{
    TextEditor::Coordinates cpos = s_editor.GetCursorPosition();

    lua_pushinteger(L, cpos.mLine);

    return 1;
}

static int
SetEditorBkPts(lua_State* L)
{
    uint32_t bkpt_cnt = (uint32_t)luaL_checkinteger(L, 1);
    if (bkpt_cnt) {
        float* fbuff = (float*)WmMalloc(bkpt_cnt * sizeof(float));
        ReadFBufferFromLua(fbuff, bkpt_cnt, 2);

        TextEditor::Breakpoints bkpts;
        for (uint32_t i = 0; i < bkpt_cnt; i++) {
            bkpts.insert(fbuff[i]);
        }
        s_editor.SetBreakpoints(bkpts);

        WmFree(fbuff);
    } else {
        // clear
        TextEditor::Breakpoints bkpts;
        s_editor.SetBreakpoints(bkpts);
    }
    return 0;
}

static int
ShowTextEditor(lua_State* L)
{
    UNUSED_VAR(L);

    s_editor.Render("Editor");

    return 0;
}

//-----------------------------------------------------------------------------
