void vrx_init_lua();
void vrx_close_lua();
int vrx_lua_get_int(char* varname, double default_var);
double vrx_lua_get_double(char* varname);
const char* vrx_lua_get_string(char* varname);
void vrx_lua_run_map_settings(char* mapname);
double vrx_lua_get_variable(char* varname, double default_var);
void Lua_RunSettingScript(const char* filename);
qboolean vrx_lua_start_table_iter(const char* tablename);
int vrx_lua_iter_next_string(char** out);

void vrx_lua_event(const char* eventname);