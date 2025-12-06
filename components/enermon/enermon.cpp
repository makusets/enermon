#include "enermon.h"
#include <time.h>

namespace esphome {
namespace enermon {

void Enermon::set_config(const std::vector<int> &ct_pins,
                         const std::vector<float> &ct_cal,
                         int voltage_pin,
                         float voltage_cal,
                         float voltage_phase,
                         unsigned int sample_count) {
  ct_pins_      = ct_pins;
  ct_cal_       = ct_cal;
  voltage_pin_  = voltage_pin;
  voltage_cal_  = voltage_cal;
  voltage_phase_= voltage_phase;
  sample_count_ = sample_count;

  ct_pins_.resize(4, -1);
  ct_cal_.resize(4, 1111.0f);

  for (int i = 0; i < 4; ++i) {
    last_irms_[i]           = 0.0f;
    energy_daily_wh_[i]     = 0.0;
    energy_weekly_wh_[i]    = 0.0;
    energy_monthly_wh_[i]   = 0.0;
  }
}

void Enermon::setup() {
  for (int i = 0; i < 4; ++i) {
    if (!sensor_current_rms[i]) sensor_current_rms[i] = new esphome::sensor::Sensor();
    if (!sensor_power_w[i]) sensor_power_w[i] = new esphome::sensor::Sensor();
    if (!sensor_energy_daily_wh[i]) sensor_energy_daily_wh[i] = new esphome::sensor::Sensor();
    if (!sensor_energy_weekly_wh[i]) sensor_energy_weekly_wh[i] = new esphome::sensor::Sensor();
    if (!sensor_energy_monthly_wh[i]) sensor_energy_monthly_wh[i] = new esphome::sensor::Sensor();
  }
  if (!sensor_voltage_rms) sensor_voltage_rms = new esphome::sensor::Sensor();
  if (!sensor_wifi_rssi) sensor_wifi_rssi = new esphome::sensor::Sensor();

  // Configure EmonLib instances: current per CT, same voltage pin/cal/phase for all
  for (int i = 0; i < 4; ++i) {
    if (ct_pins_[i] >= 0) {
      emon_ct_[i].current(ct_pins_[i], double(ct_cal_[i]));
      if (voltage_pin_ >= 0) {
        emon_ct_[i].voltage(voltage_pin_, double(voltage_cal_), voltage_phase_);
      }
    }
  }

  last_sample_time_ms_ = millis();

  time_t now = time(nullptr);
  last_day_reset_ = now;
  last_week_reset_ = now;
  last_month_reset_ = now;
}

void Enermon::maybe_reset_counters_() {
  time_t now = time(nullptr);
  if (now == ((time_t) 0)) return;
  struct tm *tm_now = localtime(&now);
  struct tm *tm_last = localtime(&last_day_reset_);

  if (tm_now->tm_yday != tm_last->tm_yday || tm_now->tm_year != tm_last->tm_year) {
    for (int i = 0; i < 4; ++i) energy_daily_wh_[i] = 0.0;
    last_day_reset_ = now;
  }

  int week_now = (tm_now->tm_yday / 7) + tm_now->tm_year * 52;
  int week_last = (tm_last->tm_yday / 7) + tm_last->tm_year * 52;
  if (week_now != week_last) {
    for (int i = 0; i < 4; ++i) energy_weekly_wh_[i] = 0.0;
    last_week_reset_ = now;
  }

  if (tm_now->tm_mon != tm_last->tm_mon || tm_now->tm_year != tm_last->tm_year) {
    for (int i = 0; i < 4; ++i) energy_monthly_wh_[i] = 0.0;
    last_month_reset_ = now;
  }
}

void Enermon::loop() {
  unsigned long now_ms = millis();
  const unsigned long sample_interval = 200;  // ms between publishes
  if (now_ms - last_sample_time_ms_ < sample_interval) return;
  last_sample_time_ms_ = now_ms;

  double elapsed_h = sample_interval / 1000.0 / 3600.0;

  for (int i = 0; i < 4; ++i) {
    if (ct_pins_[i] < 0) continue;

    emon_ct_[i].calcVI(20, sample_count_);
    float irms = float(emon_ct_[i].Irms);
    float vrms = float(emon_ct_[i].Vrms);
    float real_p = float(emon_ct_[i].realPower);  // signed active power (W)

    last_irms_[i] = irms;
    if (sensor_current_rms[i]) sensor_current_rms[i]->publish_state(irms);

    if (sensor_power_w[i]) sensor_power_w[i]->publish_state(real_p);

    last_vrms_ = vrms;
    if (sensor_voltage_rms) sensor_voltage_rms->publish_state(vrms);

    double wh = real_p * elapsed_h;
    energy_daily_wh_[i] += wh;
    energy_weekly_wh_[i] += wh;
    energy_monthly_wh_[i] += wh;

    if (sensor_energy_daily_wh[i]) sensor_energy_daily_wh[i]->publish_state(energy_daily_wh_[i]);
    if (sensor_energy_weekly_wh[i]) sensor_energy_weekly_wh[i]->publish_state(energy_weekly_wh_[i]);
    if (sensor_energy_monthly_wh[i]) sensor_energy_monthly_wh[i]->publish_state(energy_monthly_wh_[i]);
  }

  if (sensor_wifi_rssi) sensor_wifi_rssi->publish_state(WiFi.RSSI());
  maybe_reset_counters_();
}

}  // namespace enermon
}  // namespace esphome