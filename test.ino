/*
 * See documentation at https://nRF24.github.io/RF24
 * See License information at root directory of this library
 * Author: Brendan Doherty 2bndy5
 */

/**
 * A simple example of streaming data from 1 nRF24L01 transceiver to another.
 *
 * This example was written to be used on 2 devices acting as "nodes".
 * Use the Serial Monitor to change each node's behavior.
 */
#include <SPI.h>
#include "printf.h"
#include "RF24.h"

// instantiate an object for the nRF24L01 transceiver
RF24 radio(10, 9); // using pin 7 for the CE pin, and pin 8 for the CSN pin
const short TIME_COLUMN = 0;
const short STATE_COLUMN = 1;


// Let these addresses be used for the pair
uint8_t address[][6] = {"1Node", "2Node"};
// It is very helpful to think of an address as a path instead of as
// an identifying device destination

// to use different addresses on a pair of radios, we need a variable to
// uniquely identify which address this radio will use to transmit
bool radioNumber = 1; // 0 uses address[0] to transmit, 1 uses address[1] to transmit

// Used to control whether this node is sending or receiving
bool role = radioNumber;

// For this example, we'll be sending 32 payloads each containing
// 32 bytes of data that looks like ASCII art when printed to the serial
// monitor. The TX node and RX node needs only a single 32 byte buffer.
#define SIZE 2            // this is the maximum for this example. (minimum is 1)
const short COMMANDS_TO_SEND = 5;
unsigned int buffer[SIZE + 1];     // for the RX node
uint8_t counter = 0;       // for counting the number of received payloads
void makePayload(uint8_t); // prototype to construct a payload dynamically


void setup() {
  Serial.begin(115200);
  while (!Serial) {
    // some boards need to wait to ensure access to serial over USB
  }

  // initialize the transceiver on the SPI bus
  if (!radio.begin()) {
    Serial.println(F("radio hardware is not responding!!"));
    while (1) {} // hold in infinite loop
  }

  while (!Serial.available()) {
    // wait for user input
  }
  Serial.print(F("radioNumber = "));
  Serial.println((int)radioNumber);

  // Set the PA Level low to try preventing power supply related problems
  // because these examples are likely run with nodes in close proximity to
  // each other.
  radio.setPALevel(RF24_PA_LOW);  // RF24_PA_MAX is default.

  // save on transmission time by setting the radio to only transmit the
  // number of bytes we need to transmit
  radio.setPayloadSize(SIZE);     // default value is the maximum 32 bytes

  // set the TX address of the RX node into the TX pipe
  radio.openWritingPipe(address[radioNumber]);     // always uses pipe 0

  // set the RX address of the TX node into a RX pipe
  radio.openReadingPipe(1, address[!radioNumber]); // using pipe 1

  // additional setup specific to the node's role
  if (role) {
    radio.stopListening();  // put radio in TX mode
  } else {
    radio.startListening(); // put radio in RX mode
  }

  // For debugging info
  // printf_begin();             // needed only once for printing details
  // radio.printDetails();       // (smaller) function that prints raw register values
  // radio.printPrettyDetails(); // (larger) function that prints human readable data

} // setup()

bool dataSent = false;

void loop() {

  if (role) {
    // This device is a TX node

    radio.flush_tx();
    uint8_t i = 0;
    uint8_t failures = 0;
    unsigned long start_timer = micros();       // start the timer
    if (dataSent) {
      return;
    }

    while (i < COMMANDS_TO_SEND)
    {
      Serial.print("Value of 'i' before makePayload: ");
      Serial.println(i);
      makePayload(i);
      if (!radio.writeFast(&buffer, SIZE)) {
        failures++;
        radio.reUseTX();
      } else {
        i++;
      }

      if (failures >= 100) {
        Serial.print(F("Too many failures detected. Aborting at payload "));
        Serial.println(buffer[0]);
        break;
      }
    }
    // dataSent = true;
    unsigned long end_timer = micros();         // end the timer

    Serial.print(F("Time to transmit = "));
    Serial.print(end_timer - start_timer);      // print the timer result
    Serial.print(F(" us with "));
    Serial.print(failures);                     // print failures detected
    Serial.println(F(" failures detected"));

    // to make this example readable in the serial monitor
    delay(1000);  // slow transmissions down by 1 second

  } else {
    // This device is a RX node

    if (radio.available()) {         // is there a payload?
      radio.read(&buffer, SIZE);     // fetch payload from FIFO
      Serial.print("Time value on RCV: ");
      Serial.println(buffer[TIME_COLUMN]);
      Serial.print("State value on RCV: ");
      Serial.println(buffer[STATE_COLUMN]);
      Serial.print(F("Received: "));
      for (size_t i = 0; i < SIZE; i++)
      {
        Serial.print(buffer[i]);
        Serial.print(" ");
      }
      Serial.print(F(" / counter = "));
      Serial.println(counter++);     // print the received counter
    }
  } // role

  if (Serial.available()) {
    // change the role via the serial monitor

    char c = toupper(Serial.read());
    if (c == 'T' && !role) {
      // Become the TX node

      role = true;
      counter = 0; //reset the RX node's counter
      Serial.println(F("*** CHANGING TO TRANSMIT ROLE -- PRESS 'R' TO SWITCH BACK"));
      radio.stopListening();

    } else if (c == 'R' && role) {
      // Become the RX node

      role = false;
      Serial.println(F("*** CHANGING TO RECEIVE ROLE -- PRESS 'T' TO SWITCH BACK"));
      radio.startListening();
    }
  }

} // loop

unsigned int timeStateValue[COMMANDS_TO_SEND][2] { { 3000, 255 },
                                         { 4000, 0 },
                                         { 5000, 255 },
                                         { 6000, 0 },
                                         { 7000, 255 }};
void makePayload(uint8_t i) {
    buffer[TIME_COLUMN] = timeStateValue[i][TIME_COLUMN];
    buffer[STATE_COLUMN] = timeStateValue[i][STATE_COLUMN];
    Serial.print("Time value: ");
    Serial.println(buffer[TIME_COLUMN]);
    Serial.print("State value: ");
    Serial.println(buffer[STATE_COLUMN]);
}
