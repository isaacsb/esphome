#include "m5stack_coreink_display.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace m5stack_coreink_display {

static const char *const TAG = "m5stack_coreink_display";

static const uint8_t LUT_SIZE = 42;
static const uint8_t LUT_VCOM_DC[LUT_SIZE] = {0x01, 0x04, 0x04, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
                                              0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                              0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t LUT_WHITE_TO_WHITE[LUT_SIZE] = {0x01, 0x04, 0x04, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
                                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t LUT_BLACK_TO_WHITE[LUT_SIZE] = {0x01, 0x84, 0x84, 0x83, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
                                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t LUT_WHITE_TO_BLACK[LUT_SIZE] = {0x01, 0x44, 0x44, 0x43, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
                                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t LUT_BLACK_TO_BLACK[LUT_SIZE] = {0x01, 0x04, 0x04, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
                                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void M5StackCoreInkDisplay::setup_pins_() {
  this->init_internal_(this->get_buffer_length_());
  this->dc_pin_->setup();  // OUTPUT
  this->dc_pin_->digital_write(false);
  this->reset_pin_->setup();  // OUTPUT
  this->reset_pin_->digital_write(true);
  this->busy_pin_->setup();  // INPUT
  this->spi_setup();

  this->reset_();
}

void M5StackCoreInkDisplay::setup_prev_buffer_() {
  if (this->prev_buffer_ != nullptr) {
    ExternalRAMAllocator<uint8_t> allocator(ExternalRAMAllocator<uint8_t>::ALLOW_FAILURE);
    this->prev_buffer_ = allocator.allocate(get_buffer_length_());
    if (this->prev_buffer_ == nullptr) {
      ESP_LOGE(TAG, "Could not allocate buffer for display!");
      return;
    }
    std::fill_n(prev_buffer_, get_buffer_length_(), 0);
  }
}

float M5StackCoreInkDisplay::get_setup_priority() const { return setup_priority::PROCESSOR; }
void M5StackCoreInkDisplay::command(uint8_t value) {
  this->start_command_();
  this->write_byte(value);
  this->end_command_();
}
void M5StackCoreInkDisplay::data(uint8_t value) {
  this->start_data_();
  this->write_byte(value);
  this->end_data_();
}
bool M5StackCoreInkDisplay::wait_until_idle_() {
  const uint32_t start = millis();
  while (this->busy_pin_->digital_read()) {
    if (millis() - start > this->idle_timeout_()) {
      ESP_LOGE(TAG, "Timeout while displaying image!");
      return false;
    }
    delay(10);
  }
  return true;
}

int M5StackCoreInkDisplay::waitbusy_(uint16_t time) {
  while (!this->busy_pin_->digital_read() && (time > 0)) {
    time--;
    delay(1);
  }
  return time;
}

void M5StackCoreInkDisplay::update() {
  this->do_update_();
  this->display();
}
void M5StackCoreInkDisplay::fill(Color color) {
  // flip logic
  const uint8_t fill = color.is_on() ? 0x00 : 0xFF;
  for (uint32_t i = 0; i < this->get_buffer_length_(); i++)
    this->buffer_[i] = fill;
}
void HOT M5StackCoreInkDisplay::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (x >= this->get_width_internal() || y >= this->get_height_internal() || x < 0 || y < 0)
    return;

  const uint32_t pos = (x + y * this->get_width_internal()) / 8u;
  const uint8_t subpos = x & 0x07;
  // flip logic
  if (!color.is_on()) {
    this->buffer_[pos] |= 0x80 >> subpos;
  } else {
    this->buffer_[pos] &= ~(0x80 >> subpos);
  }
}
uint32_t M5StackCoreInkDisplay::get_buffer_length_() {
  return this->get_width_internal() * this->get_height_internal() / 8u;
}
void M5StackCoreInkDisplay::start_command_() {
  this->dc_pin_->digital_write(false);
  this->enable();
}
void M5StackCoreInkDisplay::end_command_() { this->disable(); }
void M5StackCoreInkDisplay::start_data_() {
  this->dc_pin_->digital_write(false);
  this->enable();
}
void M5StackCoreInkDisplay::end_data_() { this->disable(); }
void M5StackCoreInkDisplay::on_safe_shutdown() { this->deep_sleep(); }

void M5StackCoreInkDisplay::deep_sleep() {
  this->command(0x50);
  this->data(0xF7);
  this->command(0x02);  // power off
  this->wait_until_idle_();
  this->command(0x07);  // deep sleep
  this->data(0xA5);
}

void M5StackCoreInkDisplay::initialize() {
  // panel setting
  this->command(0x00);
  this->data(0xDF);
  this->data(0x0E);
  // FITIinternal code
  this->command(0x4D);
  this->data(0x55);
  // resolution setting
  this->command(0xAA);
  this->data(0x0F);
  this->command(0xE9);
  this->data(0x02);
  this->command(0xB6);
  this->data(0x11);
  this->command(0xF3);
  this->data(0x0A);
  this->command(0x61);
  this->data(0xC8);
  this->data(0x00);
  this->data(0xC8);
  // Tcon setting
  this->command(0x60);
  this->data(0x00);
  // Power on
  this->command(0x50);
  this->data(0xD7);
  this->command(0xE3);
  this->data(0x00);
  this->command(0x04);  // no data
}
void M5StackCoreInkDisplay::dump_config() {
  LOG_DISPLAY("", "M5Stack CoreInk Display", this);
  ESP_LOGCONFIG(TAG, "  Full Update Every: %u", this->full_update_every_);
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
  LOG_PIN("  DC Pin: ", this->dc_pin_);
  LOG_PIN("  CS Pin: ", this->cs_);
  LOG_PIN("  Busy Pin: ", this->busy_pin_);
  LOG_UPDATE_INTERVAL(this);
}

void HOT M5StackCoreInkDisplay::full_update_() { this->initialize(); }
void HOT M5StackCoreInkDisplay::partial_update_(bool prev_full) {
  if (prev_full) {
    this->initialize();
  }
  this->command(0x00);
  this->data(0xFF);
  this->data(0x0E);

  this->write_lut_(0x20, LUT_VCOM_DC, LUT_SIZE);
  this->write_lut_(0x21, LUT_WHITE_TO_WHITE, LUT_SIZE);
  this->write_lut_(0x22, LUT_BLACK_TO_WHITE, LUT_SIZE);
  this->write_lut_(0x23, LUT_WHITE_TO_BLACK, LUT_SIZE);
  this->write_lut_(0x24, LUT_BLACK_TO_BLACK, LUT_SIZE);
}

void HOT M5StackCoreInkDisplay::set_draw_addr_(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
  // Enter partial mode
  this->command(0x91);
  // Resolution setting
  this->command(0x90);

  // X start and end
  this->data(x);
  this->data(x + width - 1);
  this->data(0);  // reserved

  // Y start and end
  this->data(y);
  this->data(0);  // reserved
  this->data(y + height);
  this->data(0x01);
}

void HOT M5StackCoreInkDisplay::display() {
  bool full_update = this->at_update_ == 0;
  bool prev_full_update = this->at_update_ == 1;

  if (!this->wait_until_idle_()) {
    this->status_set_warning();
    return;
  }

  if (this->full_update_every_ >= 1) {
    if (full_update != prev_full_update) {
      if (full_update) {
        this->full_update_();
      } else {
        this->partial_update_(prev_full_update);
      }
    }
    this->at_update_ = (this->at_update_ + 1) % this->full_update_every_;
  }

  this->set_draw_addr_(0, 0, this->get_width_internal(), this->get_height_internal() >> 3);

  if (!this->wait_until_idle_()) {
    this->status_set_warning();
    return;
  }

  this->command(0x10);
  for (int i = 0; i < this->get_buffer_length_(); ++i) {
    data(this->prev_buffer_[i]);
  }
  if (!this->wait_until_idle_()) {
    this->status_set_warning();
    return;
  }

  this->command(0x13);
  for (int i = 0; i < this->get_buffer_length_(); ++i) {
    data(this->buffer_[i]);
    this->prev_buffer_[i] = buffer_[i];
  }
  if (!this->wait_until_idle_()) {
    this->status_set_warning();
    return;
  }

  this->command(0x12);

  this->status_clear_warning();
}

void M5StackCoreInkDisplay::write_lut_(uint8_t reg, const uint8_t *lut, const uint8_t size) {
  // COMMAND WRITE LUT REGISTER
  this->command(reg);
  for (uint8_t i = 0; i < size; i++)
    this->data(lut[i]);
}
void M5StackCoreInkDisplay::set_full_update_every(uint32_t full_update_every) {
  this->full_update_every_ = full_update_every;
}

}  // namespace m5stack_coreink_display
}  // namespace esphome
