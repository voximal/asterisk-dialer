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

#include <string>
#include <iostream>
#include <time.h>
#include "itos.h"

// The purpose of this class is to make it so that
// we can keep track of abandons and calls throughout
// the life of the campaign without having to constantly
// write to queues.conf.  Doing so has proven to be a
// bad idea for a multitude of reasons.

// Each campaign will have an AbnHelper object

#ifndef ABNHELPER
#define ABNHELPER

class AbnHelper
{

public:

  AbnHelper()
  {
  }
  ~AbnHelper()
  {
  }

  void Read(const std::string & name)
  {

    std::string tDateString;

//  time_t thetime;
    timeval tv;
    gettimeofday(&tv, NULL);
    time_t thetime = tv.tv_sec;
    tm *ptm = localtime(&thetime);

    tDateString =
      itos(ptm->tm_mon + 1) + "-" + itos(ptm->tm_mday) + "-" +
      itos(ptm->tm_year + 1900);

    std::ifstream InFile;
    std::string filename = "/tmp/" + name + ".helper." + tDateString;

//        std::system(("touch " + filename).c_str());
//        std::system(("chmod a+rw " + filename).c_str());

//        InFile.open(filename.c_str(),std::ios::app);


    InFile.open(std::string("/tmp/" + name + ".helper" + "." +
        tDateString).c_str());
    if (!InFile)
    {

      calls = "0";
      totalcalls = "0";
      abandons = "0";
      totalabandons = "0";
      datestring = tDateString;
      // add dialer stats
      disconnects = "0";
      noanswers = "0";
      busies = "0";
      congestions = "0";
      ansmachs = "0";

    }
    else
    {

      std::getline(InFile, calls, '\n');
      std::getline(InFile, totalcalls, '\n');
      std::getline(InFile, abandons, '\n');
      std::getline(InFile, totalabandons, '\n');
      std::getline(InFile, datestring, '\n');
      // add dialer stats
      std::getline(InFile, disconnects, '\n');
      std::getline(InFile, noanswers, '\n');
      std::getline(InFile, busies, '\n');
      std::getline(InFile, congestions, '\n');
      std::getline(InFile, ansmachs, '\n');

      //new day reset all the counters
      if (datestring != tDateString)
      {

        calls = "0";
        abandons = "0";
        datestring = tDateString;
        // add dialer stats
        disconnects = "0";
        noanswers = "0";
        busies = "0";
        congestions = "0";
        ansmachs = "0";
      }

    }

  }

  void Write(const std::string & name)
  {

    std::string tDateString;

//      time_t thetime;
    timeval tv;
    gettimeofday(&tv, NULL);
    time_t thetime = tv.tv_sec;
    tm *ptm = localtime(&thetime);

    tDateString =
      itos(ptm->tm_mon + 1) + "-" + itos(ptm->tm_mday) + "-" +
      itos(ptm->tm_year + 1900);

    std::ofstream OutFile;

//        OutFile.open(static_cast<std::string>("/tmp/" + name + ".helper").c_str());
    OutFile.open(std::string("/tmp/" + name + ".helper" + "." +
        tDateString).c_str());

    if (!OutFile)
    {
      std::cerr << "Error writing to abandon helper file!" << std::endl;
    }
    else
    {

      OutFile << calls << std::endl;
      OutFile << totalcalls << std::endl;
      OutFile << abandons << std::endl;
      OutFile << totalabandons << std::endl;
      OutFile << datestring << std::endl;
      // add dialer stats
      OutFile << disconnects << std::endl;
      OutFile << noanswers << std::endl;
      OutFile << busies << std::endl;
      OutFile << congestions << std::endl;
      OutFile << ansmachs << std::endl;

      OutFile.close();
      std::string filename = "/tmp/" + name + ".helper." + tDateString;
      std::system(("chmod a+rw " + filename).c_str());
    }
  }

  const std::string & GetCalls() const
  {
    return calls;
  }
  const std::string & GetTotalCalls() const
  {
    return totalcalls;
  }
  const std::string & GetAbandons() const
  {
    return abandons;
  }
  const std::string & GetTotalAbandons() const
  {
    return totalabandons;
  }
  const std::string & GetDateString() const
  {
    return datestring;
  }
// add dialer stats
  const std::string & GetDisconnects() const
  {
    return disconnects;
  }
  const std::string & GetNoanswers() const
  {
    return noanswers;
  }
  const std::string & GetBusies() const
  {
    return busies;
  }
  const std::string & GetCongestions() const
  {
    return congestions;
  }
  const std::string & GetAnsmachs() const
  {
    return ansmachs;
  }

  void IncrementDisconnects()
  {
    disconnects = itos(atoi(disconnects.c_str()) + 1);
  }
  void IncrementNoanswers()
  {
    noanswers = itos(atoi(noanswers.c_str()) + 1);
  }
  void IncrementBusies()
  {
    busies = itos(atoi(busies.c_str()) + 1);
  }
  void IncrementCongestions()
  {
    congestions = itos(atoi(congestions.c_str()) + 1);
  }
  void IncrementAnsmachs()
  {
    ansmachs = itos(atoi(ansmachs.c_str()) + 1);
  }
  void DecrementAnsmachs()
  {
    ansmachs = itos(atoi(ansmachs.c_str()) - 1);
  }

  void IncrementAbandons()
  {

//  std::cout << "Current abandons: " << abandons << std::endl;

    abandons = itos(atoi(abandons.c_str()) + 1);
    totalabandons = itos(atoi(totalabandons.c_str()) + 1);

//  std::cout << "Abandons now: " << abandons << std::endl;

  }

  void AddCallsDialed(const unsigned int &numCalls)
  {

    calls = itos(atoi(calls.c_str()) + numCalls);
    totalcalls = itos(atoi(totalcalls.c_str()) + numCalls);

  }

private:

  std::string calls, totalcalls, abandons, totalabandons, abndate, datestring,
    disconnects, noanswers, busies, congestions, ansmachs;
  std::string itsName;

};

#endif
