#include <cstdint>
#include <queue>

#include "boost/ut.hpp"
#include "config/ConfigManager.h"

#ifndef ADC_11db
#define ADC_11db 3
#endif

namespace native_mock {
std::queue<int> adc_values;
unsigned long fake_millis = 1000;
}

int analogRead(uint8_t) {
    if (native_mock::adc_values.empty()) {
        return 2048;
    }
    const int v = native_mock::adc_values.front();
    native_mock::adc_values.pop();
    return v;
}

void analogReadResolution(uint8_t) {}
void analogSetAttenuation(int) {}
void delay(unsigned long ms) { native_mock::fake_millis += ms; }

class SerialMock {
public:
    void println(const char*) {}
    template <typename... Args>
    void printf(const char*, Args...) {}
};

SerialMock Serial;
LogClass Log;

#define String(x) std::to_string(x)
#include "device/energy/WCS1800/WCS1800.cpp"
#undef String

int main() {
    using namespace boost::ut;
    using overseer::device::energy::WCS1800;

    "wcs1800 initializes with mocked hardware"_test = [] {
        WCS1800 sensor(1);
        expect(sensor.begin());
        expect(sensor.isInitialized());
        expect(sensor.getPin() == static_cast<uint8_t>(1));
    };

    "wcs1800 update uses mocked adc samples"_test = [] {
        native_mock::adc_values = {};
        native_mock::adc_values.push(2048); // begin() test read
        native_mock::adc_values.push(2048); // update read

        WCS1800 sensor(2);
        expect(sensor.begin());

        sensor.update();
        const auto data = sensor.getData();

        expect(data.valid_reading);
        expect(data.total_samples == 1ull);
    };
}
