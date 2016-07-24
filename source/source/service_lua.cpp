#include "service_lua.h"
#include "utils.h"
#include "globallogger.h"
#include "dassert.h"
#include "luautils.h"

#include "lua.hpp"

namespace servicelua
{
   static staticLuaStorage<luafile> sFile; // provide access to the calling file C++ object.
   
   extern "C" int l_addconfig(lua_State *L)
   {
      if (lua_gettop(L) != 1)
         return luaL_error(L, "Expected exactly one argument (the docker container to stop) for dstop.");
      std::string cname = lua_tostring(L, 1); // first argument. http://stackoverflow.com/questions/29449296/extending-lua-check-number-of-parameters-passed-to-a-function

      cResult rval = kRSuccess;
      lua_pushinteger(L, rval);
      return 1; // one argument to return.
   }

   extern "C" int l_addvolume(lua_State *L)
   {
      if (lua_gettop(L) != 1)
         return luaL_error(L, "Expected exactly one argument (the docker container to stop) for dstop.");
      std::string cname = lua_tostring(L, 1); // first argument. http://stackoverflow.com/questions/29449296/extending-lua-check-number-of-parameters-passed-to-a-function

      cResult rval = kRSuccess;
      lua_pushinteger(L, rval);
      return 1; // one argument to return.
   }

   extern "C" int l_addcontainer(lua_State *L)
   {
      if (lua_gettop(L) != 1)
         return luaL_error(L, "Expected exactly one argument (the name of the container) for addcontainer.");
      std::string cname = lua_tostring(L, 1); // first argument. http://stackoverflow.com/questions/29449296/extending-lua-check-number-of-parameters-passed-to-a-function

      sFile.get(L)->addContainer(cname);

      cResult rval = kRSuccess;
      lua_pushinteger(L, rval);
      return 1; // one argument to return.
   }

   luafile::luafile(const servicePaths & p) : mServicePaths(p), mServiceVars(p)
   {
      drunner_assert(mServicePaths.getPathServiceLua().isFile(),"Coding error: path provided to simplefile is not a file!");
   }

   extern "C" static const struct luaL_Reg d_lualib[] = {
      { "addconfig", l_addconfig },
      { "addcontainer", l_addcontainer },
      { "addvolume", l_addvolume },
      { NULL, NULL }  /* sentinel */
   };
      
   extern "C" int luaopen_dlib(lua_State *L) {
      luaL_newlib(L, d_lualib);
      return 1;
   }
      
   cResult luafile::loadlua()
   {
      _safeloadvars(); // set defaults, load real values if we can

      Poco::Path path = mServicePaths.getPathServiceLua();
      drunner_assert(path.isFile(),"Coding error: path provided to loadyml is not a file.");
      drunner_assert(utils::fileexists(path),"The expected file does not exist: "+ path.toString()); // ctor of simplefile should have set mReadOkay to false if this wasn't true.

      dLuaState L;
      luaL_openlibs(L);

      staticmonitor<luafile> monitor(L, this, &sFile);

      lua_pushcfunction(L, l_addconfig);   // see also http://stackoverflow.com/questions/2907221/get-the-lua-command-when-a-c-function-is-called
      lua_setglobal(L, "addconfig");
      lua_pushcfunction(L, l_addvolume);
      lua_setglobal(L, "addvolume");
      lua_pushcfunction(L, l_addcontainer);
      lua_setglobal(L, "addcontainer");

      int loadok = luaL_loadfile(L, path.toString().c_str());
      if (loadok != 0)
         fatal("Failed to load " + path.toString());
      if (lua_pcall(L, 0, LUA_MULTRET, 0) != 0)
         fatal("Failed to execute " + path.toString() + " " + lua_tostring(L, -1));

      // pull out the relevant config items.
      lua_getglobal(L, "drunner_setup");
      if (lua_pcall(L, 0, 0, 0) != LUA_OK)
         fatal("Error running drunner_setup, " + std::string(lua_tostring(L, -1)));
      
      // update IMAGENAME if not yet set.
      if (mServiceVars.getVariables().hasKey("IMAGENAME"))
         drunner_assert(utils::stringisame(mServiceVars.getVariables().getVal("IMAGENAME"), getImageName()), "Service's IMAGENAME has changed.");
      mServiceVars.setVariable("IMAGENAME", getImageName());

      return kRSuccess;
   }

   void luafile::getManageDockerVolumeNames(std::vector<std::string> & vols) const
   {
      drunner_assert(vols.size() == 0,"Coding error: passing dirty volume vector to getManageDockerVolumeNames");
      for (const auto & v : mVolumes)
         if (v.external)
            vols.push_back(v.name);
   }
   void luafile::getBackupDockerVolumeNames(std::vector<std::string> & vols) const
   {
      drunner_assert(vols.size() == 0, "Coding error: passing dirty volume vector to getBackupDockerVolumeNames");
      for (const auto & v : mVolumes)
         if (v.backup)
            vols.push_back(v.name);
   }

   void luafile::addContainer(std::string cname)
   {
      drunner_assert(cname.size() > 0, "Empty container name passed to addContainer.");
      drunner_assert(std::find(mContainers.begin(), mContainers.end(), cname) == mContainers.end(), "Container already exists " + cname);
      mContainers.push_back(cname);
   }

   cResult luafile::_safeloadvars()
   {
      // set standard vars (always present).
      mServiceVars.setVariable("SERVICENAME", mServicePaths.getName());
      drunner_assert(mContainers.size() == 0, "_safeloadvars: containers already defined.");

      // set defaults for custom vars.
      for (const auto & ci : mConfigItems)
         mServiceVars.setVariable(ci.name, ci.defaultval);

      // attempt to load config file.
      if (mServiceVars.loadconfig() != kRSuccess)
         logmsg(kLDEBUG, "Couldn't load service variables.");

      // we always succeed.
      return kRSuccess;
   }

   const std::vector<std::string> & luafile::getContainers() const
   {
      return mContainers;
   }
   const std::vector<Configuration> & luafile::getConfigItems() const
   {
      return mConfigItems;
   }

   std::string luafile::getImageName() const
   {
      drunner_assert(mContainers.size() > 0, "getImageName: no containers defined.");
      return mContainers[0];
   }


} // namespace