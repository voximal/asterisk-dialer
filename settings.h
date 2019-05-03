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

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include "options.h"
#include "queue.h"


void addGlobalSettings(std::string context, const char *filename = NULL)
{

  Queue TheQueueGlobals(filename);
  TheQueueGlobals.ParseQueue(context);
  TheQueueGlobals.SupSetting("debug", "false");
  TheQueueGlobals.SupSetting("log", "false");
  TheQueueGlobals.SupSetting("mysqlhost", "localhost");
  TheQueueGlobals.SupSetting("mysqluser", "root");
  TheQueueGlobals.SupSetting("mysqlpassword", "");
  TheQueueGlobals.SupSetting("mysqldatabase", "dialer");
  TheQueueGlobals.SupSetting("asteriskhost", "localhost");
  TheQueueGlobals.SupSetting("asteriskport", "5038");
  TheQueueGlobals.SupSetting("asteriskuser", "dialer");
  TheQueueGlobals.SupSetting("asteriskpassword", "1234");
  TheQueueGlobals.Write();
}

void addBasicSettings(std::string campaign, const char *filename = NULL)
{

  Queue TheQueue(filename);
  TheQueue.ParseQueue(campaign);
  TheQueue.SupSetting("active", "false");
  TheQueue.SupSetting("mode", "voximal");
  TheQueue.SupSetting("timestart", "0");
  TheQueue.SupSetting("timestop", "24");
  TheQueue.SupSetting("calltoday", "false");
  TheQueue.SupSetting("blacklist", "none");
  TheQueue.SupSetting("multiplecalls", "false");
  TheQueue.SupSetting("maxlines", "1");
  TheQueue.SupSetting("attempts", "1");
  TheQueue.SupSetting("attemptsdelay", "600");
  TheQueue.SupSetting("timeout", "20000");
  TheQueue.SupSetting("dialformat", "SIP/%s");
  TheQueue.SupSetting("callerid", "0123456789");
  TheQueue.SupSetting("filter", "none");
  TheQueue.SupSetting("chanvar", "0");
  TheQueue.SupSetting("chanstr", "0");
  TheQueue.SupSetting("debug", "false");
  TheQueue.SupSetting("orderby", "attempts");
  TheQueue.SupSetting("ringonly", "false");
  TheQueue.SupSetting("unanswered", "false");
  TheQueue.SupSetting("chanvar1", "");
  TheQueue.SupSetting("chanvar2", "");
  TheQueue.SupSetting("sipheader1", "");
  TheQueue.SupSetting("sipheader2", "");

  TheQueue.Write();
}
