/*
 *  Copyright (C) 1997-2015 JDE Developers Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see http://www.gnu.org/licenses/.
 *  Authors :
 *       Victor Arribas Raigadas <v.arribas.urjc@gmai.com>
 */


#include "quadrotor/quadrotorice.hh"

using namespace quadrotor;
using namespace quadrotor::interfaces;
using namespace Ice;


QuadrotorIce::QuadrotorIce(CommunicatorPtr ic, const QuadRotorSensors *sensors, QuadrotorControl *control):
    ic(ic),
    sensor(sensors),
    control(control)
{
    assert(ic != 0);
}

QuadrotorIce::~QuadrotorIce(){
    stop();
}

void
QuadrotorIce::stop(){
    ONDEBUG_INFO(std::cout << "QuadrotorIce::stop()" << std::endl;)
    lock.lock();
    if (ic && !ic->isShutdown()){
        std::cout << "Shuting down Ice..." << std::endl;
        adapter->deactivate();
        ic->shutdown();
    }
    lock.unlock();
}

void
QuadrotorIce::start(){
    /// boost::thread "thread" lifetime survives "object" lifetime if
    /// thread was running when object is deleted
    /// Is same effect that detach()
    /// Therefore, there is no needed to manage object lifetime neither
    /// delete it.
   boost::thread(boost::bind(&QuadrotorIce::run, this));
}

void
QuadrotorIce::run(){
#if 0
    // Register to handle the signals that indicate when the server should exit.
    // It is safe to register for the same signal multiple times in a program,
    // provided all registration for the specified signal is made through Asio.
    // API changed from 1.47 to 1.59 (now requires io_service)
    boost::asio::io_service _io_service;
    boost::asio::signal_set _signals(_io_service);
    _signals.add(SIGINT);
    _signals.add(SIGTERM);
    _signals.async_wait(boost::bind(&QuadrotorIce::stop, this));
#endif

    bootstrap();

    ic->waitForShutdown();

    std::cout << "Ice is down now" << std::endl;
}

void QuadrotorIce::bootstrap(){
    lock.lock();
    if (ic->isShutdown()) return;

    prop = ic->getProperties();

    adapter = ic->createObjectAdapter("Quadrotor.Adapter");
    std::cout << "Ice adapter listening at " << std::endl;
    std::cout << "\t" << adapter->getEndpoints()[0]->toString() << std::endl;

    std::string name;
    name = prop->getProperty("Quadrotor.Pose3D.Name");
    ObjectPtr posei = new Pose3DI(sensor);
    adapter->add(posei, ic->stringToIdentity(name));

    ObjectPtr navdatai = new NavdataI(sensor);
    name = prop->getProperty("Quadrotor.Navdata.Name");
    adapter->add(navdatai, ic->stringToIdentity(name));

    ObjectPtr dronecontroli = new DroneControlI(control);
    name = prop->getProperty("Quadrotor.Extra.Name");
    adapter->add(dronecontroli, ic->stringToIdentity(name));

    ObjectPtr cmdveli = new CMDVelI(control);
    name = prop->getProperty("Quadrotor.CMDVel.Name");
    adapter->add(cmdveli, ic->stringToIdentity(name));

    adapter->activate();

    std::cout<< "Ice booststrap done." << std::endl;
    lock.unlock();
}
