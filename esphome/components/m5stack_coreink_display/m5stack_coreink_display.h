#pragma once

#include "esphome/core/component.h"
#include "esphome/components/spi/spi.h"
#include "esphome/components/display/display_buffer.h"

namespace esphome {
namespace m5stack_coreink_display {

class M5StackCoreInkDisplay : public PollingComponent,
                              public display::DisplayBuffer,
                              public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST, spi::CLOCK_POLARITY_HIGH,
                                                    spi::CLOCK_PHASE_TRAILING, spi::DATA_RATE_10MHZ> {
 public:
  void set_dc_pin(GPIOPin *dc_pin) { dc_pin_ = dc_pin; }
  float get_setup_priority() const override;
  void set_reset_pin(GPIOPin *reset) { this->reset_pin_ = reset; }
  void set_busy_pin(GPIOPin *busy) { this->busy_pin_ = busy; }
  void set_reset_duration(uint32_t reset_duration) { this->reset_duration_ = reset_duration; }

  void command(uint8_t value);
  void data(uint8_t value);

  void display();
  void initialize();
  void deep_sleep();

  void update() override;

  void fill(Color color) override;

  void setup() override {
    this->setup_prev_buffer_();
    this->setup_pins_();
    this->initialize();
  }

  void on_safe_shutdown() override;

  display::DisplayType get_display_type() override { return display::DisplayType::DISPLAY_TYPE_BINARY; }

  void set_full_update_every(uint32_t full_update_every);

  void dump_config() override;

 protected:
  void draw_absolute_pixel_internal(int x, int y, Color color) override;

  bool wait_until_idle_();

  int waitbusy_(uint16_t time);

  void setup_pins_();

  void setup_prev_buffer_();

  void reset_() {
    if (this->reset_pin_ != nullptr) {
      this->reset_pin_->digital_write(false);
      delay(reset_duration_);  // NOLINT
      this->reset_pin_->digital_write(true);
      delay(200);  // NOLINT
    }
  }

  void write_lut_(uint8_t reg, const uint8_t *lut, uint8_t size);
  void full_update_();
  void partial_update_(bool prev_full);
  void set_draw_addr_(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

  int get_width_internal() override { return 200; }

  int get_height_internal() override { return 200; }

  uint32_t full_update_every_{30};
  uint32_t at_update_{0};

  uint32_t get_buffer_length_();
  uint32_t reset_duration_{840};

  void start_command_();
  void end_command_();
  void start_data_();
  void end_data_();

  uint8_t *prev_buffer_{nullptr};

  GPIOPin *reset_pin_;
  GPIOPin *dc_pin_;
  GPIOPin *busy_pin_;
  uint32_t idle_timeout_() { return 1000u; }  // NOLINT(readability-identifier-naming)
};

}  // namespace m5stack_coreink_display
}  // namespace esphome
