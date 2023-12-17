#pragma once

#include "esphome/components/i2c/i2c.h"
#include "esphome/components/touchscreen/touchscreen.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace cst820 {

struct CST820TouchscreenStore {
  volatile bool touch;
  ISRInternalGPIOPin pin;

  static void gpio_intr(CST820TouchscreenStore *store);
};

using namespace touchscreen;

enum {
  CST820_WORK_MODE_REGISTER = 0x00,

  /**
   * BIT 7 ~ BIT 4:
   * BIT 3 ~ BIT 0: touch points[3:0]
   */
  CST820_TOUCH_NUMBER_REGISTER = 0x02,

  /**
   * BIT 7 ~ BIT 6: event_flag
   * BIT 5 ~ BIT 4:
   * BIT 3 ~ BIT 0: X_position[11:8]
   */
  CST820_TOUCH_1_XH_REGISTER = 0x03,

  /**
   * BIT 7 ~ BIT 0: X_position[7:0]
   */
  CST820_TOUCH_1_XL_REGISTER = 0x04,

  /**
   * BIT 7 ~ BIT 4: touch_ID[3:0]
   * BIT 3 ~ BIT 0: Y_position[11:8]
   */
  CST820_TOUCH_1_YH_REGISTER = 0x05,

  /**
   * BIT 7 ~ BIT 0: Y_position[7:0]
   */
  CST820_TOUCH_1_YL_REGISTER = 0x06,

  /**
   * 01H Enter gesture recognition mode
   * 00H Exit gesture mode
   */
  CST820_GESTURE_STATE_REGISTER = 0xD0,

  /**
   * Gesture mode must be enabled to be effective
   * double click: 0x24
   * up: 0x22
   * down: 0x23
   * left: 0x20
   * rignt: 0x21
   * C: 0x34
   * e:0x33
   * m:0x32
   * O: 0x30
   * S: 0x46
   * V: 0x54
   * W: 0x31
   * Z:0x65
   */
  CST820_GESTURE_ID_REGISTER = 0xD3,
};

class CST820Touchscreen : public Touchscreen, public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  void set_interrupt_pin(InternalGPIOPin *pin) { this->interrupt_pin_ = pin; }
  void set_reset_pin(GPIOPin *pin) { this->reset_pin_ = pin; }

  float get_setup_priority() const { return setup_priority::DATA; }

 protected:
  void hard_reset_();

  InternalGPIOPin *interrupt_pin_{nullptr};
  GPIOPin *reset_pin_{nullptr};
  CST820TouchscreenStore store_;
};

}  // namespace cst820
}  // namespace esphome
