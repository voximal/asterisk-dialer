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
#include <vector>
#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <time.h>
#include "call.h"
#include "abnhelper.h"
#include "exceptions.h"
#include "options.h"

#ifndef QUEUE
#define QUEUE

const char *QUEUES_CONF = PREFIX "/etc/dialer.conf";
const char *QUEUES_MARK = "";
const char QUEUES_EQUAL = ':';

bool debugQueueH = false;

class Setting
{

public:

  Setting(const std::string & rawSetting)
  {

    std::string tempSetting;
//  std::cout << "Setting Constructor called!" << std::endl;
    // format: ;:setting:asdf:jklsemicolon:qwerty
    if (QUEUES_MARK[0])
      tempSetting = rawSetting.substr(10, rawSetting.length() - 10);
    else
      tempSetting = rawSetting;
    std::stringstream SettingStream;
    SettingStream << tempSetting;
    for (std::string tempLine; std::getline(SettingStream, tempLine, '=');)
    {
      //std::cout << "Setting !" << tempLine << std::endl;
      itsWords.push_back(tempLine);
    }
  }

  void Set(const std::string & rawSetting)
  {

    std::vector < std::string > tempWords;
    std::string tempSetting;
    // format: ;:setting:asdf:jklsemicolon:qwerty
    if (QUEUES_MARK[0])
      tempSetting = rawSetting.substr(10, rawSetting.length() - 10);
    else
      tempSetting = rawSetting;
    std::stringstream SettingStream;
    SettingStream << tempSetting;
    for (std::string tempLine; std::getline(SettingStream, tempLine, '=');)
    {
      tempWords.push_back(tempLine);
    }
    itsWords = tempWords;
  }

  int WordCount() const
  {
    return itsWords.size() - 1;
  }
  const std::string & GetType() const
  {
    return itsWords.at(0);
  }
  const std::string & GetWord(int whichWord) const
  {
    return itsWords.at(whichWord + 1);
  }
// took out const and and string
//std::string & GetAttribute(std::string & attribute) {
  std::string GetAttribute(const std::string & attribute)
  {

    notFound = "Not Found";
    for (unsigned int i = 0; i < itsWords.size(); i++)
    {
      if (itsWords.at(i) == attribute)
      {
        return itsWords.at(i + 1);
      }
    }
    return notFound;
  }

  const bool AttributeExists(const std::string & attribute)
  {

    for (std::vector < std::string >::iterator it = itsWords.begin();
      it != itsWords.end(); it++)
    {
      if ((*it) == attribute)
      {
        return true;
      }
    }
    return false;
  }

  const bool AttributeExists(const std::string & field,
    const std::string & value)
  {

    for (unsigned int i = 0; i < itsWords.size() - 1; i++)
    {
      if (itsWords.at(i) == field && itsWords.at(i + 1) == value)
      {
        return true;
      }
    }
    return false;
  }

  int SetAttribute(const std::string & field, const std::string & value)
  {

    unsigned int counter = 0, indexOfValue = 0;
    while (counter < itsWords.size())
    {
      if (itsWords.at(counter) == field)
      {
        indexOfValue = counter + 1;
      }
      counter++;
    }

    if (indexOfValue)
    {
      if (indexOfValue < itsWords.size())
      {
        itsWords.at(indexOfValue) = value;
        return 1;
      }
      else
      {
        return -1;
      }
    }
    else
    {
      return 0;
    }
  }

  int AddAttribute(const std::string & field, const std::string & value)
  {

    if (field.empty() == false && value.empty() == false)
    {
      itsWords.push_back(field);
      itsWords.push_back(value);
      return 1;
    }
    else
    {
      return -1;
    }
  }

  int SupAttribute(const std::string & field, const std::string & value)
  {

    int res = SetAttribute(field, value);

    if (res == -1)
    {
      return -1;
    }
    else
    {
      if (res == 0)
      {
        if (AddAttribute(field, value) == 1)
        {
          return 2;
        }
        else
        {
          return -1;
        }
      }
      else
      {
        return 1;
      }
    }
  }

  void DelAttribute(const std::string & field)
  {

    std::vector < std::string > tempWords;

    for (unsigned int i = 0; i < itsWords.size(); i++)
    {
      if (itsWords.at(i) != field)
      {
        tempWords.push_back(itsWords.at(i));
      }
      else
      {
        i++;
      }
    }
    itsWords = tempWords;
  }

  std::string GetAttr()
  {
    return std::string("hello");
  }

//const std::string & GetWord(const std::string & type, int whichWord) {
  std::string GetWord(const std::string & type, int whichWord)
  {

    notFound = "Not Found";

    for (unsigned int i = 0; i < itsWords.size(); i++)
    {
      if (itsWords.at(i) == type)
      {
        return GetWord(whichWord);
      }
    }
    return notFound;
  }

  const std::string & Get() const
  {
    if (itsWords.size() > 1)
      return itsWords.at(1);
    else
      return emptyWord;
  }

  const int GetInt() const
  {
    return atoi(itsWords.at(1).c_str());
  }
  double GetFloat() const
  {
    return atof(itsWords.at(1).c_str());
  }
  bool GetBool() const
  {

    if (itsWords.at(1) == "true")
      return true;
    else if (itsWords.at(1) == "false")
      return false;
    else if (itsWords.at(1) == "0")
      return true;
    else
      return false;
  }

  ~Setting()
  {
  }

private:

  std::vector < std::string > itsWords;
  std::string notFound;
  std::string emptyWord;
};

const Setting ReturnSetting(const std::string & rawSetting)
{
  Setting TheSetting(rawSetting);
  return TheSetting;
}

class Queue
{

public:

  Queue(const char *filename = NULL)
  {
    if (filename)
    configurationFilename = filename;
    else
    configurationFilename = QUEUES_CONF;

    //std::cout << "QUEUE File conf:" << configurationFilename << std::endl;

    changed = false;
  }
  ~Queue()
  {
  }

  bool ParseQueue(const std::string & name)
  {

    itsName = name;
    int pos = 0, tempMemberNumber = 0, nameFound = 0;

    if (itsName != "general")
    {
      itsAbnHelper.Read(name);
    }

    std::ifstream QueueIn;
    std::ofstream QueueOut;
    //QueueIn.exceptions ( std::ifstream::eofbit );
    //QueueIn.exceptions ( std::ifstream::eofbit | std::ifstream::failbit | std::ifstream::badbit );
    //QueueIn.exceptions (std::ios_base::failbit);

//  try {
    //QueueIn.open("QUEUES_CONF");
    QueueIn.open(configurationFilename.c_str(), std::ios::in | std::ios::out);

//  if (!QueueIn) {
//  throw xFileOpenError("queues.conf");
//  }

    for (std::string tempLine; std::getline(QueueIn, tempLine, '\n');)
    {
      if (tempLine.length() > 1)
      {
        if (tempLine.find("[" + name + "]", 0) != std::string::npos)
        {
          nameFound = 1;
          if (tempLine[0] == '[')
          {
            while (std::getline(QueueIn, tempLine, '\n'))
            {
              if (tempLine.length() > 1)
              {
                if (tempLine[0] == '[')
                  break;
                
                else
                {
                  if (QUEUES_MARK[0] == 0)
                    itsSettings.push_back(ReturnSetting(tempLine));
                  else if (tempLine.find(QUEUES_MARK, 0) != std::string::npos)
                    itsSettings.push_back(ReturnSetting(tempLine));
                  else
                  {
                    if (tempLine.find("Agent/", 0) != std::string::npos)
                    {
                      pos = tempLine.find("Agent/", 0) + 6;
                      tempLine = tempLine.substr(pos, tempLine.length() - pos);
                      tempMemberNumber = atoi(tempLine.c_str());
                      itsMembersNumbers.push_back(tempMemberNumber);
                      //std::cout << name << ": itsSettings size - " << itsSettings.size() << std::endl;
                      //std::cout << name << ": itsMembersNumbers size - " << itsMembersNumbers.size() << std::endl;
                    }
                    else
                    {
                      otherSettings.push_back(tempLine);
                      //std::cout << name << ": otherSettings size - " << otherSettings.size() << std::endl;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    QueueIn.close();
    if (nameFound == 0)
    {
      QueueOut.open(configurationFilename.c_str(), std::ofstream::out | std::ofstream::app);
      std::
        cout << "The name " << name << " was not found so we have to create it"
        << std::endl;
      QueueOut << "\n[" << name << "]" << std::endl;
      QueueOut.close();
    }

//  }

//  catch(std::ios_base::failure f) {
//    std::cout << "Exception opening/reading file: " << f.what() << std::endl;
//  }

//  catch (std::ifstream::failure e) {
//        std::cout << "Exception opening/reading file: " << e.what() << std::endl;
//  }

//not sure why this stuff is here
//  time_t raw;
//  tm * ptm;
//  time(&raw);
//  ptm = localtime(&raw);

    return true;

//  std::string timestamp = itos(ptm->tm_year) + "-" + itos(ptm->tm_mon) + "-" + itos(ptm->tm_mday);
  }

  void WriteAbn()
  {
    itsAbnHelper.Write(itsName);
  }

  const std::string & GetCalls() const
  {
    return itsAbnHelper.GetCalls();
  }
  const std::string & GetTotalCalls() const
  {
    return itsAbnHelper.GetTotalCalls();
  }
  const std::string & GetDateString() const
  {
    return itsAbnHelper.GetDateString();
  }
  const std::string & GetDisconnects() const
  {
    return itsAbnHelper.GetDisconnects();
  }
  const std::string & GetNoanswers() const
  {
    return itsAbnHelper.GetNoanswers();
  }
  const std::string & GetBusies() const
  {
    return itsAbnHelper.GetBusies();
  }
  const std::string & GetCongestions() const
  {
    return itsAbnHelper.GetCongestions();
  }

  int OccurencesOf(const std::string & type) const
  {

    int occurences = 0;
    for (unsigned int i = 0; i < itsSettings.size(); i++)
    {
      if (itsSettings.at(i).GetType() == type)
      {
        occurences++;
      }
    }
    return occurences;
  }

  int SettingCount() const
  {
    return itsSettings.size();
  }
  Setting GetSetting(int whichSetting)const
  {
    return itsSettings.at(whichSetting);
  }
// took const out after the func
  const bool SettingExists(const int &whichSetting, const std::string & type)
  {

    int occurence = 0;
    for (unsigned int i = 0; i < itsSettings.size(); i++)
    {
      //std::cout << "item " << i << itsSettings.at(i).GetType() << std::endl;

      if (itsSettings.at(i).GetType() == type)
      {
        occurence++;
      }
      if (occurence == whichSetting + 1)
      {
        return true;
      }
    }

    return false;
  }
  
  Setting & GetSetting(const int &whichSetting, const std::string & type)
  {

    int occurence = 0;
    for (unsigned int i = 0; i < itsSettings.size(); i++)
    {
      //std::cout << "item " << i << itsSettings.at(i).GetType() << std::endl;

      if (itsSettings.at(i).GetType() == type)
      {
        occurence++;
      }
      if (occurence == whichSetting + 1)
      {
        return itsSettings.at(i);
      }
    }

    throw(xLoopEnd("Exception thrown: Didn't find a setting at " +
        itos(whichSetting)));
  }

  const bool DelSetting(const int &whichSetting, const std::string & type)
  {

    int occurence = 0;
    std::vector < Setting > tempSettings;

    for (unsigned int i = 0; i < itsSettings.size(); i++)
    {
      if (itsSettings.at(i).GetType() == type)
      {
        occurence++;
      }
      if (occurence != whichSetting + 1)
      {
        tempSettings.push_back(itsSettings.at(i));
      }
    }

    if (itsSettings.size() == tempSettings.size())
    {
      return false;
    }
    else
    {
      itsSettings = tempSettings;
      return true;
    }
  }

  const bool DelSetting(const std::string & type)
  {

    std::vector < Setting > tempSettings;

    for (std::vector < Setting >::iterator it = itsSettings.begin();
      it != itsSettings.end(); it++)
    {
      if (it->GetType() != type)
      {
        tempSettings.push_back((*it));
      }
    }

    if (itsSettings.size() == tempSettings.size())
    {
      return false;
    }
    else
    {
      itsSettings = tempSettings;
      return true;
    }
  }

  int GetSettingNumber(const int &whichSetting, const std::string & type) const
  {

    int occurence = 0;
    for (unsigned int i = 0; i < itsSettings.size(); i++)
    {
      if (itsSettings.at(i).GetType() == type)
      {
        occurence++;
      }
      if (occurence == whichSetting + 1)
      {
        return i;
      }
    }

    throw(xLoopEnd("Exception thrown: Didn't find any setting at " +
        itos(whichSetting)));
  }

// took const out afterward here as well

  const bool SettingExists(const std::string & type, const std::string & field,
    const std::string & value)
  {

    for (unsigned int i = 0; i < itsSettings.size(); i++)
    {
      if (itsSettings.at(i).GetType() == type)
      {
        if (itsSettings.at(i).AttributeExists(field, value))
        {
          return true;
        }
      }
    }

    return false;
  }

  void DelSetting(const std::string & type, const std::string & field,
    const std::string & value)
  {

    std::vector < Setting > tempSettings;

    for (std::vector < Setting >::iterator it = itsSettings.begin();
      it != itsSettings.end(); it++)
    {
      if (it->GetType() != type)
      {
        tempSettings.push_back((*it));
      }
      else
      {
        if (!it->AttributeExists(field, value))
        {
          tempSettings.push_back((*it));
        }
      }
    }

    itsSettings = tempSettings;
  }

  const bool SettingExists(const std::string & type)
  {
    return SettingExists(0, type);
  }

  const Setting & GetSetting(const std::string & type)
  {
    return GetSetting(0, type);
  }

  int GetSettingNumber(const std::string & type)
  {
    return GetSettingNumber(0, type);
  }

  void SetSetting(const std::string & type, const std::string & rawSetting)
  {
    itsSettings.at(GetSettingNumber(type)).Set(QUEUES_MARK + type + "=" +
      rawSetting);
    changed = true;
  }

  void AddSetting(const std::string & type, const std::string & settingString)
  {
    itsSettings.push_back(ReturnSetting(QUEUES_MARK + type + "=" +
        settingString));
  }

  void SupSetting(const std::string & type, const std::string & settingString)
  {
    bool exists = false;
    for (unsigned int i = 0; i < itsSettings.size(); i++)
    {
      if (itsSettings.at(i).GetType() == type && exists == false)
      {
        exists = true;
      }
    }
    if (!exists)
      AddSetting(type, settingString);
  }

// void SetName(std::string name) { itsName = name; }
  std::string GetName()const
  {
    return itsName;
  }

  int GetMemberNumber(int whichMember) const
  {
    return itsMembersNumbers.at(whichMember);
  }
  std::string GetMemberName(int whichMember)const
  {
    return itsMembersNames.at(whichMember);
  }

  bool HasMemberNumber(int whichMember)const
  {
    for (unsigned int i = 0; i < itsMembersNumbers.size(); i++)
    {
      if (itsMembersNumbers.at(i) == whichMember)
      {
        return true;
      }
    }
    return false;
  }

  void IncrementDisconnects()
  {
    itsAbnHelper.IncrementDisconnects();
  }
  void IncrementNoanswers()
  {
    itsAbnHelper.IncrementNoanswers();
  }
  void IncrementBusies()
  {
    itsAbnHelper.IncrementBusies();
  }
  void IncrementCongestions()
  {
    itsAbnHelper.IncrementCongestions();
  }
  void IncrementAnsmachs()
  {
    itsAbnHelper.IncrementAnsmachs();
  }
  void DecrementAnsmachs()
  {
    itsAbnHelper.DecrementAnsmachs();
  }

  void WriteCalls()
  {
    itsAbnHelper.Write(itsName);
  }

  void AddCallsDialed(const int &calls)
  {
    itsAbnHelper.AddCallsDialed(calls);
  }

  int size() const
  {
    return itsMembersNumbers.size();
  }

  void Write()
  {

    std::stable_sort(itsMembersNumbers.begin(), itsMembersNumbers.end());

    std::stringstream QueueStream;

    std::ifstream QueuesIn;
    QueuesIn.open(configurationFilename.c_str());

    if (!QueuesIn.is_open())
      std::cout << "Write failed!" << std::endl;

    for (std::string tempLine;
      tempLine.find("[" + itsName + "]", 0) == std::string::npos;)
    {
      std::getline(QueuesIn, tempLine, '\n');
      QueueStream << tempLine << std::endl;
    }

//  std::cout << "[" << itsName << "]" << std::endl;

    for (unsigned int i = 0; i < otherSettings.size(); i++)
    {
      QueueStream << otherSettings.at(i) << std::endl;
    }

    for (unsigned int i = 0; i < itsMembersNumbers.size(); i++)
    {
      QueueStream << "member => Agent/";
      QueueStream << itsMembersNumbers.at(i) << std::endl;
    }

    for (unsigned int i = 0; i < itsSettings.size(); i++)
    {
      QueueStream << QUEUES_MARK << itsSettings.at(i).GetType();
      if (itsSettings.at(i).WordCount() == 0)
      {
        QueueStream << "=";
      }
      else
        for (int j = 0; j < itsSettings.at(i).WordCount(); j++)
        {
          QueueStream << "=" << itsSettings.at(i).GetWord(j);
        }
      QueueStream << std::endl;
    }

    QueueStream << std::endl;

    for (std::string tempLine; std::getline(QueuesIn, tempLine, '\n');)
    {
      if (tempLine[0] == '[')
      {
        QueueStream << tempLine << std::endl;
        break;
      }
    }

    for (std::string tempLine; std::getline(QueuesIn, tempLine, '\n');)
    {
      QueueStream << tempLine << std::endl;
    }

    //QueuesIn.close();

    std::ofstream QueuesOut;
    QueuesOut.open(configurationFilename.c_str());
    if (!QueuesOut)
      std::cout << "Error writing!" << std::endl;
    for (std::string tempLine; std::getline(QueueStream, tempLine, '\n');)
    {
      QueuesOut << tempLine << std::endl;
    }
    QueuesOut.close();
    QueuesIn.close();
  }

private:

  std::string itsName;
  std::vector < int >itsMembersNumbers;
  std::vector < std::string > itsMembersNames, otherSettings;
  std::vector < Setting > itsSettings;
  bool changed;
  CallCache itsCalls;
  AbnHelper itsAbnHelper;
  std::string emptyWord;  
  
  std::string configurationFilename;
};

Queue ReturnQueue(std::string name, const char *filename = NULL)
{
  Queue TheQueue(filename);
  TheQueue.ParseQueue(name);
  return TheQueue;
}

class QueueList
{

public:

  QueueList(const char *filename = NULL)
  {
    if (filename)
    configurationFilename = filename;
    else
    configurationFilename = QUEUES_CONF;

    //std::cout << "QUEUELIST File conf:" << configurationFilename << std::endl;
  }
  ~QueueList()
  {
  }

  void ParseQueues()
  {

//  std::cout << "Got here beginning of ParseQueues" << std::endl;

    ItsQueues.clear();

    std::vector < std::string > queueNames;

//  std::cout << "Got here 1" << std::endl;

    std::ifstream QueuesIn;
    QueuesIn.open(configurationFilename.c_str());
    for (std::string tempLine; std::getline(QueuesIn, tempLine, '\n');)
    {
      //if (tempLine.length() > 1 &&
      //  tempLine.find("default", 0) == std::string::npos)
      if (tempLine.length() > 1)
      {
        if (tempLine[0] == '[' && tempLine.find("general") == std::string::npos)
        {
          queueNames.push_back(tempLine.substr(1, tempLine.find("]", 0) - 1));
//        std::cout << "pushing back..." << std::endl;
        }
      }
    }
    QueuesIn.close();

//  std::cout << "Actual push_backs now..." << std::endl;

    std::stable_sort(queueNames.begin(), queueNames.end());

    for (unsigned int i = 0; i < queueNames.size(); i++)
    {
//    std::cout << "pushnig back" << std::endl;
      ItsQueues.push_back(ReturnQueue(queueNames.at(i), configurationFilename.c_str()));
    }

//  std::cout << "Got to end of ParseQueues" << std::endl;
//  std::cout << "size of queues " << ItsQueues.size() << std::endl;

  }

  Queue at(int whichQueue) const
  {
    return ItsQueues.at(whichQueue);
  }
  int size()
  {
    return ItsQueues.size();
  }

  Queue where(const std::string & queue)
  {
    for (unsigned int i = 0; i < ItsQueues.size(); i++)
    {
      if (ItsQueues.at(i).GetName() == queue)
      {
        return ItsQueues.at(i);
      }
    }

    throw(xLoopEnd("Exception thrown: Didn't find a queue with the name " +
        queue));

  }

  Queue & rWhere(const std::string & queue)
  {
    for (unsigned int i = 0; i < ItsQueues.size(); i++)
    {
      if (ItsQueues.at(i).GetName() == queue)
      {
        return ItsQueues.at(i);
      }
    }

    throw(xLoopEnd("Exception thrown: Didn't find a queue with the name: " +
        queue));
  }

  bool exists(const std::string & name)
  {
    for (unsigned int i = 0; i < ItsQueues.size(); i++)
    {
      if (ItsQueues.at(i).GetName() == name)
      {
        return true;
      }
    }
    return false;
  }

private:

  std::vector < Queue > ItsQueues;
  
  std::string configurationFilename;
};

#endif
