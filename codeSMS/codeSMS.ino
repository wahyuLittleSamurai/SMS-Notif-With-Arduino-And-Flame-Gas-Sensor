#include <LiquidCrystal.h>
#include <SoftwareSerial.h>

LiquidCrystal lcd(5, 6, 7, 8, 9, 10);
char lcdBuff[16];

SoftwareSerial SIM900(2,3);
//==========================
int maxTime,counterCommand;// variable yg digunakan untuk parsing data serial sim900
boolean found = false;
boolean autoReset = false;
//##########################
   
#define flame A5
#define buzzer  11

#define setPointGas 400 //nilai ambang batas gas %
#define setPointFlame 940// nilai ambang batas flame
#define MQ_PIN A4    //define which analog input channel you are going to use

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600); 
  SIM900.begin(9600);
  lcd.begin(16, 2);
  pinMode(flame, INPUT);
  pinMode(buzzer, OUTPUT);

  lcd.setCursor(0,0);
  lcd.print("Bismillah...");
  delay(2500);
  lcd.setCursor(0,1);
  lcd.print("Please Wait...");
  delay(15000);               //tunggu selama 15detik agar jaringan sim stabil
  while(counterCommand<=3)  //urutan perintah AT command
  {
    switch(counterCommand)
    {
      case 0: atCommand("AT",1,"OK");break; //panggil func atCommand dengan perintah AT, waktu 1 detik dengan balasan OK
      case 1: atCommand("AT+CMGF=1",1,"OK");break; //setting di cmgf sebagai mode text
      case 2: atCommand("AT+CMGL=\"ALL\",0",2,"OK");break; //baca seluruh sms yg ada, dengan maksimal waktu 2 detik dan balasan OK
      case 3: atCommand("AT+CMGD=1,4",1,"OK");break;//hapus sms yg ada
    }
  }
  counterCommand = 0;//urutan perintah di kemablikan ke 0 
  lcd.clear();
}

void loop() {
  // put your main code here, to run repeatedly:
  while(counterCommand<=2)  //urutan perintah untuk setting awal mode sms
  {
    switch(counterCommand)
    {
      case 0: atCommand("AT",1,"OK");break; //at command untuk cek koneksi
      case 1: atCommand("AT+CMGF=1",1,"OK");break;//setting mode text
      case 2: atCommand("AT+CNMI=2,1,0,0,0",1,"OK");break;//setting notifikasi jika ada sms masuk
    }
  }
  lcd.setCursor(0,0);
  lcd.print("Ready.......");
  delay(500);
  lcd.clear();
  while(counterCommand>2)//jika setting-an sudah selesai jalankan perintah berikut...
  {
    int nilaiGas = analogRead(MQ_PIN); // ubah ke prosentase
    int nilaiFlame = 1024 - analogRead(flame); // baca nilai analog flame (di invert, karena semakin besar cahaya semakin kecil data)
    
    Serial.print(analogRead(MQ_PIN));
    Serial.print(",");
    Serial.println(1024-analogRead(flame));
    
    lcd.setCursor(0,0);
    lcd.print("**Flame || Gas**");
    lcd.setCursor(0,1);
    sprintf(lcdBuff," [%4d] [%4d]", nilaiFlame, nilaiGas); //tampilkan nilai flame dan gas
    lcd.print(lcdBuff);
  
    
    if(nilaiGas > setPointGas)//jika nilai gas melebihi set point
    {
      lcd.setCursor(0,0);
      lcd.print("**** DANGER ****");  //tampil di lcd sesuai tulisan
      lcd.setCursor(0,1);
      lcd.print("Gas Terdeteksi!!");
      for(int p=0; p<5; p++)          //ulang bunyi bip sebanyak 5x
      {
        digitalWrite(buzzer, HIGH);   //output logic high
        delay(500);                   //tahan set detik
        digitalWrite(buzzer, LOW);    //output logic low
        delay(500);                   //tahan set detik
      }
    }
    if(nilaiFlame > setPointFlame)  //jika nilai flame melebihi ambang set point
    {
      lcd.setCursor(0,0);
      lcd.print("Danger,Send SMS"); //tampilan LCD
      lcd.setCursor(0,1);
      lcd.print("Api Terdeteksi!!");

      atCommand("AT+CMGF=1",1,"OK");  //set mode text untuk sim900
      atCommand("AT+CMGS=\"085850474633\"",1,">");//setting nomer tujuan  
      atCommand("Dangerrr.... Api Terdeteksi!!!",1,">");//isi dari sms
      Serial.println("Mengirim Char Ctrl+Z / ESC untuk keluar dari menu SMS");
      SIM900.println((char)26);//ctrl+z untuk keluar dari menginputkan isi sms
      delay(10000); //tahan selama 1 detik
      
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Done...");
      delay(1000);
      lcd.clear();
    }
    
  }
 
}

void atCommand(String iCommand, int timing, char myText[]) //func untuk parsing balasan at command
{
  Serial.println("###Start###");
  Serial.print("Command Ke ->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> ");
  Serial.print(counterCommand); //urutan perintah ke ...
  Serial.print("Kirim=>");Serial.println(iCommand);//tampil ke serial monitor untuk perintah
  while(timing>maxTime)//ulangi sampai dengan maximal waktu 
  {
    SIM900.println(iCommand);//kirim perintah sesuai dengan variable iCommand
    if(SIM900.find(myText))//cari balasan dari sim900 yg sesuai dengan variable myText
    {
      found = true; //jika ada maka ubah variable found menjadi true
      break;
    }
    Serial.print(maxTime);Serial.print(",");
    maxTime++;//counter untuk batas maximal waktu
  }
  if(found == true)//jika balasan yg diterima sesuai dengan yg di inginkan (contoh "OK")
  {
   
    autoReset = false;//variable autoReset dijadikan false

    counterCommand++; // urutan perintah di increment
   
    Serial.println("==============================>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> oke");
  }
  else //jika tidak ditemukan maka akan di anggap error
  {
    SIM900.write("AT+CMGF=1");//setting ke mode text
    delay(1000);
    SIM900.write("AT+CMGD=1,4"); // hapus seluruh sms yg ada
    delay(1000);
    Serial.print("\nautoReset=true\n");
    autoReset = true; //variable autoreset dijadikan true 
    Serial.println("--------============>>>>>>>> AT Command Error");
    Serial.println("--------============>>>>>>>> Proses reset");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Proses Reset");
    //digitalWrite(resetPin, HIGH); //jika ingin mereset secara otomatis hilangkan "//"
    //delay(200);
    //digitalWrite(resetPin, LOW);
    //delay(15000);
    lcd.clear(); 
    lcd.setCursor(0,0);
    lcd.print("selesai reset");
    counterCommand = 0; // setelah reset maka counterCommand akan di 0 kan untuk melakukan setting dari awal lagi
    delay(500);
    lcd.clear();
  }
  if(counterCommand >=100)//optional untuk pembatasan banyaknya perintah
  {
    counterCommand = 3;
  }
  Serial.println("sebelum");
  Serial.print(found);Serial.print(",");Serial.println(autoReset);
  found = false;
  maxTime=0;
  Serial.println("setelah");
  Serial.print(found); Serial.print(","); Serial.println(autoReset);
  Serial.println("***end***");
  
}
