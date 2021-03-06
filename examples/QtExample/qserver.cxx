/*=========================================================================

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "qserver.h"

#include <remus/server/Server.h>
#include <remus/server/WorkerFactory.h>

#include <iostream>

qserver::qserver():
  QObject(0),
  Server( new remus::server::Server(
                remus::server::WorkerFactory( "fofof" ) ) )
{
  //set factory to an extension that is never used so that
  //all the workers come from the controls class threads
  this->Server->startBrokering(remus::server::Server::NONE);
  std::cout << "server is launched" << std::endl;
  emit started();
}

qserver::~qserver()
{
  this->Server->stopBrokering();
  emit stopped();
}
