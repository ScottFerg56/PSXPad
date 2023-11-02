/* gutted, simple abstraction version of the Arduino DigitalIO Library by William Greiman */

#ifndef DigitalPin_h
#define DigitalPin_h

//==============================================================================
/**
 * @class DigitalPin
 * @brief digital port I/O abstraction layer
 */
template<uint8_t PinNumber>
class DigitalPin {
 public:
  //----------------------------------------------------------------------------
  /** Constructor */
  DigitalPin() {}
  //----------------------------------------------------------------------------
  /** Asignment operator.
   * @param[in] value If true set the pin's level high else set the
   *  pin's level low.
   *
   * @return This DigitalPin instance.
   */
  inline DigitalPin & operator = (bool value) __attribute__((always_inline)) {
    write(value);
    return *this;
  }
  //----------------------------------------------------------------------------
  /** Parenthesis operator.
   * @return Pin's level
   */
  inline operator bool () const __attribute__((always_inline)) {
    return read();
  }
  //----------------------------------------------------------------------------
  /** Set pin configuration.
   * @param[in] mode: INPUT or OUTPUT.
   * @param[in] level If mode is OUTPUT, set level high/low.
   */
  inline __attribute__((always_inline))
  void config(uint8_t mode, bool level) {
    pinMode(PinNumber, mode);
    if (mode == OUTPUT)
        digitalWrite(PinNumber, level);
  }
  //----------------------------------------------------------------------------
  /**
   * Set pin level high if output mode or enable 20K pullup if input mode.
   */
  inline __attribute__((always_inline))
  void high() {write(true);}
  //----------------------------------------------------------------------------
  /**
   * Set pin level low if output mode or disable 20K pullup if input mode.
   */
  inline __attribute__((always_inline))
  void low() {write(false);}
  //----------------------------------------------------------------------------
  /**
   * Set pin mode.
   * @param[in] mode: INPUT, OUTPUT, or INPUT_PULLUP.
   *
   * The internal pullup resistors will be enabled if mode is INPUT_PULLUP
   * and disabled if the mode is INPUT.
   */
  inline __attribute__((always_inline))
  void mode(uint8_t mode) {
    pinMode(PinNumber, mode);
  }
  //----------------------------------------------------------------------------
  /** @return Pin's level. */
  inline __attribute__((always_inline))
  bool read() const {
    return digitalRead(PinNumber);
  }
  //----------------------------------------------------------------------------
  /** Toggle a pin.
   *
   * If the pin is in output mode toggle the pin's level.
   */
  inline __attribute__((always_inline))
  void toggle() {
    digitalWrite(PinNumber, !digitalRead(PinNumber));
  }
  //----------------------------------------------------------------------------
  /** Write the pin's level.
   * @param[in] value If true set the pin's level high else set the
   *  pin's level low.
   */
  inline __attribute__((always_inline))
  void write(bool value) {
    digitalWrite(PinNumber, value);
  }
};
#endif  // DigitalPin_h
/** @} */
