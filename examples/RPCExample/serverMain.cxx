/*=========================================================================

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "RPCService.h"
#include <remus/server/WorkerFactory.h>

int main ()
{
  //create a custom worker factory that creates children processes
  //we cap it at having only 3 children at any time
  remus::server::WorkerFactory factory;
  factory.setMaxWorkerCount(3);


  //create a default server with the default factory
  BasicRPCServer s(factory);

  //start accepting connections for clients and workers
  bool valid = s.startBrokering();
  return valid ? 0 : 1;
}
