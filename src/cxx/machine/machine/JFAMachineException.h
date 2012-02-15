/**
 * @file cxx/machine/machine/JFAMachineException.h
 * @date Wed Feb 15 21:55:43 2012 +0200
 * @author Laurent El Shafey <Laurent.El-Shafey@idiap.ch>
 *
  * @brief Exceptions used for the JFAMachine
 *
 * Copyright (C) 2011-2012 Idiap Research Institute, Martigny, Switzerland
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

#ifndef BOB_MACHINE_JFAMACHINEEXCEPTION_H 
#define BOB_MACHINE_JFAMACHINEEXCEPTION_H

#include <cstdlib>
#include "machine/Exception.h"

namespace bob { namespace machine {

  class JFAMachineNoUBM: public Exception {
    public:
      JFAMachineNoUBM() throw();
      virtual ~JFAMachineNoUBM() throw();
      virtual const char* what() const throw();

    private:
      mutable std::string m_message;
  };

}}

#endif 