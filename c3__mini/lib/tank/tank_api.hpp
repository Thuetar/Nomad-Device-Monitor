#pragma once

class AsyncWebServer;

namespace overseer::feature::tank {

class TankService;

void registerTankApi(AsyncWebServer& server, TankService& service);

}  // namespace overseer::feature::tank
