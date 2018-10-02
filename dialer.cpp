// $Id: dialer.cpp,v 1.86 2017/08/24 07:44:31 borja.sixto Exp $
/*
 * GnuDialer - Complete, free predictive dialer
 *
 * Complete, free predictive dialer for contact centers.
 *
 * Copyright (C) 2006, GnuDialer Project
 *
 * Heath Schultz <heath1444@yahoo.com>
 * Richard Lyman <richard@dynx.net>
 * Jamerson Medley <nixtux2003@yahoo.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License.
 *
 */

#include <iomanip>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <queue>
#include <algorithm>
#include <sys/time.h>
#include <mysql.h>
#include "options.h"
#include "debug.h"
#include "queue.h"
#include "Socket.h"
#include "exceptions.h"
#include "call.h"
#include "log.h"
#include "settings.h"
#include "color.h"


#define DIALERVERSION 1.3
#define CRASH do { fprintf(stderr, "!! Forcing immediate crash a-la abort !!\n"); *((int *)0) = 0; } while(0)

#define SEPERATOR ","

const bool debugPos = false;
#define HERE(x)       if (debugPos) { std::cout << "Here:" << #x << std::endl; }

const bool debugCampaignSettings = false;

bool debug = false;
//bool debug = true;
bool clip = false;

bool safeMode = false;
bool daemonMode = false;
bool daemonLock = true;


//                -8 == Burned (We give up)      ---o
//                -7 == Disconnected Number         | -- Don't get called back
//                -6 == Fax Machine              ---o
//                -5 == Network Congestion ---o
//                -4 == Busy                  |
//                -3 == Ringing               | -- Get called back ---o
//                -2 == No Answer             |                       |
//                -1 == Undefined*         ---o                       |
//                                                                    |
//                1 == Not Yet Called** -- Get called back -----------|
//                                                                    |
//                0 == Callback  ---o                                 |
//                2 == No Answer    |                                 |
//                3 == Voicemail    | -- Get called back -------------o
//                4 == Busy         |
//                5 == Fast Busy ---o
//                6 == Fax Machine           ---o
//                7 == Disconnected Number      |
//                8 == Do Not Call              | -- Don't count as contacts
//                9 == Invalid Lead             |
//                10 == No Sale***           ---o
//                11 == No Sale***    ---o These both...
//                12 == Sale          ---o count as contacts

struct dispotype
{
  int disponum;
  const char *shortname;
  const char *longname;
};

struct dispotype dispos[] = {
  {-7, "M_Disc", "Machine - Disconnect"},
  {-6, "M_FaxM", "Machine - Fax Machine"},
  {-5, "M_Cong", "Machine - Congestion"},
  {-4, "M_Busy", "Machine - Busy"},
  {-3, "M_AnsM", "Machine - Answering Machine"},
  {-2, "M_NoAns", "Machine - No Answer"},
  {-1, "M_Resv", "Machine - Reserved"},
  {0, "Callback", "Agent - Callback"},
  {1, "Fresh", "Fresh Record"},
  {2, "NoAns", "Agent - No Answer"},
  {3, "AnsMach", "Agent - Answering Machine"},
  {4, "Busy", "Agent - Busy"},
  {5, "FBusy", "Agent - Fast Busy"},
  {6, "FaxMach", "Agent - Fax Machine"},
  {7, "Disco", "Agent - Disconnect"},
  {8, "DNC", "Agent - Do Not Call"},
  {9, "Invalid", "Agent - Invalid"},
  {10, "Other", "Agent - Other"},
  {11, "NoSale", "Agent - No Sale"},
  {12, "Sale", "Agent - Sale"},
};

struct dispotype reasons[] = {
  {0, "0", "No such extension or number"},
  {1, "1", "Other end has hungup (reject)"},
  {2, "2", "Local ring"},
  {3, "3", "Remote end is ringing"},
  {4, "4", "Remote end has answered"},
  {5, "5", "Remote end is busy"},
  {6, "6", "Make it go off hook"},
  {7, "7", "Line is off hook"},
  {8, "8", "Congestion (circuits busy)"},
};

static const char *dispo2long(int dispo)
{
  unsigned int x;
  for (x = 0; x < sizeof(dispos) / sizeof(dispos[0]); x++)
    if (dispos[x].disponum == dispo)
      return dispos[x].longname;
  return "Unknown";
}

static const char *reason2long(int reason)
{
  unsigned int x;
  for (x = 0; x < sizeof(reasons) / sizeof(reasons[0]); x++)
    if (reasons[x].disponum == reason)
      return reasons[x].longname;
  return "Unknown";
}

const int &selectLessorOf(const int &lhs, const int &rhs)
{
  if (lhs < rhs)
  {
    return lhs;
  }
  else
  {
    return rhs;
  }
}

long int stoi(const std::string & theString)
{
  return atoi(theString.c_str());
}

void sig_handler(int sig)
{

  if (sig == SIGSEGV)
  {
    if (safeMode == true)
    {
      if (doColorize)
      {
        std::
          cout << fg_light_red << "FATAL ERROR! Segmentation Fault!" << normal
          << std::endl;
      }
      else
      {
        std::cout << "FATAL ERROR! Segmentation Fault!" << std::endl;
      }
      std::
        cout <<
        "You are running in safe mode, so Dialer will attempt to restart itself!"
        << std::endl << std::endl;
        
      if (daemonMode)
      std::system("sleep 1 && /opt/dialer/dialer --safe --start &");
      else
      std::system("sleep 1 && /opt/dialer/dialer --safe &");
    }
    else
    {
      CRASH;
      if (doColorize)
      {
        std::
          cout << fg_light_red << "FATAL ERROR! Segmentation Fault!" << normal
          << std::endl;
      }
      else
      {
        std::cout << "FATAL ERROR! Segmentation Fault!" << std::endl;
      }
      std::cout << "Please report this to our team." << std::endl;
      std::
        cout <<
        "Please also be advised that you can start Dialer in \"safe\" mode which will"
        << std::endl;
      std::
        cout << "automatically restart Dialer if you receive a fatal error." <<
        std::endl;
      std::cout << "Type: \"Dialer --help\" for more information." << std::
        endl << std::endl;
    }
    
    DBG_CLOSE();
    exit(0);
  }

  if (sig == SIGCHLD)
  {
    int stat;
    while (waitpid(-1, &stat, WNOHANG) > 0) ;
  }
  return;
}

/*
void doRedirect(const std::string & channel,
  const std::string & agent,
  const std::string & campaign,
  const std::string & leadid,
  const std::string & managerUser,
  const std::string & managerPass, const bool & doChangeCallerId)
{
  std::string response;
  if (atoi(agent.c_str()) || (agent == "699" && doChangeCallerId))
  {
    if (doColorize)
    {
      std::
        cout << campaign << fg_magenta << ": Transferring - " << channel <<
        " to Agent: " << agent << normal << std::endl;
    }
    else
    {
      std::
        cout << campaign << ": Transferring - " << channel << " to Agent: " <<
        agent << std::endl;
    }
    writeDialerLog(campaign + ": Transferring - " + channel + " to Agent: " +
      agent + "");
    ClientSocket AsteriskRedir(getMainHost(), atoi(getMainPort().c_str()));
    AsteriskRedir >> response;
    AsteriskRedir << "Action: Login\r\nUserName: " + managerUser +
      "\r\nSecret: " + managerPass + "\r\nEvents:off\r\n\r\n";
    AsteriskRedir >> response;
    AsteriskRedir << "Action: Redirect\r\n";
    AsteriskRedir << "Channel: " + channel + "\r\n";
    AsteriskRedir << "Exten: " + agent + "\r\n";

    if (!doChangeCallerId)
    {
      AsteriskRedir << "Context: agent\r\n";
    }
    else
    {
      AsteriskRedir << "Context: closer\r\n";
    }

    AsteriskRedir << "Priority: 1\r\n\r\n";
    AsteriskRedir >> response;
    AsteriskRedir << "Action: Logoff\r\n\r\n";
    AsteriskRedir >> response;
    usleep(10000000);
  }
}

void doSetVarTransfer(const std::string & channel,
  const std::string & agent,
  const std::string & managerUser, const std::string & managerPass)
{
  std::string response;
  std::cout << "doSetVarTransfer - " << channel << std::endl;
  //writeDialerLog(campaign + ": doSetVarTransfer - " + channel + " from Agent: " + agent + "");
  ClientSocket AsteriskRedir(getMainHost(), atoi(getMainPort().c_str()));
  AsteriskRedir >> response;
  AsteriskRedir << "Action: Login\r\nUserName: " + managerUser +
    "\r\nSecret: " + managerPass + "\r\nEvents:off\r\n\r\n";
  AsteriskRedir >> response;
  //action: SetVar
  //channel: Zap/49-1
  //variable: ISTRANSFER=TRANSFER
  AsteriskRedir << "Action: SetVar\r\n";
  AsteriskRedir << "Channel: " + channel + "\r\n";
  AsteriskRedir << "Variable: ISTRANSFER=TRANSFER\r\n\r\n";
  AsteriskRedir >> response;
  AsteriskRedir << "Action: Logoff\r\n\r\n";
  AsteriskRedir >> response;
  if (response == "Success")
  {
    std::cout << "doSetVarTransfer - SUCCESSFUL " << channel << std::endl;
  }
  AsteriskRedir >> response;
}

void doHangupCall(const std::string & channel,
  const std::string & agent,
  const std::string & managerUser, const std::string & managerPass)
{
  std::string response;
  std::cout << "doHangupCall - " << channel << std::endl;
  writeDialerLog("doHangupCall - " + channel + " from Agent: " + agent + "");
  ClientSocket AsteriskRedir(getMainHost(), atoi(getMainPort().c_str()));
  AsteriskRedir >> response;
  AsteriskRedir << "Action: Login\r\nUserName: " + managerUser +
    "\r\nSecret: " + managerPass + "\r\nEvents:off\r\n\r\n";
  AsteriskRedir >> response;
  AsteriskRedir << "Action: Hangup\r\n";
  AsteriskRedir << "Channel: " + channel + "\r\n\r\n";
  AsteriskRedir >> response;
  AsteriskRedir << "Action: Logoff\r\n\r\n";
  AsteriskRedir >> response;
}

void doMonitorStart(const std::string & channel,
  const std::string & agent,
  const std::string & campaign,
  const std::string & leadid,
  const std::string & managerUser, const std::string & managerPass)
{
  std::string response;
  std::cout << "doMonitorStart - " << channel << std::endl;
  writeDialerLog("doMonitorStart - " + channel + " from Agent: " + agent + "");
  ClientSocket AsteriskRedir(getMainHost(), atoi(getMainPort().c_str()));
  AsteriskRedir >> response;
  AsteriskRedir << "Action: Login\r\nUserName: " + managerUser +
    "\r\nSecret: " + managerPass + "\r\nEvents:off\r\n\r\n";
  AsteriskRedir >> response;
  AsteriskRedir << "Action: Monitor\r\n";
  AsteriskRedir << "Channel: " + channel + "\r\n";
  AsteriskRedir << "File: " + agent + "-" + campaign + "-" + leadid + "\r\n";
  AsteriskRedir << "Mix: 1\r\n\r\n";
  AsteriskRedir >> response;
  AsteriskRedir << "Action: Logoff\r\n\r\n";
  AsteriskRedir >> response;
}

void doMonitorStop(const std::string & channel,
  const std::string & agent,
  const std::string & campaign,
  const std::string & leadid,
  const std::string & managerUser, const std::string & managerPass)
{
  std::string response;
  std::cout << "doMonitorStop - " << channel << std::endl;
  writeDialerLog("doMonitorStop - " + channel + " from Agent: " + agent + "");
  ClientSocket AsteriskRedir(getMainHost(), atoi(getMainPort().c_str()));
  AsteriskRedir >> response;
  AsteriskRedir << "Action: Login\r\nUserName: " + managerUser +
    "\r\nSecret: " + managerPass + "\r\nEvents:off\r\n\r\n";
  AsteriskRedir >> response;
  AsteriskRedir << "Action: StopMonitor\r\n";
  AsteriskRedir << "Channel: " + channel + "\r\n\r\n";
  AsteriskRedir >> response;
  AsteriskRedir << "Action: Logoff\r\n\r\n";
  AsteriskRedir >> response;
}
*/

int main(int argc, char **argv)
{
  //usleep(100000);

  std::string commandAction;
  std::string commandCampaign;
  std::string commandParameter;
  std::string commandParameter2;
  char *commandConfigurationFile;


  //set default console color to white on black
  if (doColorize)
  {
    std::cout << fg_light_white << std::endl;
  }

  commandCampaign = "default";
  commandConfigurationFile = NULL;

  for (int i = 1; i < argc; ++i)
  {
    const std::string arg(argv[i]);

    if (arg == "stop" || arg == "-stop" || arg == "--stop")
    {
      writeDialerLog("Dialer: Stopped");
      if (doColorize)
      {
        std::cout << fg_light_green << "Dialer: Stopped" << normal << std::endl;
      }
      else
      {
        std::cout << "Dialer: Stopped" << std::endl;
      }
      std::system("killall dialer");
      exit(0);
    }

    if (arg == "start" || arg == "-start" || arg == "--start")
    {
      writeDialerLog("Dialer: Starting");
      if (doColorize)
      {
        std::cout << fg_light_green << "Dialer: Starting" << normal << std::endl;
      }
      else
      {
        std::cout << "Dialer: Starting" << std::endl;
      }
      
      daemonMode = true;
    }
    
    if (arg == "-h" || arg == "--help" || arg == "-help")
    {
      std::cout << std::endl << std::endl;
      std::cout << "Usage: Dialer <options>" << std::endl << std::endl;
      std::cout <<
        "\t--fileconfiguration            Set the file configuration." <<
        std::endl;
      std::cout <<
        "\t--safe                         Starts in \"safe\" mode (auto-restart)."
        << std::endl;
      std::cout <<
        "\t--nolock                       Don't generate the /tmp/dialer.lock file."
        << std::endl;
      std::cout << 
        "\t--help|-h                      This help screen." << std::
        endl;
      std::cout <<
        "\t--version|-V                   Prints dialer's version." <<
        std::endl;
      std::cout <<
        "\t--stop                         Unconditionally stop the dialer." <<
        std::endl;
      std::cout << 
        "\t--start                        Run as a daemon." <<
        std::endl;
      std::cout << 
        "\t--action                       Execute a command." <<
        std::endl;
      std::cout << 
        "\t   createdatabase              Create the database." <<
        std::endl;
      std::cout << 
        "\t   dropdatabase                Drop the database." <<
        std::endl;
      std::cout << 
        "\t   create                      Create the campaign table." <<
        std::endl;
      std::cout <<
        "\t   createextended              Create the campaign table (with more history)." <<
        std::endl;
      std::cout <<
        "\t   drop                        Drop the campaign table." <<
        std::endl;
      std::cout << 
        "\t   truncate                    Remove all the phones." <<
        std::endl;
      std::cout << 
        "\t   import                      Use the text file to fill the campaign." <<
        std::endl;
      std::cout << 
        "\t   export                      Dump the campaign in text file." <<
        std::endl;
      std::cout << 
        "\t   insert                      Insert a phone in the campaign." <<
        std::endl;
      std::cout << 
        "\t   delete                      Delete a phone in the campaign." <<
        std::endl;
      std::cout << 
        "\t   count                       Phones in the campaign." <<
        std::endl;
      std::cout << 
        "\t   dump                        Dump the campaign in csv." <<
        std::endl;
      std::cout << 
        "\t   fulldump                    Dump the campaign in csv with header." <<
        std::endl;
      std::cout << 
        "\t   alldump                     Dump the campaign in csv with dispositions." <<
        std::endl;
      std::cout << 
        "\t   simpledump                  Dump the campaign in csv, only number and parameters." <<
        std::endl;
      std::cout << 
        "\t   statistics                  Statistics od the campaign." <<
        std::endl;
      std::cout << 
        "\t--campaign                     Name of the campaign for the command." <<
        std::endl;
      std::cout << 
        "\t--paramater                    Parameter for the command." <<
        std::endl;

      std::cout << std::endl << std::endl;
      return 0;
    }
    if (arg == "-V" || arg == "--version" || arg == "-version")
    {
      std::cout << "Asterisk Dialer version : "<<DIALERVERSION<<" (CVS:$Revision: 1.86 $) " << std::endl;
      return 0;
    }
    if (arg == "-s" || arg == "--safe" || arg == "-safe")
    {
      std::cout << "Dialer: SAFE MODE enabled!" << std::endl;
      safeMode = true;
    }
    if (arg == "-d" || arg == "--debug" || arg == "-debug")
    {
      std::cout << "Dialer: DEBUG enabled!" << std::endl;
      debug = true;
    }
    if (arg == "-a" || arg == "--action" || arg == "-action")
    {
      daemonMode = false;
      daemonLock = false;    
      if (i+1 < argc)
      {
        i++;
        commandAction = argv[i];
      }
      else
      {
        commandAction = "none";
      }
    }
    if (arg == "-c" || arg == "--campaign" || arg == "-campaign")
    {
      if (i+1 < argc)
      {
        i++;
        commandCampaign = argv[i];
      }
    }
    if (arg == "-p" ||
      arg == "--parameter" ||
      arg == "-parameter" || arg == "--phone" || arg == "-phone")
    {
      if (i+1 < argc)
      {
        i++;
        commandParameter = argv[i];
        commandParameter2 = "";
        if ((i+1) < argc)        
        {
          char* first = argv[i+1];
                    
          // Case --phone 6566665 hello
          if (first[0] != '-')
          {  
            i++;        
            commandParameter2 = argv[i];
          }
        }
      }
    }
    if (arg == "-n" || arg == "--nolock" || arg == "-nolock")
    {
      daemonLock = false;
    }

    if (arg == "-f" || arg == "--fileconfiguration" || arg == "-fileconfiguration")
    {
      if (i+1 < argc)
      {
        i++;
        commandConfigurationFile = argv[i];
      }
    }

    if (arg == "-k" || arg == "--clip" || arg == "-clip")
    {
      std::cout << "Dialer: CLIP enabled!" << std::endl;
      clip = true;
    }

  }

  if (daemonMode)
  {
    std::cout << "Dialer: Deamon mode." << std::endl;
    
    int daemonizer = 0;
    daemonizer = fork();
    if (daemonizer < 0)
    {
      std::
        cout << "Dialer: Error setting up daemon process... Aborting." << std::
        endl;
      exit(1);
    }
    if (daemonizer > 0)
    {
      exit(0);
    }
  }
  
  if (safeMode)
  {
    std::cout << "Dialer: Set signal handler." << std::endl;
    signal(SIGCHLD, sig_handler);
    signal(SIGSEGV, sig_handler);
  }

  if (daemonLock)
  {
    //umask(017);
    //chdir("/tmp");
    int lfp = open("/tmp/dialer.lock", O_RDWR | O_CREAT, 0664);
    if (lfp < 0)
    {
      std::cout << "Dialer: Error opening lock file!" << std::endl;
      exit(1);
    }
    if (lockf(lfp, F_TLOCK, 0) < 0)
    {
      std::cout << "Dialer: process already running!" << std::endl;
      exit(0);
    }
    char str[80];
    sprintf(str, "%d\n", getpid());
    write(lfp, str, strlen(str));
    
    close(lfp);
  }

  bool gDebug, gLog;
  addGlobalSettings("general", commandConfigurationFile);

  Queue TheQueueGlobals(commandConfigurationFile);
  TheQueueGlobals.ParseQueue("general");

  try
  {
    std::string value;

    gDebug = TheQueueGlobals.GetSetting("debug").GetBool();
    gLog = TheQueueGlobals.GetSetting("log").GetBool();
    value = TheQueueGlobals.GetSetting("mysqluser").Get();
    setMySqlUser(value);
    value = TheQueueGlobals.GetSetting("mysqlpassword").Get();
    setMySqlPassword(value);
    value = TheQueueGlobals.GetSetting("mysqlhost").Get();
    setMySqlHost(value);
    value = TheQueueGlobals.GetSetting("mysqldatabase").Get();
    setMySqlDbName(value);
    value = TheQueueGlobals.GetSetting("asteriskuser").Get();
    setAsteriskUser(value);
    value = TheQueueGlobals.GetSetting("asteriskpassword").Get();
    setAsteriskPassword(value);
    value = TheQueueGlobals.GetSetting("asteriskhost").Get();
    setAsteriskHost(value);
    value = TheQueueGlobals.GetSetting("asteriskport").Get();
    setAsteriskPort(value);
  }
  catch(xLoopEnd e)
  {
    std::cout << "Error reading configuration..." << std::endl;
    std::cout << e.what();
    std::cout << std::endl << std::endl;
    return (1);
  }

  if (gDebug)
  {
    DBG_OPEN("Dialer");
    DBG_level(DBG_LEVEL_ALL, 1);
  }

  DBG_mode(DBG_MODE_FLUSH, 1);

  std::string mainHost = getMainHost();

  DBG_TRACE(DBG_MYSQL, (0, "Starting MySQL connexion"));

  HERE(MYSQLBEGIN) MYSQL *mysql = NULL;
  MYSQL_RES *result;
  MYSQL_ROW row;
  unsigned int rows;
  HERE(MYSQLEND) mysql = mysql_init(NULL);
  
  //Important for import for CSV file inserts
  mysql_options( mysql, MYSQL_OPT_LOCAL_INFILE,0);

  if (mysql == NULL)
  {
    std::cerr << "Dialer: MySql init failed!" << std::endl;
    DBG_TRACE(DBG_MYSQL, (0, "MySQL initialization failed"));
    return 1;
  }
  if (!mysql_real_connect(mysql, getMySqlHost().c_str(),
      getMySqlUser().c_str(),
//      getMySqlPass().c_str(), getDbName().c_str(), 3306, NULL, 0))
      getMySqlPass().c_str(), NULL, 3306, NULL, 0))
  {
    std::cerr << "Dialer: MySql connection failed!" << std::endl;
    DBG_TRACE(DBG_MYSQL, (0, "MySQL connection failed"));
    return 1;
  }

  std::string response, request, previous, block, queue, mode, calltoday, blacklist, multiplecalls,
    query, callerid, url, channel;

  std::string tempagent, dialformat;
  std::string chanvar1, chanvar2, sipheader1, sipheader2;
  std::string extravars, filter;
  //  bool debug;
  //  bool log;
  int attempts = 2, skip;
  std::string orderby;
  std::string managerUser = getManagerUsername();
  std::string managerPass = getManagerPassword();
  std::stringstream BlockStream;
  unsigned int maxlines = 0, maxcaps = 10, timeout = 0, linesdialing = 0, linestodial =
    0, counter = 0, attemptsdelay = 600;
  bool ringonly = false;
  size_t  pos = 0, end = 0, pos2 = 0, end2 = 0;
  unsigned long int calls = 0;
  int timestart=0, timestop=24;
  
  previous = "";

  //std::cout << "commandAction " << commandAction << "!" << std::endl;

  if (commandAction != "")
  {
    if (commandAction == "createdatabase")
    {
      query = "CREATE DATABASE `" + commandParameter + "`";

      if (mysql_query(mysql, query.c_str()) != 0)
      {
        std::cout << "Error creating " << commandParameter << "!" << std::endl;
      }
      else
      {
        std::cout << "Action: CREATEDATABASE OK" << std::endl;
      }
    }

    if (commandAction == "dropdatabase")
    {
      query = "DROP DATABASE `" + commandParameter + "`";

      if (mysql_query(mysql, query.c_str()) != 0)
      {
        std::cout << "Error droping " << commandParameter << "!" << std::endl;
      }
      else
      {
        std::cout << "Action: CREATEDATABASE OK" << std::endl;
      }
    }

    if (mysql_select_db(mysql, getDbName().c_str()))
    {
      std::cerr << "Dialer: MySql cannot use database " << getDbName().c_str() << "!" << std::endl;
      DBG_TRACE(DBG_MYSQL, (0, "MySQL cannot use database %s", getDbName().c_str()));
      return 1;
    } 

    if (commandAction == "create")
    {
      query = "CREATE TABLE `" + commandCampaign + "` (";
      query += "`id` int(8) NOT NULL auto_increment,";
      query += "`phone` varchar(64) default NULL,";
      query += "`datetime` timestamp NOT NULL default '0000-00-00 00:00:00',";
      query += "`attempts` int(8) default 0,";
      query += "`pickups` int(8) default 0,";
      query += "`disposition` int(8) default 0,";
      query += "`reason` varchar(255) default 'Unknow',";
      query += "`duration` int(8) default 0,";
      query += "`param` varchar(255) default '',";
      query += "`result` varchar(255) default '',";
      query += "`cause` varchar(255) default '',";
      query += "`lastupdated` timestamp NOT NULL default '0000-00-00 00:00:00' on update CURRENT_TIMESTAMP,";
      query += "PRIMARY KEY  (`id`),";
      query += "KEY `datetime` (`datetime`),";
      query += "KEY `lastupdated` (`lastupdated`)";
      query += " )";

      if (mysql_query(mysql, query.c_str()) != 0)
      {
        std::cout << "Error creating " << commandCampaign << "!" << std::endl;
        std::cout << "Error creating " << query << "!" << std::endl;
      }
      else
      {
        std::cout << "Action: CREATE OK" << std::endl;
      }
    }

    if (commandAction == "createextended")
    {
      query = "CREATE TABLE `" + commandCampaign + "` (";
      query += "`id` int(8) NOT NULL auto_increment,";
      query += "`phone` varchar(64) default NULL,";
      query += "`datetime1` timestamp NOT NULL default '0000-00-00 00:00:00',";
      query += "`datetime2` timestamp NOT NULL default '0000-00-00 00:00:00',";
      query += "`datetime` timestamp NOT NULL default '0000-00-00 00:00:00',";
      query += "`attempts` int(8) default 0,";
      query += "`pickups` int(8) default 0,";
      query += "`disposition` int(8) default 0,";
      query += "`reason1` varchar(255) default '',";
      query += "`reason2` varchar(255) default '',";
      query += "`reason` varchar(255) default '',";
      query += "`duration` int(8) default 0,";
      query += "`param` varchar(255) default '',";
      query += "`result` varchar(255) default '',";
      query += "`cause1` varchar(255) default '',";
      query += "`cause2` varchar(255) default '',";
      query += "`cause` varchar(255) default '',";
      query += "`lastupdated` timestamp NOT NULL default '0000-00-00 00:00:00' on update CURRENT_TIMESTAMP,";
      query += "PRIMARY KEY  (`id`),";
      query += "KEY `datetime` (`datetime`),";
      query += "KEY `datetime1` (`datetime1`),";
      query += "KEY `datetime2` (`datetime2`),";
      query += "KEY `lastupdated` (`lastupdated`)";
 
      query += " )";

      if (mysql_query(mysql, query.c_str()) != 0)
      {
        std::cout << "Error creating " << commandCampaign << "!" << std::endl;
        std::cout << "Error creating " << query << "!" << std::endl;
      }
      else
      {
        std::cout << "Action: CREATE OK" << std::endl;
      }
    }
    
    if (commandAction == "drop")
    {
      query = "DROP TABLE `" + commandCampaign + "`";

      if (mysql_query(mysql, query.c_str()) != 0)
      {
        std::cout << "Error droping " << commandCampaign << "!" << std::endl;
      }
      else
      {
        std::cout << "Action: DROP OK" << std::endl;
      }
    }

    if (commandAction == "insert" || commandAction == "add")
    {
      query = "INSERT INTO `" + commandCampaign + "` (phone,param) ";
      query += "values('" + commandParameter + "','" + commandParameter2 + "')";

      if (mysql_query(mysql, query.c_str()) != 0)
      {
        std::cout << "Error inserting " << commandCampaign << query <<"!" << std::endl;
      }
      else
      {
        std::cout << "Action: INSERT OK" << std::endl;
      }
    }

    if (commandAction == "truncate")
    {
      query = "TRUNCATE TABLE `" + commandCampaign + "`";

      if (mysql_query(mysql, query.c_str()) != 0)
      {
        std::cout << "Error truncating " << commandCampaign << "!" << std::endl;
      }
      else
      {
        std::cout << "Action: TRUNCATE OK" << std::endl;
      }
    }

    if (commandAction == "file" || commandAction == "import")
    {
      std::ifstream commandStream;
      std::cout << "Action: Open file " << commandParameter << std::endl;
      commandStream.open(commandParameter.c_str());
      if (commandStream)
      {
        /*
        '\r\n' = Windows
        '\n'   = UNIX and Mac OS X
        '\r'   = Old Mac
        */

        //Detect WIN, UNIX or MAC (OLD)
        std::string winunix,mac,delim,hdelim;
        
        std::getline(commandStream,winunix,'\n');
        commandStream.seekg(0);
        std::getline(commandStream,mac,'\r');
        commandStream.seekg(0);
        if(mac.size()+1 == winunix.size())
        {
          //Windows
          delim = "\\r\\n";hdelim = "Windows(CRLF)";
        }
        else if(mac.size() < winunix.size())
        {
          //Mac
          delim = "\\r";hdelim = "Mac(CR)";
        }
        else
        {
          //Unix
          delim = "\\n";hdelim = "Unix(LF)";
        }

        std::cout << "Line terminator : " << hdelim <<" = '"<< delim <<"'"<<  std::endl;

        //LOAD DATA LOCAL INFILE '/tmp/win.txt' INTO TABLE `default` 
        //FIELDS TERMINATED BY ','  LINES TERMINATED BY '\r\n' (phone,param);
        
        query  = "LOAD DATA LOCAL INFILE '"+ commandParameter+"' INTO TABLE  `" + commandCampaign + "` ";
        query += "FIELDS TERMINATED BY ','  LINES TERMINATED BY '"+ delim +"' (phone,param)";

        if (mysql_query(mysql, query.c_str()) != 0)
        {
          std::cout << "Error creating " << commandCampaign << "!" << std::endl;
          std::cout << "Query:"<< query <<  std::endl;
          std::cout << "Mysql Error :" << mysql_error( mysql )  << std::endl;
        }
        else
        {
          std::cout << "Action: LOAD DATA LOCAL INFILE OK" << std::endl;
        }

      }
      commandStream.close();
    }

    if (commandAction == "delete" || commandAction == "remove")
    {
      query =
        "DELETE FROM `" + commandCampaign + "` WHERE phone='" + commandParameter + "'";

      if (mysql_query(mysql, query.c_str()) != 0)
      {
        std::cout << "Error deleting " << commandCampaign << "!" << std::endl;
        std::cout << "Error deleting " << query << "!" << std::endl;
      }
      else
      {
        std::cout << "Action: DELETE OK" << std::endl;
      }
    }

    if (commandAction == "count")
    {
      query = "SELECT COUNT(id) FROM `" + commandCampaign + "`";

      if (mysql_query(mysql, query.c_str()) != 0)
      {
        std::cout << "Error creating " << commandCampaign << "!" << std::endl;
      }
      else
      {
        int counter = 0;
        result = mysql_use_result(mysql);
        row = mysql_fetch_row(result);
        if (row)
        counter = atoi(row[0]);
        std::cout << commandCampaign << " Total : " << counter << std::endl;
        if (gDebug)
        {
          if (counter)
          {
            std::cout << commandCampaign << " Total : " << counter << std::endl;
          }
        }
        mysql_free_result(result);
      }
    }

    if ((commandAction == "dump") || (commandAction == "fulldump") || (commandAction == "alldump"))
    {
      int header = 0;
      query = "SELECT * FROM `" + commandCampaign + "`";      
      if (mysql_query(mysql, query.c_str()) != 0)
        std::cout << "Error dumping " << commandCampaign << "!" << std::endl;
      else
      {
        result = mysql_use_result(mysql);
        row = mysql_fetch_row(result);
        rows = mysql_num_fields(result);

        //dump loop
        do
        {
          if (!header)
          {
            if (commandAction == "fulldump")
            {
              std::cout << "id" << SEPERATOR << "phone";
              if (rows == 18)
              {
              std::cout << SEPERATOR << "datetime1";
              std::cout << SEPERATOR << "datetime2";
              }
              std::cout << SEPERATOR << "datetime";
              //std::cout << SEPERATOR << "attempts" << SEPERATOR << "pickups" << SEPERATOR << "disposition";
              std::cout << SEPERATOR << "attempts" << SEPERATOR << "pickups";
              if (rows == 18)
              {
              std::cout << SEPERATOR << "reason1";
              std::cout << SEPERATOR << "reason2";
              }
              std::cout << SEPERATOR << "reason";        
              std::cout << SEPERATOR << "duration" << SEPERATOR << "parameter" << SEPERATOR << "result"; 
              if (rows == 18)
              {
              std::cout << SEPERATOR << "cause1";
              std::cout << SEPERATOR << "cause2";
              }
              std::cout << SEPERATOR << "cause";
              //std::cout << SEPERATOR << "lastdate";
              std::cout << std::endl;
            }
            
            header = 1;
          }          
          
          if (row == NULL)
            continue;
          else
          if (commandAction == "simpledump")
          {
            std::cout << row[1]; // Phone
            if (rows == 18)
            {
              if (row[8+4][0] != 0)
              std::cout << SEPERATOR << row[8+4]; // parameter
            }            
            else
            {
              if (row[8][0] != 0)
              std::cout << SEPERATOR << row[8]; // parameter
            }
            std::cout << std::endl;
          }
          else
          {
            int offset = 0;
            
            std::cout << row[0] << SEPERATOR << "\"" << row[1] << "\""; // Id, Phone
            //std::cout << "ROWS " << rows << "!" << std::endl;
            if (rows == 18)
            {
            std::cout << SEPERATOR << row[2]; // date1
            std::cout << SEPERATOR << row[3]; // date2
            offset = 2;
            }
            std::cout << SEPERATOR << row[2+offset]; // date
            std::cout << SEPERATOR << row[3+offset] << SEPERATOR << row[4+offset]; // attempts, pickups
            if (commandAction == "alldump")
            std::cout << SEPERATOR << row[5+offset]; // disposition
            if (rows == 18)
            {
            std::cout << SEPERATOR << "\"" << row[6+offset] << "\""; // reason1
            std::cout << SEPERATOR << "\"" << row[7+offset] << "\""; // reason2
            offset = 4;
            }            
            std::cout << SEPERATOR << "\"" << row[6+offset] << "\""; // reason
            std::cout << SEPERATOR << row[7+offset]; // duration
            std::cout << SEPERATOR << row[8+offset] << SEPERATOR << row[9+offset]; // parameter, result
            if (rows == 18)
            {
            std::cout << SEPERATOR << "\"" << row[10+offset] << "\""; // cause1
            std::cout << SEPERATOR << "\"" << row[11+offset] << "\""; // cause2
            offset = 6;
            }                         
            std::cout << SEPERATOR << "\"" << row[10+offset] << "\""; // cause 
            //std::cout << SEPERATOR << row[11]; (lastupdated)
            std::cout << std::endl;
          }
        } while ( (row = mysql_fetch_row(result)) );
        mysql_free_result(result);        
      }
    }

    if (commandAction == "export") 
    {
      query = "SELECT * FROM `" + commandCampaign + "`";      
      if (mysql_query(mysql, query.c_str()) != 0)
        std::cout << "Error creating " << commandCampaign << "!" << std::endl;
      else
      {
        result = mysql_use_result(mysql);
        row = mysql_fetch_row(result);
        rows = mysql_num_fields(result);
        do {
          if (row != NULL) 
          {
            std::cout << row[1]; // Phone
            if (rows == 18)
            {
              if (row[8+4][0] != 0)
              std::cout << SEPERATOR << row[8+4]; // parameter
            }            
            else
            {
              if (row[8][0] != 0)
              std::cout << SEPERATOR << row[8]; // parameter
            }
            std::cout << std::endl;
          }
          else
            continue;
        } while( (row = mysql_fetch_row(result)) );
        mysql_free_result(result);        
      }
    }
    
    if (commandAction == "statistics")
    {
      int counter = 0;
      int todo = 0;
      int attempts = 0;
      int pickups = 0;
      int results = 0;
      int noanswer = 0;
      int ringing = 0;
      int busy = 0;
      int congestion = 0;
      int disconnected = 0;
            
      query = "SELECT COUNT(id) FROM `" + commandCampaign + "`";
      if (mysql_query(mysql, query.c_str()) != 0)
      {
        std::cout << "Error creating " << commandCampaign << "!" << std::endl;
      }
      else
      {
        result = mysql_use_result(mysql);
        row = mysql_fetch_row(result);
        if (row)
        counter = atoi(row[0]);
        else
        counter = -1;
        mysql_free_result(result);
      }    

      query = "SELECT COUNT(id) FROM `" + commandCampaign + "` WHERE 1 ";
      if (filter.empty() == false && filter != "0" && filter != "None" &&
        filter != "none")
      {
        query += " AND " + filter;
      }
      else
      {
        query += " AND ((disposition > -6 AND disposition < 6))";             
      }
      if (mysql_query(mysql, query.c_str()) != 0)
      {
        std::cout << "Error creating " << commandCampaign << "!" << std::endl;
      }
      else
      {
        result = mysql_use_result(mysql);
        row = mysql_fetch_row(result);
        if (row)
        todo = atoi(row[0]);
        else
        todo = -1;
        mysql_free_result(result);
      }

      query = "SELECT COUNT(id) FROM `" + commandCampaign + "` WHERE 1 ";
      query += " AND attempts>0";      
      if (mysql_query(mysql, query.c_str()) != 0)
      {
        std::cout << "Error creating " << commandCampaign << "!" << std::endl;
      }
      else
      {
        result = mysql_use_result(mysql);
        row = mysql_fetch_row(result);
        if (row)
        attempts = atoi(row[0]);
        else
        attempts = -1;
        mysql_free_result(result);
      }

      query = "SELECT COUNT(id) FROM `" + commandCampaign + "` WHERE 1 ";
      query += " AND pickups>0";      
      if (mysql_query(mysql, query.c_str()) != 0)
      {
        std::cout << "Error creating " << commandCampaign << "!" << std::endl;
      }
      else
      {
        result = mysql_use_result(mysql);
        row = mysql_fetch_row(result);
        if (row)
        pickups = atoi(row[0]);
        else
        pickups = -1;
        mysql_free_result(result);
      }

      query = "SELECT COUNT(id) FROM `" + commandCampaign + "` WHERE 1 ";
      query += " AND result<>''";      
      if (mysql_query(mysql, query.c_str()) != 0)
      {
        std::cout << "Error creating " << commandCampaign << "!" << std::endl;
      }
      else
      {
        result = mysql_use_result(mysql);
        row = mysql_fetch_row(result);
        if (row)
        results = atoi(row[0]);
        else
        results = -1;
        mysql_free_result(result);
      }

      query = "SELECT COUNT(id) FROM `" + commandCampaign + "` WHERE 1 ";
      query += " AND disposition=-2";      
      if (mysql_query(mysql, query.c_str()) != 0)
      {
        std::cout << "Error creating " << commandCampaign << "!" << std::endl;
      }
      else
      {
        result = mysql_use_result(mysql);
        row = mysql_fetch_row(result);
        if (row)
        noanswer = atoi(row[0]);
        else
        noanswer = -1;
        mysql_free_result(result);
      }

      query = "SELECT COUNT(id) FROM `" + commandCampaign + "` WHERE 1 ";
      query += " AND disposition=-3";      
      if (mysql_query(mysql, query.c_str()) != 0)
      {
        std::cout << "Error creating " << commandCampaign << "!" << std::endl;
      }
      else
      {
        result = mysql_use_result(mysql);
        row = mysql_fetch_row(result);
        if (row)
        ringing = atoi(row[0]);
        else
        ringing = -1;
        mysql_free_result(result);
      }
      
      query = "SELECT COUNT(id) FROM `" + commandCampaign + "` WHERE 1 ";
      query += " AND disposition=-4";      
      if (mysql_query(mysql, query.c_str()) != 0)
      {
        std::cout << "Error creating " << commandCampaign << "!" << std::endl;
      }
      else
      {
        result = mysql_use_result(mysql);
        row = mysql_fetch_row(result);
        if (row)
        busy = atoi(row[0]);
        else
        busy = -1;
        mysql_free_result(result);
      }

      query = "SELECT COUNT(id) FROM `" + commandCampaign + "` WHERE 1 ";
      query += " AND disposition=-5";      
      if (mysql_query(mysql, query.c_str()) != 0)
      {
        std::cout << "Error creating " << commandCampaign << "!" << std::endl;
      }
      else
      {
        result = mysql_use_result(mysql);
        row = mysql_fetch_row(result);
        if (row)
        congestion = atoi(row[0]);
        else
        congestion = -1;
        mysql_free_result(result);
      }

      query = "SELECT COUNT(id) FROM `" + commandCampaign + "` WHERE 1 ";
      query += " AND disposition=-7";      
      if (mysql_query(mysql, query.c_str()) != 0)
      {
        std::cout << "Error creating " << commandCampaign << "!" << std::endl;
      }
      else
      {
        result = mysql_use_result(mysql);
        row = mysql_fetch_row(result);
        if (row)
        disconnected = atoi(row[0]);
        else
        disconnected = -1;
        mysql_free_result(result);
      }
                  
      std::cout << "Count        : " << itos(counter) <<  std::endl;
      std::cout << "Todo         : " << itos(todo) <<  std::endl;
      std::cout << "Attempts     : " << itos(attempts) <<  std::endl;
      std::cout << "Pickups      : " << itos(pickups) <<  std::endl;
      std::cout << "NoAnswer     : " << itos(noanswer) <<  std::endl;
      std::cout << "Ringing      : " << itos(ringing) <<  std::endl;
      std::cout << "Busy         : " << itos(busy) <<  std::endl;
      std::cout << "Congestion   : " << itos(congestion) <<  std::endl;
      std::cout << "Disconnected : " << itos(disconnected) <<  std::endl;
      std::cout << "Results      : " << itos(results) <<  std::endl;
    }

    return 0;
  }

  if (mysql_select_db(mysql, getDbName().c_str()))
  {
    std::cerr << "Dialer: MySql cannot use database " << getDbName().c_str() << "!" << std::endl;
    DBG_TRACE(DBG_MYSQL, (0, "MySQL cannot use database %s", getDbName().c_str()));
    return 1;
  } 
  
  try
  {
    // Not nested for convenience.
    if (gLog)
    {
      writeDialerLog("Dialer: Started");
    }
    if (gDebug)
    {
      if (doColorize)
      {
        std::cout << fg_light_green << "Dialer: Started" << normal << std::endl;
      }
      else
      {
        std::cout << "Dialer: Started" << std::endl;
      }
    }
    CallCache *TheCallCache;
    try
    {
      TheCallCache = new CallCache();
    }
    catch(xTooFewFields)
    {
      std::cerr << "Exception: Too few fields in gdhosts.conf!" << std::endl;
      return 1;
    }
    catch(xTooManyFields)
    {
      std::cerr << "Exception: Too many fields in gdhosts.conf!" << std::endl;
      return 1;
    }
    catch(xInvalidWeightValue)
    {
      std::cerr << "Exception: Invalid weight value in gdhosts.conf!" << std::
        endl;
      return 1;
    }
    catch(const xFileOpenError & e)
    {
      std::cerr << "Exception: Error opening " << e.
        GetFilename() << "!" << std::endl;
      return 1;
    }
    catch(xNoHostsDefined)
    {
      std::cerr << "Exception: No hosts defined in gdhosts.conf!" << std::endl;
      return 1;
    }

    ClientSocket AsteriskManager(getMainHost(), atoi(getMainPort().c_str()));
    AsteriskManager.setRecvTimeout(1000);
                  
    response = AsteriskManager.recv();
      
    request = "Action: login\r\nUsername: " + managerUser +
      "\r\nSecret: " + managerPass + "\r\n\r\n";      
    AsteriskManager.send(request);

    response = AsteriskManager.recv();
    
    //std::cout << "READ AsteriskManager" << response 
    //  << std::endl;

    QueueList TheQueues(commandConfigurationFile);
    TheQueues.ParseQueues();

    HERE(SETTINGS CHECK) std::string tempCheckCampaign;
    for (int i = 0; i < TheQueues.size(); i++)
    {
      tempCheckCampaign = TheQueues.at(i).GetName();
      if (gDebug)
      {
        std::cout << tempCheckCampaign << ": Settings Pre-Check " << std::endl;
      }
      addBasicSettings(tempCheckCampaign, commandConfigurationFile);

      if (gDebug)
      {
        std::cout << tempCheckCampaign << ": Resetting Filters " << std::endl;
      }
    }

    if (gDebug)
    {
      std::cout <<  "Parse Queues " << std::endl;
    }

    timeval tv;
    unsigned long int timeSinceLastQueueUpdate = 0, currentTime = 0;
    int JobToDo = 1;
    
    response = "";

    if (gDebug)
    {
      std::cout << "Start the loop " << std::endl;
    }

    struct timeval tv_start;
    struct timeval tv_lastrecv;
    struct timeval tv_recv;
    struct timeval tv_lastsql;
    struct timeval tv_sql;

    gettimeofday(&tv_start, NULL);
    gettimeofday(&tv_lastrecv, NULL);
    gettimeofday(&tv_lastsql, NULL);

    time_t now;
    time_t lasttime = 0;

    for (unsigned long int t = 0; JobToDo; t++)
    {
      gettimeofday(&tv, NULL);
      currentTime = tv.tv_sec % 1000000;
      if ((t != 0 && t % 10 == 0 && currentTime - timeSinceLastQueueUpdate > 5)
        || (currentTime - timeSinceLastQueueUpdate > 20 && t != 0))
      {
        if (gDebug)
        {
          if (doColorize)
          {
            std::
              cout << "Dialer: " << fg_light_yellow <<
              "Updating Campaign Settings " << fg_light_cyan << "(" <<
              currentTime << ")" << normal << std::endl;
          }
          else
          {
            std::
              cout << "Dialer: Updating Campaign Settings " << "(" <<
              currentTime << ")" << std::endl;

          }
        }

        //TheQueues.ParseQueues();
        //TheQueueGlobals.ParseQueue("general");

        try
        {
          gDebug = TheQueueGlobals.GetSetting("debug").GetBool();
          gLog = TheQueueGlobals.GetSetting("log").GetBool();
          //gDebug = 1;
          //gLog = 1;
        }
        catch(xLoopEnd e)
        {
          std::
            cout << "Caught exception while trying to get debug & log settings!"
            << std::endl;
          std::cout << e.what();
          std::cout << std::endl << std::endl;
        }

        timeSinceLastQueueUpdate = currentTime;
      }
      
      if (gDebug)
      {
        std::cout << "Read from Asterisk" << std::endl;
      }
      
      //response = "";
      //usleep(1000);
      //AsteriskManager >> response;
      response = AsteriskManager.recv();


      if (gDebug)
      {
        int tcp_rcvbuf = AsteriskManager.get_rcvbuf();
        int tcp_cinq = AsteriskManager.get_cinq();

        std::cout << "READ AsteriskManager " << tcp_cinq << "/" << tcp_rcvbuf << std::endl;
      }

      if (gDebug)
      {
        gettimeofday(&tv_recv, NULL);

        long elapsed = (tv_recv.tv_sec-tv_lastrecv.tv_sec)*1000000 + tv_recv.tv_usec-tv_lastrecv.tv_usec;

        std::cout << "READ AsteriskManager " << elapsed << "us" << std::endl;

        tv_lastrecv = tv_recv;
      }

      //Response can be empty, non blocking mode
      if(errno == 0 && response.empty())
        if (gDebug) std::cout << "Asterisk response is empty" << std::endl;

      if (errno == 107 || errno == 9 )
      {
	    if (safeMode == true)
	    {
		if (doColorize)
			{
			  std::
			    cout << fg_light_red << "Try a reconnect !!! " <<
			    normal << std::endl;
			}
			else
			{
			  std::cout << "Try a reconnect !!! " << std::endl;
			}
			if (gLog)
			{
			  writeDialerLog
			    ("Try a reconnect !!! ");
			}
	      std::
		cout <<
		"You are running in safe mode, so Dialer will attempt to restart itself!"
		<< std::endl << std::endl;
		
		usleep(3000000);

	      if (daemonMode)
	      std::system("sleep 1 && /opt/dialer/dialer --safe --start &");
	      else
	      std::system("sleep 1 && /opt/dialer/dialer --safe &");
	      JobToDo = 0;
	      if (gDebug) std::cout << "JobToDo = 0" << std::endl;
	      }
	      else
	      {
		if (gDebug) std::cout << "JobToDo = 0" << std::endl;
		JobToDo = 0;
	      }
	}


      if (gDebug)
      {
        std::cout << "After read from Asterisk" << std::endl;
        //Responses can be read from /tmp/Dialer_Traces...
        //std::cout << "Read " << response << std::endl;
      }

      if (!previous.empty())
      {
        //std::cout << "CONCAT PREVIOUS!"
        //  << std::endl;

        //response.insert(0, previous);
        response = previous + response;
        previous = "";
      }

      if (response.empty())
      {
        BlockStream.str("Event: NoEvent\r\n\r\n");
        previous = "";
      }
      else
      {
        BlockStream.clear();
        
        if (((pos = response.find("\r\n\r\n", 0)) != std::string::npos) || ((pos2 = response.find("\n\n", 0)) != std::string::npos))
        {
          if (pos2 > pos)
          end = pos2 + 2;
          else
          end = pos + 4;


                    
          while (((pos = response.find("\r\n\r\n", end)) != std::string::npos) || ((pos2 = response.find("\n\n", end)) != std::string::npos))
          {

          if (pos2 > pos)
          end = pos2 + 2;
          else
          end = pos + 4;      
          
          if (int(end) == int(response.length()))
          break;
          }        
          
          BlockStream.str(response.substr(0, end));
          previous = response.substr(end, std::string::npos);
         
        }
        else
        {
          //std::cout << "NO FOUND THE COMMAND!"
          //  << std::endl;
          BlockStream.str("Event: NoEvent\r\n\r\n");
        }
      }


      // TRIGGER LOOP
      for (std::string tempLine; std::getline(BlockStream, tempLine, '\n');)
      {
        tempLine = tempLine.substr(0, tempLine.length() - 1);

        //std::cout << std::endl << tempLine.c_str() << std::endl;
        //log("AST > %s\n", tempLine.c_str());
        DBG_TRACE(DBG_ASTERISK, (0, "> %s", tempLine.c_str()));

        if (gDebug)
        std::cout << "line loop " << tempLine << std::endl;

        // strip '\r'
        if (tempLine.empty())
        {
          pos = 0;
          end = 0;
          pos2 = 0;
          end2 = 0;
          // Begin block analysis
          //const std::string param(const std::string & block, const std::string & type) {
          //         int pos = 0, len = 0;
          //         if (block.find(type + ": ",0) == std::string::npos) {
          //                 return static_cast<const std::string>("");
          //         } else {
          //                 pos = block.find(type + ": ",0) + 2;
          //                 len = type.length();
          //                 return block.substr(pos + len,block.find("\n",pos) - (pos + len));
          //                 }
          //         }
          //***********************************************************************************

          // THIS CAUSES Dialer TO BE KILLED WHEN ASTERISK WAS SHUTDOWN
          // THIS DOES NOT HANDLE OTHER ASTERISK EXITS, LIKE CORES
          if (block.find("Event: Shutdown", 0) != std::string::npos)
          {
            if (gDebug)
            {
              std::cout << "Dialer: Asterisk Shutdown - Dialer Killed" << std::
                endl;
            }
            if (gLog)
            {
              writeDialerLog("Dialer: Asterisk Shutdown - Dialer Killed");
            }
            std::system("killall Dialer");
          }

      if (gDebug)
      {
        gettimeofday(&tv_recv, NULL);

        long elapsed = (tv_recv.tv_sec-tv_lastrecv.tv_sec)*1000000 + tv_recv.tv_usec-tv_lastrecv.tv_usec;

        std::cout << "READ AsteriskManager MARK1.1 " << elapsed << "us" << std::endl;
      }

          //***********************************************************************************
          if (block.find("Event: OriginateResponse", 0) != std::string::npos &&
            block.find("CallerIDName: ", 0) != std::string::npos)
          {
            int iTheReason = 0;
            int iTheAttempts = 0;
            std::string theReason, theUniqueid, theCallerIDName, theCampaign, theLeadid, theString;
            std::string theDispo = "0";
            std::string theReasonDesc = "Unknown";

            if (block.find("Reason: ", 0) != std::string::npos)
            {
              pos = block.find("Reason: ", 0) + 8;
              end = block.find("\n", pos);
              theReason = block.substr(pos, end - pos);
              iTheReason = atoi(theReason.c_str());
              theReasonDesc = reason2long(iTheReason);              
            }
            if (block.find("CallerIDName: ", 0) != std::string::npos &&
              block.find("~", 0) != std::string::npos && !theReason.empty())
            {
              pos = block.find("CallerIDName: ", 0) + 15;
              end = block.find("\n", pos);
              theCallerIDName = block.substr(pos, end - pos);

              if (block.find("~", 0) != std::string::npos)
              {
                pos = theCallerIDName.find("~", end) + 1;
                end = theCallerIDName.find("-", pos + 1);
                pos2 = end + 1;
                end2 = theCallerIDName.find("-", pos2);
                theCampaign = theCallerIDName.substr(pos, end - pos);
                theLeadid = theCallerIDName.substr(pos2, end2 - pos2);
              }
              
              if (block.find("Uniqueid: ", 0) != std::string::npos)
              {
                pos = block.find("Uniqueid: ", 0) + 10;
                end = block.find("\n", pos);
                theUniqueid = block.substr(pos, end - pos);
              }
              else
              {
                if (gLog)
                {
                  writeDialerLog
                    ("OriginateSuccess - PARSE ERROR - No Uniqueid or Empty Cause (block: "
                    + block + ")");
                }
              }

              if (!theCampaign.empty() && !theLeadid.empty() && !theUniqueid.empty() &&
                TheQueues.exists(theCampaign))
              {
                TheCallCache->SetUniqueid(theCampaign, theLeadid, theUniqueid);

                if (gDebug)
                {
                  if (doColorize)
                  {
                    std::
                      cout << theCampaign << fg_green <<
                      ": OriginateSuccess - theLeadid: " << theLeadid <<
                      " theUniqueid: " << theUniqueid << normal <<
                      std::endl;
                  }
                  else
                  {
                    std::
                      cout << theCampaign << ": OriginateSuccess - theLeadid: "
                      << theLeadid << " theUniqueid: " << theUniqueid << std::endl;
                  }

                }

                if (gLog)
                {
                  writeDialerLog(theCampaign +
                    ": OriginateSuccess - theLeadid: " + theLeadid +
                    " theUniqueID: " + theUniqueid);
                }
              }

              if (!theCampaign.empty() && !theLeadid.empty() &&
                TheQueues.exists(theCampaign))
              {
                if (attempts > 1)
                {
                query = "SELECT attempts FROM `" + theCampaign + "` WHERE id=" + theLeadid + "";   
                if (mysql_query(mysql, query.c_str()) == 0)
                {
                  result = mysql_use_result(mysql);
                  row = mysql_fetch_row(result);
                  if (row)
                  {
                    iTheAttempts = atoi(row[0]);
                  }
                  else
                  iTheAttempts = -1;

                  mysql_free_result(result);
                  if (gDebug)                  
                  std::cout << "Attempts = " << iTheAttempts << std::endl;                  
                }      
                else        
                iTheAttempts = 0;
                }
              
                //unknown failure, disconnect
                if (theReason == "0")
                {
                  theDispo = "-7";
                  theString = "disposition='" + theDispo + "',reason='" + theReasonDesc + "'";
                  query = "UPDATE `" + theCampaign + "` SET " + theString + " WHERE id=" + theLeadid + "";     
                  if (mysql_query(mysql, query.c_str()) != 0)
                  {
                    std::cerr << "Error Updating " << query << std::endl;
                  }
                                                        
                  TheQueues.rWhere(theCampaign).IncrementDisconnects();
                  TheCallCache->SetUnanswered(theCampaign, theLeadid);
                }
                // AST_CONTROL_RINGING
                //timed out while ringing, no answer
                if (theReason == "3")
                {
                  if (!ringonly)
                  theDispo = "-3";
                  else
                  theDispo = "7";
                  theString = "disposition='" + theDispo + "',reason='" + theReasonDesc + "'";
                  query = "UPDATE `" + theCampaign + "` SET " + theString + " WHERE id=" + theLeadid + "";     
                  if (mysql_query(mysql, query.c_str()) != 0)
                  {
                    std::cerr << "Error Updating " << query << std::endl;
                  }                  
                  TheQueues.rWhere(theCampaign).IncrementNoanswers();
                  TheCallCache->SetUnanswered(theCampaign, theLeadid);
                }
                // AST_CONTROL_BUSY
                //busy
                if (theReason == "5")
                {
                  theDispo = "-4";
                  theString = "disposition='" + theDispo + "',reason='" + theReasonDesc + "'";
                  query = "UPDATE `" + theCampaign + "` SET " + theString + " WHERE id=" + theLeadid + "";     
                  if (mysql_query(mysql, query.c_str()) != 0)
                  {
                    std:: cerr << "Error Updating " << query << std::endl;
                  }                  
                  TheQueues.rWhere(theCampaign).IncrementBusies();
                  TheCallCache->SetUnanswered(theCampaign, theLeadid);
                }
                // AST_CONTROL_HANGUP
                //hangup, no answer
                if (theReason == "1")
                {
                  theDispo = "-2";
                  theString = "disposition='" + theDispo + "',reason='" + theReasonDesc + "'";
                  query = "UPDATE `" + theCampaign + "` SET " + theString + " WHERE id=" + theLeadid + "";     
                  if (mysql_query(mysql, query.c_str()) != 0)
                  {
                    std:: cerr << "Error Updating " << query << std::endl;
                  }                  
                  TheQueues.rWhere(theCampaign).IncrementNoanswers();
                  TheCallCache->SetUnanswered(theCampaign, theLeadid);
                }
                // AST_CONTROL_CONGESTION
                //congestion
                if (theReason == "8")
                {
                  theDispo = "-5";
                  theString = "disposition='" + theDispo + "',reason='" + theReasonDesc + "'";
                  query = "UPDATE `" + theCampaign + "` SET " + theString + " WHERE id=" + theLeadid + "";     
                  if (mysql_query(mysql, query.c_str()) != 0)
                  {
                    std:: cerr << "Error Updating " << query << std::endl;
                  }                  
                  TheQueues.rWhere(theCampaign).IncrementCongestions();
                  TheCallCache->SetUnanswered(theCampaign, theLeadid);
                }
                // AST_CONTROL_ANSWERED
                //Answered
                if (theReason == "4")
                {
                  theDispo = "7";
                  theString = "disposition='" + theDispo + "',reason='" + theReasonDesc + "'";
                  query = "UPDATE `" + theCampaign + "` SET " + theString + " WHERE id=" + theLeadid + "";     
                  if (mysql_query(mysql, query.c_str()) != 0)
                  {
                    std:: cerr << "Error Updating " << query << std::endl;
                  }                  
                  TheQueues.rWhere(theCampaign).IncrementAnsmachs();
                  TheCallCache->SetAnswered(theCampaign, theLeadid);
                }
                result = mysql_use_result(mysql);
                mysql_free_result(result);
                
                {
                  if (attempts > 0)
                  {
                    if (iTheAttempts == 1)
                    {
                      query = "UPDATE `" + theCampaign + "` SET " + "reason1='" + theReasonDesc + "',datetime1=datetime" + " WHERE id=" + theLeadid + "";     
                      if (mysql_query(mysql, query.c_str()) != 0)
                      {
                        if (gDebug)
                        std::cout << "Warning Updating " << query << std::endl;
                      }                      
                    }
                    if (iTheAttempts == 2)
                    {
                      query = "UPDATE `" + theCampaign + "` SET " + "reason2='" + theReasonDesc + "',datetime2=datetime" + " WHERE id=" + theLeadid + "";     
                      if (mysql_query(mysql, query.c_str()) != 0)
                      {
                        if (gDebug)
                        std::cout << "Warning Updating " << query << std::endl;
                      }                      
                    }
                  }
                }                  
                
                if (theDispo == "0")
                {
                  if (gDebug)
                  {
                  if (doColorize)
                  {
                    std::
                      cout << theCampaign << fg_light_red <<
                      ": OriginateResponse - " << theReasonDesc << " (" <<
                      dispo2long(stoi(theDispo)) << ") " << normal << std::endl;
                  }
                  else
                  {
                    std::
                      cout << theCampaign << ": OriginateResponse - " <<
                      theReasonDesc << " (" << dispo2long(stoi(theDispo)) << ") " <<
                      std::endl;
                  }
                  }
                }

                //stats testing
                TheQueues.rWhere(theCampaign).WriteAbn();

                if (gDebug)
                {
                  if (doColorize)
                  {
                    std::
                      cout << theCampaign << fg_light_green <<
                      ": OriginateResponse - theLeadid: " << theLeadid <<
                      " theReasonDesc: " << theReasonDesc << " theDispo: " << theDispo
                      << " (" << dispo2long(stoi(theDispo)) << ") " << normal <<
                      std::endl;
                  }
                  else
                  {
                    std::
                      cout << theCampaign << ": OriginateResponse - theLeadid: "
                      << theLeadid << " theReasonDesc: " << theReasonDesc <<
                      " theDispo: " << theDispo << " (" <<
                      dispo2long(stoi(theDispo)) << ") " << std::endl;
                  }

                }
                if (gLog)
                {
                  writeDialerLog(theCampaign +
                    ": OriginateResponse - theLeadid: " + theLeadid +
                    " theReasonDesc: " + theReasonDesc + " theDispo: " + theDispo);
                }
              }
              else
              {
                if (doColorize)
                {
                  std::
                    cout << fg_light_red << "OriginateResponse: PARSE ERROR " <<
                    normal << std::endl;
                }
                else
                {
                  std::cout << "OriginateResponse: PARSE ERROR " << std::endl;
                }
                if (gLog)
                {
                  writeDialerLog
                    ("OriginateFailure - PARSE ERROR - Something Empty (block: "
                    + block + ")");
                }
              }
            }
            else
            {
              if (gLog)
              {
                writeDialerLog
                  ("OriginateResponse - PARSE ERROR - No CallerIDName or Empty Reason (block: \n"
                  + block + ")");
              }
            }
          }

      if (gDebug)
      {
        gettimeofday(&tv_recv, NULL);

        long elapsed = (tv_recv.tv_sec-tv_lastrecv.tv_sec)*1000000 + tv_recv.tv_usec-tv_lastrecv.tv_usec;

        std::cout << "READ AsteriskManager MARK1.2 " << elapsed << "us" << std::endl;
      }

          //***********************************************************************************
          if (block.find("Event: OriginateFailure", 0) != std::string::npos &&
            block.find("CallerIDName: ", 0) != std::string::npos)
          {
            int iTheReason = 0;
            int iTheAttempts = 0;
            std::string theReason, theCallerIDName, theCampaign, theLeadid, theString;
            std::string theDispo = "0";
            std::string theReasonDesc = "Unknown";

            if (block.find("Reason: ", 0) != std::string::npos)
            {
              pos = block.find("Reason: ", 0) + 8;
              end = block.find("\n", pos);
              theReason = block.substr(pos, end - pos);
              iTheReason = atoi(theReason.c_str());
              theReasonDesc = reason2long(iTheReason);
            }
            if (block.find("CallerIDName: ", 0) != std::string::npos &&
              block.find("~", 0) != std::string::npos && !theReason.empty())
            {
              pos = block.find("CallerIDName: ", 0) + 15;
              end = block.find("\n", pos);
              theCallerIDName = block.substr(pos, end - pos);

              if (block.find("~", 0) != std::string::npos)
              {
                pos = theCallerIDName.find("~", end) + 1;
                end = theCallerIDName.find("-", pos + 1);
                pos2 = end + 1;
                end2 = theCallerIDName.find("-", pos2);
                theCampaign = theCallerIDName.substr(pos, end - pos);
                theLeadid = theCallerIDName.substr(pos2, end2 - pos2);
              }
              if (!theCampaign.empty() && !theLeadid.empty() &&
                TheQueues.exists(theCampaign))
              {
                if (attempts > 1)
                {
                query = "SELECT attempts FROM `" + theCampaign + "` WHERE id=" + theLeadid + "";   
                if (mysql_query(mysql, query.c_str()) == 0)
                {
                  result = mysql_use_result(mysql);
                  row = mysql_fetch_row(result);
                  if (row)
                  {
                    iTheAttempts = atoi(row[0]);
                  }
                  else
                  iTheAttempts = -1;

                  mysql_free_result(result);
                  if (gDebug)
                  std::cout << "Attempts = " << iTheAttempts << std::endl;
                }                            
                else
                iTheAttempts = 0;
                }
              
                //unknown failure, disconnect
                if (theReason == "0")
                {
                  theDispo = "-7";
                  theString = "disposition='" + theDispo + "',reason='" + theReasonDesc + "'";
                  query = "UPDATE `" + theCampaign + "` SET " + theString + " WHERE id=" + theLeadid + "";     
                  if (mysql_query(mysql, query.c_str()) != 0)
                  {
                    std:: cerr << "Error Updating " << query << std::endl;
                  }                  
                                      
                  TheQueues.rWhere(theCampaign).IncrementDisconnects();
                }
                // AST_CONTROL_RINGING
                //timed out while ringing, no answer
                if (theReason == "3")
                {
                  theDispo = "-2";
                  theString = "disposition='" + theDispo + "',reason='" + theReasonDesc + "'";
                  query = "UPDATE `" + theCampaign + "` SET " + theString + " WHERE id=" + theLeadid + "";     
                  if (mysql_query(mysql, query.c_str()) != 0)
                  {
                    std:: cerr << "Error Updating " << query << std::endl;
                  }                  
                  TheQueues.rWhere(theCampaign).IncrementNoanswers();
                }
                // AST_CONTROL_BUSY
                //busy
                if (theReason == "5")
                {
                  theDispo = "-4";
                  theString = "disposition='" + theDispo + "',reason='" + theReasonDesc + "'";
                  query = "UPDATE `" + theCampaign + "` SET " + theString + " WHERE id=" + theLeadid + "";     
                  if (mysql_query(mysql, query.c_str()) != 0)
                  {
                    std:: cerr << "Error Updating " << query << std::endl;
                  }                  
                  TheQueues.rWhere(theCampaign).IncrementBusies();
                }
                // AST_CONTROL_HANGUP
                //hangup, no answer
                if (theReason == "1")
                {
                  theDispo = "-2";
                  theString = "disposition='" + theDispo + "',reason='" + theReasonDesc + "'";
                  query = "UPDATE `" + theCampaign + "` SET " + theString + " WHERE id=" + theLeadid + "";     
                  if (mysql_query(mysql, query.c_str()) != 0)
                  {
                    std:: cerr << "Error Updating " << query << std::endl;
                  }                  
                  TheQueues.rWhere(theCampaign).IncrementNoanswers();
                }
                // AST_CONTROL_CONGESTION
                //congestion
                if (theReason == "8")
                {
                  theDispo = "-5";
                  theString = "disposition='" + theDispo + "',reason='" + theReasonDesc + "'";
                  query = "UPDATE `" + theCampaign + "` SET " + theString + " WHERE id=" + theLeadid + "";     
                  if (mysql_query(mysql, query.c_str()) != 0)
                  {
                    std:: cerr << "Error Updating " << query << std::endl;
                  }                  
                  TheQueues.rWhere(theCampaign).IncrementCongestions();
                }
                result = mysql_use_result(mysql);
                mysql_free_result(result);

                {
                  if (attempts > 0)
                  {
                    if (iTheAttempts == 1)
                    {
                      query = "UPDATE `" + theCampaign + "` SET " + "reason1='" + theReasonDesc + "',datetime1=datetime" + " WHERE id=" + theLeadid + "";     
                      if (mysql_query(mysql, query.c_str()) != 0)
                      {
                        if (gDebug)
                        std::cout << "Warning Updating " << query << std::endl;
                      }                      
                    }
                    if (iTheAttempts == 2)
                    {
                      query = "UPDATE `" + theCampaign + "` SET " + "reason2='" + theReasonDesc + "',datetime2=datetime" + " WHERE id=" + theLeadid + "";     
                      if (mysql_query(mysql, query.c_str()) != 0)
                      {
                        if (gDebug)
                        std::cout << "Warning Updating " << query << std::endl;
                      }                      
                    }
                  }
                }  
                                
                if (theDispo == "0")
                {
                  if (doColorize)
                  {
                    std::
                      cout << theCampaign << fg_light_red <<
                      ": OriginateFailure - " << theReason << " (" <<
                      dispo2long(stoi(theDispo)) << ") " << normal << std::endl;
                  }
                  else
                  {
                    std::
                      cout << theCampaign << ": OriginateFailure - " <<
                      theReason << " (" << dispo2long(stoi(theDispo)) << ") " <<
                      std::endl;
                  }
                }

                //stats testing
                TheQueues.rWhere(theCampaign).WriteAbn();

                if (gDebug)
                {
                  if (doColorize)
                  {
                    std::
                      cout << theCampaign << fg_light_green <<
                      ": OriginateFailure - theLeadid: " << theLeadid <<
                      " theReason: " << theReason << " theDispo: " << theDispo
                      << " (" << dispo2long(stoi(theDispo)) << ") " << normal <<
                      std::endl;
                  }
                  else
                  {
                    std::
                      cout << theCampaign << ": OriginateFailure - theLeadid: "
                      << theLeadid << " theReason: " << theReason <<
                      " theDispo: " << theDispo << " (" <<
                      dispo2long(stoi(theDispo)) << ") " << std::endl;
                  }

                }
                if (gLog)
                {
                  writeDialerLog(theCampaign +
                    ": OriginateFailure - theLeadid: " + theLeadid +
                    " theReason: " + theReason + " theDispo: " + theDispo);
                }
              }
              else
              {
                if (doColorize)
                {
                  std::
                    cout << fg_light_red << "OriginateFailure: PARSE ERROR " <<
                    normal << std::endl;
                }
                else
                {
                  std::cout << "OriginateFailure: PARSE ERROR " << std::endl;
                }
                if (gLog)
                {
                  writeDialerLog
                    ("OriginateFailure - PARSE ERROR - Something Empty (block: "
                    + block + ")");
                }
              }
            }
            else
            {
              if (gLog)
              {
                writeDialerLog
                  ("OriginateFailure - PARSE ERROR - No CallerIDName or Empty Reason (block: \n"
                  + block + ")");
              }
            }
          }

      if (gDebug)
      {
        gettimeofday(&tv_recv, NULL);

        long elapsed = (tv_recv.tv_sec-tv_lastrecv.tv_sec)*1000000 + tv_recv.tv_usec-tv_lastrecv.tv_usec;

        std::cout << "READ AsteriskManager MARK1.3 " << elapsed << "us" << std::endl;
      }


          //***********************************************************************************
          if (block.find("Event: OriginateSuccess", 0) != std::string::npos &&
            block.find("CallerIDName: ", 0) != std::string::npos)
          if (!clip)
          {
            int iTheReason = 0;
            int iTheAttempts = 0;
            std::string theReason, theUniqueid, theCallerIDName, theCampaign, theLeadid, theString;
            std::string theDispo = "-3";
            std::string theReasonDesc = "Unknown";
                        
            if (block.find("Reason: ", 0) != std::string::npos)
            {
              pos = block.find("Reason: ", 0) + 8;
              end = block.find("\n", pos);
              theReason = block.substr(pos, end - pos);
              iTheReason = atoi(theReason.c_str());
              theReasonDesc = reason2long(iTheReason);
            }
            
            if (block.find("CallerIDName: ", 0) != std::string::npos &&
              block.find("~", 0) != std::string::npos && !theReason.empty())
            {
              pos = block.find("CallerIDName: ", 0) + 15;
              end = block.find("\n", pos);
              theCallerIDName = block.substr(pos, end - pos);

              if (block.find("~", 0) != std::string::npos)
              {
                pos = theCallerIDName.find("~", end) + 1;
                end = theCallerIDName.find("-", pos + 1);

                pos2 = end + 1;
                end2 = theCallerIDName.find("-", pos2);

                theCampaign = theCallerIDName.substr(pos, end - pos);
                theLeadid = theCallerIDName.substr(pos2, end2 - pos2);
              }


              if (block.find("Uniqueid: ", 0) != std::string::npos)
              {
                pos = block.find("Uniqueid: ", 0) + 10;
                end = block.find("\n", pos);
                theUniqueid = block.substr(pos, end - pos);
              }
              else
              {
                if (gLog)
                {
                  writeDialerLog
                    ("OriginateSuccess - PARSE ERROR - No Uniqueid or Empty Cause (block: "
                    + block + ")");
                }
              }

              if (!theCampaign.empty() && !theLeadid.empty() && !theUniqueid.empty() &&
                TheQueues.exists(theCampaign))
              {
                TheCallCache->SetUniqueid(theCampaign, theLeadid, theUniqueid);

                if (gDebug)
                {
                  if (doColorize)
                  {
                    std::
                      cout << theCampaign << fg_green <<
                      ": OriginateSuccess - theLeadid: " << theLeadid <<
                      " theUniqueid: " << theUniqueid << normal <<
                      std::endl;
                  }
                  else
                  {
                    std::
                      cout << theCampaign << ": OriginateSuccess - theLeadid: "
                      << theLeadid << " theUniqueid: " << theUniqueid << std::endl;
                  }

                }

                if (gLog)
                {
                  writeDialerLog(theCampaign +
                    ": OriginateSuccess - theLeadid: " + theLeadid +
                    " theUniqueID: " + theUniqueid);
                }
              }


              if (!theCampaign.empty() && !theLeadid.empty() &&
                TheQueues.exists(theCampaign))
              {
                if (attempts > 1)
                {
                query = "SELECT attempts FROM `" + theCampaign + "` WHERE id=" + theLeadid + "";   
                if (mysql_query(mysql, query.c_str()) == 0)
                {
                  result = mysql_use_result(mysql);
                  row = mysql_fetch_row(result);
                  if (row)
                  {
                    iTheAttempts = atoi(row[0]);
                  }
                  else
                  iTheAttempts = -1;
                  
                  mysql_free_result(result);
                  if (gDebug)
                  std::cout << "Attempts = " << iTheAttempts << std::endl;
                }                            
                else
                iTheAttempts = 0;
                }              
              
                // answer, we'll assume voicemail/answering machine
                // if it passes talkdetect it will be sent to agent
                // or abandons++                
                if (theReason == "4")
                {
                  theDispo = "-3";
                  theString = "disposition='" + theDispo + "',reason='" + theReasonDesc + "'";
                  query = "UPDATE `" + theCampaign + "` SET " + theString + " WHERE id=" + theLeadid + "";     
                  if (mysql_query(mysql, query.c_str()) != 0)
                  {
                    std:: cerr << "Error Updating" << std::endl;
                  }                  
                  TheQueues.rWhere(theCampaign).IncrementAnsmachs();
                }
                //timed out while ringing, no answer
                if (theReason == "3")
                {
                  theDispo = "-2";
                  theString = "disposition='" + theDispo + "',reason='" + theReasonDesc + "'";
                  query = "UPDATE `" + theCampaign + "` SET " + theString + " WHERE id=" + theLeadid + "";     
                  if (mysql_query(mysql, query.c_str()) != 0)
                  {
                    std:: cerr << "Error Updating" << std::endl;
                  }                  
                }
                //busy
                if (theReason == "5")
                {
                  theDispo = "-4";
                  theString = "disposition='" + theDispo + "',reason='" + theReasonDesc + "'";
                  query = "UPDATE `" + theCampaign + "` SET " + theString + " WHERE id=" + theLeadid + "";     
                  if (mysql_query(mysql, query.c_str()) != 0)
                  {
                    std:: cerr << "Error Updating" << std::endl;
                  }                  
                }
                //hangup, no answer
                if (theReason == "1")
                {
                  theDispo = "-2";
                  theString = "disposition='" + theDispo + "',reason='" + theReasonDesc + "'";
                  query = "UPDATE `" + theCampaign + "` SET " + theString + " WHERE id=" + theLeadid + "";     
                  if (mysql_query(mysql, query.c_str()) != 0)
                  {
                    std:: cerr << "Error Updating" << std::endl;
                  }                  
                }
                //congestion
                if (theReason == "8")
                {
                  theDispo = "-5";
                  theString = "disposition='" + theDispo + "',reason='" + theReasonDesc + "'";
                  query = "UPDATE `" + theCampaign + "` SET " + theString + " WHERE id=" + theLeadid + "";     
                  if (mysql_query(mysql, query.c_str()) != 0)
                  {
                    std:: cerr << "Error Updating" << std::endl;
                  }                  
                }
                result = mysql_use_result(mysql);
                mysql_free_result(result);

                {
                  if (attempts > 0)
                  {
                    if (iTheAttempts == 1)
                    {
                      query = "UPDATE `" + theCampaign + "` SET " + "reason1='" + theReasonDesc + "',datetime1=datetime" + " WHERE id=" + theLeadid + "";     
                      if (mysql_query(mysql, query.c_str()) != 0)
                      {
                        if (gDebug)
                        std::cout << "Error Updating " << query << std::endl;
                      }                      
                    }
                    if (iTheAttempts == 2)
                    {
                      query = "UPDATE `" + theCampaign + "` SET " + "reason2='" + theReasonDesc + "',datetime2=datetime" + " WHERE id=" + theLeadid + "";     
                      if (mysql_query(mysql, query.c_str()) != 0)
                      {
                        if (gDebug)
                        std::cout << "Error Updating " << query << std::endl;
                      }                      
                    }
                  }
                }  

                if (theDispo == "0")
                {
                  if (doColorize)
                  {
                    std::
                      cout << theCampaign << fg_light_red <<
                      ": OriginateSuccess - UNKNOWN REASON - " << theReason <<
                      normal << std::endl;
                  }
                  else
                  {
                    std::
                      cout << theCampaign <<
                      ": OriginateSuccess - UNKNOWN REASON - " << theReason <<
                      std::endl;
                  }
                  if (gLog)
                  {
                    writeDialerLog(theCampaign +
                      ": OriginateSuccess - UNKNOWN REASON - theLeadid: " +
                      theLeadid + " theReason: " + theReason + " theDispo: " +
                      theDispo);
                  }
                }

                //stats testing
                TheQueues.rWhere(theCampaign).WriteAbn();

                if (gDebug)
                {
                  if (doColorize)
                  {
                    std::
                      cout << theCampaign << fg_green <<
                      ": OriginateSuccess - theLeadid: " << theLeadid <<
                      " theReason: " << theReason << " theDispo: " << theDispo
                      << " (" << dispo2long(stoi(theDispo)) << ") " << normal <<
                      std::endl;
                  }
                  else
                  {
                    std::
                      cout << theCampaign << ": OriginateSuccess - theLeadid: "
                      << theLeadid << " theReason: " << theReason <<
                      " theDispo: " << theDispo << " (" <<
                      dispo2long(stoi(theDispo)) << ") " << std::endl;
                  }

                }
                if (gLog)
                {
                  writeDialerLog(theCampaign +
                    ": OriginateSuccess - theLeadid: " + theLeadid +
                    " theReason: " + theReason + " theDispo: " + theDispo);
                }
              }
              else
              {
                if (doColorize)
                {
                  std::
                    cout << fg_light_red << "OriginateSuccess: PARSE ERROR " <<
                    normal << std::endl;
                }
                else
                {
                  std::cout << "OriginateSuccess: PARSE ERROR " << std::endl;
                }
                if (gLog)
                {
                  writeDialerLog
                    ("OriginateSuccess - PARSE ERROR - Something Empty (block: "
                    + block + ")");
                }
              }
            }
            else
            {
              if (gLog)
              {
                writeDialerLog
                  ("OriginateSuccess - PARSE ERROR - No CallerIDName or Empty Reason (block: \n"
                  + block + ")");
              }
            }
          }

      if (gDebug)
      {
        gettimeofday(&tv_recv, NULL);

        long elapsed = (tv_recv.tv_sec-tv_lastrecv.tv_sec)*1000000 + tv_recv.tv_usec-tv_lastrecv.tv_usec;

        std::cout << "READ AsteriskManager MARK1.4 " << elapsed << "us" << std::endl;
      }


          //***********************************************************************************
          if (block.find("Event: Newstate", 0) != std::string::npos &&
            block.find("CallerIDName: ", 0) != std::string::npos && 
            block.find("Uniqueid: ", 0) != std::string::npos) 
          {
            std::string theUniqueid, theCallerIDName, theCampaign, theLeadid;

            if (block.find("CallerIDName: ", 0) != std::string::npos &&
              block.find("~", 0) != std::string::npos)
            {
              pos = block.find("CallerIDName: ", 0) + 15;
              end = block.find("\n", pos);
              theCallerIDName = block.substr(pos, end - pos);

              if (block.find("~", 0) != std::string::npos)
              {
                pos = theCallerIDName.find("~", end) + 1;
                end = theCallerIDName.find("-", pos + 1);

                pos2 = end + 1;
                end2 = theCallerIDName.find("-", pos2);

                theCampaign = theCallerIDName.substr(pos, end - pos);
                theLeadid = theCallerIDName.substr(pos2, end2 - pos2);
              }
            }          
            else
            {
              if (gLog)
              {
                writeDialerLog
                  ("Newevent - No CallerIDName (block: \n"
                  + block + ")");
              }
            }
            
            if (block.find("Uniqueid: ", 0) != std::string::npos)
            {
              pos = block.find("Uniqueid: ", 0) + 10;
              end = block.find("\n", pos);
              theUniqueid = block.substr(pos, end - pos);              
            }          
            else
            {
              if (gLog)
              {
                writeDialerLog
                  ("Newevent - PARSE ERROR - No Uniqueid (block: "
                  + block + ")");
              }
            }            
                    
            {

              if (!theCampaign.empty() && !theLeadid.empty() && !theUniqueid.empty() &&
                TheQueues.exists(theCampaign))
              {                                            
                TheCallCache->SetUniqueid(theCampaign, theLeadid, theUniqueid);

                if (gDebug)
                {
                  if (doColorize)
                  {
                    std::
                      cout << theCampaign << fg_green <<
                      ": Newevent - theLeadid: " << theLeadid <<
                      " theUniqueid: " << theUniqueid << normal <<
                      std::endl;
                  }
                  else
                  {
                    std::
                      cout << theCampaign << ": Newevent - theLeadid: "
                      << theLeadid << " theUniqueid: " << theUniqueid << std::endl;
                  }

                }
                
                if (gLog)
                {
                  writeDialerLog(theCampaign +
                    ": Newevent - theLeadid: " + theLeadid +
                    " theUniqueID: " + theUniqueid);
                }                
                               
              }
            }

          }

      if (gDebug)
      {
        gettimeofday(&tv_recv, NULL);

        long elapsed = (tv_recv.tv_sec-tv_lastrecv.tv_sec)*1000000 + tv_recv.tv_usec-tv_lastrecv.tv_usec;

        std::cout << "READ AsteriskManager MARK1.4.1 " << elapsed << "us" << std::endl;
      }


          //***********************************************************************************
          //if (block.find("Event: Hangup", 0) != std::string::npos &&
          //  block.find("CallerIDName: ", 0) != std::string::npos)
          if (block.find("Event: Hangup", 0) != std::string::npos &&
            block.find("Cause: ", 0) != std::string::npos)
          //if (!clip)
          {
            //if (block.find("Event: OriginateSuccess",0) != std::string::npos) {
            int iTheCause = 0;
            int iTheAttempts = 0;            
            std::string theCause, theCallerIDName, theCampaign, theLeadid, theUniqueid, theString, theAttempts;
            if (block.find("Cause: ", 0) != std::string::npos)
            {
              pos = block.find("Cause: ", 0) + 7;
              end = block.find("\n", pos);
              theCause = block.substr(pos, end - pos);
              iTheCause = atoi(theCause.c_str());

              writeDialerLog
                ("Hangup - TheCause="+theCause);

                if (gDebug)
                {
                  std::cout << "TheCause " << iTheCause << std::endl;
                }
            }
            if (block.find("Cause-txt: ", 0) != std::string::npos)
            {
              pos = block.find("Cause-txt: ", 0) + 11;
              end = block.find("\n", pos);
              theCause = block.substr(pos, end - pos);
            }
            if (block.find("CallerIDName: ", 0) != std::string::npos &&
              block.find("~", 0) != std::string::npos && !theCause.empty())
            {
              pos = block.find("CallerIDName: ", 0) + 15;
              end = block.find("\n", pos);
              theCallerIDName = block.substr(pos, end - pos);

              if (block.find("~", 0) != std::string::npos)
              {
                pos = theCallerIDName.find("~", end) + 1;
                end = theCallerIDName.find("-", pos + 1);

                pos2 = end + 1;
                end2 = theCallerIDName.find("-", pos2);

                theCampaign = theCallerIDName.substr(pos, end - pos);
                theLeadid = theCallerIDName.substr(pos2, end2 - pos2);

                writeDialerLog
                  ("Hangup - theLeadid="+theLeadid);
                writeDialerLog
                  ("Hangup - theCampaign"+theCampaign);


              }
              else
              {
                writeDialerLog
                  ("Hangup - PARSE ERROR - CallerIDName invalid");
              }

            }
            else
            {
              if (gLog)
              {
                writeDialerLog
                  ("Hangup - No CallerIDName (block: \n"
                  + block + ")");
              }
            }
            
            if (block.find("Uniqueid: ", 0) != std::string::npos && 
              //theCallerIDName.empty())
              theLeadid.empty())
            {
              pos = block.find("Uniqueid: ", 0) + 10;
              end = block.find("\n", pos);
              theUniqueid = block.substr(pos, end - pos);

                writeDialerLog
                  ("Hangup - Uniqueid = "
                  + theUniqueid + "");

              
              theCampaign = TheCallCache->GetCampaign(theUniqueid);
              theLeadid = TheCallCache->GetLeadId(theUniqueid);              
            }
            else
            {
              if (gLog)
              {
                writeDialerLog
                  ("Hangup - PARSE ERROR - No Uniqueid (block: \n"
                  + block + ")");
              }
            }
            
            {
              if (!theCampaign.empty() && !theLeadid.empty() &&
                TheQueues.exists(theCampaign))
              {

      if (gDebug)
      {
        gettimeofday(&tv_recv, NULL);

        long elapsed = (tv_recv.tv_sec-tv_lastrecv.tv_sec)*1000000 + tv_recv.tv_usec-tv_lastrecv.tv_usec;

        std::cout << "READ AsteriskManager MARK1.4.2 " << elapsed << "us" << std::endl;
      }

                if (attempts > 1)
                {
                query = "SELECT attempts FROM `" + theCampaign + "` WHERE id=" + theLeadid + "";   
                if (mysql_query(mysql, query.c_str()) == 0)
                {
                  result = mysql_use_result(mysql);
                  row = mysql_fetch_row(result);
                  if (row)
                  iTheAttempts = atoi(row[0]);
                  else
                  iTheAttempts = -1;
                  
                  mysql_free_result(result);
                  if (gDebug)
                  std::cout << "Attempts = " << iTheAttempts << std::endl;
                }                            
                else
                iTheAttempts = 0;
                }

      if (gDebug)
      {
        gettimeofday(&tv_recv, NULL);

        long elapsed = (tv_recv.tv_sec-tv_lastrecv.tv_sec)*1000000 + tv_recv.tv_usec-tv_lastrecv.tv_usec;

        std::cout << "READ AsteriskManager MARK1.4.3 " << elapsed << "us" << std::endl;
      }
                // answer, we'll assume voicemail/answering machine
                // if it passes talkdetect it will be sent to agent
                // or abandons++                             
                
                theString = "cause='" + theCause +"'";
                
                query = "UPDATE `" + theCampaign + "` SET " + theString + " WHERE id=" + theLeadid + "";     
                if (mysql_query(mysql, query.c_str()) != 0)
                {
                  std:: cerr << "Error Updating" << std::endl;
                } 

                TheCallCache->RemoveCall(theCampaign, theLeadid);

                //stats testing
                TheQueues.rWhere(theCampaign).WriteAbn();

                if (gDebug)
                {
                  if (doColorize)
                  {
                    std::
                      cout << theCampaign << fg_green <<
                      ": Hangup - theLeadid: " << theLeadid <<
                      " theCause: " << theCause << normal <<
                      std::endl;
                  }
                  else
                  {
                    std::
                      cout << theCampaign << ": Hangup - theLeadid: "
                      << theLeadid << " theCause: " << theCause << std::endl;
                  }

                }
                
                if (gLog)
                {
                  writeDialerLog(theCampaign +
                    ": Hangup - theLeadid: " + theLeadid +
                    " theCause: " + theCause);
                }                
                
                result = mysql_use_result(mysql);
                mysql_free_result(result);

                {
                  if (attempts > 0)
                  {
                    if (iTheAttempts == 1)
                    {
                      query = "UPDATE `" + theCampaign + "` SET " + "cause1='" + theCause + "',datetime1=NOW()" + " WHERE id=" + theLeadid + "";
                      if (mysql_query(mysql, query.c_str()) != 0)
                      {
                        if (gDebug)
                        std::cout << "Warning Updating " << query << std::endl;
                      }                      
                    }
                    if (iTheAttempts == 2)
                    {
                      query = "UPDATE `" + theCampaign + "` SET " + "cause2='" + theCause + "',datetime2=NOW()" + " WHERE id=" + theLeadid + "";
                      if (mysql_query(mysql, query.c_str()) != 0)
                      {

                        if (gDebug)
                        std::cout << "Warning Updating " << query << std::endl;
                      }                      
                    }
                  }
                }                  
              }
              else
              {
                if (doColorize)
                {
                  std::
                    cout << fg_light_red << "Hangup: PARSE ERROR " <<
                    normal << std::endl;
                }
                else
                {
                  std::cout << "Hangup: PARSE ERROR " << std::endl;
                }
                if (gLog)
                {
                  writeDialerLog
                    ("Hangup - PARSE ERROR - Something Empty (block: "
                    + block + ")");
                }
              }
            }
            
          }
          
          
      if (gDebug)
      {
        gettimeofday(&tv_recv, NULL);

        long elapsed = (tv_recv.tv_sec-tv_lastrecv.tv_sec)*1000000 + tv_recv.tv_usec-tv_lastrecv.tv_usec;

        std::cout << "READ AsteriskManager MARK1.5 " << elapsed << "us" << std::endl;
      }

          //***********************************************************************************
          if ((block.find("Event: UserEventPickup", 0) != std::string::npos) ||
            (block.find("UserEvent: Pickup", 0) != std::string::npos))
          if (!clip)
          {
            std::string theCallerIDName, theCampaign, theLeadid, theString;
            DBG_TRACE(DBG_ASTERISK, (0, "< Event: UserEventPickup detected !"));
            if (gDebug)
            {
              std::cout << "UserEvent - Pickup ";
            }

            if (block.find("CallerIDName: ", 0) != std::string::npos &&
              block.find("~", 0) != std::string::npos)
            {
              pos = block.find("CallerIDName: ", 0) + 15;
              end = block.find("\n", pos);
              theCallerIDName = block.substr(pos, end - pos);
              DBG_TRACE(DBG_ASTERISK, (0, "theCallerIDName : %s",
                  theCallerIDName.c_str()));

              pos = theCallerIDName.find("~", end) + 1;
              end = theCallerIDName.find("-", pos + 1);

              pos2 = end + 1;
              end2 = theCallerIDName.find("-", pos2);

              theCampaign = theCallerIDName.substr(pos, end - pos);
              if (gDebug)
              {
                std::cout << " theCampaign: " << theCampaign;
              }

              DBG_TRACE(DBG_ASTERISK, (0, "theCampaign: %s",
                  theCampaign.c_str()));

              theLeadid = theCallerIDName.substr(pos2, end2 - pos2);
              if (gDebug)
              {
                std::cout << " theLeadid: " << theLeadid;
              }

              DBG_TRACE(DBG_ASTERISK, (0, "theLeadid: %s", theLeadid.c_str()));

              if (gDebug)
              {
                std::cout << std::endl;
              }

              if (!theCampaign.empty() && !theLeadid.empty() &&
                TheQueues.exists(theCampaign))
              {                                                          
                theString = "disposition='6',pickups=pickups+1";
                query = "UPDATE `" + theCampaign + "` SET " + theString + " WHERE id=" + theLeadid + "";     
                if (mysql_query(mysql, query.c_str()) != 0)
                {
                  std:: cerr << "Error Updating" << std::endl;
                }                                               
                TheCallCache->SetAnswered(theCampaign, theLeadid);
                                    
                if (gDebug)
                {
                  std::
                    cout << theCampaign << ": writeDBString - Pickup " << std::
                    endl;
                }
                if (gLog)
                {
                  writeDialerLog(theCampaign + ": theLeadid - " + theLeadid +
                    " was picked-up");
                }
              }
              else
              {
                DBG_TRACE(DBG_ASTERISK, (0, "Parse ERROR"));
                if (gDebug)
                {
                  std::cout << "UserEventPickup: Parse ERROR " << std::endl;
                }
              }
            }
          }

      if (gDebug)
      {
        gettimeofday(&tv_recv, NULL);

        long elapsed = (tv_recv.tv_sec-tv_lastrecv.tv_sec)*1000000 + tv_recv.tv_usec-tv_lastrecv.tv_usec;

        std::cout << "READ AsteriskManager MARK1.6 " << elapsed << "us" << std::endl;
      }


          //***********************************************************************************
          if ((block.find("Event: UserEventHangup", 0) != std::string::npos) ||
            (block.find("UserEvent: Hangup", 0) != std::string::npos))
          if (!clip)
          {
            std::string theCallerIDName, theCampaign, theDuration, theResult, theLeadid, theString;
            DBG_TRACE(DBG_ASTERISK, (0, "< Event: UserEventHangup detected !"));
            if (gDebug)
            {
              std::cout << "UserEvent - Hangup ";
            }

            if (block.find("CallerIDName: ", 0) != std::string::npos &&
              block.find("~", 0) != std::string::npos)
            {
              pos = block.find("CallerIDName: ", 0) + 15;
              end = block.find("\n", pos);
              theCallerIDName = block.substr(pos, end - pos);
              DBG_TRACE(DBG_ASTERISK, (0, "theCallerIDName : %s",
                  theCallerIDName.c_str()));

              pos = theCallerIDName.find("~", end) + 1;
              end = theCallerIDName.find("-", pos + 1);

              pos2 = end + 1;
              end2 = theCallerIDName.find("-", pos2);

              theCampaign = theCallerIDName.substr(pos, end - pos);
              if (gDebug)
              {
                std::cout << " theCampaign: " << theCampaign;
              }

              DBG_TRACE(DBG_ASTERISK, (0, "theCampaign: %s",
                  theCampaign.c_str()));

              theLeadid = theCallerIDName.substr(pos2, end2 - pos2);
              if (gDebug)
              {
                std::cout << " theLeadid: " << theLeadid;
              }

              DBG_TRACE(DBG_ASTERISK, (0, "theLeadid: %s", theLeadid.c_str()));

              if (gDebug)
              {
                std::cout << std::endl;
              }

              if (!theCampaign.empty() && !theLeadid.empty() &&
                TheQueues.exists(theCampaign))
              {
                theString = "disposition='7'";
                query = "UPDATE `" + theCampaign + "` SET " + theString + " WHERE id=" + theLeadid + "";     

                if (mysql_query(mysql, query.c_str()) != 0)
                {
                  std:: cerr << "Error Updating" << std::endl;
                }    
                                                                                                 
                //TheCallCache->RemoveCall(theCampaign, theLeadid);
                
                if (gDebug)
                {
                  std::
                    cout << theCampaign << ": writeDBString - Hangup " << std::
                    endl;
                }
                if (gLog)
                {
                  writeDialerLog(theCampaign + ": theLeadid - " + theLeadid +
                    " was hanged-up");
                }
              }
              else
              {
                DBG_TRACE(DBG_ASTERISK, (0, "Parse ERROR"));
                if (gDebug)
                {
                  std::cout << "UserEventHangup: Parse ERROR " << std::endl;
                }
              }
            }

            if (block.find("Duration: ",0) != std::string::npos)
            {
              pos = block.find("Duration: ",0) + 10;
              end = block.find("\n",pos);
              theDuration = block.substr(pos,end-pos);
              
              theString = "duration='" + theDuration + "'";
              query = "UPDATE `" + theCampaign + "` SET " + theString + " WHERE id=" + theLeadid + "";     
              if (mysql_query(mysql, query.c_str()) != 0)
              {
                std:: cerr << "Error Updating" << std::endl;
              }                  
              
              if (gDebug)
              {
                std::cout << " Duration: " << theDuration << std::endl;
              }
            }            
            
            if (block.find("Result: ",0) != std::string::npos)
            {
              pos = block.find("Result: ",0) + 8;
              end = block.find("\n",pos);
              theResult = block.substr(pos,end-pos);
              theString = "result='" + theResult + "'";              
              query = "UPDATE `" + theCampaign + "` SET " + theString + " WHERE id=" + theLeadid + "";     
              if (mysql_query(mysql, query.c_str()) != 0)
              {
                std:: cerr << "Error Updating" << std::endl;
              }                  

              
              if (gDebug)
              {
                std::cout << " Result: " << theResult << std::endl;
              }
            }            
          }

      if (gDebug)
      {
        gettimeofday(&tv_recv, NULL);

        long elapsed = (tv_recv.tv_sec-tv_lastrecv.tv_sec)*1000000 + tv_recv.tv_usec-tv_lastrecv.tv_usec;

        std::cout << "READ AsteriskManager MARK1.7 " << elapsed << "us" << std::endl;
      }

          
          //***********************************************************************************
          if ((block.find("Event: UserEventAction", 0) != std::string::npos) ||
            (block.find("UserEvent: Action", 0) != std::string::npos))
          if (!clip)
          {
            std::string theCallerIDName, theCampaign, theDuration, theResult, theLeadid, theString;
            DBG_TRACE(DBG_ASTERISK, (0, "< Event: UserEventAction detected !"));
            if (gDebug)
            {
              std::cout << "UserEvent - Action ";
            }

            if (block.find("CallerIDName: ", 0) != std::string::npos &&
              block.find("~", 0) != std::string::npos)
            {
              pos = block.find("CallerIDName: ", 0) + 15;
              end = block.find("\n", pos);
              theCallerIDName = block.substr(pos, end - pos);
              DBG_TRACE(DBG_ASTERISK, (0, "theCallerIDName : %s",
                  theCallerIDName.c_str()));

              pos = theCallerIDName.find("~", end) + 1;
              end = theCallerIDName.find("-", pos + 1);

              pos2 = end + 1;
              end2 = theCallerIDName.find("-", pos2);

              theCampaign = theCallerIDName.substr(pos, end - pos);
              if (gDebug)
              {
                std::cout << " theCampaign: " << theCampaign;
              }

              DBG_TRACE(DBG_ASTERISK, (0, "theCampaign: %s",
                  theCampaign.c_str()));

              theLeadid = theCallerIDName.substr(pos2, end2 - pos2);
              if (gDebug)
              {
                std::cout << " theLeadid: " << theLeadid;
              }

              DBG_TRACE(DBG_ASTERISK, (0, "theLeadid: %s", theLeadid.c_str()));

              if (gDebug)
              {
                std::cout << std::endl;
              }

              if (!theCampaign.empty() && !theLeadid.empty() &&
                TheQueues.exists(theCampaign))
              {
              }
              else
              {
                DBG_TRACE(DBG_ASTERISK, (0, "Parse ERROR"));
                if (gDebug)
                {
                  std::cout << "UserEventResult: Parse ERROR " << std::endl;
                }
              }
            }
            
            if (block.find("Result: ",0) != std::string::npos)
            {
              pos = block.find("Result: ",0) + 8;
              end = block.find("\n",pos);
              theResult = block.substr(pos,end-pos);
              theString = "result='" + theResult + "'";              
              query = "UPDATE `" + theCampaign + "` SET " + theString + " WHERE id=" + theLeadid + "";     
              if (mysql_query(mysql, query.c_str()) != 0)
              {
                std:: cerr << "Error Updating" << std::endl;
              }                  

              
              if (gDebug)
              {
                std::cout << " Result: " << theResult << std::endl;
              }
            }            
          }

      if (gDebug)
      {
        gettimeofday(&tv_recv, NULL);

        long elapsed = (tv_recv.tv_sec-tv_lastrecv.tv_sec)*1000000 + tv_recv.tv_usec-tv_lastrecv.tv_usec;

        std::cout << "READ AsteriskManager MARK1.8 " << elapsed << "us" << std::endl;
      }

          //***********************************************************************************
          // End block analysis
          block = "";
        }
        else
        {
          block += tempLine + "\n";
        }
      }

      if (gDebug)
      {
        std::cout << "Dialing loop for " << TheQueues.
          size() << " Campaigns." << std::endl;
      }
      
      // DIALING LOOP
      now = time(NULL);

      if (now==lasttime)
      {
        JobToDo = 1;
      }
      else
      for (int i = 0; i < TheQueues.size(); i++)
      {
        lasttime=now;

        queue = TheQueues.at(i).GetName();

				if (gDebug)
        std::cout << "Processing queue: " << queue << std::endl;

				if (gDebug)
        std::cout << "Queue active: " << TheQueues.at(i).GetSetting("active").Get() << std::endl;

        if (TheQueues.at(i).GetSetting("active").Get() == "true")
        {
          if (gDebug)
          std::cout << "active queue: " << queue << std::endl;

          if (gDebug)
          {
            std::cout << tempCheckCampaign << ": Get infos " << std::endl;
          }

          maxlines = TheQueues.at(i).GetSetting("maxlines").GetInt();
          
          //clipping option override global -k
          if (TheQueues.at(i).SettingExists("clipping")){
            clip = TheQueues.at(i).GetSetting("clipping").GetBool();
            if (gDebug &&  clip )
              std::cout << "Dialer: CLIP enabled!" << std::endl;
            if (gDebug && !clip )
              std::cout << "Dialer: CLIP disabled!" << std::endl;
          }


          if (TheQueues.at(i).SettingExists("maxcaps"))
          maxcaps = TheQueues.at(i).GetSetting("maxcaps").GetInt();
          if (maxcaps <= 0)
          maxcaps = maxlines;

          if (gDebug)
          {
            std::cout << tempCheckCampaign << ": Get infos (1) " << std::endl;
          }

          timestart = TheQueues.at(i).GetSetting("timestart").GetInt();
          timestop = TheQueues.at(i).GetSetting("timestop").GetInt();

          mode = TheQueues.at(i).GetSetting("mode").Get();
          calltoday = TheQueues.at(i).GetSetting("calltoday").Get();
          blacklist = TheQueues.at(i).GetSetting("blacklist").Get();
          callerid = TheQueues.at(i).GetSetting("callerid").Get();
          if (TheQueues.at(i).SettingExists("url"))
          url = TheQueues.at(i).GetSetting("url").Get();
          else
          url=callerid;          
          filter = TheQueues.at(i).GetSetting("filter").Get();
          timeout = TheQueues.at(i).GetSetting("timeout").GetInt();
          dialformat = TheQueues.at(i).GetSetting("dialformat").Get();
          attempts = TheQueues.at(i).GetSetting("attempts").GetInt();
          bool debug = TheQueues.at(i).GetSetting("debug").GetBool();
          skip = 0; //TheQueues.at(i).GetSetting("skip").GetInt();
          multiplecalls = TheQueues.at(i).GetSetting("multiplecalls").Get();
          orderby = TheQueues.at(i).GetSetting("orderby").Get();
          attemptsdelay = TheQueues.at(i).GetSetting("attemptsdelay").GetInt();
          ringonly = TheQueues.at(i).GetSetting("ringonly").GetBool();

          extravars.clear();

          for (int c = 0; c < TheQueues.at(i).OccurencesOf("chanvars"); c++)
          {
            std::string cnum =
              TheQueues.at(i).GetSetting(c, "chanvars").GetAttribute("number");
            std::string cvar =
              TheQueues.at(i).GetSetting(c, "chanvars").GetAttribute("var");
            std::string cstring =
              TheQueues.at(i).GetSetting(c, "chanvars").GetAttribute("string");
            std::string cenabled =
              TheQueues.at(i).GetSetting(c, "chanvars").GetAttribute("enable");

            if (cenabled == "true")
            {
              extravars += "Variable: __" + cvar + "=" + cstring + "\r\n";
            }
          }

          chanvar1 = TheQueues.at(i).GetSetting("chanvar1").Get();
          if (!chanvar1.empty())
          {
            extravars += "Variable: __" + chanvar1 + "\r\n";
          }

          chanvar2 = TheQueues.at(i).GetSetting("chanvar2").Get();
          if (!chanvar2.empty())
          {
            extravars += "Variable: __" + chanvar2 + "\r\n";
          }

          sipheader1 = TheQueues.at(i).GetSetting("sipheader1").Get();
          if (!sipheader1.empty())
          {
            extravars += "Variable: __SIPADDHEADER1=" + sipheader1 + "\r\n";
          }

          sipheader2 = TheQueues.at(i).GetSetting("sipheader2").Get();
          if (!sipheader2.empty())
          {
            extravars += "Variable: __SIPADDHEADER2=" + sipheader2 + "\r\n";
          }


          //*****************************************************************************************

          //put these down here so that most important ones get processed last
          calls = atoi(TheQueues.at(i).GetCalls().c_str());
          linesdialing = TheCallCache->LinesDialing(queue);
          linestodial = maxlines - linesdialing;
          if (linestodial > maxcaps)
          linestodial = maxcaps;

          if (gDebug)
          {
            std::cout << "  debug: " << debug << std::endl;
            std::cout << "  maxlines: " << maxlines << std::endl;
            std::cout << "  dialformat: " << dialformat << std::endl;
            std::cout << "  currentTime: " << currentTime << std::endl;
            std::
              cout << "  timeSinceLastQueueUpdate: " << timeSinceLastQueueUpdate
              << std::endl;
            std::cout << "  linestodial: " << linestodial << std::endl;
            std::cout << "  linesdialing: " << linesdialing << std::endl;
          }

          if (gDebug)
          TheCallCache->DumpAll();

          //end testing area
          //********************************************************************************************8
          //

          if (linestodial >= 5000 || linesdialing >= 5000)
          {
            std::
              cout << queue <<
              ": linestodial or linesdialing are greater than 5000, something is WRONG!"
              << std::endl;
            std::
              cout << queue << ": ldg: " << linesdialing << " mr: " << maxlines
              << " mode: " << mode << " calls: " << calls << " l2d: " << linestodial 
              << " ld: " << linesdialing << std::endl;
          }
          if (debugCampaignSettings)
          {
            std::
              cout << queue << ": ldg: " << linesdialing << " mr: " << maxlines
              << " mode: '" << mode << "' calls: " << calls << " l2d: " << linestodial 
              << " ld: " << linesdialing << std::endl;
          }

          //std::cout << queue << ": processing this campaign" << std::endl;
          
          time_t now;
          struct tm* tm_info;

          time(&now);
          tm_info = localtime(&now);

          //MAIN SECTION for dialing calls for a particular campaign
          if (linestodial)
          if (timestart<=tm_info->tm_hour && timestop>tm_info->tm_hour)
          {
            //this is just a base to get the building of the query string going
            query = "SELECT DISTINCT id, phone, param FROM `" + queue + "` WHERE 1 ";

            //this allows a record to be called back multiple times the same day
            //without needing to be a specific callback (like closer callback)
            //be careful, if your viable data is running low this will loop thru
            //and burn all your data (very fast), and piss off alot of people!
            if (calltoday != "true")
            {
              query +=
                " AND (LEFT(lastupdated,10) = LEFT(NOW(),10) AND disposition = 1) OR LEFT(lastupdated,10) <> LEFT(NOW(),10) ";
            }

            //this is an extra filter if you want to attempt to call a specific data subset
            //(this also lets you test a filter before adding it to the primary filters)
            if (filter.empty() == false && filter != "0" && filter != "None" &&
              filter != "none")
            {
              if (gDebug)
              {
                std::cout << queue << ": filter - " << filter << std::endl;
              }
              query += " AND " + filter;
            }
            else
            {
              if (gDebug)
              {
                std::cout << queue << ": attempts - " << attempts << std::endl;
              }
              query += " AND ((disposition > -6 AND disposition < 6) AND attempts < " + itos(attempts) + ")";             
            }

            query += " AND ((datetime=0) OR (ADDDATE(datetime, INTERVAL " + itos(attemptsdelay) +" SECOND)<CURRENT_TIMESTAMP))";

            //these are the primary filters, it will default to filter 0 on startup
            //so make sure that is your 'fresh + main' calling data

            int y = 0;
            for (int x = 0; x < TheQueues.at(i).OccurencesOf("filters"); x++)
            {            
              std::string fnum, fstring, enabled;
              fnum =
                TheQueues.at(i).GetSetting(x, "filters").GetAttribute("number");
              fstring =
                TheQueues.at(i).GetSetting(x, "filters").GetAttribute("string");
              enabled =
                TheQueues.at(i).GetSetting(x, "filters").GetAttribute("enable");
              if (enabled == "true")
              {
                if (y == 0)
                query += " AND (";

                y++;
                if (y > 1)
                {
                  query += " OR ";
                }
                if (gDebug)
                {
                  std::cout << queue << ": filter - " << fstring << std::endl;
                }
                query += fstring;
              }
            }            
            if (y > 0)
            query += ") ";

    
            //areacode based filter
            /*
               if (f_areacode.empty() == false && f_areacode != "0") {
               if (gDebug) {
               std::cout << queue << ": f_areacode - " << f_areacode << std::endl;
               }
               query += " AND LEFT(phone,3)='" + f_areacode + "'";
               }
             */

            //this is a realtime 'do not call' filter that does a lookup against dialer.DNC table when calling
            //this is resource intensive so DO NOT load dialer.DNC with alot of numbers
            //i suggest you 'pre-process' your calling data against DNC numbers, as you dial, dispo 8's
            //will be added to dialer.DNC, so you can make sure this campaign you don't call them back
            //then at the end of a campaign, add them to your main set of DNC's to once again
            //'pre-process' against your main calling data for the next campaign.
            if (blacklist != "none")
            {
              query += " AND phone NOT IN (SELECT phone FROM `" + blacklist + "`) ";
            }

            //original order by line, keeping till i work all the bugs out of the multiple
            //order by methods below
            //query += " ORDER BY attempts + pickups ASC LIMIT " + itos(skip) + "," + itos(linestodial);

            //SELECT DISTINCT id,phone,lastupdated FROM 308CLD05 WHERE (id > 0)
            //AND (LEFT(lastupdated,10) = LEFT(NOW(),10) AND disposition = 1)
            //OR (LEFT(lastupdated,10) <> LEFT(NOW(),10)) AND
            //((disposition > -6 AND disposition < 6) AND attempts < 3)
            //ORDER BY id,id,lastupdated ASC LIMIT 0,3;

            if (orderby == "id")
            {
              query += " ORDER BY id,lastupdated ASC ";
            }
            else
            if (orderby == "phone")
            {
              query += " ORDER BY phone,id,lastupdated ASC ";
            }
            else
            {
              query += " ORDER BY attempts+pickups,id,lastupdated ASC ";
            }

            //self explanitory
            query += " LIMIT " + itos(skip) + "," + itos(linestodial);

            if (gDebug)
            {
              std::cout << queue << ": query - " << query << std::endl;
            }

            if (gDebug)
            {
              std::
                cout << queue << ": Dialing " << linestodial << " calls (" <<
                skip << ") skipped" << std::endl;
            }

            if (gDebug)
            {
              gettimeofday(&tv_lastsql, NULL);
            }

            if (mysql_query(mysql, query.c_str()) != 0)
            {
              std::
                cerr <<
                "Error selecting leads from mysql! (check table structures)" <<
                std::endl;
              //return 1;
            }
            else
            {

      if (gDebug)
      {
        gettimeofday(&tv_recv, NULL);

        long elapsed = (tv_recv.tv_sec-tv_lastrecv.tv_sec)*1000000 + tv_recv.tv_usec-tv_lastrecv.tv_usec;

        std::cout << "READ AsteriskManager MARK1 " << elapsed << "us" << std::endl;
      }


              if (gDebug)
              {
                gettimeofday(&tv_sql, NULL);

                long elapsed = (tv_sql.tv_sec-tv_lastsql.tv_sec)*1000000 + tv_sql.tv_usec-tv_lastsql.tv_usec;

                std::cout << "MYSQL Query " << query << std::endl;
                std::cout << "MYSQL Query " << elapsed << "us" << std::endl;
              }


              result = mysql_use_result(mysql);              
              query = "UPDATE `" + queue + "` SET attempts=attempts+1,datetime=CURRENT_TIMESTAMP WHERE ";
              int added = 0;
              for (counter = 0; (row = mysql_fetch_row(result)); counter++)
              {
                if ((multiplecalls == "false") &&
                  TheCallCache->IsCalling(queue, row[1]))
                {
                  if (gDebug)
                  std::cerr << "Allready calling " << row[1] << "" << std::endl;
                }
                else
                {
                  char *caller;
                  
                  if (added)
                  {
                    query += " OR ";
                  }
                  query += " id=" + std::string(row[0]);
                  added++;
                  
                  caller = strchr(row[1], ';');
                  if (caller == NULL)
                  caller = strchr(row[1], ',');                  
                  if (caller)
                  *caller++ = 0;
                  else
                  if (callerid == "param")
                  caller = row[2];
                  else
                  caller = (char*)callerid.c_str();

      if (gDebug)
      {
        gettimeofday(&tv_recv, NULL);

        long elapsed = (tv_recv.tv_sec-tv_lastrecv.tv_sec)*1000000 + tv_recv.tv_usec-tv_lastrecv.tv_usec;

        std::cout << "READ AsteriskManager MARK2.1 " << elapsed << "us" << std::endl;
      }

                  
                  TheCallCache->AddCall(queue, row[0], row[1], url, row[2],
                    timeout, dialformat, caller,
                    mode, 
                    extravars);

      if (gDebug)
      {
        gettimeofday(&tv_recv, NULL);

        long elapsed = (tv_recv.tv_sec-tv_lastrecv.tv_sec)*1000000 + tv_recv.tv_usec-tv_lastrecv.tv_usec;

        std::cout << "READ AsteriskManager MARK2.2 " << elapsed << "us" << std::endl;
      }

                }
              }

              if (mysql_errno(mysql))
              {
                std::cerr << "Error fetching rows from mysql!" << std::endl;
                return 1;
              }
              if (!counter)
              {
                if (doColorize)
                {
                  if (gDebug)
                  std::
                    cout << queue << fg_light_red <<
                    ": has ran out of leads! (CHECK YOUR FILTERS!!!)" << normal
                    << std::endl;
                }
                else
                {
                  if (gDebug)
                  std::
                    cout << queue <<
                    ": has ran out of leads! (CHECK YOUR FILTERS!!!)" << std::
                    endl;
                }
              }
              else if (counter < linestodial)
              {
                if (doColorize)
                {
                  if (gDebug)
                  std::
                    cout << queue << fg_light_red <<
                    ": is running very low on leads!" << normal << std::endl;
                }
                else
                {
                  if (gDebug)
                  std::
                    cout << queue << ": is running very low on leads!" << std::
                    endl;
                }
              }

              mysql_free_result(result);
              if (added)
              {
                TheQueues.rWhere(queue).AddCallsDialed(counter);
                TheQueues.rWhere(queue).WriteCalls();
	            if (gDebug)
	            {
                  std::cout << queue << ": JobToDo " << JobToDo << std::endl;
                  std::cout << queue << ": query - " << query << std::endl;
	            }

      if (gDebug)
      {
        gettimeofday(&tv_recv, NULL);

        long elapsed = (tv_recv.tv_sec-tv_lastrecv.tv_sec)*1000000 + tv_recv.tv_usec-tv_lastrecv.tv_usec;

        std::cout << "READ AsteriskManager MARK2.3 " << elapsed << "us" << std::endl;
      }


                if (mysql_query(mysql, query.c_str()) != 0)
                {
                  std::
                    cerr << "Error updating leads in mysql!" << query << std::
                    endl;
                  return 1;
                }

      if (gDebug)
      {
        gettimeofday(&tv_recv, NULL);

        long elapsed = (tv_recv.tv_sec-tv_lastrecv.tv_sec)*1000000 + tv_recv.tv_usec-tv_lastrecv.tv_usec;

        std::cout << "READ AsteriskManager MARK2.4 " << elapsed << "us" << std::endl;
      }

              }
            }
          }
        }
	else
	{
           if (gDebug)
           std::cout << queue << ": Queue not active!" << std::endl;
	}
      }

      if (gDebug)
      {
        gettimeofday(&tv_recv, NULL);

        long elapsed = (tv_recv.tv_sec-tv_lastrecv.tv_sec)*1000000 + tv_recv.tv_usec-tv_lastrecv.tv_usec;

        std::cout << "READ AsteriskManager MARK2 " << elapsed << "us" << std::endl;
      }

      try
      {
        TheCallCache->CallAll(mainHost);
      }
      catch(xOutOfHosts)
      {
        std::cerr << "Exception: Ran out of hosts!" << std::endl;
        return 1;
      }
      catch(xForkError)
      {
        std::cerr << "Exception: Unable to fork the parent process!" << std::
          endl;
        return 1;
      }

      if (gDebug)
      {
        gettimeofday(&tv_recv, NULL);

        long elapsed = (tv_recv.tv_sec-tv_lastrecv.tv_sec)*1000000 + tv_recv.tv_usec-tv_lastrecv.tv_usec;

        std::cout << "READ AsteriskManager MARK3 " << elapsed << "us" << std::endl;
      }

      if (!daemonMode)
      {
      if (TheCallCache->countCalls() == 0)
      {        
        query = "SELECT COUNT(id) FROM `" + queue + "` WHERE 1 ";
        query += " AND ((disposition > -6 AND disposition < 6) AND attempts < " + itos(attempts) + ")";             

        if (gDebug)
        {
          std::cout << queue << ": query - " << query << std::endl;
        }

        if (mysql_query(mysql, query.c_str()) != 0)
        {
          std::cout << "Error checking " << queue << "!" << std::endl;
          JobToDo = 0;
        }
        else
        {
          result = mysql_use_result(mysql);
          row = mysql_fetch_row(result);
          if (row)
          JobToDo = atoi(row[0]);
          else
          JobToDo = 0;
          mysql_free_result(result);
        } 

				if (commandConfigurationFile)
        {
        std::fstream foo;

        foo.open(commandConfigurationFile);

        if (gDebug)
        {
          std::cout << "Check Configuration file" << commandConfigurationFile << std::endl;
				}

        if(foo.is_open() == false)
				{
          std::cout << "Configuration file removed !" << std::endl;
          JobToDo = 0;
				}
        }
            
        if (doColorize)
        {
          if (gDebug)
          std::cout << queue << fg_light_red <<
            ": JobToDo " << JobToDo << normal
            << std::endl;
        }
        else
        {
          if (gDebug)
          std::cout << queue <<
            ": JobToDo " << JobToDo << std::endl;
        }             
      
        //JobToDo = 0;
      }
      else
      {
          if (gDebug)
          std::cout << queue <<
            ": JobToDo " << JobToDo << std::
            endl;
      
      }
      }
    }

    mysql_close(mysql);

    DBG_CLOSE();
  }                             // Not nested for convenience.


  catch(const std::exception & e)
  {
    std::cerr << "Caught Exception: " << e.what() << std::endl;
    return 1;
  }

  return 0;

}
