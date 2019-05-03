/*
 * GnuDialer - Complete, free predictive dialer
 *
 * Complete, free predictive dialer for contact centers.
 *
 * Copyright (C) 2006, GnuDialer Project
 *
 * Heath Schultz <heath1444@yahoo.com>
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License.
 */

#include <string>

#ifndef EXCEPTIONS
#define EXCEPTIONS

class xLoopEnd
{

public:

  xLoopEnd(const std::string & reason)
  {
    itsReason = reason;
  }
  ~xLoopEnd()
  {
  }

  const std::string what()
  {
    return itsReason;
  }

private:

  std::string itsReason;

};

class xConnectionError
{
public:
  xConnectionError(const std::string & host)
  {
    itsHost = host;
  }
  ~xConnectionError()
  {
  }
  const std::string & GetHost() const
  {
    return itsHost;
  }
private:
   std::string itsHost;
};

class xTooFewFields
{
public:
  xTooFewFields()
  {
  }
  ~xTooFewFields()
  {
  }
};

class xTooManyFields
{
public:
  xTooManyFields()
  {
  }
  ~xTooManyFields()
  {
  }
};

class xInvalidWeightValue
{
public:
  xInvalidWeightValue()
  {
  }
  ~xInvalidWeightValue()
  {
  }
};

class xNoHostsDefined
{
public:
  xNoHostsDefined()
  {
  }
  ~xNoHostsDefined()
  {
  }
};

class xOutOfHosts
{
public:
  xOutOfHosts()
  {
  }
  ~xOutOfHosts()
  {
  }
};

class xForkError
{
public:
  xForkError()
  {
  }
  ~xForkError()
  {
  }
};

class xFileOpenError
{
public:
  xFileOpenError(const std::string & filename)
  {
    std::cout << filename << std::endl;
    itsFilename = filename;
  }
  ~xFileOpenError()
  {
  }
  const std::string & GetFilename() const
  {
    return itsFilename;
  }
private:
   std::string itsFilename;
};

#endif
