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
#include <fstream>
#include <stdio.h>
#include <mysql.h>

void __writeCampaignLog(const std::string & thelogstring,
  const std::string & filename)
{

  std::fstream OutFile;
  OutFile.open(filename.c_str(), std::ios::in | std::ios::out);
  OutFile.seekg(0, std::ios::end);
  OutFile << thelogstring << std::endl;
  OutFile.close();

}

void writeCampaignLog(const std::string & campaign,
  const std::string & thelogstring)
{

  __writeCampaignLog(thelogstring, "/var/log/dialer-" + campaign);

}

void __writeDialerLog(const std::string & thelogstring,
  const std::string & filename)
{

  timeval tv;
  gettimeofday(&tv, NULL);
  time_t thetime = tv.tv_sec;
  tm *ptm = localtime(&thetime);
  char mytimestamp[14];
  std::string theTimeStamp;

  sprintf(mytimestamp, "%4.4i%2.2i%2.2i%2.2i%2.2i%2.2i", ptm->tm_year + 1900,
    ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
  theTimeStamp = std::string(mytimestamp);

  std::system(("touch " + filename).c_str());

  std::fstream OutFile;
  OutFile.open(filename.c_str(),
    std::ios::in | std::ios::out | std::ios::binary);
  OutFile.seekg(0, std::ios::end);
  OutFile << theTimeStamp << ": " << thelogstring << std::endl;
  OutFile.close();

}

void writeDialerLog(const std::string & thelogstring)
{
  __writeDialerLog(thelogstring, "/var/log/dialer.log");
}
