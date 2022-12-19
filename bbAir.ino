#define pumpTime 40

#define OUT1 4
#define OUT2 5

void setup() {
  // put your setup code here, to run once:
  pinMode(OUT1,OUTPUT);
  pinMode(OUT2,OUTPUT);
  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("OUT1 HIGH");
  digitalWrite(OUT1,HIGH);
  delay(pumpTime);
  Serial.println("OUT1 LOW");
  digitalWrite(OUT1,LOW);  

  for(int i = 0;i<3;i++){
    digitalWrite(OUT2,HIGH);
    delay(20);
    digitalWrite(OUT2,LOW);
digitalWrite(OUT1,HIGH);    
    delay(pumpTime);
digitalWrite(OUT1,LOW);  
delay(200);    
  }
  
  
  delay(2000);
  
}
