#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct lua_State lua_State;
    typedef const char* (*lua_Reader)(lua_State*, void*, size_t*);

    lua_State* InitLuaState(void);

    lua_State* GetLuaState(void);

    int ParseLuaBinary(const char* name, void* chunk_data, lua_Reader reader);

    int ParseLuaFile(const char* filename);

    void ReloadModule(const char* module_name);

    int EnterLuaCallback(int global_ref, int func_ref);

    int ExitLuaCallback(void);

    int GetLuaMethodReference(const char* l_class,
                              const char* func,
                              int*        global_handle); // = nullptr
    int GetLuaGlobalReference(const int l_class, const char* func);

    //------------------------------------------------------------------------------

    typedef struct Vec4
    {
        union
        {
            struct
            {
                float x, y, z, w;
            };
            float raw[4];
        };
    } Vec4;

    void PushFBufferToLua(float fbuff[], int fbuff_sz);

    void ReadFBufferFromLua(float fbuff[], int fbuff_sz, int index);

#ifdef __cplusplus
}
#endif
