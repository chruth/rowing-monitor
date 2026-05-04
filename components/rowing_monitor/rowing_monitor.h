#pragma once

#include "esphome/components/api/custom_api_device.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace rowing_monitor {

enum StrokePhase : uint8_t {
  PHASE_IDLE,
  PHASE_ENTERED,
  PHASE_TRACK_MIN,
  PHASE_RECOVERY,
};

class RowingMonitorComponent : public Component, public api::CustomAPIDevice {
 public:
  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::HARDWARE - 1.0f; }

  void set_step1_pin(InternalGPIOPin *pin) { this->step1_pin_ = pin; }
  void set_step2_pin(InternalGPIOPin *pin) { this->step2_pin_ = pin; }
  void set_reset_pin(InternalGPIOPin *pin) { this->reset_pin_ = pin; }

  void set_spm_sensor(sensor::Sensor *s) { this->spm_sensor_ = s; }
  void set_distance_sensor(sensor::Sensor *s) { this->distance_sensor_ = s; }
  void set_total_strokes_sensor(sensor::Sensor *s) { this->total_strokes_sensor_ = s; }
  void set_active_time_sensor(sensor::Sensor *s) { this->active_time_sensor_ = s; }
  void set_uptime_sensor(sensor::Sensor *s) { this->uptime_sensor_ = s; }
  void set_valid_strokes_sensor(sensor::Sensor *s) { this->valid_strokes_sensor_ = s; }
  void set_short_strokes_sensor(sensor::Sensor *s) { this->short_strokes_sensor_ = s; }
  void set_micro_strokes_sensor(sensor::Sensor *s) { this->micro_strokes_sensor_ = s; }

 protected:
  InternalGPIOPin *step1_pin_{nullptr};
  InternalGPIOPin *step2_pin_{nullptr};
  InternalGPIOPin *reset_pin_{nullptr};

  sensor::Sensor *spm_sensor_{nullptr};
  sensor::Sensor *distance_sensor_{nullptr};
  sensor::Sensor *total_strokes_sensor_{nullptr};
  sensor::Sensor *active_time_sensor_{nullptr};
  sensor::Sensor *uptime_sensor_{nullptr};
  sensor::Sensor *valid_strokes_sensor_{nullptr};
  sensor::Sensor *short_strokes_sensor_{nullptr};
  sensor::Sensor *micro_strokes_sensor_{nullptr};

  HighFrequencyLoopRequester high_freq_;

  uint8_t prev_state_{0};
  int32_t travel_{0};

  StrokePhase phase_{PHASE_IDLE};
  int32_t stroke_min_travel_{0};
  uint32_t stroke_enter_ms_{0};

  float distance_m_{0};
  float current_spm_{0};
  float active_time_s_{0};
  uint32_t uptime_s_{0};
  uint32_t total_strokes_{0};
  uint32_t valid_strokes_{0};
  uint32_t short_strokes_{0};
  uint32_t micro_strokes_{0};

  uint32_t last_movement_activity_ms_{0};
  uint32_t last_accepted_stroke_ms_{0};
  uint32_t prev_accepted_stroke_for_spm_ms_{0};

  uint32_t last_publish_ms_{0};

  bool reset_last_sample_{false};
  uint32_t reset_sample_change_ms_{0};
  bool reset_stable_pressed_prev_{false};

  void process_quadrature_(uint32_t now_ms);
  void advance_stroke_fsm_(int8_t delta, uint32_t now_ms);
  void complete_stroke_(uint32_t now_ms);

  uint32_t last_activity_reference_ms_() const;
  bool movement_reference_live_(uint32_t now_ms) const;
  void tick_spm_decay_(uint32_t now_ms);

  void process_reset_pin_(uint32_t now_ms);

  void perform_session_reset_();

  void publish_every_sensor_();
};

}  // namespace rowing_monitor
}  // namespace esphome
