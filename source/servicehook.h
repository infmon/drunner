#ifndef __SERVICE_HOOK_H
#define __SERVICE_HOOK_H

#include <string>

#include "enums.h"
#include "params.h"
#include "cresult.h"

class service;

// class to allow dService to hook into any supported action.
class servicehook
{
public:
   servicehook(const service * const svc, std::string actionname, const std::vector<std::string> & hookparams, const params & p);
   cResult starthook();
   cResult endhook();
private:
   void setNeedsHook(const service * const svc);
   cResult runHook(std::string se);

   std::string mActionName;
   const std::vector<std::string> & mHookParams;
   const params & mParams;

   std::string mServiceRunner;

   std::string mStartCmd;
   std::string mEndCmd;
};

#endif
