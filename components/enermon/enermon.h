#pragma once
#include "esphome.h"
#include "EmonLib.h"
#include <vector>
#include <array>

namespace esphome {
namespace enermon {

class Enermon : public Component {
 public:
  Enermon(const std::vector<int> &ct_pins,
          const std::vector<float> &a_per_v,
          const std::vector<float> &cal_gain,
          const std::vector<float> &cal_offset,
          const std::vector<float> &ct_cal,
          int voltage_pin,
          float voltage_divider = 80.0,
          float voltage_cal = 23426.0,
          int voltage_phase = 0,
          unsigned int sample_count = 200);

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
  std::vector<float> a_per_v_;
  std::vector<float> cal_gain_;
  std::vector<float> cal_offset_;
  std::vector<float> ct_cal_;
  int voltage_pin_;
  float voltage_divider_;
  float voltage_cal_;
  int voltage_phase_;
  unsigned int sample_count_;

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