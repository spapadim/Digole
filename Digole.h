#ifndef Digole_h
#define Digole_h

#include <inttypes.h>
//#if defined(ESP8266)
#  define assert(x) 
//#else
//#  define __ASSERT_USE_STDERR
//#  include <assert.h>
//#endif

#include <Arduino.h>
#include <Print.h>

#if defined(DIGOLE_I2C) && DIGOLE_I2C
#include <Wire.h>
#endif // DIGOLE_I2C

#if defined(DIGOLE_SPI) && DIGOLE_SPI
#include <SPI.h>
#endif // DIGOLE_SPI

#include "Digole_config.h"

#if defined(REENTRANT) && REENTRANT
#  define _STATICBUF
#else
#  define _STATICBUF static
#endif

namespace Digole {

struct Color {
  uint8_t r, g, b;

  Color(uint8_t r_ = 0, uint8_t g_ = 0, uint8_t b_ = 0)
  : r(r_), g(g_), b(b_) { }
              
  inline explicit operator uint8_t() const {
    return ((r & 0xe0) | ((g &0xe0) >> 3) | ((b & 0xc0) >> 6));
  }

  inline explicit operator uint16_t() const {
    return (((uint16_t)r & 0xf8) << 8) | 
           (((uint16_t)g & 0xfc) << 3) |
           (((uint16_t)b & 0xf8) >> 3);
  }

  inline Color operator^ (const Color &other) const {
    return Color(r ^ other.r, g ^ other.g, b ^ other.b);
  }

  inline Color operator& (const Color &other) const {
    return Color(r & other.r, g & other.g, b & other.b);
  }

  inline Color operator| (const Color &other) const {
    return Color(r | other.r, g | other.g, b | other.b);
  }

  inline Color operator~ () const {
    return Color(~r, ~g, ~b);
  }
};


enum orientation_t : char {
  ROT0 = '0',
  ROT90 = '1',
  ROT180 = '2',
  ROT270 = '3'
};

enum bitmap_t : uint8_t {
  BITMAP_8,
  BITMAP_256,
  BITMAP_262K
};

enum text_position_t : uint8_t {
  CHARACTER,
  PIXEL
};

enum touch_mode_t : uint8_t {
  TOUCH_DOWN = 'W',
  TOUCH_DOWN_NONBLOCKING = 'I',
  TOUCH_UP = 'C'  // aka "click"
};

enum draw_mode_t : uint8_t {
  MODE_COPY = 'C',
  MODE_NOT = '!',
  MODE_OR = '|',
  MODE_XOR = '^',
  MODE_AND = '&'
};

enum lcd_chip_t : uint8_t {
  CHIP_ST7920 = '0',
  CHIP_KS0108 = '1',
  CHIP_ST7565 = '2'
};

  
template <class COM>
class DigoleDisplay : public Print {
public:

  // The following four methods implement compile-time inheritance
  // (inspired by http://hackaday.io/project/6038 ; see also
  //  https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern)
  inline size_t writeRaw (uint8_t c) {
    return (static_cast<COM*>(this))->_writeRaw(c);
  }
  inline size_t writeRaw (const uint8_t *buffer, size_t size) {
    return (static_cast<COM*>(this))->_writeRaw(buffer, size);
  }
  inline uint8_t read() {
    return (static_cast<COM*>(this))->_read(); 
  }
  inline uint16_t readInt() {
    return (static_cast<COM*>(this))->_readInt();
  }


  inline size_t writeRaw(const char *str) {
    if (str == NULL) return 0;
    return writeRaw((const uint8_t *)str, strlen(str));
  }

  inline size_t writeRaw(const char *buffer, size_t size) {
    return writeRaw((const uint8_t *)buffer, size);
  }

  inline size_t writeRawInt(uint16_t v) {
    _STATICBUF uint8_t buf[2];
    size_t s = copyRawInt(buf, v);
    return writeRaw(buf, s);
  }

  inline size_t copyRawInt(uint8_t *buf, uint16_t v) {
    if (v < 255) {
      buf[0] = (uint8_t)v;
      return 1;
    } else {
      buf[0] = 255;
      buf[1] = (uint8_t)(v - 255);
      return 2;
    }
  }

  // TODO: protected
  inline void _newline() {
    writeRaw("TRT", 3);
  }

  /**** Print virtual methods ****/

  size_t write(uint8_t c) override {
    if (c == '\r' || c =='\n') {
      _newline();
    } else {
      _STATICBUF char buf[4] = {'T', 'T', 'c', '\x0d'};
      buf[2] = c;
      return (writeRaw(buf, 4) > 0) ? 1 : 0;
    }
  }

  size_t write(const uint8_t *buffer, size_t size) override {
    
    const uint8_t *p = buffer;

    while (true) {
      const uint8_t *endl;
      // Look for newline or for end of buffer
      // Note: unlike Unix, the '\r' is interpreted the same way as an '\n'
      for (endl = p; (endl < buffer + size) && *endl != '\n' && *endl != '\r'; endl++)
        ;
      // Write this line's text, if non-empty
      if (endl > p) {
        writeRaw("TT", 2);
        writeRaw(p, endl - p);
        writeRaw('\x0d');
      }
      // Advance index pointer
      p = endl;

      // If string ended, stop
      if (p >= buffer + size)
        break;

      // Else, there must have been a newline
      _newline();
      p += 1; // Skip the '\r' or '\n'
      // Treat the "\r\n" sequence as one newline
      if (p[-1] == '\r' || p[0] == '\n') {
        p += 1;  // Skip an extra character, the '\n'
      }
    }
  }


  /**** Settings ****/

  void setCursor(bool enabled) {
    writeRaw(enabled ? "CS1" : "CS0", 3);
  }

  void setDisplayConfig(bool enabled) {
    writeRaw(enabled ? "DC\x1" : "DC\x0", 3);
  }

  void setRotation(orientation_t orient) {
    _STATICBUF uint8_t cmd[3] = { 'S', 'D', 'x' };  // w/o 'x' machine code is larger
    cmd[2] = orient;
    writeRaw(cmd, 3);
  }

  void setContrast(uint8_t v) {
    _STATICBUF uint8_t cmd[3] = { 'C', 'T', 'x' };
    cmd[2] = v;
    writeRaw(cmd, 3);
  }

  void setBacklight(uint8_t v) {
    _STATICBUF uint8_t cmd[3] = { 'B', 'L', 'x' };
    cmd[2] = v;
    writeRaw(cmd, 3);
  }

  void setColor(uint8_t r, uint8_t g, uint8_t b) {
     _STATICBUF uint8_t cmd[6] = { 'E', 'S', 'C', 'r', 'g', 'b' };
    cmd[3] = r;
    cmd[4] = g;
    cmd[5] = b;
    writeRaw(cmd, 6);
    //Serial.print("ESC: "); Serial.print(r); Serial.print(','); Serial.print(g); Serial.print(','); Serial.println(b);  // DEBUG
  }

  void setColor(uint8_t color) {
    _STATICBUF uint8_t cmd[3] = { 'S', 'C', 'x' };
    cmd[2] = color;
    writeRaw(cmd, 3);
  }

  void setColor(const Color &color, bool to_8bit = false) {
    if (to_8bit) {
      setColor(static_cast<uint8_t>(color));
    } else {
      setColor(color.r >> 2, color.g >> 2, color.b >> 2);
    }
  }

  void setBackgroundColor() {
    writeRaw("BGC", 3);
  }

  void setDrawMode(draw_mode_t mode) {
    _STATICBUF uint8_t cmd[3] = { 'D', 'M', 'x' };
    cmd[2] = mode;
    writeRaw(cmd, 3);
  }


  /**** Drawing ****/

  void clearScreen() {
    writeRaw("CL", 2);
  }

  void drawPixel(uint16_t x, uint16_t y, uint8_t color = 1) {
    _STATICBUF uint8_t cmd[7] = { 'D', 'P', 'x', 'x', 'y', 'y', 'c' };
    uint8_t *p = cmd + 2;
    p += copyRawInt(p, x);
    p += copyRawInt(p, y);
    *p = color;
    ++p;  // FIXME
    writeRaw(cmd, p - cmd);
  }

  void drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    _STATICBUF uint8_t cmd[10] = {
      'L', 'N', 'x', 'x', 'y', 'y', 'x', 'x', 'y', 'y'
    };
    uint8_t *p = cmd + 2;
    p += copyRawInt(p, x0);
    p += copyRawInt(p, y0);
    p += copyRawInt(p, x1);
    p += copyRawInt(p, y1);
    writeRaw(cmd, p - cmd);
  }

  void drawLineTo(uint16_t x, uint16_t y) {
    _STATICBUF uint8_t cmd[6] = { 'L', 'T', 'x', 'x', 'y', 'y' };
    uint8_t *p = cmd + 2;
    p += copyRawInt(p, x);
    p += copyRawInt(p, y);
    writeRaw(cmd, p - cmd);
  }

  void drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool filled = false) {
    _STATICBUF uint8_t cmd[10] = {
      'D', 'R', 'x', 'x', 'y', 'y', 'x', 'x', 'y', 'y'
    };
    if (filled)
      cmd[0] = 'F';
    uint8_t *p = cmd + 2;
    p += copyRawInt(p, x);
    p += copyRawInt(p, y);
    p += copyRawInt(p, x + w);
    p += copyRawInt(p, y + h);
    writeRaw(cmd, p - cmd);
  }

  void drawCircle(uint16_t x, uint16_t y, uint16_t r, bool filled = false) {
    _STATICBUF uint8_t cmd[9] = {
      'C', 'C', 'x', 'x', 'y', 'y', 'r', 'r', 'f'
    };
    uint8_t *p = cmd + 2;
    p += copyRawInt(p, x);
    p += copyRawInt(p, y);
    p += copyRawInt(p, r);
    *p = (filled ? 1 : 0);
    p++;  // FIXME
    writeRaw(cmd, p - cmd);
  }

  void drawBitmap(bitmap_t type,
                  uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                  const uint8_t *data) {
    // TODO: Test
    _STATICBUF uint8_t hdr[11] = {
      'E', 'D', 'I', 'M', 'n', 'x', 'x', 'y', 'y', 'w', 'w', 'h', 'h'
    };
    uint8_t *p = 0;
    switch (type) {
    case BITMAP_8:
      memcpy(hdr, "DIM", 3);
      p = hdr + 3;
      break;
    case BITMAP_256:
      hdr[4] = '1';
      p = hdr + 5;
      break;
    case BITMAP_262K:
      hdr[4] = '3';
      p = hdr + 5;
      break;
    default:
      assert(false);
      // Should not happen!
    } 
    p += copyRawInt(x);
    p += copyRawInt(y);
    p += copyRawInt(w);
    p += copyRawInt(h);
    writeRaw(hdr, p - hdr);

    const int num_bytes = (type == BITMAP_8) ? h * ((w + 7) / 8) : h * w;
    for (int j = 0;  j < num_bytes;  j++)
      writeRaw(pgm_read_byte_near(data+ j)); // TODO: Chunked?
  }

  void moveArea (uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                 uint8_t dx, uint8_t dy) {
    _STATICBUF uint8_t cmd[12] = {
      'M', 'A', 'x', 'x', 'y', 'y', 'x', 'x', 'y', 'y', 'd', 'd'
    };
    uint8_t *p = cmd + 2;
    p += copyRawInt(p, x0);
    p += copyRawInt(p, y0);
    p += copyRawInt(p, x1);
    p += copyRawInt(p, y1);
    *p = dx;
    p++;  // FIXME
    *p = dy;
    p++;
    writeRaw(cmd, p - cmd);
  }

  void setLinePattern(uint8_t pattern) {
    _STATICBUF uint8_t cmd[4] = { 'S', 'L', 'P', 'p' };
    cmd[3] = pattern;
    writeRaw(cmd, 4);
  }

  void setGraphicsPosition(uint16_t x, uint16_t y) {
    _STATICBUF uint8_t cmd[6] = { 'G', 'P', 'x', 'x', 'y', 'y' };
    uint8_t *p = cmd + 2;
    p += copyRawInt(p, x);
    p += copyRawInt(p, y);
    writeRaw(cmd, p - cmd);
  }


  void setDrawWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    _STATICBUF uint8_t cmd[13] = {
      'D', 'W', 'W', 'I', 'N', 'x', 'x', 'y', 'y', 'w', 'w', 'h', 'h'
    };
    uint8_t *p = cmd + 5;
    p += copyRawInt(p, x);
    p += copyRawInt(p, y);
    p += copyRawInt(p, w);
    p += copyRawInt(p, h);
    writeRaw(cmd, p - cmd);
  }

  void resetDrawWindow() {
    writeRaw("RSTDW", 5);
  }

  void clearDrawWindow() {
    writeRaw("WINCL", 5);
  }


  /**** Text ****/

  void setFont(uint8_t font) {
    _STATICBUF uint8_t cmd[3] = { 'S', 'F', 'x' };
    cmd[2] = font;
    writeRaw(cmd, 3);
  }

  void setTextPosition(uint16_t x, uint16_t y, text_position_t unit = CHARACTER) {
    _STATICBUF uint8_t cmd[7] = { 'E', 'T', 'P', 'x', 'x', 'y', 'y' };
    uint8_t *p = cmd + 3;
    p += copyRawInt(p, x);
    p += copyRawInt(p, y);
    if (unit == PIXEL)
      writeRaw(cmd, p - cmd);
    else
      writeRaw(cmd + 1, p - (cmd + 1));  // Caution: this only works because command prefixes are similar
  }

  void backspace() {
    writeRaw("ETB", 3);
  }

  void setTextPositionOffset(uint8_t dx, uint8_t dy) {
    _STATICBUF uint8_t cmd[5] = { 'E', 'T', 'O', 'x', 'y' };
    cmd[3] = dx;
    cmd[4] = dy;
    writeRaw(cmd, 5);
  }


  /**** Touchscreen ****/

  void calibrateTouchscreen() {
    writeRaw("TUCHC", 5);
  }

  // CAUTION: Docs claim that a no-touch in non-blocking (instant) mode
  //   will return both coordinates x, y == 0xffff, 0xffff,
  //   but that does NOT seem to be the case; seems, hower, that
  //   at least one of the coordinates will be larger than 0xff00

  void readTouchscreen(uint16_t &x, uint16_t &y, touch_mode_t mode) {
    _STATICBUF uint8_t cmd[6] = { 'R', 'P', 'N', 'X', 'Y', 'm' };
    cmd[5] = mode;
    writeRaw(cmd, 6);
    //delay(5);  // DEBUG; Wait for ADC
    x = readInt();
    y = readInt();
    //delay(10);  // DEBUG; Wait for ADC ?
  }

  uint16_t readBattery() {
    writeRaw("RDBAT", 5);
    return readInt();
  }

  uint16_t readAux() {
    writeRaw("RDAUX", 5);
    return readInt();
  }

  uint16_t readTemperature() {
    writeRaw("RDTMP", 5);
    return readInt();
  }


  /**** Fonts and splashscreen ****/

  void uploadStartScreen(const uint8_t *data, uint16_t length) {
    _STATICBUF uint8_t hdr[5] = { 'S', 'S', 'S', 'l', 'l' };
    hdr[3] = (uint8_t)(length && 0xff);
    hdr[4] = (uint8_t)((length >> 8) && 0xff);
    writeRaw(hdr, 5);
    delay(300);
    _writeData(data, length);
  }

  void uploadUserFont(uint8_t section, const uint8_t *data, uint16_t length) {
    // Each section is 4K; if font is larger, contiguous sections may be used,
    // according to docs
    assert(section < 4 - length / 4096);

    while (true) {
      _STATICBUF uint8_t hdr[6] = { 'S', 'U', 'F', 's', 'l', 'l' };
      hdr[3] = section;
      hdr[4] = (uint8_t)(length && 0xff);
      hdr[5] = (uint8_t)((length >> 8) && 0xff);
      writeRaw(hdr, 6);
      delay(200);
      _writeData(data, length);

      // We have to check here because length is unsigned
      // (can't have length > 0 in loop condition)
      if (length < 4096)
        break;

      length -= 4096;
      data += 4096;
      ++section;
    }
  }

  // TODO: protected
  // TODO: chunked?
  void _writeData(const uint8_t *data, uint16_t length) {
    for (int j = 0;  j < length;  j++) {
      if (j % 32 == 0)
        delay(50);
      delay(6);
      writeRaw(pgm_read_byte_near(data + j));
    }
  }

  /**** Flash ****/
  void flashErase (uint32_t address, uint32_t length) {
    _STATICBUF uint8_t buf[11] = {
      'F', 'L', 'M', 'E', 'R', 'a', 'a', 'a', 'l', 'l', 'l'
    };
    _copyInt24(buf + 5, address);
    _copyInt24(buf + 8, length);
    writeRaw(buf, 11);
  }

  //void flashReadStart(unsigned long int addr, unsigned long int len);
  void flashRead(uint8_t *dest, uint32_t address, uint32_t length) {
    _STATICBUF uint8_t buf[11] = {
      'F', 'L', 'M', 'R', 'D', 'a', 'a', 'a', 'l', 'l', 'l'
    };
    _copyInt24(buf + 5, address);
    _copyInt24(buf + 8, length);
    writeRaw(buf, 11);

    // TODO XXX
    // Next part is not described in documentation, and is untested
    for (uint32_t i = 0;  i < length;  i++) {
      *dest = read();
      dest++; // FIXME
      if (i % 32 == 0)
        yield();
    }
  }

  void flashWrite(uint32_t address, const uint8_t *data, uint32_t length) {
    static const uint16_t chunk_size = 1024;

    while (length > 1024) {
      _flashWriteChunk(address, data, chunk_size);
      data += chunk_size;
      address += chunk_size;
      length -= chunk_size;
    }
    if (length > 0)
      _flashWriteChunk(address, data, chunk_size);
  }

  // TODO: protected
  inline void _copyInt24(uint8_t *dest, uint32_t val) {
    dest[0] = (uint8_t)((val >> 16) & 0xff);
    dest[1] = (uint8_t)((val >> 8)  & 0xff);
    dest[2] = (uint8_t)( val        & 0xff);
  }

  // TODO: protected
  void _flashWriteChunk(uint32_t address, const uint8_t *data, uint16_t length) {
    _STATICBUF uint8_t buf[11] = {
      'F', 'L', 'M', 'W', 'R', 'a', 'a', 'a', 0, 'l', 'l'
    };
    _copyInt24(buf + 5, address);
    _copyInt24(buf + 8, length);
    writeRaw(buf, 11);

    for (uint16_t i = 0;  i < 0;  i++)
      writeRaw(pgm_read_byte_near(data + i));

    // Wait for ack (XON)
    while (read() != 17) yield();
  }

  void setFlashFont (uint32_t address) {
    _STATICBUF uint8_t buf[6] = { 'S', 'F', 'F', 'a', 'a', 'a' };
    _copyInt24(buf + 3, address);
    writeRaw(buf, 6);
  }

  void runFlashCommandSet (uint32_t address) {
    _STATICBUF uint8_t buf[8] = {
      'F', 'L', 'M', 'C', 'S', 'a', 'a', 'a'
    };
    _copyInt24(buf + 5, address);
    writeRaw(buf, 8);
  }
  

  /**** Low-level ****/

  void setLCDChip(lcd_chip_t chip) {
    _STATICBUF uint8_t cmd[5] = { 'S', 'L', 'C', 'D', 'x' };
    cmd[4] = chip;
    writeRaw(cmd, 5);
  }

  void setLCDSize(uint8_t cols, uint8_t rows) {
    _STATICBUF uint8_t cmd[10] = {
      'S', 'T', 'C', 'R', 'c', 'r', '\x80', '\xC0', '\x94', '\xD4'
    };
    cmd[4] = cols;
    cmd[5] = rows;
    writeRaw(cmd, 10);
  }

  void sendRawCommand(uint8_t command) {
    _STATICBUF uint8_t cmd[4] = { 'M', 'C', 'D', 'x' };
    cmd[3] = command;
    writeRaw(cmd, 4);
  }

  void sendRawData(uint8_t v) {
    _STATICBUF uint8_t cmd[4] = { 'M', 'D', 'T', 'x' };
    cmd[3] = v;
    writeRaw(cmd, 4);
  }

  void digitalWrite(uint8_t v) {
    _STATICBUF uint8_t cmd[5] = { 'D', 'O', 'U', 'T', 'x' };
    cmd[4] = v;
    writeRaw(cmd, 5);
  }

private:
};


#if defined(DIGOLE_SERIAL) && DIGOLE_SERIAL

class DigoleSerial : public DigoleDisplay<DigoleSerial> {
public:
  const static uint16_t RESET_PULSE = 100;   // in ms; how long to hold low
  const static uint16_t RESET_DELAY = 1000;  // in ms; how long to wait afterwards

  DigoleSerial(HardwareSerial &serial, unsigned long baud = 115200, uint8_t reset_pin = 0xff) :
    _serial(serial), _baud(baud), _reset_pin(reset_pin) { }

  void begin() {
    if (_reset_pin != 0xff) {
       pinMode(_reset_pin, OUTPUT);
       ::digitalWrite(_reset_pin, LOW);
       delay(RESET_PULSE);
       ::digitalWrite(_reset_pin, HIGH);
       delay(RESET_DELAY);
    }
    _serial.begin(9600);
#if defined(ESP8266)
    if (_reset_pin == 0xff) {
      _serial.setDebugOutput(0);
      _serial.println();  // Try to make display recover from ESP boot msgs
      delay(100);
    }
#endif
    _serial.print("SB");
    _serial.println(_baud);
    delay(100);
    _serial.begin(_baud);
#if defined(ESP8266)
    if (_reset_pin == 0xff) {
      _serial.setDebugOutput(0);  // paranoia, yeah!
      _serial.println();  // why the heck not...
      delay(100);
    }
#endif
  }

//protected:
  size_t _writeRaw (uint8_t c) {
    return _serial.write(c);
  }

  size_t _writeRaw (const uint8_t *buffer, size_t size) {
    return _serial.write(buffer, size);
  }

  uint8_t _read() {
    while (!_serial.available()) yield();
    return _serial.read();
  }

  uint16_t _readInt() {
    // Note: this was forgotten in the original DigoleSerial, guessing
    uint16_t v = (uint16_t)_read() << 8;
    v |= (uint16_t)read();
    return v;
  }

private:
  HardwareSerial &_serial;
  unsigned long _baud;
  uint8_t _reset_pin;
};

#endif  // DIGOLE_SERIAL


#if defined(DIGOLE_I2C) && DIGOLE_I2C

class DigoleI2C : public DigoleDisplay<DigoleI2C> {
public:
  DigoleI2C(TwoWire &wire, uint8_t i2c_addr = 0x27, unsigned long clock = 50000)
    : _wire(wire), _i2c_addr(i2c_addr), _clock(clock) { }

  void begin() {
    _wire.begin();
    _wire.setClock(_clock);
  }

#if defined(ESP8266)
  void begin(int sda, int scl) {
    _wire.begin(sda, scl);
    _wire.setClock(_clock);
  }
#endif

  void setI2CAddress (uint8_t i2c_addr) {
    _STATICBUF uint8_t cmd[6] = { 'S', 'I', '2', 'C', 'A', 'x' };
    cmd[5] = i2c_addr;
    writeRaw(cmd, 6);
    _i2c_addr = i2c_addr;
  }


//protected:
  size_t _writeRaw (uint8_t c) {
    _wire.beginTransmission(_i2c_addr);
    _wire.write(c);
    uint8_t status = _wire.endTransmission();
    return (status == 0) ? 1 : 0;
  }

  size_t _writeRaw (const uint8_t *buffer, size_t size) {
#if 0  // Alternative, less efficient(?) implementation
    for (size_t i = 0;  i < size;  i++)
      _writeRaw(buffer[i]);
    return size;
#endif

#if 1
    _wire.beginTransmission(_i2c_addr);
    _wire.write(buffer, size);
    uint8_t status = _wire.endTransmission();
    return (status == 0) ? size : 0;
#endif

#if 0  // TODO FIXME - Dumps core
    size_t remain = size;
    while (remain > 0) {
      _wire.beginTransmission(_i2c_addr);
      _wire.write(buffer, min(remain, BUFFER_LENGTH));
      uint8_t status = _wire.endTransmission();
      if (status == 0)
        return size - remain;  // Caveat: if status == 3, some bytes from current chunk may actually have been transmitted, but we can't know how many
      remain -= BUFFER_LENGTH;
      buffer += BUFFER_LENGTH;
    }
    return size;
#endif
  }

  uint8_t _read() {
    if (_wire.requestFrom(_i2c_addr, (uint8_t)1) != 1) {
      Serial.println("_read fail!"); Serial.flush();  // DEBUG
      return 0xff;
    }
    while (!_wire.available()) yield();
    return _wire.read();
  }

  uint16_t _readInt() {
    if (_wire.requestFrom(_i2c_addr, (uint8_t)2) != 2) {
      Serial.println("_readInt fail!"); Serial.flush();  // DEBUG
      return 0xffff;
    }
    while (!_wire.available()) yield();
    uint16_t v = (uint16_t)_wire.read() << 8;
    while (!_wire.available()) yield();
    v |= (uint16_t)_wire.read();
    return v;
  }

private:
  TwoWire &_wire;
  uint8_t _i2c_addr;
  unsigned long _clock;
};

#endif  // DIGOLE_I2C

#if defined(DIGOLE_SPI) && DIGOLE_SPI
class DigoleSoftSPI : public DigoleDisplay<DigoleSoftSPI> {
public:
  DigoleSoftSPI(uint8_t ss, uint8_t mosi, uint8_t miso, uint8_t clk) :
    _clk_pin(clk), _miso_pin(miso), _ss_pin(ss), _mosi_pin(mosi) { }

  void begin() {
    pinMode(_clk_pin, OUTPUT);
    pinMode(_miso_pin, OUTPUT);
    pinMode(_ss_pin, OUTPUT);
    pinMode(_mosi_pin, INPUT);  // needs pulldown

    ::digitalWrite(_ss_pin, HIGH);
    ::digitalWrite(_clk_pin, LOW);
    ::digitalWrite(_miso_pin, LOW);
  }

//protected:
  size_t _writeRaw (uint8_t c) {
    ::digitalWrite(_ss_pin, LOW);
    delayMicroseconds(6);
    shiftOut(_miso_pin, _clk_pin, MSBFIRST, c);
    ::digitalWrite(_ss_pin, HIGH);
    return 1;
  }

  size_t _writeRaw (const uint8_t *buffer, size_t size) {
    for (size_t i = 0;  i < size;  i++)
      _writeRaw(buffer[i]);
    return size;
  }

  uint8_t _read() {
    while (digitalRead(_mosi_pin) == LOW) yield();
    ::digitalWrite(_ss_pin, LOW);
    delayMicroseconds(10);
    uint8_t v = shiftIn(_mosi_pin, _clk_pin, MSBFIRST);
    ::digitalWrite(_ss_pin, HIGH);
    return v;
  }

  uint16_t _readInt() {
    // Note: this was forgotten in the original DigoleSerial, guessing
    uint16_t v = (uint16_t)_read() << 8;
    v |= (uint16_t)read();
    return v;
  }

private:
  uint8_t _clk_pin, _miso_pin, _ss_pin, _mosi_pin;
};

class DigoleSPI : public DigoleDisplay<DigoleSPI> {
public:
  DigoleSPI(uint8_t ss, uint8_t mosi) :
    _ss_pin(ss), _mosi_pin(mosi), _spi_settings(100000, MSBFIRST, SPI_MODE1) { }

  void begin() {
    pinMode(_ss_pin, OUTPUT);
    ::digitalWrite(_ss_pin, HIGH);
    SPI.begin();
  }

//protected:
  size_t _writeRaw (uint8_t c) {
    SPI.beginTransaction(_spi_settings);
    ::digitalWrite(_ss_pin, LOW);
    delayMicroseconds(8);
    SPI.transfer(c);
    ::digitalWrite(_ss_pin, HIGH);
    SPI.endTransaction();
    return 1;
  }

  size_t _writeRaw (const uint8_t *buffer, size_t size) {
#if 0  // Alternative, less efficient implementation
    for (size_t i = 0;  i < size;  i++)
      _writeRaw(buffer[i]);
    return size;
#endif
#if 1  // Digole cannot keep up with this, even at 100KHz
    SPI.beginTransaction(_spi_settings);
    ::digitalWrite(_ss_pin, LOW);
    delayMicroseconds(8);
#if defined(ESP8266)
    SPI.writeBytes(const_cast<uint8_t *>(buffer), size);
#else
    for (size_t i = 0;  i < size;  i++)
      SPI.transfer(buffer[i]);
#endif
    ::digitalWrite(_ss_pin, HIGH);
    SPI.endTransaction();
    return size;
#endif
  }

  uint8_t _read() {
    while (digitalRead(_mosi_pin) == LOW) yield();
    ::digitalWrite(_ss_pin, LOW);
    delayMicroseconds(10);
    SPI.beginTransaction(_spi_settings);
    uint8_t v = SPI.transfer(0x00);
    SPI.endTransaction();
    ::digitalWrite(_ss_pin, HIGH);
    return v;
  }

  uint16_t _readInt() {
    // Note: this was forgotten in the original DigoleSerial, guessing
    uint16_t v = (uint16_t)_read() << 8;
    v |= (uint16_t)read();
    return v;
  }

private:
  uint8_t _ss_pin, _mosi_pin;
  SPISettings _spi_settings;
};

#endif  // DIGOLE_SPI



} // namespace Digole


#endif /* Digole_h */
