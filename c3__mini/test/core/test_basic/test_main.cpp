#include "boost/ut.hpp"
//#include "device/environment/aht/AHT_instance.h"
#include "device/energy/WCS1800/WCS1800.h"
int main() {
  using namespace boost::ut;

  "basic arithmetic"_test = [] {
    expect(2_i == 2_i);
  };
}
