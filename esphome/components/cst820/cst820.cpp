#include "cst820.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace cst820 {

static const char *const TAG = "cst820";

void CST820TouchscreenStore::gpio_intr(CST820TouchscreenStore *store) { store->touch = true; }

void CST820Touchscreen::setup() {
  ESP_LOGCONFIG(TAG, "Setting up CST820 Touchscreen...");

  this->reset_pin_->pin_mode(gpio::Flags::FLAG_OUTPUT);
  this->reset_pin_->setup();

  this->interrupt_pin_->pin_mode(gpio::Flags::FLAG_INPUT | gpio::Flags::FLAG_PULLUP);
  this->interrupt_pin_->setup();

  this->store_.pin = this->interrupt_pin_->to_isr();
  this->interrupt_pin_->attach_interrupt(CST820TouchscreenStore::gpio_intr, &this->store_,
                                         gpio::INTERRUPT_FALLING_EDGE);

  this->hard_reset_();

  // The CST820 has a high and low pulse, every ~10 milliseconds
  uint32_t timeout = millis() + 50;
  while (timeout > millis()) {
    while (!this->interrupt_pin_->digital_read()) {
      delay(20);
      timeout = millis() + 50;
    }
  }

  this->store_.touch = false;

  uint8_t enable_gesture = 0x01;
  this->write_register(CST820_GESTURE_STATE_REGISTER, &enable_gesture, 1);

  uint8_t value = 0;
  this->read_register(CST820_GESTURE_STATE_REGISTER, &value, 1);
  ESP_LOGI(TAG, "Is gestures enabled? %s", format_hex_pretty(value).c_str());
}

void CST820Touchscreen::dump_config() {
  ESP_LOGCONFIG(TAG, "CST820 Touchscreen:");
  LOG_I2C_DEVICE(this);
  LOG_PIN("  Interrupt Pin: ", this->interrupt_pin_);
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
}

void CST820Touchscreen::loop() {
  if (!this->store_.touch)
    return;
  this->store_.touch = false;

  uint8_t touch_count = 0;

  uint8_t data[7] = {0};
  this->read_register(CST820_WORK_MODE_REGISTER, data, sizeof(data));

  touch_count = data[CST820_TOUCH_NUMBER_REGISTER] & 0x0F;

  if (touch_count == 0) {
    for (auto *listener : this->touch_listeners_) {
      listener->release();
    }
    return;
  }

  TouchPoint tp;
  uint16_t x;
  uint16_t y;
  switch (touch_count) {
    case 0:
      tp.state = (uint8_t) data[CST820_TOUCH_1_XH_REGISTER] >> 6;

      x = ((data[CST820_TOUCH_1_XH_REGISTER] & 0x0F) << 8 | data[CST820_TOUCH_1_XL_REGISTER]);
      y = ((data[CST820_TOUCH_1_YH_REGISTER] & 0x0F) << 8 | data[CST820_TOUCH_1_YL_REGISTER]);
      break;

    case 1:
      tp.state = (uint8_t) data[CST820_TOUCH_1_XH_REGISTER] >> 6;

      x = ((data[CST820_TOUCH_1_XH_REGISTER] & 0x0F) << 8 | data[CST820_TOUCH_1_XL_REGISTER]);
      y = ((data[CST820_TOUCH_1_YH_REGISTER] & 0x0F) << 8 | data[CST820_TOUCH_1_YL_REGISTER]);
      break;

    default:
      ESP_LOGV(TAG, "Touch count parameter out range (must be between 0 and 1).");
      break;
  }

  tp.id = touch_count;

  //ESP_LOGI(TAG, "X: %i, Y: %i, ID %i, R: %i, Ri: %i", x, y, tp.id, this->display_->get_rotation(), this->rotation_);

  switch (this->display_->get_rotation()) {
    case ROTATE_0_DEGREES:
      tp.x = x;
      tp.y = y;
      break;
    case ROTATE_90_DEGREES:
      tp.x = y;
      tp.y = this->display_width_ - x;
      break;
    case ROTATE_180_DEGREES:
      tp.x = this->display_width_ - x;
      tp.y = this->display_height_ - y;
      break;
    case ROTATE_270_DEGREES:
      tp.x = x;
      tp.y = this->display_height_ - y;
      break;
  }

  // ESP_LOGI(TAG, "TP X: %i, Y: %i", tp.x, tp.y);

  this->defer([this, tp]() { this->send_touch_(tp); });
}

void CST820Touchscreen::hard_reset_() {
  this->reset_pin_->digital_write(false);
  delay(300);  // NOLINT
  this->reset_pin_->digital_write(true);
  delay(300);  // NOLINT
}

}  // namespace cst820
}  // namespace esphome
