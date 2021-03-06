#ifndef __PARAMS_H
#define __PARAMS_H

#include <vector>
#include <string>

#include "enums.h"

class params {
public:
   params(int argc, char **argv);
   params(eLogLevel ll);
   std::string substitute( const std::string & source ) const;

   const std::string & getVersion() const { return mVersion; }
   eCommand getCommand() const { return mCmd; }
   eLogLevel getLogLevel() const { return mLogLevel; }

   edServiceOutput getServiceOutput_hooks() const { return mServiceOutput_hooks; }
   edServiceOutput getServiceOutput_servicecmd() const { return mServiceOutput_servicecmd; }

   const std::vector<std::string> & getArgs() const { return mArgs; }
   int numArgs() const { return mArgs.size(); }

   // implicit conversion to allow easy logging.
   operator eLogLevel() const { return mLogLevel; }

   std::string getOption() const { return mOption; }

private:
   std::string mVersion;
   eCommand mCmd;
   std::vector<std::string> mArgs;
   eLogLevel mLogLevel;
   
   edServiceOutput mServiceOutput_hooks;
   edServiceOutput mServiceOutput_servicecmd;

   std::string mOption;
   eCommand parsecmd(std::string s) const;
   params();
   void setdefaults();
};

#endif
