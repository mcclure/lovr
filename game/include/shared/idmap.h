// General template utilities: Scaffolding to expose some resource to Lua using an integer identifier.

#pragma once

template<class T> struct IdMap : public std::map<unsigned int, T*> {
	unsigned int generator;

	unsigned int push(T* v) {
	  unsigned int id = generator++;
	  (*this)[id] = v;
	  return id;
	}

	unsigned int push() {
	  return push(new T);
	}

	void deleteAll() {
		for (auto &kv: *this) {
			delete kv.second;
		}
		this->clear();
	}

	void deleteOne(unsigned int id) {
		if (this->count(id)) {
			delete (*this)[id];
			this->erase(id);
		}
	}
};

#define IDMAP_PUSH(table, item) \
	lua_pushnumber(L, table.push(item))

#define IDMAP_FILLIN_NEW_IMPL(name_space, table) \
static int l_fillin_new_ ## name_space ## _ ## table (lua_State *L) { \
  lua_pushnumber(L, table.push()); \
  return 1; \
}

#define IDMAP_FILLIN_NEW(name_space, table) \
	{ "new", l_fillin_new_ ## name_space ## _ ## table }

#define IDMAP_FILLIN_DELETE_IMPL(name_space, table) \
static int l_fillin_delete_ ## name_space ## _ ## table (lua_State *L) { \
  unsigned int id = luaL_checknumber(L, 1); \
  table.deleteOne(id); \
  return 0; \
}

#define IDMAP_FILLIN_DELETE(name_space, table) \
	{ "delete", l_fillin_delete_ ## name_space ## _ ## table }
