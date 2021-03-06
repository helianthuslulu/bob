/**
 * @file machine/cxx/JFAMachineException.cc
 * @date Wed Feb 15 21:57:28 2012 +0200
 * @author Laurent El Shafey <Laurent.El-Shafey@idiap.ch>
 *
  * @brief Implements the exceptions for the EigenMachine
 *
 * Copyright (C) 2011-2013 Idiap Research Institute, Martigny, Switzerland
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <bob/machine/JFAMachineException.h>
#include <boost/format.hpp>

bob::machine::JFABaseNoUBMSet::JFABaseNoUBMSet() throw()
{
}

bob::machine::JFABaseNoUBMSet::~JFABaseNoUBMSet() throw() 
{
}

const char* bob::machine::JFABaseNoUBMSet::what() const throw() 
{
  try 
  {
    boost::format message("No UBM was set in the JFA machine.");
    m_message = message.str();
    return m_message.c_str();
  } catch (...) 
  {
    static const char* emergency = "bob::machine::JFABaseNoUBMSet: cannot format, exception raised";
    return emergency;
  }
}

bob::machine::JFAMachineNoJFABaseSet::JFAMachineNoJFABaseSet() throw()
{
}

bob::machine::JFAMachineNoJFABaseSet::~JFAMachineNoJFABaseSet() throw() 
{
}

const char* bob::machine::JFAMachineNoJFABaseSet::what() const throw() 
{
  try 
  {
    boost::format message("No UBM was set in the JFA machine.");
    m_message = message.str();
    return m_message.c_str();
  } catch (...) 
  {
    static const char* emergency = "bob::machine::JFAMachineNoJFABaseSet: cannot format, exception raised";
    return emergency;
  }
}

