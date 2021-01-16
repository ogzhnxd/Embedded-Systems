// Uzaktan kumanda ile Step Motor ve LED Kontrol�
// 181202023 O�uzhan G�vercin

//Mikrokontrolc�n�n haz�rlanmas�
#include <16F877A.h>
#fuses HS,NOWDT,NOPROTECT,NOLVP
#use delay(clock = 8000000)
#use fast_io(B)
#use fast_io(C)
#use fast_io(D)

//LED ba�lant�lar�
#define LED_1 PIN_C7
#define LED_2 PIN_C6
#define LED_3 PIN_C5
#define LED_4 PIN_C4

//De�i�ken tan�mlar�
unsigned int8 step_number = 0, speed_delay = 2, led_speed=100;
unsigned int32 remote_code;

//LED Sekans� 1
void LED_sequence1(){                                                            
   output_bit(LED_1, 1);
   delay_ms(led_speed);
   output_bit(LED_2, 1);
   delay_ms(led_speed);
   output_bit(LED_3, 1);
   delay_ms(led_speed);
   output_bit(LED_4, 1);
   delay_ms(led_speed);
   output_bit(LED_1, 0);
   output_bit(LED_2, 0);
   output_bit(LED_3, 0);
   output_bit(LED_4, 0);
   delay_ms(led_speed);
   
}

//Led Sekans� 2
void LED_sequence2(){                                                            
   output_bit(LED_1, 1);
   delay_ms(led_speed);
   output_bit(LED_1, 0);
   output_bit(LED_2, 1);
   delay_ms(led_speed);
   output_bit(LED_2, 0);
   output_bit(LED_3, 1);
   delay_ms(led_speed);
   output_bit(LED_3, 0);
   output_bit(LED_4, 1);
   delay_ms(led_speed);
   output_bit(LED_4, 0);
   output_bit(LED_1, 0);
   output_bit(LED_2, 0);
   output_bit(LED_3, 0);
   delay_ms(led_speed);

}

#INT_TIMER1                                                                     // Timer1 kesme ISR
void timer1_isr(void){
  remote_code = 0;                                                              // Kesme olu�ursa uzaktan kumanda komutnu sil
  clear_interrupt(INT_TIMER1);
  disable_interrupts(INT_TIMER1);
}

#INT_EXT                                                                        // D�� kesme ISR
void ext_isr(void){
  unsigned int8 count = 0, i;
  unsigned int32 ir_code; 
  //NEC protokol�n�n kontrol� ve decode edilmesi
  // 9ms'lik lojik High sinyalinin kontrol et
  while((input(PIN_B0) == 0) && (count < 200)){
    count++;
    delay_us(50);}
  if( (count > 199) || (count < 160))                                           // Lojik 1 8-10ms aral���nda ise ise kontrol etmeye devam et
    return;                          
  count = 0;
  // 4.5ms'lik lojik Low sinyalinin kontrol et
  while((input(PIN_B0)) && (count < 100)){
    count++;
    delay_us(50);}
  if( (count > 99) || (count < 30))                                             // Lojik 0 5-1.5ms aral���nda ise ise kontrol etmeye devam et
    return;
  // D��melere bas�l tutuldu�unda gelen komutu oku
  if(count < 60){
    count = 0;
    while((input(PIN_B0) == 0) && (count < 14)){
      count++;
      delay_us(50);}
    if( (count > 13) || (count < 8))                                            // NEC protokol� m�?
      return;
    if((remote_code == 0x00FF02FD) || (remote_code == 0x00FF22DD))              // >>| ==> 0x00FF02FD  veya   |<< ==> 0x00FF22DD d��melerine ba��l�rsa timer1'i s�f�rla
    set_timer1(0);                                                              // D��melerin bas�l� kald��� s�reyi hesaplamak i�in kesme
  }
  // Komutu oku (32 bit ==> 16 bit adres, 16 bit komut)
  for(i = 0; i < 32; i++){
    count = 0;
    while((input(PIN_B0) == 0) && (count < 14)){                                // Lojik 0 kontrol�
      count++;
      delay_us(50);}
    if( (count > 13) || (count < 8))                                            // NEC protokol� m�?
      return;                          
    count = 0;
    while((input(PIN_B0)) && (count < 40)){                                     // Lojik 1 kontrol�
      count++;
      delay_us(50);}
    if( (count > 39) || (count < 8))                                            // NEC protokol� m�?
      return;                           
    if( count > 20)                                                             // Sinyalin uzunlu�u 1ms'den b�y�k ise 1 yaz
      bit_set(ir_code, (31 - i));                                               // 1 yaz (31 - i)
    else                                                                        // Sinyalin uzunlu�u 1ms'den b�y�k ise 1 yaz
      bit_clear(ir_code, (31 - i));                                             // 0 yaz (31 - i)
  }
  
  if((ir_code == 0x00FF02FD) || (ir_code == 0x00FF22DD)){                       // >>| ==> 0x00FF02FD      |<< ==> 0x00FF22DD
    set_timer1(0);                                                              // Kesmeleri resetle ve tekrar aktive et
    clear_interrupt(INT_TIMER1);
    enable_interrupts(INT_TIMER1);}
  if(ir_code == 0x00FFE01F){                                                    //  -  ==> 0x00FFE01F   Motorun h�z�n� d���rme i�lemi   
    speed_delay++;
    if(speed_delay > 20) speed_delay = 20;
    return;}
  if(ir_code == 0x00FFA857){                                                    //  +  ==> 0x00FFA857   Motorun h�z�n� artt�rma i�lemi
    speed_delay--;
    if(speed_delay < 2) speed_delay = 2;
    return;}
  
  remote_code = ir_code;                                                        // Komutun de�i�kene aktar�lmas� 
}

void stepper(int8 step){                                                        //Step motoru Tam ad�m s�rme fonksiyonu
  switch(step){
    case 0:
      output_d(0b00000011);
    break;
    case 1:
      output_d(0b00000110);
    break;
    case 2:
      output_d(0b00001100);
    break;
    case 3:
      output_d(0b00001001);
    break;
  }
}

void main(){
  output_b(0);                                                                  // PORTB'yi temizle
  set_tris_b(0xF7);                                                             // PORTB'deki giri� ��k��lar� ayarla
  port_b_pullups(TRUE);                                                         // PORTB'deki i� kesmeleri aktive et
  output_d(0);                                                                  // PORTD'yi temizle
  set_tris_d(0);                                                                // PORTD'deki giri� ��k��lar� ayarla
  output_c(0);                                                                  // PORTD'yi temizle
  set_tris_c(0);                                                                // PORTC'deki giri� ��k��lar� ayarla
  setup_timer_1(T1_INTERNAL | T1_DIV_BY_4);                                     // Timer1'e 1/4 prescaler de�eri ata
  enable_interrupts(GLOBAL);                                                    // Global kesmeleri aktif et
  enable_interrupts(INT_EXT_H2L);                                               // D�� kesmeleri aktive et
  
  while(TRUE){
    while(remote_code == 0);                                                    //Komut al�nm�yorsa bekle
    while((remote_code == 0x00FFE21D) || (remote_code == 0x00FF02FD)){          // CH+ ==> 0x00FFE21D    veya    >>| ==> 0x00FF02FD d��melerine bas�l�rsa motoru ileri s�r   
      step_number++;                                                            //Motorun ileri s�r�lme i�lemi
      if(step_number > 3){                                                      // 4. ad�mdan sonra 1. ad�ma d�n�lmesi 
        step_number = 0;
        stepper(step_number);
        delay_ms(speed_delay);
        }
    }
    
    while((remote_code == 0x00FFA25D) || (remote_code == 0x00FF22DD)){          // CH- ==> 0x00FFA25D    veya    |<< ==> 0x00FF22DD d��melerine bas�l�rsa motoru geri s�r
      if(step_number < 1){                                                      // 2. ad�mdan sonra 4. ad�ma d�n�lmesi
        step_number = 4;
        step_number--;                                                          //Motorun geri s�r�lme i�lemi
        stepper(step_number);
        delay_ms(speed_delay);
        }
    }
    
    while(remote_code == 0x00FF30CF){                                           // LED_1'i yak ve 100ms sonra s�nd�r
      output_bit(LED_1, 1);
      delay_ms(100);
      output_bit(LED_1, 0);
    }
    
    while(remote_code == 0x00FF18E7){                                           // LED_2'i yak ve 100ms sonra s�nd�r
      output_bit(LED_2, 1);
      delay_ms(100);
      output_bit(LED_2, 0);
    }
    
    while(remote_code == 0x00FF7A85){                                           // LED_3'i yak ve 100ms sonra s�nd�r
      output_bit(LED_3, 1);
      delay_ms(100);
      output_bit(LED_3, 0);
    }
    
    while(remote_code == 0x00FF10EF){                                           // LED_4'i yak ve 100ms sonra s�nd�r
      output_bit(LED_4, 1);
      delay_ms(100);
      output_bit(LED_4, 0);
    }
    
    while(remote_code == 0x00FF38C7){                                           // LED sekans� 1'i ger�ekle�tir
      LED_sequence1();
    }
    
    while(remote_code == 0x00FF5AA5){                                           // LED sekans� 2'yi ger�ekle�tir
      LED_sequence2();
    }
    
    output_d(0);                                                                // �stenilen komutlar gelmezse portlar� temizle
    output_c(0);
    
    if((remote_code != 0x00FFE21D) && (remote_code != 0x00FFA25D))              // CH+ ==> 0x00FFE21D    CH- ==> 0x00FFA25D
      remote_code = 0;                                                          // CH+ veya CH- d��melerine bas�lmad���nda motoru durdur
    }
}

