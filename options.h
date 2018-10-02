/*
 * Dialer - Complete, free predictive dialer
 *
 * Complete, free predictive dialer for contact centers.
 *
 * Copyright (C) 2006, Dialer Project
 *
 * Heath Schultz <heath1444@yahoo.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License.
 */

#include <iostream>
#include <fstream>
#include <string>

#ifndef OPTIONS_H
#define OPTIONS_H

class Options
{

public:
  Options()
  {
  }
  ~Options()
  {
  }
private:
  // No copying:
  Options(const Options & other);
  // data:
  std::string m_WwwDialerPassword;
  std::string m_MySqlUser;
  std::string m_MySqlPass;
  std::string m_MySqlHost;
  std::string m_DbName;
  std::string m_ManagerUser;
  std::string m_ManagerPass;
  std::string m_MainHost;
  std::string m_MainPort;
public:
  std::string getWwwDialerPassword()
  {
    return (m_WwwDialerPassword);
  }
  std::string getMySqlUser()
  {
    return (m_MySqlUser);
  }
  std::string getMySqlPass()
  {
    return (m_MySqlPass);
  }
  std::string getMySqlHost()
  {
    return (m_MySqlHost);
  }
  std::string getDbName()
  {
    return (m_DbName);
  }
  std::string getManagerUser()
  {
    return (m_ManagerUser);
  }
  std::string getManagerPass()
  {
    return (m_ManagerPass);
  }
  std::string getMainHost()
  {
    return (m_MainHost);
  }
  std::string getMainPort()
  {
    return (m_MainPort);
  }

  void setWwwDialerPassword(const std::string & s)
  {
    m_WwwDialerPassword = s;
  }
  void setMySqlUser(const std::string & s)
  {
    m_MySqlUser = s;
  }
  void setMySqlPass(const std::string & s)
  {
    m_MySqlPass = s;
  }
  void setMySqlHost(const std::string & s)
  {
    m_MySqlHost = s;
  }
  void setDbName(const std::string & s)
  {
    m_DbName = s;
  }
  void setManagerUser(const std::string & s)
  {
    m_ManagerUser = s;
  }
  void setManagerPass(const std::string & s)
  {
    m_ManagerPass = s;
  }
  void setMainHost(const std::string & s)
  {
    m_MainHost = s;
  }
  void setMainPort(const std::string & s)
  {
    m_MainPort = s;
  }
};

Options OPTIONS;

std::string getMySqlUser()
{
  return OPTIONS.getMySqlUser();
}
std::string getMySqlPass()
{
  return OPTIONS.getMySqlPass();
}
std::string getMySqlPassword()
{
  return OPTIONS.getMySqlPass();
}
std::string getMySqlHost()
{
  return OPTIONS.getMySqlHost();
}
std::string getMySqlHostname()
{
  return OPTIONS.getMySqlHost();
}
std::string getDbName()
{
  return OPTIONS.getDbName();
}
std::string getManagerUser()
{
  return OPTIONS.getManagerUser();
}
std::string getManagerUsername()
{
  return OPTIONS.getManagerUser();
}
std::string getManagerPassword()
{
  return OPTIONS.getManagerPass();
}
std::string getManagerPass()
{
  return OPTIONS.getManagerPass();
}
std::string getMainHost()
{
  return OPTIONS.getMainHost();
}
std::string getMainPort()
{
  return OPTIONS.getMainPort();
}

void setMySqlUser(const std::string & s)
{
  return OPTIONS.setMySqlUser(s);
}
void setMySqlPass(const std::string & s)
{
  return OPTIONS.setMySqlPass(s);
}
void setMySqlPassword(const std::string & s)
{
  return OPTIONS.setMySqlPass(s);
}
void setMySqlHost(const std::string & s)
{
  return OPTIONS.setMySqlHost(s);
}
void setMySqlHostname(const std::string & s)
{
  return OPTIONS.setMySqlHost(s);
}
void setMySqlDbName(const std::string & s)
{
  return OPTIONS.setDbName(s);
}
void setAsteriskUser(const std::string & s)
{
  return OPTIONS.setManagerUser(s);
}
void setAsteriskPass(const std::string & s)
{
  return OPTIONS.setManagerPass(s);
}
void setAsteriskPassword(const std::string & s)
{
  return OPTIONS.setManagerPass(s);
}
void setAsteriskHost(const std::string & s)
{
  return OPTIONS.setMainHost(s);
}
void setAsteriskPort(const std::string & s)
{
  return OPTIONS.setMainPort(s);
}

#endif
