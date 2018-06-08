static void conn_autocommit_event(int fd, short event, void *_userdata) {
  struct maria_status *ms = (struct maria_status *)_userdata;
  MYSQL *conn = (MYSQL *)ms->data;
  lua_State *L = ms->L;

  int errorcode = mysql_errno(conn);
  if (errorcode) {
    utlua_resume(L, NULL, luamariadb_push_errno(L, ms->conn_data));
    UNREF_CO(ms->conn_data);
  } else {
    my_bool ret = 0;
    int status = mysql_autocommit_cont(&ret, conn, ms->status);
    if (status) {
      wait_for_status(L, ms->conn_data, conn, status, conn_autocommit_event,
                      ms->extra);
    } else if (ret == 0) {
      lua_pushboolean(L, true);
      utlua_resume(L, NULL, 1);
      UNREF_CO(ms->conn_data);
    } else {
      utlua_resume(L, NULL, luamariadb_push_errno(L, ms->conn_data));
      UNREF_CO(ms->conn_data);
    }
  }
  event_free(ms->event);
  free(ms);
}

LUA_API int conn_autocommit_start(lua_State *L) {
  conn_data *conn = getconnection(L);
  my_bool auto_mode = lua_toboolean(L, 2);

  my_bool ret = 0;
  int status = mysql_autocommit_start(&ret, &conn->my_conn, auto_mode);
  if (status) {
    REF_CO(conn);
    wait_for_status(L, conn, &conn->my_conn, status, conn_autocommit_event, 0);
    return lua_yield(L, 0);
  } else if (ret == 0) {
    lua_pushboolean(L, true);
    return 1;
  } else {
    return luamariadb_push_errno(L, conn);
  }
}