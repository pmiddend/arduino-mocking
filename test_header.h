#pragma once

#include <stdint.h>
#include <iostream>
#include <vector>

#define F_CPU 1000000UL

#define HIGH 0x1
#define LOW  0x0

#define INPUT 0x0
#define OUTPUT 0x1

#define INPUT_PULLUP 0x2
#define clockCyclesPerMicrosecond() ( F_CPU / 1000000L )
#define clockCyclesToMicroseconds(a) ( (a) / clockCyclesPerMicrosecond() )
#define microsecondsToClockCycles(a) ( (a) * clockCyclesPerMicrosecond() )

unsigned long my_current_time_us = 0;

std::vector<int> protocol;
size_t current_protocol_element = 0;

void pinMode(uint8_t const pin, uint8_t const mode) {
  std::cout << "pin " << static_cast<int>(pin) << ": " << (mode == INPUT ? "in" : mode == INPUT_PULLUP ? "pullup" : "out") << "\n";
}

void digitalWrite(uint8_t const pin, uint8_t const val) {
  std::cout << "pin " << static_cast<int>(pin) << ": write " << static_cast<int>(val) << "\n";
}

int digitalRead(uint8_t const pin) {
  if (current_protocol_element >= protocol.size()) {
    std::cout << "pin " << static_cast<int>(pin) << ", read => no data\n";
    return 0;
  }
  int const protocol_element = protocol[current_protocol_element++];
  int const result = protocol_element == pin ? HIGH : LOW;
  std::cout << "pin " << static_cast<int>(pin) << ", read => " << result << "\n";
  return result;
}

void interrupts() {
  std::cout << "enabling interrupts\n";
}

void noInterrupts() {
  std::cout << "disabling interrupts\n";
}

void delay(unsigned long ms) {
  std::cout << "waiting " << ms << "ms\n";
  my_current_time_us += ms*1000;
}

void delayMicroseconds(unsigned int us) {
  std::cout << "waiting " << us << "us\n";
  my_current_time_us += us;
}

unsigned long millis() {
  return my_current_time_us*1000;
}

unsigned long micros() {
  return my_current_time_us;
}

// uint8_t portOutputRegister(int) {
  
// }

// int digitalPinToPort() {
// }

// int digitalPinToBitMask(int) {
// }

#define MY_LOG(x) std::cout << x << "\n"
