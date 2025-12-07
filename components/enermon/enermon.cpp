#include "enermon.h"
#include <time.h>
#include <stdio.h>

namespace esphome {
namespace enermon {


void Enermon::add_ct_channel(int index, int ct_pin, float ct_cal) {
  if (index < 0) return;

  if (ct_pins_.size() < static_cast<size_t>(index + 1))
    ct_pins_.resize(index + 1, -1);
  if (ct_cal_.size() < static_cast<size_t>(index + 1))
    ct_cal_.resize(index + 1, 30.0f);

  ct_pins_[index] = ct_pin;
  ct_cal_[index] = ct_cal;
}

void Enermon::set_voltage_config(int voltage_pin,
                                 float voltage_cal,
                                 float voltage_phase,
                                 unsigned int sample_count) {
  voltage_pin_   = voltage_pin;
  voltage_cal_   = voltage_cal;
  voltage_phase_ = voltage_phase;
  sample_count_  = sample_count;

  // Initialise your per-channel state if needed
  if (ct_pins_.size() < 4)  ct_pins_.resize(4, -1);
  if (ct_cal_.size() < 4)   ct_cal_.resize(4, 1111.0f);

  for (int i = 0; i < 4; ++i) {
    last_irms_[i]         = 0.0f;
    energy_daily_wh_[i]   = 0.0;
    energy_weekly_wh_[i]  = 0.0;
    energy_monthly_wh_[i] = 0.0;
  }
}

void Enermon::set_sensor_current(int index, esphome::sensor::Sensor *sensor) {
  if (index < 0 || index >= 4) return;
  sensor_current_rms[index] = sensor;
}
void Enermon::set_sensor_power(int index, esphome::sensor::Sensor *sensor) {
  if (index < 0 || index >= 4) return;
  sensor_power_w[index] = sensor;
}
void Enermon::set_sensor_energy_daily(int index, esphome::sensor::Sensor *sensor) {
  if (index < 0 || index >= 4) return;
  sensor_energy_daily_wh[index] = sensor;
}
void Enermon::set_sensor_energy_weekly(int index, esphome::sensor::Sensor *sensor) {
  if (index < 0 || index >= 4) return;
  sensor_energy_weekly_wh[index] = sensor;
}
void Enermon::set_sensor_energy_monthly(int index, esphome::sensor::Sensor *sensor) {
  if (index < 0 || index >= 4) return;
  sensor_energy_monthly_wh[index] = sensor;
}
void Enermon::set_sensor_voltage(esphome::sensor::Sensor *sensor) { sensor_voltage_rms = sensor; }


void Enermon::setup() {
  // Initialize preferences for persistent storage
  pref_daily_ = global_preferences->make_preference<std::array<double, 4>>(fnv1_hash("enermon_daily"));
  pref_weekly_ = global_preferences->make_preference<std::array<double, 4>>(fnv1_hash("enermon_weekly"));
  pref_monthly_ = global_preferences->make_preference<std::array<double, 4>>(fnv1_hash("enermon_monthly"));
  
  struct ResetTimes {
    time_t day;
    time_t week;
    time_t month;
  };
  pref_reset_times_ = global_preferences->make_preference<ResetTimes>(fnv1_hash("enermon_resets"));

  // Load saved energy counters
  load_energy_counters_();

  for (int i = 0; i < 4; ++i) {
    char name_buf[32];
    if (!sensor_current_rms[i]) {
      sensor_current_rms[i] = new esphome::sensor::Sensor();
      snprintf(name_buf, sizeof(name_buf), "CT%d Current", i);
      sensor_current_rms[i]->set_name(name_buf);
    }
    if (!sensor_power_w[i]) {
      sensor_power_w[i] = new esphome::sensor::Sensor();
      snprintf(name_buf, sizeof(name_buf), "CT%d Power", i);
      sensor_power_w[i]->set_name(name_buf);
    }
    if (!sensor_energy_daily_wh[i]) {
      sensor_energy_daily_wh[i] = new esphome::sensor::Sensor();
      snprintf(name_buf, sizeof(name_buf), "CT%d Energy Daily", i);
      sensor_energy_daily_wh[i]->set_name(name_buf);
    }
    if (!sensor_energy_weekly_wh[i]) {
      sensor_energy_weekly_wh[i] = new esphome::sensor::Sensor();
      snprintf(name_buf, sizeof(name_buf), "CT%d Energy Weekly", i);
      sensor_energy_weekly_wh[i]->set_name(name_buf);
    }
    if (!sensor_energy_monthly_wh[i]) {
      sensor_energy_monthly_wh[i] = new esphome::sensor::Sensor();
      snprintf(name_buf, sizeof(name_buf), "CT%d Energy Monthly", i);
      sensor_energy_monthly_wh[i]->set_name(name_buf);
    }
  }
  if (!sensor_voltage_rms) {
    sensor_voltage_rms = new esphome::sensor::Sensor();
    sensor_voltage_rms->set_name("Mains Voltage");
  }

  // Configure EmonLib instances: current per CT, same voltage pin/cal/phase for all
  for (int i = 0; i < 4; ++i) {
    if (ct_pins_[i] >= 0) {
      emon_ct_[i].current(ct_pins_[i], double(ct_cal_[i]));
      if (voltage_pin_ >= 0) {
        emon_ct_[i].voltage(voltage_pin_, double(voltage_cal_), voltage_phase_);
      }
    }
  }

  last_update_ms_ = millis();

  // Only set initial reset times if not loaded from preferences
  time_t now = ::time(nullptr);
  if (last_day_reset_ == 0) last_day_reset_ = now;
  if (last_week_reset_ == 0) last_week_reset_ = now;
  if (last_month_reset_ == 0) last_month_reset_ = now;
}

void Enermon::maybe_reset_counters_() {
  time_t now = ::time(nullptr);
  if (now == ((time_t) 0)) return;
  struct tm *tm_now = localtime(&now);
  struct tm *tm_last = localtime(&last_day_reset_);

  bool changed = false;

  if (tm_now->tm_yday != tm_last->tm_yday || tm_now->tm_year != tm_last->tm_year) {
    for (int i = 0; i < 4; ++i) energy_daily_wh_[i] = 0.0;
    last_day_reset_ = now;
    changed = true;
  }

  int week_now = (tm_now->tm_yday / 7) + tm_now->tm_year * 52;
  int week_last = (tm_last->tm_yday / 7) + tm_last->tm_year * 52;
  if (week_now != week_last) {
    for (int i = 0; i < 4; ++i) energy_weekly_wh_[i] = 0.0;
    last_week_reset_ = now;
    changed = true;
  }

  if (tm_now->tm_mon != tm_last->tm_mon || tm_now->tm_year != tm_last->tm_year) {
    for (int i = 0; i < 4; ++i) energy_monthly_wh_[i] = 0.0;
    last_month_reset_ = now;
    changed = true;
  }

  // Save counters if they were reset
  if (changed) {
    save_energy_counters_();
  }
}

void Enermon::update() {
  unsigned long now_ms = millis();
  
  // Calculate time elapsed since last update (in hours)
  if (last_update_ms_ == 0) {
    last_update_ms_ = now_ms;
    return;  // Skip first update to establish baseline
  }
  
  double elapsed_h = (now_ms - last_update_ms_) / 1000.0 / 3600.0;
  last_update_ms_ = now_ms;

  for (int i = 0; i < 4; ++i) {
    if (ct_pins_[i] < 0) continue;

    emon_ct_[i].calcVI(20, sample_count_);
    float irms = float(emon_ct_[i].Irms);
    float vrms = float(emon_ct_[i].Vrms);
    float real_p = float(emon_ct_[i].realPower);

    last_irms_[i] = irms;
    if (sensor_current_rms[i]) sensor_current_rms[i]->publish_state(irms);

    if (sensor_power_w[i]) sensor_power_w[i]->publish_state(real_p);

    last_vrms_ = vrms;
    if (sensor_voltage_rms) sensor_voltage_rms->publish_state(vrms);

    // Accumulate energy based on actual elapsed time
    double wh = real_p * elapsed_h;
    energy_daily_wh_[i] += wh;
    energy_weekly_wh_[i] += wh;
    energy_monthly_wh_[i] += wh;

    if (sensor_energy_daily_wh[i]) sensor_energy_daily_wh[i]->publish_state(energy_daily_wh_[i]);
    if (sensor_energy_weekly_wh[i]) sensor_energy_weekly_wh[i]->publish_state(energy_weekly_wh_[i]);
    if (sensor_energy_monthly_wh[i]) sensor_energy_monthly_wh[i]->publish_state(energy_monthly_wh_[i]);
  }

  // Save energy counters every 5 minutes to reduce flash wear
  if (now_ms - last_save_ms_ >= 300000) {  // 5 minutes
    save_energy_counters_();
    last_save_ms_ = now_ms;
  }

  maybe_reset_counters_();
}

void Enermon::loop() {
  // Sample every 1 second for accurate energy accumulation
  static unsigned long last_sample = 0;
  unsigned long now = millis();
  
  if (now - last_sample >= 1000) {
    last_sample = now;
    update();
  }
}

void Enermon::save_energy_counters_() {
  pref_daily_.save(&energy_daily_wh_);
  pref_weekly_.save(&energy_weekly_wh_);
  pref_monthly_.save(&energy_monthly_wh_);
  
  struct ResetTimes {
    time_t day;
    time_t week;
    time_t month;
  };
  ResetTimes times = {last_day_reset_, last_week_reset_, last_month_reset_};
  pref_reset_times_.save(&times);
}

void Enermon::load_energy_counters_() {
  std::array<double, 4> daily, weekly, monthly;
  
  if (pref_daily_.load(&daily)) {
    energy_daily_wh_ = daily;
  }
  if (pref_weekly_.load(&weekly)) {
    energy_weekly_wh_ = weekly;
  }
  if (pref_monthly_.load(&monthly)) {
    energy_monthly_wh_ = monthly;
  }
  
  struct ResetTimes {
    time_t day;
    time_t week;
    time_t month;
  };
  ResetTimes times;
  if (pref_reset_times_.load(&times)) {
    last_day_reset_ = times.day;
    last_week_reset_ = times.week;
    last_month_reset_ = times.month;
  }
}

void Enermon::dump_config() {
  ESP_LOGCONFIG("enermon", "Enermon Energy Monitor:");
  ESP_LOGCONFIG("enermon", "  Voltage Pin: %d", voltage_pin_);
  ESP_LOGCONFIG("enermon", "  Voltage Cal: %.1f", voltage_cal_);
  for (size_t i = 0; i < ct_pins_.size(); ++i) {
    if (ct_pins_[i] >= 0) {
      ESP_LOGCONFIG("enermon", "  CT%d Pin: %d, Cal: %.1f", i, ct_pins_[i], ct_cal_[i]);
    }
  }
}

}  // namespace enermon
}  // namespace esphome