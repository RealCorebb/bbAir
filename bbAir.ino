#define OUT1 12
#define OUT2 3
#define OUT3 13
#define OUT4 14

void setup() {
  // put your setup code here, to run once:
  pinMode(OUT1,OUTPUT);
  pinMode(OUT3,OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(OUT3,HIGH);
  delay(100);

  for(int i = 0;i<3;i++){
    digitalWrite(OUT1,HIGH);
    delay(20);
    digitalWrite(OUT1,LOW);
    delay(200);
  }
  digitalWrite(OUT3,LOW);
  delay(1000);
}
