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
#include <sstream>

#ifndef ITOS
#define ITOS

const std::string itos(const long int &i)
{

  std::ostringstream IntStream;
  IntStream << i;
  return IntStream.str();

}

#endif
