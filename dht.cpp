#include "test_header.h"

#ifndef MY_LOG
#define MY_LOG(x)
#endif

typedef unsigned long time_type;
typedef int pin_type;

uint8_t const begin_usec = 55;
time_type const min_interval = 2000;
pin_type const my_pin = 13;
time_type last_read_time = 0;
uint8_t pull_time = begin_usec;
bool last_result = false;
uint8_t data[5];
time_type max_cycles = 0;
uint32_t const TIMEOUT = UINT32_MAX;

void dht_begin() {
  // set up the pins!
  pinMode(my_pin, INPUT_PULLUP);
  // Using this value makes sure that millis() - lastreadtime will be
  // >= MIN_INTERVAL right away. Note that this assignment wraps around,
  // but so will the subtraction.
  last_read_time = millis() - min_interval;
  last_result = false;
#ifdef __AVR
  _bit = digitalPinToBitMask(pin);
  _port = digitalPinToPort(pin);
#endif
  max_cycles =
      microsecondsToClockCycles(1000); // 1 millisecond timeout for
                                       // reading pulses from DHT sensor.

}

uint32_t dht_expect_pulse(bool const level) {
#if (F_CPU > 16000000L)
  uint32_t count = 0;
#else
  uint16_t count = 0; // To work fast enough on slower AVR boards
#endif
// On AVR platforms use direct GPIO port access as it's much faster and better
// for catching pulses that are 10's of microseconds in length:
#ifdef __AVR
  uint8_t portState = level ? _bit : 0;
  while ((*portInputRegister(_port) & _bit) == portState) {
    if (count++ >= max_cycles) {
      return TIMEOUT; // Exceeded timeout, fail.
    }
  }
// Otherwise fall back to using digitalRead (this seems to be necessary on
// ESP8266 right now, perhaps bugs in direct port access functions?).
#else
  while (digitalRead(my_pin) == level) {
    if (count++ >= max_cycles) {
      return TIMEOUT; // Exceeded timeout, fail.
    }
  }
#endif

  return count;
}

bool dht_read(bool const force) {
  time_type const current_time = millis();
  if (!force && ((current_time - last_read_time) < min_interval)) {
    MY_LOG("Returning last result");
    return last_result;
  }
  MY_LOG("Computing new result(s)");
  last_read_time = current_time;
  data[0] = data[1] = data[2] = data[3] = data[4] = 0;

  // Send start signal.  See DHT datasheet for full signal diagram:
  //   http://www.adafruit.com/datasheets/Digital%20humidity%20and%20temperature%20sensor%20AM2302.pdf

  // Go into high impedence state to let pull-up raise data line level and
  // start the reading process.
  MY_LOG("Setting pin to pullup, waiting");
  pinMode(my_pin, INPUT_PULLUP);
  delay(1);

  // First set data line low for a period according to sensor type
  MY_LOG("Setting pin to output, writing, then waiting");
  pinMode(my_pin, OUTPUT);
  digitalWrite(my_pin, LOW);
  delayMicroseconds(1100); // data sheet says "at least 1ms"

  uint32_t cycles[80];
  // End the start signal by setting data line high for 40 microseconds.
  MY_LOG("Setting high for 40us");
  pinMode(my_pin, INPUT_PULLUP);

  // Delay a moment to let sensor pull data line low.
  delayMicroseconds(pull_time);

  // Now start reading the data line to get the value from the DHT sensor.

  // Turn off interrupts temporarily because the next sections
  // are timing critical and we don't want any interruptions.
  noInterrupts();

  // First expect a low signal for ~80 microseconds followed by a high signal
  // for ~80 microseconds again.
  MY_LOG("Expect low for ~80us");
  if (dht_expect_pulse(LOW) == TIMEOUT) {
    MY_LOG("Timeout");
    last_result = false;
    return last_result;
  }
  MY_LOG("Expect high for ~80us");
  if (dht_expect_pulse(HIGH) == TIMEOUT) {
    MY_LOG("Timeout");
    last_result = false;
    return last_result;
  }

  // Now read the 40 bits sent by the sensor.  Each bit is sent as a 50
  // microsecond low pulse followed by a variable length high pulse.  If the
  // high pulse is ~28 microseconds then it's a 0 and if it's ~70 microseconds
  // then it's a 1.  We measure the cycle count of the initial 50us low pulse
  // and use that to compare to the cycle count of the high pulse to determine
  // if the bit is a 0 (high state cycle count < low state cycle count), or a
  // 1 (high state cycle count > low state cycle count). Note that for speed
  // all the pulses are read into a array and then examined in a later step.
  for (int i = 0; i < 80; i += 2) {
    MY_LOG("Reading low for bit " << i);
    cycles[i] = dht_expect_pulse(LOW);
    MY_LOG("Reading high for bit " << i);
    cycles[i + 1] = dht_expect_pulse(HIGH);
  }
  interrupts();

  MY_LOG("Now evaluating bits");

  // Inspect pulses and determine which ones are 0 (high state cycle count < low
  // state cycle count), or 1 (high state cycle count > low state cycle count).
  for (int i = 0; i < 40; ++i) {
    uint32_t lowCycles = cycles[2 * i];
    uint32_t highCycles = cycles[2 * i + 1];
    if ((lowCycles == TIMEOUT) || (highCycles == TIMEOUT)) {
      last_result = false;
      return last_result;
    }
    data[i / 8] <<= 1;
    // Now compare the low and high cycle times to see if the bit is a 0 or 1.
    if (highCycles > lowCycles) {
      MY_LOG("Bit " << i << " = 1");
      // High cycles are greater than 50us low cycle count, must be a 1.
      data[i / 8] |= 1;
    } else {
      MY_LOG("Bit " << i << " = 0");
    }
    // Else high cycles are less than (or equal to, a weird case) the 50us low
    // cycle count so this must be a zero.  Nothing needs to be changed in the
    // stored data.
  }

  // Check we read 40 bits and that the checksum matches.
  if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
    MY_LOG("checksum matches, all clear");
    last_result = true;
    return last_result;
  }
  MY_LOG("checksum failure");
  last_result = false;
  return last_result;
}

float dht_read_humidity() {
  dht_read(false);
  float const result = (((uint16_t)data[0]) << 8 | data[1]) * 0.1;
  MY_LOG("humidity bytes are " << std::hex << static_cast<int>(data[0]) << "." << static_cast<int>(data[1]));
  MY_LOG("humidity is " << result);
  return result;
}

void setup() {
  dht_begin();
}

void loop() {
  float humidity = dht_read_humidity();
}

#include "test_footer.h"
