#include <iostream>
#include <ctime>
#include <sstream>
#include <fstream>
#include <boost/algorithm/string.hpp>

#include "enums.h"
#include "params.h"
#include "logmsg.h"
#include "exceptions.h"
#include "utils.h"
#include "termcolor.h"

using namespace std;

std::string timestamp()
{
        //Notice the use of a stringstream, yet another useful stream medium!
        ostringstream stream;
        time_t rawtime;
        tm * timeinfo;

        time(&rawtime);
        timeinfo = localtime( &rawtime );

        stream << (timeinfo->tm_year)+1900<<"/"<<timeinfo->tm_mon
        <<"/"<<timeinfo->tm_mday<<" "<<timeinfo->tm_hour
        <<":"<<timeinfo->tm_min;
        // The str() function of output stringstreams return a std::string.
        return stream.str();
}

std::string levelname(eLogLevel level)
{
   switch (level)
   {
      case kLDEBUG: return "DEBUG";
      case kLINFO:  return "INFO ";
      case kLWARN:  return "WARN ";
      case kLERROR: return "ERROR";
      default: return " ??? ";
   }
}

void logverbatim(eLogLevel level, std::string s, const params & p)
{
   eLogLevel cutoff = p;
   if (level<cutoff)
      return;

   // we use stdout for normal messages, stderr for warn and error.
   switch (level)
   {
      case kLDEBUG: std::cout << termcolor::cyan << s << termcolor::reset; break;
      case kLINFO:  std::cout << termcolor::blue << s << termcolor::reset; break;
      case kLWARN:  std::cerr << termcolor::yellow <<  s << termcolor::reset; break;
      case kLERROR:
         std::cerr << termcolor::red <<  s << termcolor::reset;
         throw eExit(s.c_str());
         break;
      default:      std::cerr << termcolor::green <<  s << termcolor::reset; break;
   }
}

std::string getheader(eLogLevel level)
{
   std::ostringstream ost;
   ost << "|" << levelname(level) << "|" << timestamp() << "| ";
   return ost.str();
}


void logmsg(eLogLevel level, std::string s, const params & p)
{   
   eLogLevel cutoff = p;
   if (level < cutoff)
      return;

   std::string info = getheader(level);
   std::string s2=utils::replacestring(s,"\n","\n"+info);
   boost::erase_all(s2, "\r");

   logverbatim(level,info+s2+"\n",cutoff);
}

void fatal(std::string s)
{
   logmsg(kLERROR, s, kLERROR);
}


dServiceLogger::dServiceLogger(bool cerr, const params & p, bool isServiceCmd) :
   mCErr(cerr), mP(p), mInitialised(false)
{
   if (isServiceCmd)
      mOutputMode = p.getServiceOutput_servicecmd();
   else
      mOutputMode = p.getServiceOutput_hooks();
}

void dServiceLogger::log(const char * const buf, int n)
{
   if (mOutputMode == kOSuppressed)
      return;
   if (n == 0)
      return;

   if (mOutputMode==kORaw)
   { // output raw.
      if (mCErr)
         std::cerr.write(buf, n).flush();
      else
         std::cout.write(buf, n).flush();
      return;
   }

   // buffer and log.
   std::ostringstream oss;
   oss.write(buf, n).flush();
   processbuffer(oss.str());
}

void dServiceLogger::nonrawoutput(std::string s)
{
   if (mCErr)
      logverbatim(kLWARN, s, mP);
   else
      logverbatim(kLINFO, s, mP);
}

void dServiceLogger::processbuffer(std::string buffer)
{
   mEscapeFilter.strip(buffer);
   while (buffer.length() > 0) {
      if (!mInitialised)
      {
         mInitialised = true;
         nonrawoutput(getheader(mCErr ? kLWARN : kLINFO));
      }

      size_t pos = buffer.find('\n');
      if (pos == std::string::npos)
      {
         nonrawoutput(buffer);
         return;
      }
      nonrawoutput(buffer.substr(0, pos + 1));
      buffer.erase(0, pos + 1);
      mInitialised = false;
   }
}

escapefilter::escapefilter() : mStage(kSearching)
{
}

// This class strips standard escape sequences used for terminal color
// from a stream (e.g. raw reads that can stop at any character, including
// in the middle of an escape sequence). It stores state across calls
// to know what to strip.
void escapefilter::strip(std::string & buffer)
{
   mPos = 0; // new string so if we're triggered etc consider from beginning.

   while (true)
   { // breakable loop

      if (mStage == kSearching)
      {
         mPos = buffer.find('\033');
         if (mPos == std::string::npos)
            return;
         buffer.erase(mPos, 1);
         mStage = kGotEscape;
      }

      if (mStage == kGotEscape)
      {
         if (mPos >= buffer.length())
            return;

         if (buffer[mPos] != '[')
            mStage = kSearching;            
         else
            mStage = kTriggered;
      }

      if (mStage == kTriggered)
      {
         size_t epos = buffer.find('m', mPos);
         if (epos == std::string::npos)
         {
            buffer.erase(mPos);
            return;
         }
         buffer.erase(mPos, epos - mPos + 1);
         mStage = kSearching;
      }
   }
}

