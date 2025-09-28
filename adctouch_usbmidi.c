/*

ADC-aided capacitive touch sensing + USB_MIDI.

This demonstrates use of the ADC with cap sense pads. By using the ADC,
you can get much higher resolution than if you use the exti.  But, you are
limited by the number of ADC lines, i.e. you can only support a maximum
of eight (8) touch inputs with this method.

In exchange for that, typically you can get > 1,000 LSBs per 1 CM^2 and
it can convert faster.  Carefully written code can be as little as 2us per
conversion!

The mechanism of operation for the touch sensing on the CH32V003 is to:
* Start the ADC
* Hold an IO low.
* Use the internal pull-up to pull the line high.
* The ADC will sample the voltage on the slope.
* Lower voltage = longer RC respone, so higher capacitance.

Based on:
* ch32v003fun examples
* rv003usb examples

*/

#include "ch32fun.h"
#include "rv003usb.h"
#include <stdio.h>

#include "ch32v003_touch.h"

#define ADC_PIN_COUNT 2
#define ITERATIONS 3
#define TOUCH_THRESHOLD 1000

const uint8_t MIDI_NOTES[ADC_PIN_COUNT] = {60, 62}; // C4, D4

typedef struct {
  volatile uint8_t len;
  uint8_t buffer[8];
} midi_message_t;

midi_message_t midi_in;

#define midi_send_ready() (midi_in.len == 0)

// midi data from device -> host (IN)
void midi_send(uint8_t *msg, uint8_t len) {
  memcpy(midi_in.buffer, msg, len);
  midi_in.len = len;
}

void midi_send_note(uint8_t note, bool on) {
  uint8_t msg[4] = {0x09, on ? 0x90 : 0x80, note, 0x7f};
  while (!midi_send_ready()) {
  };
  midi_send(msg, 4);
}

int main() {
  SystemInit();
  printf("Capacitive Touch ADC + USB MIDI\n");
  usb_setup();
  printf("USB configured\n");

  // Enable GPIOD, C and ADC
  RCC->APB2PCENR |= RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1;

  InitTouchADC();

  uint8_t touchDetected[ADC_PIN_COUNT] = {0};
  uint32_t ref[ADC_PIN_COUNT] = {0};

  // get reference values
  ref[0] = ReadTouchPin(GPIOA, 2, 0, ITERATIONS);
  ref[1] = ReadTouchPin(GPIOA, 1, 1, ITERATIONS);

  while (1) {
    uint32_t sum[ADC_PIN_COUNT] = {0};

    // Sampling all touch pads, 3x should take 6030 cycles, and runs at about
    // 8kHz

    // read touch pins
    sum[0] += ReadTouchPin(GPIOA, 2, 0, ITERATIONS);
    sum[1] += ReadTouchPin(GPIOA, 1, 1, ITERATIONS);

    for (uint8_t i = 0; i < ADC_PIN_COUNT; i++) {
      sum[i] = sum[i] - ref[i];

      if (sum[i] > TOUCH_THRESHOLD && !touchDetected[i]) {
        touchDetected[i] = 1;
        printf("Pad %d touched! %d\n", i, (int)sum[i]);
        midi_send_note(MIDI_NOTES[i], 1);
      } else if (sum[i] <= TOUCH_THRESHOLD && touchDetected[i]) {
        touchDetected[i] = 0;
        midi_send_note(MIDI_NOTES[i], 0);
        printf(" Pad %d released! %d\n", i, (int)sum[i]);
      }
    }
  }
}

void usb_handle_user_in_request(struct usb_endpoint *e, uint8_t *scratchpad,
                                int endp, uint32_t sendtok,
                                struct rv003usb_internal *ist) {
  if (endp && midi_in.len) {
    usb_send_data(midi_in.buffer, midi_in.len, 0, sendtok);
    midi_in.len = 0;
  } else {
    usb_send_empty(sendtok);
  }
}

void usb_handle_other_control_message(struct usb_endpoint *e, struct usb_urb *s,
                                      struct rv003usb_internal *ist) {
  LogUEvent(SysTick->CNT, s->wRequestTypeLSBRequestMSB, s->lValueLSBIndexMSB,
            s->wLength);
}

void usb_handle_user_data(struct usb_endpoint *e, int current_endpoint,
                          uint8_t *data, int len,
                          struct rv003usb_internal *ist) {
  // if (len) midi_receive(data);
  // if (len==8) midi_receive(&data[4]);
}