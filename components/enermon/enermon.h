#pragma once
#include "esphome.h"
#include "EmonLib.h"
#include <vector>
#include <array>

namespace esphome {
namespace enermon {

class Enermon : public Component {
 public:
  Enermon() = default;  // simple default constructor

  void add_ct_channel(int index, int ct_pin, float ct_cal);
  void set_voltage_config(int voltage_pin,
                          float voltage_cal,
                          float voltage_phase,
                          unsigned int sample_count);

  void setup() override;
  void loop() override;

  std::array<esphome::sensor::Sensor*, 4> sensor_current_rms{};
  std::array<esphome::sensor::Sensor*, 4> sensor_power_w{};  // signed active power (W)
  std::array<esphome::sensor::Sensor*, 4> sensor_energy_daily_wh{};
  std::array<esphome::sensor::Sensor*, 4> sensor_energy_weekly_wh{};
  std::array<esphome::sensor::Sensor*, 4> sensor_energy_monthly_wh{};
  esphome::sensor::Sensor *sensor_voltage_rms{nullptr};
  esphome::sensor::Sensor *sensor_wifi_rssi{nullptr};

 protected:
  std::vector<int> ct_pins_;
  std::vector<float> ct_cal_;
  int voltage_pin_{-1};
  float voltage_cal_{230.0f};
  float voltage_phase_{0.0f};
  unsigned int sample_count_{200};

  unsigned long last_sample_time_ms_;

  // EmonLib instances (one per CT)
  std::array<EnergyMonitor, 4> emon_ct_;

  // energy / accumulators
  std::array<float, 4> last_irms_{};
  std::array<double, 4> energy_daily_wh_{};
  std::array<double, 4> energy_weekly_wh_{};
  std::array<double, 4> energy_monthly_wh_{};
  float last_vrms_{};

  // last reset timestamps
  time_t last_day_reset_{0};
  time_t last_week_reset_{0};
  time_t last_month_reset_{0};

  void maybe_reset_counters_();
};

}  // namespace enermon
}  // namespace esphome