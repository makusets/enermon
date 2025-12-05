#include "enermon.h"
#include <time.h>

namespace enermon {

Enermon::Enermon(const std::vector<int> &ct_pins,
                 const std::vector<float> &a_per_v,
                 const std::vector<float> &cal_gain,
                 const std::vector<float> &cal_offset,
                 int voltage_pin,
                 float voltage_divider,
                 unsigned int sample_count)
    : ct_pins_(ct_pins),
      a_per_v_(a_per_v),
      cal_gain_(cal_gain),
      cal_offset_(cal_offset),
      voltage_pin_(voltage_pin),
      voltage_divider_(voltage_divider),
      sample_count_(sample_count),
      last_sample_time_ms_(0) {
  // ensure length 4 arrays
  ct_pins_.resize(4, -1);
  a_per_v_.resize(4, 0.0f);
  cal_gain_.resize(4, 1.0f);
  cal_offset_.resize(4, 0.0f);
}

void Enermon::setup() {
  // create sensors if user hasn't provided pointers in YAML; they can override in YAML by providing
  // pointers to these member sensors (see example YAML).
  for (int i = 0; i < 4; ++i) {
    if (!sensor_current_rms[i]) sensor_current_rms[i] = new sensor::Sensor();
    if (!sensor_power_w[i]) sensor_power_w[i] = new sensor::Sensor();
    if (!sensor_energy_daily_wh[i]) sensor_energy_daily_wh[i] = new sensor::Sensor();
    if (!sensor_energy_weekly_wh[i]) sensor_energy_weekly_wh[i] = new sensor::Sensor();
    if (!sensor_energy_monthly_wh[i]) sensor_energy_monthly_wh[i] = new sensor::Sensor();
  }
  if (!sensor_voltage_rms) sensor_voltage_rms = new sensor::Sensor();
  if (!sensor_wifi_rssi) sensor_wifi_rssi = new sensor::Sensor();

  // ADC pins: no pinMode required for analogRead on ESP32 with Arduino
  last_sample_time_ms_ = millis();

  // initialize last reset times (needs time: NTP available on device)
  time_t now = time(nullptr);
  last_day_reset_ = now;
  last_week_reset_ = now;
  last_month_reset_ = now;
}

float Enermon::adc_to_voltage_(int raw) {
  // raw is 0..4095 (ESP32 ADC), convert to voltage (assume 3.3V ADC reference)
  // ESP32 ADC is non-linear; for a simple component we use a linear mapping; users should calibrate with cal_gain/cal_offset.
  const float adc_max = 4095.0f;
  const float vref = 3.3f;
  return (raw / adc_max) * vref;
}

void Enermon::maybe_reset_counters_() {
  time_t now = time(nullptr);
  if (now == ((time_t) 0)) return;  // time not available
  struct tm *tm_now = localtime(&now);
  struct tm *tm_last_day = localtime(&last_day_reset_);

  if (tm_now->tm_yday != tm_last_day->tm_yday || tm_now->tm_year != tm_last_day->tm_year) {
    // new day
    for (int i = 0; i < 4; ++i) energy_daily_wh_[i] = 0.0;
    last_day_reset_ = now;
  }

  // week: ISO week would be more correct; use tm_yday/7 simple approach
  int week_now = (tm_now->tm_yday / 7) + tm_now->tm_year * 52;
  int week_last = (tm_last_day->tm_yday / 7) + tm_last_day->tm_year * 52;
  if (week_now != week_last) {
    for (int i = 0; i < 4; ++i) energy_weekly_wh_[i] = 0.0;
    last_week_reset_ = now;
  }

  if (tm_now->tm_mon != tm_last_day->tm_mon || tm_now->tm_year != tm_last_day->tm_year) {
    for (int i = 0; i < 4; ++i) energy_monthly_wh_[i] = 0.0;
    last_month_reset_ = now;
  }
}

void Enermon::loop() {
  // accumulate samples
  unsigned long now_ms = millis();
  const unsigned long sample_interval = 200;  // run inner processing every 200ms
  if (now_ms - last_sample_time_ms_ < sample_interval) return;
  last_sample_time_ms_ = now_ms;

  // take many fast samples for RMS
  for (unsigned int s = 0; s < sample_count_; ++s) {
    for (int i = 0; i < 4; ++i) {
      if (ct_pins_[i] < 0) continue;
      int raw = analogRead(ct_pins_[i]);
      float v = adc_to_voltage_(raw);
      // center around midpoint: assume CT output is centered at Vcc/2 (1.65V). User calibrates with cal_offset.
      float ac = (v - 1.65f) * cal_gain_[i] + cal_offset_[i];
      // convert voltage at ADC to current using A/V (user provided)
      float amps = ac * a_per_v_[i];
      accum_sq_[i] += double(amps * amps);
      accum_samples_[i] += 1;
    }
    // voltage pin
    if (voltage_pin_ >= 0) {
      int rawv = analogRead(voltage_pin_);
      float v = adc_to_voltage_(rawv);
      // reverse divider to line voltage
      float line_v = v * voltage_divider_;
      voltage_accum_sq_ += double(line_v * line_v);
      voltage_accum_samples_ += 1;
    }
  }

  // compute RMS for each CT if enough samples
  double elapsed_h = sample_interval / 1000.0 / 3600.0;  // hours for integration per run
  for (int i = 0; i < 4; ++i) {
    if (accum_samples_[i] == 0 || a_per_v_[i] == 0.0f) continue;
    float irms = sqrt(accum_sq_[i] / accum_samples_[i]);
    last_irms_[i] = irms;
    sensor_current_rms[i]->publish_state(irms);

    // compute power: P = V_rms * I_rms; use measured voltage if available
    float vrms = (voltage_accum_samples_ ? sqrt(voltage_accum_sq_ / voltage_accum_samples_) : 230.0f);
    float power_w = vrms * irms;
    sensor_power_w[i]->publish_state(power_w);

    // integrate energy (Wh) over the elapsed interval
    double wh = power_w * elapsed_h;
    energy_daily_wh_[i] += wh;
    energy_weekly_wh_[i] += wh;
    energy_monthly_wh_[i] += wh;

    sensor_energy_daily_wh[i]->publish_state(energy_daily_wh_[i]);
    sensor_energy_weekly_wh[i]->publish_state(energy_weekly_wh_[i]);
    sensor_energy_monthly_wh[i]->publish_state(energy_monthly_wh_[i]);

    // reset accumulators for CT
    accum_sq_[i] = 0.0;
    accum_samples_[i] = 0;
  }

  // publish voltage RMS
  if (voltage_accum_samples_) {
    float vrms = sqrt(voltage_accum_sq_ / voltage_accum_samples_);
    last_vrms_ = vrms;
    sensor_voltage_rms->publish_state(vrms);
    voltage_accum_sq_ = 0.0;
    voltage_accum_samples_ = 0;
  }

  // publish wifi rssi
  sensor_wifi_rssi->publish_state(WiFi.RSSI());

  // check and reset daily/weekly/monthly at midnight boundaries if time available
  maybe_reset_counters_();
}

}  // namespace enermon