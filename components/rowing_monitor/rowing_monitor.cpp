#include "rowing_monitor.h"
#include <algorithm>

namespace esphome {
namespace rowing_monitor {

static constexpr int8_t QUAD_TABLE[16] = {
    0, -1, +1, 0,
    +1, 0, 0, -1,
    -1, 0, 0, +1,
    0, +1, -1, 0,
};

static constexpr int32_t TOP_ENTER_TH = -3;
static constexpr int32_t TOP_LEAVE_TH = -5;
static constexpr int32_t BOTTOM_TH = -10;
static constexpr int32_t SHORT_TH = -8;
static constexpr int32_t MICRO_TH = -4;

static constexpr uint32_t MIN_STROKE_MS = 700;
static constexpr uint32_t ACTIVE_IDLE_MS = 1500;
static constexpr uint32_t DEBOUNCE_MS = 30;

static constexpr float METERS_VALID = 0.6667f;
static constexpr float METERS_SHORT = 0.5f * METERS_VALID;
static constexpr float METERS_MICRO = 0.25f * METERS_VALID;

uint32_t RowingMonitorComponent::last_activity_reference_ms_() const {
  uint32_t m = this->last_movement_activity_ms_;
  if (this->last_accepted_stroke_ms_ != 0)
    m = std::max(m, this->last_accepted_stroke_ms_);
  return m;
}

bool RowingMonitorComponent::movement_reference_live_(uint32_t now_ms) const {
  return (now_ms - this->last_activity_reference_ms_()) <= ACTIVE_IDLE_MS;
}

void RowingMonitorComponent::setup() {
  uint32_t n = millis();
  this->last_movement_activity_ms_ = n;
  this->last_publish_ms_ = n;
  int s1 = this->step1_pin_->digital_read() ? 1 : 0;
  int s2 = this->step2_pin_->digital_read() ? 1 : 0;
  this->prev_state_ = static_cast<uint8_t>((s1 << 1) | s2);
  this->reset_last_sample_ = this->reset_pin_->digital_read();
  this->reset_sample_change_ms_ = n;
  this->reset_stable_pressed_prev_ = false;
  this->high_freq_.start();
}

void RowingMonitorComponent::tick_spm_decay_(uint32_t now_ms) {
  if (!this->movement_reference_live_(now_ms))
    this->current_spm_ = 0.0f;
}

void RowingMonitorComponent::process_quadrature_(uint32_t now_ms) {
  int s1 = this->step1_pin_->digital_read() ? 1 : 0;
  int s2 = this->step2_pin_->digital_read() ? 1 : 0;
  auto state = static_cast<uint8_t>((s1 << 1) | s2);
  auto index = static_cast<uint8_t>((this->prev_state_ << 2) | state);
  int8_t delta = QUAD_TABLE[index];
  this->prev_state_ = state;

  if (delta != 0) {
    this->last_movement_activity_ms_ = now_ms;
    this->travel_ += delta;
    this->advance_stroke_fsm_(delta, now_ms);
  }
}

void RowingMonitorComponent::advance_stroke_fsm_(int8_t delta, uint32_t now_ms) {
  switch (this->phase_) {
    case PHASE_IDLE:
      if (this->travel_ <= TOP_ENTER_TH) {
        this->phase_ = PHASE_ENTERED;
        this->stroke_enter_ms_ = now_ms;
      }
      break;

    case PHASE_ENTERED:
      if (this->travel_ > TOP_ENTER_TH) {
        this->phase_ = PHASE_IDLE;
      } else if (this->travel_ <= TOP_LEAVE_TH) {
        this->phase_ = PHASE_TRACK_MIN;
        this->stroke_min_travel_ = this->travel_;
      }
      break;

    case PHASE_TRACK_MIN:
      if (this->travel_ > TOP_ENTER_TH) {
        this->phase_ = PHASE_IDLE;
        break;
      }
      this->stroke_min_travel_ = std::min(this->stroke_min_travel_, this->travel_);
      if (delta > 0)
        this->phase_ = PHASE_RECOVERY;
      break;

    case PHASE_RECOVERY:
      if (this->travel_ > TOP_ENTER_TH)
        this->complete_stroke_(now_ms);
      break;
  }
}

void RowingMonitorComponent::complete_stroke_(uint32_t now_ms) {
  uint32_t duration = now_ms - this->stroke_enter_ms_;
  this->phase_ = PHASE_IDLE;

  if (duration < MIN_STROKE_MS)
    return;

  int32_t min_t = this->stroke_min_travel_;
  bool accepted = false;

  if (min_t <= BOTTOM_TH) {
    this->valid_strokes_++;
    this->total_strokes_++;
    this->distance_m_ += METERS_VALID;
    accepted = true;
  } else if (min_t <= SHORT_TH) {
    this->short_strokes_++;
    this->total_strokes_++;
    this->distance_m_ += METERS_SHORT;
    accepted = true;
  } else if (min_t <= MICRO_TH) {
    this->micro_strokes_++;
    this->total_strokes_++;
    this->distance_m_ += METERS_MICRO;
    accepted = true;
  }

  if (!accepted)
    return;

  this->last_accepted_stroke_ms_ = now_ms;

  if (this->prev_accepted_stroke_for_spm_ms_ == 0) {
    this->current_spm_ = 0.0f;
  } else {
    this->current_spm_ = 60000.0f / static_cast<float>(now_ms - this->prev_accepted_stroke_for_spm_ms_);
  }
  this->prev_accepted_stroke_for_spm_ms_ = now_ms;
}

void RowingMonitorComponent::perform_session_reset_() {
  this->distance_m_ = 0;
  this->total_strokes_ = 0;
  this->valid_strokes_ = 0;
  this->short_strokes_ = 0;
  this->micro_strokes_ = 0;
  this->active_time_s_ = 0;
  this->current_spm_ = 0.0f;

  this->phase_ = PHASE_IDLE;
  this->stroke_min_travel_ = 0;

  this->prev_accepted_stroke_for_spm_ms_ = 0;
  this->last_accepted_stroke_ms_ = 0;

#ifdef USE_API_HOMEASSISTANT_SERVICES
  this->fire_homeassistant_event("esphome.rower_new_session");
#endif
}

void RowingMonitorComponent::process_reset_pin_(uint32_t now_ms) {
  const bool raw = this->reset_pin_->digital_read();

  if (raw != this->reset_last_sample_) {
    this->reset_last_sample_ = raw;
    this->reset_sample_change_ms_ = now_ms;
    return;
  }

  if (now_ms - this->reset_sample_change_ms_ < DEBOUNCE_MS)
    return;

  // Configure YAML so digital_read()==true represents a pressed switch (normally inverted GPIO + pull-up).
  const bool stable_pressed = raw;
  if (stable_pressed && !this->reset_stable_pressed_prev_)
    this->perform_session_reset_();
  this->reset_stable_pressed_prev_ = stable_pressed;
}

void RowingMonitorComponent::publish_every_sensor_() {
  if (this->spm_sensor_ != nullptr)
    this->spm_sensor_->publish_state(this->current_spm_);
  if (this->distance_sensor_ != nullptr)
    this->distance_sensor_->publish_state(this->distance_m_);
  if (this->total_strokes_sensor_ != nullptr)
    this->total_strokes_sensor_->publish_state(static_cast<float>(this->total_strokes_));
  if (this->active_time_sensor_ != nullptr)
    this->active_time_sensor_->publish_state(this->active_time_s_);
  if (this->uptime_sensor_ != nullptr)
    this->uptime_sensor_->publish_state(static_cast<float>(this->uptime_s_));
  if (this->valid_strokes_sensor_ != nullptr)
    this->valid_strokes_sensor_->publish_state(static_cast<float>(this->valid_strokes_));
  if (this->short_strokes_sensor_ != nullptr)
    this->short_strokes_sensor_->publish_state(static_cast<float>(this->short_strokes_));
  if (this->micro_strokes_sensor_ != nullptr)
    this->micro_strokes_sensor_->publish_state(static_cast<float>(this->micro_strokes_));
}

void RowingMonitorComponent::loop() {
  const uint32_t now_ms = millis();

  this->process_reset_pin_(now_ms);
  this->process_quadrature_(now_ms);
  this->tick_spm_decay_(now_ms);

  if (now_ms - this->last_publish_ms_ < 1000)
    return;
  this->last_publish_ms_ = now_ms;

  this->uptime_s_++;

  if (this->movement_reference_live_(now_ms) &&
      (this->current_spm_ > 0.0f || this->phase_ != PHASE_IDLE)) {
    this->active_time_s_ += 1.0f;
  }

  this->publish_every_sensor_();
}

}  // namespace rowing_monitor
}  // namespace esphome
