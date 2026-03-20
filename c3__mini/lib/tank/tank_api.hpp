#pragma once

class AsyncWebServer;

namespace overseer::feature::tank {

class TankMonitor;

void registerTankApi(AsyncWebServer& server, TankMonitor& monitor);

}  // namespace overseer::feature::tank
