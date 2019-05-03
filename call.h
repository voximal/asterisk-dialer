/*
 * GnuDialer - Complete, free predictive dialer
 *
 * Complete, free predictive dialer for contact centers.
 *
 * Copyright (C) 2006, GnuDialer Project
 *
 * Heath Schultz <heath1444@yahoo.com>
 * Richard Lyman <richard@dynx.net>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License.
 */

#include <queue>
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include "options.h"
#include "exceptions.h"
#include "Socket.h"
#include "itos.h"
#include "time.h"

#ifndef CALL
#define CALL

const bool doColor = true;
const std::string neon = "\033[1;32m";  // set foreground color to light green
const std::string norm = "\033[0m"; // reset to system

static unsigned int counter = 0;

class Call;
std::string workingCampaign;
bool countCallsForCampaign(const Call &);

class Call
{

public:

  Call(
    const std::string & campaign,
    const std::string & leadid,
    const std::string & number,
    const std::string & url,    
    const std::string & param,
    
    const unsigned short int &timeout,
    const std::string & dialformat,
    const std::string & callerid,
    
    const std::string & transfer,
    
    const std::string & extravars
    
    )
  {
    itsCampaign = campaign;
    itsLeadId = leadid;
    itsNumber = number;
    itsUrl = url;
    itsParam = param;
 
    itsTimeout = timeout;
    itsDialFormat = dialformat;
    itsCallerId = callerid;
    
    itsMode = transfer;

    itsExtraVars = extravars;
    
    called = false;
    answered = false;
    unanswered = false;
    hangup = false;

    itsTime = 0;
  }

  const std::string & GetNumber() const
  {
    return itsNumber;
  }
  const std::string & GetUrl() const
  {
    return itsUrl;
  }
  const std::string & GetParam() const
  {
    return itsParam;
  }
  const std::string & GetCampaign() const
  {
    return itsCampaign;
  }
  const std::string & GetLeadId() const
  {
    return itsLeadId;
  }
  const std::string & GetCallerId() const
  {
    return itsCallerId;
  }
  const std::string & GetUniqueId() const
  {
    return itsUniqueId;
  }
  void SetUniqueId(const std::string & uniqueid)
  {
    itsUniqueId = uniqueid;
    return;
  }
  const std::string & GetDialFormat() const
  {
    return itsDialFormat;
  }
  const std::string & GetExtraVars() const
  {
    return itsExtraVars;
  }
  const std::string & GetTransfer() const
  {
    return itsMode;
  }
  const unsigned long int &GetTime() const
  {
    return itsTime;
  }
  const unsigned short int &GetTimeout() const
  {
    return itsTimeout;
  }

  const bool & HasBeenCalled() const
  {
    return called;
  }
  
  const bool & HasBeenAnswered() const
  {
    return answered;
  }

  void SetAnswered()
  {
    answered = true;
    itsTimeout = 0;    
  }

  void SetUnanswered(const std::string & value)
  {
    unanswered = true;
    reason = value;
  }

  const bool & HasBeenUnanswered() const
  {
    return unanswered;
  }
  
  void SetHangup(const std::string & value)
  {
    hangup = true;
    cause = value;
  }

  const bool & HasBeenHangup() const
  {
    return hangup;
  }  
  
  void DoCall(const std::string & mainHost)
  {
    std::string response, request;
    char uniqueid[20];

    sprintf(uniqueid, "%li.%d", time(NULL), counter++);

    called = true;
    host = mainHost;

    timeval tv;
    gettimeofday(&tv, NULL);
    itsTime = tv.tv_sec % 1000000;

    if (0)
    std::cout << "CALL: " << itsNumber << " now !!!"  << std::endl;

    signal(SIGCLD, SIG_IGN);
    sigignore(SIGPIPE);    

    int pid = fork();

    if (pid == 0)
    {
      usleep(5000);
      usleep(rand()%500000);

      ClientSocket AsteriskManager(getMainHost(), atoi(getMainPort().c_str()));
      
      response = AsteriskManager.recv();
      
      request =  "Action: Login\r\nUserName: " +
        getManagerUsername() +
        "\r\nSecret: " + getManagerPassword() + "\r\nEvents: off\r\n\r\n";
      AsteriskManager.send(request);
        
      response = AsteriskManager.recv();

      if (itsCallerId == "called")
      {
        itsCallerId = itsNumber;
      }

      request = "Action: Originate\r\n";      

      // For Video 3G
      if ((itsMode == "VIDEO") || (itsMode == "video") || (itsMode == "videovxml") || (itsMode == "videovoximal"))
      {
        if (0)
        std::cout << "Set CHANNEL(transfercapability)=VIDEO" << std::endl;

        request = request + "Variable: CHANNEL(transfercapability)=VIDEO\r\n";
        request = request + "Codecs: h263p,h263,h264,alaw,ulaw\r\n";
      }

      if (itsDialFormat != "")
      {
        char channel[200];
        sprintf(channel, itsDialFormat.c_str(), itsNumber.c_str());
        
        request = request + "Channel: " + channel + "\r\n";        
        //std::cout << "Channel: " << channel << std::endl;
      }
      else
      {
        if (0)
        std::cout << mainHost << ": NO DIALFORMAT DEFINED! (currently set to: " +
          itsDialFormat + ")" << std::endl;
        exit(0);
      }
      
      //std::cout << "VXML URL: " << itsUrl << std::endl;
      
      if ((itsMode == "VXML") || (itsMode == "vxml") || (itsMode == "videovxml"))
      {
        request = request + "Application: Voximal\r\n";
        if (itsUrl.empty() || (itsUrl == "None") || (itsUrl == "none"))
        request = request + "Data: " + itsCallerId + "\r\n";
        else
        request = request + "Data: " + itsUrl + "\r\n";
      }
      else
      if ((itsMode == "VOXIMAL") || (itsMode == "voximal") || (itsMode == "videovoximal"))
      {
        request = request + "Application: Voximal\r\n";
        if (itsUrl.empty() || (itsUrl == "None") || (itsUrl == "none"))
        request = request + "Data: " + itsCallerId + "\r\n";
        else
        request = request + "Data: " + itsUrl + "\r\n";
      }
      else
      if ((itsMode == "VXI") || (itsMode == "vxi"))
      {
        request = request + "Application: Vxml\r\n";
        if (itsUrl.empty() || (itsUrl == "None") || (itsUrl == "none"))
        request = request + "Data: " + itsCallerId + "\r\n";
        else
        request = request + "Data: " + itsUrl + "\r\n";
      }
      else      {
        request = request + "Context: default\r\n";
        request = request + "Exten: " + itsCallerId + "\r\n";
        request = request + "Priority: 1\r\n";
      }
      
      request = request + "Variable: __LEADID=" + itsLeadId + "\r\n";
      request = request + "Variable: __CAMPAIGN=" + itsCampaign + "\r\n";
      if ((itsMode == "VOXIMAL") || (itsMode == "voximal") || (itsMode == "videovoximal"))
      {
      request = request + "Variable: __VOXIMAL_LOCAL=" + itsCallerId + "\r\n";
      request = request + "Variable: __VOXIMAL_REMOTE=" + itsNumber + "\r\n";
      request = request + "Variable: __VOXIMAL_URL2=" + itsUrl + "\r\n";
      request = request + "Variable: __VOXIMAL_ID=" + itsLeadId + "\r\n";
      request = request + "Variable: __VOXIMAL_PARAM=" + itsParam + "\r\n";
      }
      if ((itsMode == "VXI") || (itsMode == "vxi"))
      {
      request = request + "Variable: __VXML_LOCAL=" + itsCallerId + "\r\n";
      request = request + "Variable: __VXML_REMOTE=" + itsNumber + "\r\n";
      request = request + "Variable: __VXML_URL2=" + itsUrl + "\r\n";
      request = request + "Variable: __VXML_ID=" + itsLeadId + "\r\n";
      request = request + "Variable: __VXML_PARAM=" + itsParam + "\r\n";
      }
      if (!itsExtraVars.empty())
      {
        request = request + itsExtraVars;
      }
      request = request + "Account: " + itsCampaign + "\r\n";
      request = request + "Async: true\r\n";
      request = request + "Timeout: " + itos(itsTimeout) + "\r\n";

      //request = request + "Variable: __CALLERIDNAME=~" + itsCampaign + "-" +
      //  itsLeadId + "-" + itsNumber + "~\r\n";

      if (itsCallerId != "")
      {
        std::string privacy="off";

        //Hide callerid, set to hidden
        if (itsCallerId == "hidden")
          privacy = "full";

        if (itsDialFormat.find("voztele", 0) != std::string::npos)
        request = request + "Variable: __SIPADDHEADER0=Remote-Party-ID: <sip:" +
          itsCallerId + "@voztele.com;user=phone>;privacy=" + privacy + ";party=calling\r\n";
        else
        if (itsDialFormat.find(".weepee.", 0) != std::string::npos)
        {
          size_t pos = 0;

          pos=itsDialFormat.find("@");
          
          if (pos != std::string::npos)
          {
            pos++;
            request = request + "Variable: __SIPADDHEADER0=Remote-Party-ID: \""+itsCallerId+"\" <sip:" +
            itsCallerId + "@"+itsDialFormat.substr(pos)+";user=phone>;screen=no;privacy=" + privacy + ";party=calling\r\n";
          }
        }
        else
        if (itsDialFormat.find("bics", 0) != std::string::npos)
        {
          size_t pos = 0;

          pos=itsDialFormat.find("@");

          if (pos != std::string::npos)
          {
            pos++;
            request = request + "Variable: __SIPADDHEADER0=P-Asserted-Identity: \""+itsCallerId+"\" <sip:" +
            itsCallerId + "@"+itsDialFormat.substr(pos)+";user=phone>\r\n";
            request = request + "Variable: __SIPADDHEADER1=Privacy: id\r\n";
          }
        }

        request = request + "CallerID: " + itsCallerId  + "<" + itsCallerId + ">\r\n";

        //request = request + "ActionID: ~" + itsCampaign + "-" + itsLeadId + "-" +
        //  itsNumber  + "~\r\n";
        request = request + "ChannelId: ~" + itsCampaign + "-" + itsLeadId + "-" +
          itsNumber  + "~";
        request = request + uniqueid;
        request = request + +"\r\n";
      }
      else
      {
        //request = request + "CallerID: ~" + itsCampaign + "-" + itsLeadId + "-" +
        //  itsNumber + "~<" + itsCallerId + ">\r\n";
        //request = request + "ActionID: ~" + itsCampaign + "-" + itsLeadId + "-" +
        //  itsNumber + "~\r\n";
        request = request + "ChannelId: ~" + itsCampaign + "-" + itsLeadId + "-" +
          itsNumber  + "~";
        request = request + uniqueid;
        request = request + +"\r\n";
      }

      request = request + "\r\n";

      //std::cout << "!!! DBG " << request << std::endl;
      
      AsteriskManager.send(request);

      response = AsteriskManager.recv();
      
      request = "Action: Logoff\r\n\r\n";
      AsteriskManager.send(request);
      
      response = AsteriskManager.recv();

      if (doColor)
      {
        if (0)
        std::cout << mainHost << neon << ": " + itsCampaign + " - " +
          itsNumber + " - " + itsLeadId + " - " +
          "" << norm << std::endl;
      }
      else
      {
        if (0)
        std::cout << mainHost << ": " + itsCampaign + " - " + itsNumber +
          " - " + itsLeadId + " - " + "" << std::endl;
      }

      //usleep(10000000);
			usleep(50000);      

      exit(0);

    }

    if (pid == -1)
    {

      throw xForkError();

    }

  }

  void DoRequest()
  {
    std::string response, request;
    char uniqueid[20];

    sprintf(uniqueid, "%li.%d", time(NULL), counter++);

    const std::string mainHost = host;

    timeval tv;
    gettimeofday(&tv, NULL);
    itsTime = tv.tv_sec % 1000000;

    if (1)
    std::cout << "REQUEST: " << itsNumber << " now !!!"  << std::endl;

    signal(SIGCLD, SIG_IGN);
    sigignore(SIGPIPE);

    int pid = fork();

    if (pid == 0)
    {
      usleep(5000);
      usleep(rand()%500000);

      ClientSocket AsteriskManager(getMainHost(), atoi(getMainPort().c_str()));

      response = AsteriskManager.recv();

      request =  "Action: Login\r\nUserName: " +
        getManagerUsername() +
        "\r\nSecret: " + getManagerPassword() + "\r\nEvents: off\r\n\r\n";
      AsteriskManager.send(request);

      response = AsteriskManager.recv();

      if (itsCallerId == "called")
      {
        itsCallerId = itsNumber;
      }

      request = "Action: Command\r\n";

      std::cout << "VXML URL: " << itsUrl << std::endl;
      std::cout << "VXML MODE: " << itsMode << std::endl;

      if ((itsMode == "VOXIMAL") || (itsMode == "voximal") || (itsMode == "videovoximal"))
      {
        request = request + "Command: voximal function VOXIMAL(url,";
        request = request + itsUrl + ",";
        request = request + cause + ":" + reason +",";
        request = request + itsParam + ",";
        request = request + itsCallerId + ",";
        request = request + itsNumber + ",";
        request = request + "mark";
        request = request + ")\r\n";

        /*
        if (itsUrl.empty() || (itsUrl == "None") || (itsUrl == "none"))
        request = request + "Data: " + itsCallerId + "\r\n";
        else
        request = request + "Data: " + itsUrl + "\r\n";
        */
      }

      request = request + "\r\n";

      //std::cout << "!!! DBG " << request << std::endl;

      AsteriskManager.send(request);

      response = AsteriskManager.recv();

      request = "Action: Logoff\r\n\r\n";
      AsteriskManager.send(request);

      response = AsteriskManager.recv();

      if (doColor)
      {
        if (0)
        std::cout << mainHost << neon << ": " + itsCampaign + " - " +
          itsNumber + " - " + itsLeadId + " - " +
          "" << norm << std::endl;
      }
      else
      {
        if (0)
        std::cout << mainHost << ": " + itsCampaign + " - " + itsNumber +
          " - " + itsLeadId + " - " + "" << std::endl;
      }

			usleep(50000);

      exit(0);

    }

    if (pid == -1)
    {

      throw xForkError();

    }

  }

  ~Call()
  {
  }

private:

  std::string itsCampaign, itsLeadId, itsNumber, itsUrl, itsParam, 
    itsCallerId,
    itsUniqueId, 
    itsDialFormat, 
    itsMode,
    itsExtraVars;
  unsigned long int itsTime;
  unsigned short int itsTimeout;
  bool called, answered, unanswered, hangup;
  std::string cause;
  std::string reason;
  std::string host;
};

class CallCache
{

public:

  CallCache()
  {
  }
  ~CallCache()
  {
  }

  void push_back(const Call & TheCall)
  {

    itsCalls.push_back(TheCall);

  }

  void AddCall(
    const std::string & campaign,
    const std::string & leadid,
    const std::string & phone,
    const std::string & url,
    const std::string & param,
    
    const unsigned short int &itsTimeout,
    const std::string & dialformat,
    const std::string & callerid,
    
    const std::string & transfer,
     
    const std::string & extravars
    )
  {

    Call TheCall(campaign, leadid, phone, url, param,
      itsTimeout, dialformat, callerid, 
      transfer,
      extravars );
    itsCalls.push_back(TheCall);

  }
  
  void SetUniqueid(const std::string & campaign, const std::string & leadid, const std::string & uniqueid)
  {
    for (unsigned int i = 0; i < itsCalls.size(); i++)
    {

      if (itsCalls.at(i).GetCampaign() == campaign &&
        itsCalls.at(i).GetLeadId() == leadid)

      {
        itsCalls.at(i).SetUniqueId(uniqueid);
      }
    }
  }  

  const std::string & GetCampaign(const std::string & uniqueid)
  {
    for (unsigned int i = 0; i < itsCalls.size(); i++)
    {

      if (itsCalls.at(i).GetUniqueId() == uniqueid)
      {
        return itsCalls.at(i).GetCampaign();
      }
    }

    return itsEmpty;
  }

  const std::string & GetLeadId(const std::string & uniqueid)
  {
    for (unsigned int i = 0; i < itsCalls.size(); i++)
    {
      if (itsCalls.at(i).GetUniqueId() == uniqueid)
      {
        return itsCalls.at(i).GetLeadId();
      }
    }
    
    return itsEmpty;
  }
  
  void SetAnswered(const std::string & campaign, const std::string & leadid)
  {
    for (unsigned int i = 0; i < itsCalls.size(); i++)
    {

      if (itsCalls.at(i).GetCampaign() == campaign &&
        itsCalls.at(i).GetLeadId() == leadid)
      {  
        itsCalls.at(i).SetAnswered();
      }
    }
  }

  void SetUnanswered(const std::string & campaign, const std::string & leadid, const std::string & reason)
  {
    for (unsigned int i = 0; i < itsCalls.size(); i++)
    {

      if (itsCalls.at(i).GetCampaign() == campaign &&
        itsCalls.at(i).GetLeadId() == leadid)
      {
        if (itsCalls.at(i).HasBeenHangup())
        {
          itsCalls.at(i).SetUnanswered(reason);
          if (!reason.empty())
          itsCalls.at(i).DoRequest();
          itsCalls.erase(itsCalls.begin()+i);
        }
        else
        {
          itsCalls.at(i).SetUnanswered(reason);
        }
      }
    }
  }

  void SetHangup(const std::string & campaign, const std::string & leadid, const std::string & cause)
  {
    for (unsigned int i = 0; i < itsCalls.size(); i++)
    {

      if (itsCalls.at(i).GetCampaign() == campaign &&
        itsCalls.at(i).GetLeadId() == leadid)
      {
        if (itsCalls.at(i).HasBeenAnswered())
        {
          itsCalls.at(i).SetHangup(cause);
          itsCalls.erase(itsCalls.begin()+i);
        }
        else
        if (itsCalls.at(i).HasBeenUnanswered())
        {
          itsCalls.at(i).SetHangup(cause);
          if (!cause.empty())
          itsCalls.at(i).DoRequest();
          itsCalls.erase(itsCalls.begin()+i);
        }
        else
        {
          itsCalls.at(i).SetHangup(cause);
        }
      }
    }
  }

  int IsCalling(const std::string & campaign, const std::string & number)
  {

    for (unsigned int i = 0; i < itsCalls.size(); i++)
    {

      if (itsCalls.at(i).GetCampaign() == campaign &&
        itsCalls.at(i).GetNumber() == number)
      {
        return (1);
      }
    }

    return (0);
  }

  unsigned int LinesDialing(const std::string & campaign)
  {

    workingCampaign = campaign;

    timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long int cur = tv.tv_sec % 1000000;

    /*
    for (unsigned long int cur = tv.tv_sec % 1000000; 
     itsCalls.size()
     && (itsCalls.front().GetTimeout() != 0)     
     && (cur - itsCalls.front().GetTime() > itsCalls.front().GetTimeout() / 1000);
     )
    {
      itsCalls.pop_front();
    }
    */
    
    for (unsigned int i = 0; i < itsCalls.size(); i++)
    {
     if ((itsCalls.at(i).GetTimeout() != 0)
        && (itsCalls.at(i).GetTime() != 0)
        && (abs(int(cur - itsCalls.at(i).GetTime())) > ((itsCalls.at(i).GetTimeout() / 1000)+10)))
      {
         std::cerr << "CALL : Remove expired call = " << itsCalls.at(i).GetCampaign() << " " << itsCalls.at(i).
          GetNumber() << "  " << itsCalls.at(i).GetLeadId() << "  " << itsCalls.at(i).GetUniqueId() << "! " << std::endl;

        itsCalls.at(i).DoRequest();
        itsCalls.erase(itsCalls.begin()+i);
        i--;
      }
    }

    //return std::count_if(itsCalls.begin(), itsCalls.end(),
    //  countCallsForCampaign);
    return itsCalls.size();

  }

  void CallAll(const std::string & mainHost)
  {

    for (unsigned int i = 0; i < itsCalls.size(); i++)
    {
      if (!itsCalls.at(i).HasBeenCalled())
      {
        try
        {
          itsCalls.at(i).DoCall(mainHost);

	}
        catch(const xConnectionError & e)
        {
          std::cerr << "Exception: Unable to connect to " << e.
            GetHost() << "! Disabling host." << std::endl;
        }
      }
    }

  }

  void DumpAll()
  {
    timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long int cur = tv.tv_sec % 1000000;

    for (unsigned int i = 0; i < itsCalls.size(); i++)
    {
      if (itsCalls.at(i).HasBeenHangup())
      {
        std::cerr << "CALL : Hangup " << itsCalls.at(i).GetCampaign() << " " << itsCalls.at(i).
          GetNumber() << "  " << itsCalls.at(i).GetLeadId() << "  " << itsCalls.at(i).GetUniqueId() << "! " << std::endl;
      }
      else
      if (itsCalls.at(i).HasBeenAnswered())
      {
        std::cerr << "CALL : Answered " << itsCalls.at(i).GetCampaign() << " " << itsCalls.at(i).
          GetNumber() << "  " << itsCalls.at(i).GetLeadId() << "  " << itsCalls.at(i).GetLeadId() << "! " << itsCalls.at(i).GetTimeout() << std::endl;
      }
      else 
      if (itsCalls.at(i).HasBeenCalled())
      {
        std::cerr << "CALL : Called " << itsCalls.at(i).GetCampaign() << " " << itsCalls.at(i).
          GetNumber() << "  " << itsCalls.at(i).GetLeadId() << "  " << itsCalls.at(i).GetLeadId() << "! " << std::endl;
      }
      else
      {
        std::cerr << "CALL : Other " << itsCalls.at(i).GetCampaign() << " " << itsCalls.at(i).
          GetNumber() << "  " << itsCalls.at(i).GetLeadId() << "  " << itsCalls.at(i).GetLeadId() << "! " << std::endl;
      }

      if ((itsCalls.at(i).GetTimeout() != 0)
        && (itsCalls.at(i).GetTime() != 0))
      {
        std::cerr << "CALL : Timeout = " << (abs(int(cur - itsCalls.at(i).GetTime())) ) << std::endl;
      }

    }
  }

  int countCalls()
  {
    return itsCalls.size();
  }

private:

  std::deque < Call > itsCalls;
  std::string itsEmpty;  

};

bool countCallsForCampaign(const Call & TheCall)
{

  if (TheCall.GetCampaign() == workingCampaign &&
    TheCall.HasBeenAnswered() == false)
  {

    return true;

  }
  else
  {

    return false;

  }

}

#endif
