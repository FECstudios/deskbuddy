int volume = 0;
int direction = 1;

void setup() {
  Serial.begin(115200);
  delay(1000);
}

void loop() {
  Serial.println("VOL:" + String(volume));

  volume += direction;

  if (volume >= 100) {
    volume = 100;
    direction = -1;
  } else if (volume <= 0) {
    volume = 0;
    direction = 1;
  }

  delay(200);
}
