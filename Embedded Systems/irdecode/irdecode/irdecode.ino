#include <IRremote.hpp>
 
#define IR_RECEIVE_PIN 3 // Pin, do którego podpięty jest sygnał
 
void setup() {
  Serial.begin(9600); // Start komunikacji z komputerem
   
  // Uruchomienie odbiornika
  Serial.println("Startowanie odbiornika IR...");
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
}
 
void loop() {
  // Sprawdzamy, czy odebrano jakiś sygnał
  if (IrReceiver.decode()) {
    if (IrReceiver.decodedIRData.decodedRawData != 0) {
       
      Serial.print("Odebrano kod przycisku: 0x");
      // Wyświetlamy kod w formacie szesnastkowym (HEX)
      uint16_t command = IrReceiver.decodedIRData.command;
      Serial.println(command);
    }
 
    // Bardzo ważne: Wznów nasłuchiwanie, aby odebrać kolejny sygnał
    IrReceiver.resume(); 
  }
}