#include <avr/io.h>
#include <avr/interrupt.h>
#define BAUD_RATE 9600
#define UBRR_SETTING ((F_CPU / 16 / BAUD_RATE) - 1)

// Definizione dei bit dei pin per il semaforo S1
#define ROSSO_SOTTO_BIT PL1
#define GIALLO_SOTTO_BIT PL3
#define VERDE_SOTTO_BIT PL5

// Definizione dei bit dei pin per il semaforo S2
#define ROSSO_SOPRA_BIT PL0
#define GIALLO_SOPRA_BIT PL2
#define VERDE_SOPRA_BIT PL4

// Definizione dei bit dei pin per il semaforo S3
#define ROSSO_DESTRO_BIT PB7
#define GIALLO_DESTRO_BIT PB6
#define VERDE_DESTRO_BIT PB5

// Definizione dei bit dei pin per il semaforo S4
#define ROSSO_SINISTRO_BIT PH6
#define GIALLO_SINISTRO_BIT PH5
#define VERDE_SINISTRO_BIT PH4

//definisco i pulsanti
#define BTN_SOPRA_BIT PC7
#define BTN_SOTTO_BIT PC5
#define BTN_DESTRO_BIT PC3
#define BTN_SINISTRO_BIT PC1

//variabili di stato per pulsanti
bool btnSopraCliccato;
bool btnSottoCliccato;
bool btnDestroCliccato;
bool btnSinistroCliccato;
bool pulsanteVerticaleCliccato;
bool pulsanteOrizzontaleCliccato;
unsigned long tempoUltimaChiamataVerticale;
unsigned long tempoUltimaChiamataOrizzontale;
bool chiamataOrizzontalePossibile;
bool chiamataVerticalePossibile;

// Variabili per il controllo del tempo
unsigned long tempoRimanente;
unsigned long TempoTrascorso;
unsigned long TempoTrascorsoGRS1;
unsigned long TempoTrascorsoGRS2;
unsigned long TempoTrascorsoGRS3;
unsigned long TempoTrascorsoGRS4;
unsigned long TempoMassimoDiAttraversamento = 10000;
int statoSemaforo = 1; // 0 = stato 1, 1 = stato 2, 2 = stato 3, 3 = stato 4, 4 = stato 5, 5 = stato 6
int prossimoStato=2;
int tAssestamento = 2000;
int tsAssestamento = tAssestamento;
int tVerde = 15000;
int tGiallo = 3000;
int tRosso = tAssestamento + tVerde + tGiallo + tAssestamento;
bool Cns=false;
bool Ceo=false;

//semaforo giallo lampeggiante
int attivaGialloLampeggiante = 0;
int g = 0;

int cicloGRS1 = 0;
int cicloGRS2 = 0;
int cicloGRS3 = 0;
int cicloGRS4 = 0;
bool chiamaCicloGRS1 = false;
bool chiamaCicloGRS2 = false;
bool chiamaCicloGRS3 = false;
bool chiamaCicloGRS4 = false;

bool velocizzaVerdeV=false;
bool velocizzaVerdeO=false;

bool TrafficoNord = false;
bool TrafficoSud = false;
bool TrafficoEst = false;
bool TrafficoOvest = false;

unsigned long durataRossoS1 = tAssestamento;
unsigned long durataRossoS2 = tAssestamento;
unsigned long durataRossoS3 = tAssestamento+ tVerde + tGiallo + tAssestamento;
unsigned long durataRossoS4 = tAssestamento+ tVerde + tGiallo + tAssestamento;
unsigned long durataVerdeS1 = 0;
unsigned long durataVerdeS2 = 0;
unsigned long durataVerdeS3 = 0;
unsigned long durataVerdeS4 = 0;
unsigned long durataGialloS1 = 0;
unsigned long durataGialloS2 = 0;
unsigned long durataGialloS3 = 0;
unsigned long durataGialloS4 = 0;

unsigned long durataVerde = tVerde;
unsigned long durataGiallo = tGiallo;

void SerialInit() {
  UBRR0H = (unsigned char)(UBRR_SETTING >> 8);  // Imposta l'high byte del registro UBRR
  UBRR0L = (unsigned char)UBRR_SETTING;         // Imposta il low byte del registro UBRR
  UCSR0B = (1 << TXEN0) | (1 << RXEN0);         // Abilita la trasmissione (TXEN0) e la ricezione (RXEN0)
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);       // Imposta il formato dei dati: 8 bit di dati, 1 bit di stop
}

void setup() {
  SerialInit();
  TempoTrascorso = 0;
  TempoTrascorsoGRS1 = 0;
  TempoTrascorsoGRS2 = 0;
  TempoTrascorsoGRS3 = 0;
  TempoTrascorsoGRS4 = 0;
  pulsanteVerticaleCliccato = false;
  pulsanteOrizzontaleCliccato = false;
  chiamataOrizzontalePossibile = true;
  chiamataVerticalePossibile = true;
  tempoUltimaChiamataOrizzontale=0;
  tempoUltimaChiamataVerticale=0;
  // Inizializzazione dei pin del semaforo SOPRA come OUTPUT
  DDRL |= (1 << ROSSO_SOPRA_BIT) | (1 << GIALLO_SOPRA_BIT) | (1 << VERDE_SOPRA_BIT);

  // Inizializzazione dei pin del semaforo SOTTO come OUTPUT
  DDRL |= (1 << ROSSO_SOTTO_BIT) | (1 << GIALLO_SOTTO_BIT) | (1 << VERDE_SOTTO_BIT);

  // Inizializzazione dei pin dei semafori DESTRO come OUTPUT
  DDRB |= (1 << ROSSO_DESTRO_BIT) | (1 << GIALLO_DESTRO_BIT) | (1 << VERDE_DESTRO_BIT);

  // Inizializzazione dei pin dei semafori SINISTRO come OUTPUT
  DDRH |= (1 << ROSSO_SINISTRO_BIT) | (1 << GIALLO_SINISTRO_BIT) | (1 << VERDE_SINISTRO_BIT);

  DDRC &= ~((1 << BTN_SOPRA_BIT) | (1 << BTN_SOTTO_BIT) | (1 << BTN_DESTRO_BIT) | (1 << BTN_SINISTRO_BIT));

  // Inizializza il TIMER0 e TIMER1  per l'ISR
  cli(); // Disabilita gli interrupt globali
  TCCR0A = 0; // Imposta il registro di controllo A del timer0 a 0
  TCCR0B = 0; // Imposta il registro di controllo B del timer0 a 0
  TCNT0 = 0;  // Azzera il contatore del timer0
  OCR0A = 1;  // Imposta un valore di confronto basso per un intervallo molto breve
  TCCR0B |= (1 << WGM01); // Modalità CTC (Clear Timer on Compare Match)
  TCCR0B |= (1 << CS00);  // Imposta il prescaler a 1 (nessuna divisione)
  TIMSK0 |= (1 << OCIE0A); // Abilita l'interruzione di confronto A
  TCCR1A = 0; // Azzera i registri di controllo del timer1
  TCCR1B = 0;
  TCNT1 = 0; // Azzera il conteggio del timer1
  OCR1A = 15625; //16MHZ/(1024*1)=15625Hz     Imposta il valore di confronto per generare un interrupt ogni secondo (poiché il clock è a 16 MHz e il prescaler è 1024)
  TCCR1B |= (1 << WGM12); // Imposta il timer in modalità CTC
  TCCR1B |= (1 << CS12) | (1 << CS10); // Abilita il timer con un prescaler di 1024
  TIMSK1 |= (1 << OCIE1A); // Abilita l'interrupt sul confronto con OCR1A
  sei(); // Abilita gli interrupt globali 
}

void loop() {
  foreground();
}

void setSemafori(int rossoSopra, int gialloSopra, int verdeSopra,
                 int rossoSotto, int gialloSotto, int verdeSotto,
                 int rossoDestro, int gialloDestro, int verdeDestro,
                 int rossoSinistro, int gialloSinistro, int verdeSinistro) {

  PORTL = (PORTL & ~((1 << ROSSO_SOPRA_BIT) | (1 << GIALLO_SOPRA_BIT) | (1 << VERDE_SOPRA_BIT))) | (rossoSopra << ROSSO_SOPRA_BIT) | (gialloSopra << GIALLO_SOPRA_BIT) | (verdeSopra << VERDE_SOPRA_BIT);

  PORTL = (PORTL & ~((1 << ROSSO_SOTTO_BIT) | (1 << GIALLO_SOTTO_BIT) | (1 << VERDE_SOTTO_BIT))) | (rossoSotto << ROSSO_SOTTO_BIT) | (gialloSotto << GIALLO_SOTTO_BIT) | (verdeSotto << VERDE_SOTTO_BIT);

  PORTH = (PORTH & ~((1 << ROSSO_SINISTRO_BIT) | (1 << GIALLO_SINISTRO_BIT) | (1 << VERDE_SINISTRO_BIT))) | (rossoSinistro << ROSSO_SINISTRO_BIT) | (gialloSinistro << GIALLO_SINISTRO_BIT) | (verdeSinistro << VERDE_SINISTRO_BIT);

  PORTB = (PORTB & ~((1 << ROSSO_DESTRO_BIT) | (1 << GIALLO_DESTRO_BIT) | (1 << VERDE_DESTRO_BIT))) | (rossoDestro << ROSSO_DESTRO_BIT) | (gialloDestro << GIALLO_DESTRO_BIT) | (verdeDestro << VERDE_DESTRO_BIT);
}

void Stato1() {
  setSemafori(1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0);
}
void Stato2() {
  setSemafori(0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0);
}
void Stato3() {
  setSemafori(0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0);
}
void Stato4() {
  setSemafori(1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1);
}
void Stato5() {
  setSemafori(1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0);
}
void Stato6() {
  setSemafori(0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0);
}
void Stato7() {
  setSemafori(1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0); 
}
void Stato8() {
  setSemafori(1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0);
}
void Stato9() {
  setSemafori(0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0);
}
void Stato10() {
  setSemafori(0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0);
}
void Stato11() {
  setSemafori(0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0);
}
void Stato12() {
  setSemafori(1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1);
}
void Stato13() {
  setSemafori(1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1);
}
void Stato14() {
  setSemafori(1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0);
}
void Stato15() {
  setSemafori(1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0);
}
void Stato16() {
  setSemafori(1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0);
}
void Stato17() {
  setSemafori(1, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0);
}
void Stato18() {
  if(g==0){
    setSemafori(0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0);
    g=1; 
  }else{
    setSemafori(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    g=0;
  }
}

// Interrupt Service Routine (ISR) per il timer1, il semaforo viene gestito in background
ISR(TIMER1_COMPA_vect) {
  if(chiamataVerticalePossibile==false){
    tempoUltimaChiamataVerticale += 1000;
  }
  if(chiamataOrizzontalePossibile==false){
    tempoUltimaChiamataOrizzontale += 1000;
  }

  if(attivaGialloLampeggiante==1){
    statoSemaforo=18;
    TempoTrascorso=0;
    TempoTrascorsoGRS1 = 0;
    TempoTrascorsoGRS2 = 0;
  } else if(TrafficoNord && TrafficoSud){
    TempoTrascorsoGRS1 += 1000;
    TempoTrascorsoGRS2 += 1000;
    TempoTrascorso += 1000;
  } else if(TrafficoEst && TrafficoOvest){
    TempoTrascorsoGRS3 += 1000;
    TempoTrascorsoGRS4 += 1000;
    TempoTrascorso += 1000;
  } else if(TrafficoNord){
    TempoTrascorsoGRS1 += 1000;
    TempoTrascorso += 1000;
    if(TrafficoEst){
      TempoTrascorsoGRS3 += 1000;
    }else if(TrafficoOvest){
      TempoTrascorsoGRS4 += 1000;
    }
  } else if(TrafficoSud){
    TempoTrascorsoGRS2 += 1000;
    TempoTrascorso += 1000;
    if(TrafficoEst){
      TempoTrascorsoGRS3 += 1000;
    }else if(TrafficoOvest){
      TempoTrascorsoGRS4 += 1000;
    }
  } else if(TrafficoEst){
    TempoTrascorsoGRS3 += 1000;
    TempoTrascorso += 1000;
    if(TrafficoNord){
      TempoTrascorsoGRS1 += 1000;
    }else if(TrafficoSud){
      TempoTrascorsoGRS2 += 1000;
    }
  } else if(TrafficoOvest){
    TempoTrascorsoGRS4 += 1000;
    TempoTrascorso += 1000;
    if(TrafficoNord){
      TempoTrascorsoGRS1 += 1000;
    }else if(TrafficoSud){
      TempoTrascorsoGRS2 += 1000;
    }
  } else if (chiamaCicloGRS1 && chiamaCicloGRS2) {
    TempoTrascorsoGRS1 += 1000;
    TempoTrascorsoGRS2 += 1000;
    TempoTrascorso += 1000;
  } else if (chiamaCicloGRS3 && chiamaCicloGRS4) {
    TempoTrascorsoGRS3 += 1000;
    TempoTrascorsoGRS4 += 1000;
    TempoTrascorso += 1000;
  } else if (chiamaCicloGRS1) {
    TempoTrascorsoGRS1 += 1000;
    TempoTrascorso += 1000;
  } else if (chiamaCicloGRS2) {
    TempoTrascorsoGRS2 += 1000;
    TempoTrascorso += 1000;
  } else if (chiamaCicloGRS3) {
    TempoTrascorsoGRS3 += 1000;
    TempoTrascorso += 1000;
  } else if (chiamaCicloGRS4) {
    TempoTrascorsoGRS4 += 1000;
    TempoTrascorso += 1000;
  } else {
    TempoTrascorso += 1000;
  }


  if (statoSemaforo == 1) {
    if(prossimoStato==2){
        //semaforo S1
      durataRossoS1=tAssestamento-TempoTrascorso;
      durataVerdeS1=0;
      durataGialloS1=0;
      //semaforo S2
      durataRossoS2=tAssestamento-TempoTrascorso;
      durataVerdeS2=0;
      durataGialloS2=0;
      //semaforoS3
      durataRossoS3=tAssestamento+durataVerde+tGiallo+tAssestamento-TempoTrascorso;
      durataVerdeS3=0;
      durataGialloS3=0;
      //semaforoS4
      durataRossoS4=tAssestamento+durataVerde+tGiallo+tAssestamento-TempoTrascorso;
      durataVerdeS4=0;
      durataGialloS4=0;
    }else if(prossimoStato==4){
              //semaforo S1
      durataRossoS1=tAssestamento+durataVerde+tGiallo+tAssestamento-TempoTrascorso;
      durataVerdeS1=0;
      durataGialloS1=0;
      //semaforo S2
      durataRossoS2=tAssestamento+durataVerde+tGiallo+tAssestamento-TempoTrascorso;
      durataVerdeS2=0;
      durataGialloS2=0;
      //semaforoS3
      durataRossoS3=tAssestamento-TempoTrascorso;
      durataVerdeS3=0;
      durataGialloS3=0;
      //semaforoS4
      durataRossoS4=tAssestamento-TempoTrascorso;
      durataVerdeS4=0;
      durataGialloS4=0;
    }
    
    if(Cns && TempoTrascorso >= tAssestamento){
      Cns=false;
      tAssestamento=tsAssestamento;
      if(TrafficoOvest== false && TrafficoEst== false){
        statoSemaforo=4;
        TempoTrascorso=0;
      }else if(TrafficoOvest== false){
        statoSemaforo=13;
        TempoTrascorso=0;
      }else if(TrafficoEst== false){
        statoSemaforo=16;
        TempoTrascorso=0;
      }
    }else if(Ceo && TempoTrascorso >= tAssestamento){
      Ceo=false;
      tAssestamento=tsAssestamento;
      if(TrafficoNord== false && TrafficoSud== false){
        statoSemaforo=2;
        TempoTrascorso=0;
      }else if(TrafficoNord== false){
        statoSemaforo=10;
        TempoTrascorso=0;
      }else if(TrafficoSud== false){
        statoSemaforo=7;
        TempoTrascorso=0;
      }
    }else if(prossimoStato==2 && TempoTrascorso >= tAssestamento){
      durataVerde=tVerde;
      statoSemaforo=2;
      TempoTrascorso=0;
    }else if(prossimoStato==4 && TempoTrascorso >= tAssestamento){
      durataVerde=tVerde;
      statoSemaforo=4;
      TempoTrascorso=0;
    }else if(prossimoStato==13 && TempoTrascorso >= tAssestamento){
      statoSemaforo=13;
      TempoTrascorso=0;
    }else if(prossimoStato==16 && TempoTrascorso >= tAssestamento){
      statoSemaforo=16;
      TempoTrascorso=0;
    }else if(prossimoStato==7 && TempoTrascorso >= tAssestamento){
      statoSemaforo=7;
      TempoTrascorso=0;
    }else if(prossimoStato==10 && TempoTrascorso >= tAssestamento){
      statoSemaforo=10;
      TempoTrascorso=0;
    }
  } else if (statoSemaforo == 2) {
        //semaforo S1
    durataRossoS1=0;
    durataVerdeS1=durataVerde-TempoTrascorso;
    durataGialloS1=0;
    //semaforo S2
    durataRossoS2=0;
    durataVerdeS2=durataVerde-TempoTrascorso;
    //semaforoS3
    durataRossoS3=durataVerde+tGiallo+tAssestamento-TempoTrascorso;
    durataVerdeS3=0;
    durataGialloS3=0;
    //semaforoS4
    durataRossoS4=durataVerde+tGiallo+tAssestamento-TempoTrascorso;
    durataVerdeS4=0;
    durataGialloS4=0;
    if(TrafficoEst && TrafficoOvest && Cns==false){
      statoSemaforo = 2;
    } else if (chiamaCicloGRS1 || TrafficoNord) {
      statoSemaforo = 6;
    } else if (chiamaCicloGRS2 || TrafficoSud) {
      statoSemaforo = 9;
    } else if (TempoTrascorso >= tVerde) {
      statoSemaforo = 3;
      TempoTrascorso = 0;
    }
  } else if (statoSemaforo == 3) {
    //semaforo S1
    durataRossoS1=0;
    durataVerdeS1=0;
    durataGialloS1=tGiallo-TempoTrascorso;
    //semaforo S2
    durataRossoS2=0;
    durataVerdeS2=0;
    durataGialloS2=tGiallo-TempoTrascorso;
    //semaforoS3
    durataRossoS3=tGiallo+tAssestamento-TempoTrascorso;
    durataVerdeS3=0;
    durataGialloS3=0;
    //semaforoS4
    durataRossoS4=tGiallo+tAssestamento-TempoTrascorso;
    durataVerdeS4=0;
    durataGialloS4=0;
    if (chiamaCicloGRS1 || TrafficoNord) {
      if (TempoTrascorsoGRS1 >= tGiallo) {
        durataRossoS1=tGiallo-TempoTrascorsoGRS2;
        statoSemaforo = 8;
        TempoTrascorsoGRS1 = 0;
      }
    }
    if (chiamaCicloGRS2 || TrafficoSud) {
      if (TempoTrascorsoGRS2 >= tGiallo) {
        statoSemaforo = 11;
        TempoTrascorsoGRS2 = 0;
      }
    }
    if (TempoTrascorso >= tGiallo) {
      statoSemaforo = 1;
      TempoTrascorso = 0;
    }
    if(TrafficoEst && TrafficoOvest){
      prossimoStato=2;
    } else if(TrafficoEst){
      prossimoStato=13;
    } else if(TrafficoOvest){
      prossimoStato=16;
    } else{
      prossimoStato=4;
    }
  } else if (statoSemaforo == 4) {
    //semaforo S1
    durataRossoS1=durataVerde+tGiallo+tAssestamento-TempoTrascorso;;
    durataVerdeS1=0;
    durataGialloS1=0;
    //semaforo S2
    durataRossoS2=durataVerde+tGiallo+tAssestamento-TempoTrascorso;
    durataVerdeS2=0;
    durataGialloS2=0;
    //semaforoS3
    durataRossoS3=0;
    durataVerdeS3=durataVerde-TempoTrascorso;
    durataGialloS3=0;
    //semaforoS4
    durataRossoS4=0;
    durataVerdeS4=durataVerde-TempoTrascorso;
    durataGialloS4=0;
    if(TrafficoNord && TrafficoSud && Ceo==false){
      statoSemaforo = 4;
    } else if (chiamaCicloGRS3 || TrafficoEst) {
      statoSemaforo = 12;
    } else if (chiamaCicloGRS4 || TrafficoOvest) {
      statoSemaforo = 15;
    } else if (TempoTrascorso >= tVerde) {
      statoSemaforo = 5;
      TempoTrascorso = 0;
    }
  } else if (statoSemaforo == 5) {
    //semaforo S1
    durataRossoS1=tGiallo+tAssestamento-TempoTrascorso;;
    durataVerdeS1=0;
    durataGialloS1=0;
    //semaforo S2
    durataRossoS2=tGiallo+tAssestamento-TempoTrascorso;
    durataVerdeS2=0;
    durataGialloS2=0;
    //semaforoS3
    durataRossoS3=0;
    durataVerdeS3=0;
    durataGialloS3=tGiallo-TempoTrascorso;
    //semaforoS4
    durataRossoS4=0;
    durataVerdeS4=0;
    durataGialloS4=tGiallo-TempoTrascorso;
    if (chiamaCicloGRS3) {
      if (TempoTrascorsoGRS3 >= tGiallo) {
        statoSemaforo = 14;
        TempoTrascorsoGRS3 = 0;
      }
    }
    if (chiamaCicloGRS4) {
      if (TempoTrascorsoGRS4 >= tGiallo) {
        statoSemaforo = 17;
        TempoTrascorsoGRS4 = 0;
      }
    }
    if (TempoTrascorso >= tGiallo) {
      statoSemaforo = 1;
      TempoTrascorso = 0;
    }
    if(TrafficoNord && TrafficoSud){
      prossimoStato=4;
    } else if(TrafficoNord){
      prossimoStato=7;
    } else if(TrafficoSud){
      prossimoStato=10;
    } else{
      prossimoStato=2;
    }
  } else if (statoSemaforo == 6) {
    if (TempoTrascorso >= tVerde || chiamaCicloGRS2 || TrafficoSud) {
      //giallo-giallo
      statoSemaforo = 3;
      TempoTrascorso = 0;
      TempoTrascorsoGRS2 = 0;
    } else if (TempoTrascorsoGRS1 >= tGiallo) {
      statoSemaforo = 7;
      TempoTrascorsoGRS1 = 0;
    }
  } else if (statoSemaforo == 7) {
    if (TempoTrascorso >= tVerde || chiamaCicloGRS2 == true || TrafficoSud) {
      statoSemaforo = 8;
      TempoTrascorso = 0;
      TempoTrascorsoGRS2 = 0;
    }
  } else if (statoSemaforo == 8) {
    if(TempoTrascorso >= tGiallo){
      statoSemaforo = 1;
      TempoTrascorso = 0;
      chiamaCicloGRS2 = false;
      chiamaCicloGRS1 = false;
      TempoTrascorsoGRS1 = 0;
      TempoTrascorsoGRS2 = 0;
      prossimoStato=4;
    }
    if(TrafficoEst && TrafficoOvest && TempoTrascorso >= tGiallo){
      prossimoStato=2;
    }else if(TrafficoEst && TempoTrascorso >= tGiallo){
      prossimoStato=13;
    }else if(TrafficoOvest && TempoTrascorso >= tGiallo){
      prossimoStato=16;
    }
  } else if (statoSemaforo == 9) {
    if (TempoTrascorso >= tVerde || chiamaCicloGRS1 == true || TrafficoNord) {
      //giallo-giallo
      statoSemaforo = 3;
      TempoTrascorso = 0;
      TempoTrascorsoGRS1 = 0;
    } else if (TempoTrascorsoGRS2 >= tGiallo) {
      statoSemaforo = 10;
      TempoTrascorsoGRS2 = 0;
    }
  } else if (statoSemaforo == 10) {
    if (TempoTrascorso >= tVerde || chiamaCicloGRS1 == true || TrafficoNord) {
      statoSemaforo = 11;
      TempoTrascorso = 0;
      TempoTrascorsoGRS1 = 0;
    }
  } else if (statoSemaforo == 11 && TempoTrascorso >= tGiallo) {
    statoSemaforo = 1;
    TempoTrascorso = 0;
    chiamaCicloGRS2 = false;
    chiamaCicloGRS1 = false;
    TempoTrascorsoGRS1 = 0;
    TempoTrascorsoGRS2 = 0;
    prossimoStato=4;
    if(TrafficoEst && TrafficoOvest){
      prossimoStato=2;
    }else if(TrafficoEst){
      prossimoStato=13;
    }else if(TrafficoOvest){
      prossimoStato=16;
    }
  } else if (statoSemaforo == 12) {
    if (TempoTrascorso >= tVerde || chiamaCicloGRS4 || TrafficoOvest) {
      //giallo-giallo
      statoSemaforo = 5;
      TempoTrascorso = 0;
      TempoTrascorsoGRS4 = 0;
    } else if (TempoTrascorsoGRS3 >= tGiallo) {
      statoSemaforo = 13;
      TempoTrascorsoGRS3 = 0;
    }
  } else if (statoSemaforo == 13) {
    if (TempoTrascorso >= tVerde || chiamaCicloGRS4 == true || TrafficoOvest) {
      statoSemaforo = 14;
      TempoTrascorso = 0;
      TempoTrascorsoGRS2 = 0;
    }
  } else if (statoSemaforo == 14 && TempoTrascorso >= tGiallo) {
    statoSemaforo = 1;
    TempoTrascorso = 0;
    chiamaCicloGRS3 = false;
    chiamaCicloGRS4 = false;
    TempoTrascorsoGRS3 = 0;
    TempoTrascorsoGRS4 = 0;
    prossimoStato=2;
    if(TrafficoNord && TrafficoSud){
      prossimoStato=4;
    }else if(TrafficoNord){
      prossimoStato=7;
    }else if(TrafficoSud){
      prossimoStato=10;
    }
  } else if (statoSemaforo == 15) {
    if (TempoTrascorso >= tVerde || chiamaCicloGRS3 == true || TrafficoEst) {
      //giallo-giallo
      statoSemaforo = 5;
      TempoTrascorso = 0;
      TempoTrascorsoGRS3 = 0;
    } else if (TempoTrascorsoGRS4 >= tGiallo) {
      statoSemaforo = 16;
      TempoTrascorsoGRS4 = 0;
    }
  } else if (statoSemaforo == 16) {
    if (TempoTrascorso >= tVerde || chiamaCicloGRS3 == true || TrafficoEst) {
      statoSemaforo = 17;
      TempoTrascorso = 0;
      TempoTrascorsoGRS3 = 0;
    }
  } else if (statoSemaforo == 17 && TempoTrascorso >= tGiallo) {
    statoSemaforo = 1;
    TempoTrascorso = 0;
    chiamaCicloGRS4 = false;
    chiamaCicloGRS3 = false;
    TempoTrascorsoGRS4 = 0;
    TempoTrascorsoGRS3 = 0;
    prossimoStato=2;
    if(TrafficoNord && TrafficoSud){
      prossimoStato=4;
    }else if(TrafficoNord){
      prossimoStato=7;
    }else if(TrafficoSud){
      prossimoStato=10;
    }
  }

  switch (statoSemaforo) {
    case 1: Stato1(); break;
    case 2: Stato2(); break;
    case 3: Stato3(); break;
    case 4: Stato4(); break;
    case 5: Stato5(); break;
    case 6: Stato6(); break;
    case 7: Stato7(); break;
    case 8: Stato8(); break;
    case 9: Stato9(); break;
    case 10: Stato10(); break;
    case 11: Stato11(); break;
    case 12: Stato12(); break;
    case 13: Stato13(); break;
    case 14: Stato14(); break;
    case 15: Stato15(); break;
    case 16: Stato16(); break;
    case 17: Stato17(); break;
    case 18: Stato18(); break;
  }

}

// Dichiarazione della funzione per il tuo task periodico
void TaskPulsante() {
  //verifico se il pulsante sopra è stato cliccato
  btnSopraCliccato = PINC & (1 << BTN_SOPRA_BIT);
  //verifico se il pulsante sotto è stato cliccato
  btnSottoCliccato = PINC & (1 << BTN_SOTTO_BIT);
  //verifico se il pulsante destro è stato cliccato
  btnDestroCliccato = PINC & (1 << BTN_DESTRO_BIT);
  //verifico che il pulsante sinistro sia stato cliccato
  btnSinistroCliccato = PINC & (1 << BTN_SINISTRO_BIT);

  if ((btnSopraCliccato || btnSottoCliccato) && (statoSemaforo==2 || statoSemaforo==6 || statoSemaforo==7 || statoSemaforo==8 || statoSemaforo==9 || statoSemaforo==10 || statoSemaforo==11) && chiamataVerticalePossibile==true) {
    pulsanteVerticaleCliccato = true;
    chiamataVerticalePossibile=false;
    durataVerde=tVerde/2+1000;
  }
  if ((btnDestroCliccato || btnSinistroCliccato) && (statoSemaforo==4 || statoSemaforo==12 || statoSemaforo==13 || statoSemaforo==14 || statoSemaforo==15 || statoSemaforo==16 || statoSemaforo==17) && chiamataOrizzontalePossibile==true) {
    pulsanteOrizzontaleCliccato = true;
    chiamataOrizzontalePossibile= false;
    durataVerde=tVerde/2+1000;
  }

  //attivo la chiamata dalla metà in poi dello stato 2
  if(TempoTrascorso >= tVerde/2 && TrafficoEst && TrafficoOvest && pulsanteVerticaleCliccato){
    Cns=true;
    tAssestamento = tVerde;
    pulsanteVerticaleCliccato = false;
  }else if (TempoTrascorso >= tVerde/2 && pulsanteVerticaleCliccato && TempoTrascorso < 60000) {
    TempoTrascorso = tVerde;
    pulsanteVerticaleCliccato = false;
  }

  //attivo la chiamata dalla metà in poi dello stato 5
  if(TempoTrascorso >= tVerde/2 && TrafficoNord && TrafficoSud && pulsanteOrizzontaleCliccato){
    Ceo=true;
    tAssestamento = tVerde;
    pulsanteOrizzontaleCliccato = false;
  } else if (TempoTrascorso >= tVerde / 2 && pulsanteOrizzontaleCliccato && TempoTrascorso < 60000) {
    TempoTrascorso = tVerde;
    pulsanteOrizzontaleCliccato = false;
  }

  //aspetto che passino 50 secondi dalla chiamata precedente
  if (tempoUltimaChiamataVerticale > 50000) {
    chiamataVerticalePossibile = true;
    tempoUltimaChiamataVerticale = 0;
  }
  if (tempoUltimaChiamataOrizzontale > 50000) {
    chiamataOrizzontalePossibile = true;
    tempoUltimaChiamataOrizzontale = 0;
  }
}

// Interrupt handler per il timer0
ISR(TIMER0_COMPA_vect) {
  TaskPulsante(); // Esegui il tuo task periodico
}

//FOREGROUND

//ALTRO FILE PER GESTIONE I/0 SERIALE

// Funzione per verificare se ci sono dati disponibili sulla seriale
int SerialAvailable() {
  return (UCSR0A & (1 << RXC0)) ? 1 : 0;
}

char SerialReadInt() {
  while (!(UCSR0A & (1 << RXC0)));  // Aspetta che un carattere sia disponibile
  return UDR0;                       // Restituisci il carattere letto
}

// Funzione per scrivere una stringa sulla seriale
void SerialWriteString(const char* str) {
  while (*str) {
    SerialWrite(*str);
    str++;
  }
}

// Funzione per scrivere un carattere sulla seriale
void SerialWrite(char c) {
  while (!(UCSR0A & (1 << UDRE0)))
    ;
  UDR0 = c;
}

void SerialWriteInt(int value) {
  char buffer[20];
  int i = 0;
  if (value < 0) {
    SerialWrite('-');
    value = -value;
  }
  if (value == 0) {
    SerialWrite('0');
    return;
  }
  while (value > 0) {
    buffer[i++] = '0' + (value % 10);
    value /= 10;
  }
  while (i > 0) {
    SerialWrite(buffer[--i]);
  }
}

// Funzione per leggere un numero intero dalla seriale
int leggiInteroSeriale() {
  char inputString[20]; // Un array di caratteri per memorizzare i caratteri letti dal seriale
  char c;
  int index = 0;
  bool isNegative = false;
  int integerValue = 0;

  while (1) {
    if (SerialAvailable()) {
      c = SerialReadInt(); // Legge un carattere dalla seriale

      if (c == '-') {
        isNegative = true; // Segna che il numero è negativo
      } else if (c >= '0' && c <= '9') {
        // Aggiungi il carattere all'array e incrementa l'indice
        inputString[index++] = c;
        inputString[index] = '\0'; // Aggiungi il terminatore di stringa

        // Converti il carattere in un valore numerico e aggiornalo nell'intero
        integerValue = integerValue * 10 + (c - '0');
      } else if (c == '\n') { // Se è un carattere di nuova riga, restituisci il valore intero
        return isNegative ? -integerValue : integerValue;
      }
    }
  }
}

// Funzione per ottenere lo stato del semaforo SOPRA
char* getSemaforoNord() {
  if (statoSemaforo == 7 || statoSemaforo == 8 || statoSemaforo == 1 || statoSemaforo==4 || statoSemaforo==12 || statoSemaforo==13 || statoSemaforo==14 || statoSemaforo==15 || statoSemaforo==16 || statoSemaforo==17 || statoSemaforo==5) {
    return "Rosso";
  } else if (statoSemaforo == 6 || statoSemaforo==3 || statoSemaforo==11) {
    return "Giallo";
  } else if(statoSemaforo==2 || statoSemaforo==9 || statoSemaforo==10){
    return "Verde";
  }else{
    return "Giallo lampeggiante";
  }
}

// Funzione per ottenere lo stato del semaforo SOPRA
char* getSemaforoSud() {
  if (statoSemaforo == 10 || statoSemaforo == 11 || statoSemaforo == 1 || statoSemaforo==4 || statoSemaforo==12 || statoSemaforo==13 || statoSemaforo==14 || statoSemaforo==15 || statoSemaforo==16 || statoSemaforo==17 || statoSemaforo==5) {
    return "Rosso";
  } else if (statoSemaforo == 8 || statoSemaforo==3 || statoSemaforo==9) {
    return "Giallo";
  } else if(statoSemaforo==2 || statoSemaforo==6 || statoSemaforo==7){
    return "Verde";
  }else{
    return "Giallo lampeggiante";
  }
}

// Funzione per ottenere lo stato del semaforo SOPRA
char* getSemaforoEst() {
  if (statoSemaforo == 2 || statoSemaforo == 3 || statoSemaforo == 6 || statoSemaforo==7 || statoSemaforo==8 || statoSemaforo==9 || statoSemaforo==10 || statoSemaforo==11 || statoSemaforo==13 || statoSemaforo==14 || statoSemaforo==1) {
    return "Rosso";
  } else if (statoSemaforo == 17 || statoSemaforo==5 || statoSemaforo==12) {
    return "Giallo";
  } else if(statoSemaforo==4 || statoSemaforo==15 || statoSemaforo==16){
    return "Verde";
  }else{
    return "Giallo lampeggiante";
  }
}

// Funzione per ottenere lo stato del semaforo SOPRA
char* getSemaforoOvest() {
  if (statoSemaforo == 2 || statoSemaforo == 3 || statoSemaforo == 6 || statoSemaforo==7 || statoSemaforo==8 || statoSemaforo==9 || statoSemaforo==10 || statoSemaforo==11 || statoSemaforo==16 || statoSemaforo==17 || statoSemaforo==1) {
    return "Rosso";
  } else if (statoSemaforo == 15 || statoSemaforo==5 || statoSemaforo==14) {
    return "Giallo";
  } else if(statoSemaforo==4 || statoSemaforo==12 || statoSemaforo==13){
    return "Verde";
  }else{
    return "Giallo lampeggiante";
  }
}

unsigned long getTempoRimanente(int semaforo) {
  if(semaforo==1){
    if(durataRossoS1 != 0){
      return durataRossoS1/ 1000;
    }else if(durataVerdeS1!=0){
      return durataVerdeS1/ 1000;
    }else{
      return durataGialloS1/1000;
    }
  }else if(semaforo==2){
    if(durataRossoS2 != 0){
      return durataRossoS2/ 1000;
    }else if(durataVerdeS2!=0){
      return durataVerdeS2/ 1000;
    }else{
      return durataGialloS2/1000;
    }
  }else if(semaforo==3){
    if(durataRossoS3 != 0){
      return durataRossoS3/ 1000;
    }else if(durataVerdeS3!=0){
      return durataVerdeS3/ 1000;
    }else{
      return durataGialloS3/1000;
    }
  }else if(semaforo==4){
        if(durataRossoS4 != 0){
      return durataRossoS4/ 1000;
    }else if(durataVerdeS4!=0){
      return durataVerdeS4/ 1000;
    }else{
      return durataGialloS4/1000;
    }
  }
  
}
/*
unsigned long tempoRimanente = durataStati[statoSemaforo] - TempoTrascorso;
    return tempoRimanente / 1000; // Converte il tempo in millisecondi in secondi
    */

void foreground() {

  SerialWriteString("\nMenu:");
  SerialWriteString("\n1. Visualizza stato semafori");
  SerialWriteString("\n2. Modifica tempi");
  SerialWriteString("\n3. Avvia immediatamente ciclo G-R su un semaforo");
  SerialWriteString("\n4. attiva giallo lampeggiante");
  SerialWriteString("\n5. disattiva giallo lampeggiante");
  SerialWriteString("\n6. Simula traffico");
  SerialWriteString("\nSeleziona un'opzione:");

  while (!SerialAvailable()) {
    // Attendi che l'utente selezioni un'opzione
  }

  int choice = leggiInteroSeriale(); // Leggi l'opzione selezionata dall'utente

  if (choice == 1) {
    SerialWriteString("\n---------------------------------------------------");
    while (true) {
      SerialWriteString("\n1. Verifica stato e tempo rimanente del semaforo S1");
      SerialWriteString("\n2. Verifica stato e tempo rimanente del semaforo S2");
      SerialWriteString("\n3. Verifica stato e tempo rimanente del semaforo S3");
      SerialWriteString("\n4. Verifica stato e tempo rimanente del semaforo S4");
      SerialWriteString("\n5. Esci");
      while (!SerialAvailable()) {
        // Attendi che l'utente selezioni un'opzione
      }
      int subChoice = leggiInteroSeriale();
      // Verifica la sottoscelta dell'utente
      char* semaforoNome;
      char* semaforoStato;
      if (subChoice == 1) {
        SerialWriteString("\n---------------------------------------------------");
        SerialWriteString("\nSemaforo S1");
        SerialWriteString(" - Stato: ");
        SerialWriteString(getSemaforoNord());
        if(statoSemaforo==18){
          SerialWriteString(" - Tempo rimanente allo stato successivo indefinito");
        }else{
          SerialWriteString(" - Tempo rimanente (secondi): ");
          SerialWriteInt(getTempoRimanente(1));
        }
        SerialWriteString("\n---------------------------------------------------");
      } else if (subChoice == 2) {
        SerialWriteString("\n---------------------------------------------------");
        SerialWriteString("\nSemaforo S2");
        SerialWriteString(" - Stato: ");
        SerialWriteString(getSemaforoSud());
        if(statoSemaforo==18){
          SerialWriteString(" - Tempo rimanente allo stato successivo indefinito");
        }else{
          SerialWriteString(" - Tempo rimanente (secondi): ");
          SerialWriteInt(getTempoRimanente(2));
        }
        SerialWriteString("\n---------------------------------------------------");
      } else if (subChoice == 3) {
        SerialWriteString("\n---------------------------------------------------");
        SerialWriteString("\nSemaforo S3");
        SerialWriteString(" - Stato: ");
        SerialWriteString(getSemaforoEst());
        if(statoSemaforo==18){
          SerialWriteString(" - Tempo rimanente allo stato successivo indefinito");
        }else{
          SerialWriteString(" - Tempo rimanente (secondi): ");
          SerialWriteInt(getTempoRimanente(3));
        }
        SerialWriteString("\n---------------------------------------------------");
      } else if (subChoice == 4) {
        SerialWriteString("\n---------------------------------------------------");
        SerialWriteString("\nSemaforo S4");
        SerialWriteString(" - Stato: ");
        SerialWriteString(getSemaforoOvest());
        if(statoSemaforo==18){
          SerialWriteString(" - Tempo rimanente allo stato successivo indefinito");
        }else{
          SerialWriteString(" - Tempo rimanente (secondi): ");
          SerialWriteInt(getTempoRimanente(4));
        }
        SerialWriteString("\n---------------------------------------------------");
      } else if (subChoice == 5) {
        // L'utente ha selezionato "Esci", esci dal loop interno
        break;
      } else {
        SerialWriteString("\nOpzione non valida.");
      }
    }
  } else if (choice == 2) {
    while (true) {
      SerialWriteString("\n---------------------------------------------------");
      SerialWriteString("\nMenu Modifica Tempi:");
      SerialWriteString("\n1: Modifica tempo verde");
      SerialWriteString("\n2: Modifica tempo giallo");
      SerialWriteString("\n3: Modifica tempo rosso");
      SerialWriteString("\n4: Esci");
      while (!SerialAvailable()) {
        // Attendi che l'utente selezioni un'opzione
      }
      int subChoice = leggiInteroSeriale();
      // Verifica la sottoscelta dell'utente
      if (subChoice == 1) {
        SerialWriteString("\nInserisci il nuovo tempo (secondi): ");
        while (!SerialAvailable()) {
          // Attendi che l'utente selezioni un'opzione
        }

        int newTime = leggiInteroSeriale();

        if (newTime > 0) {
          tVerde = newTime * 1000;
          SerialWriteString("\nTempo aggiornato con successo!");
        } else {
          SerialWriteString("\nTempo non valido. Deve essere maggiore di 0.");
        }
      } else if (subChoice == 2) {
        SerialWriteString("\nInserisci il nuovo tempo");

        while (!SerialAvailable()) {
          // Attendi che l'utente selezioni un'opzione
        }

        int newTime = leggiInteroSeriale();

        // Verifica che il nuovo tempo sia maggiore di 0
        if (newTime > 0) {
          tGiallo = newTime * 1000;
          SerialWriteString("\nTempo aggiornato con successo!");
        } else {
          SerialWriteString("\nTempo non valido. Deve essere maggiore di 0.");
        }
      } else if (subChoice == 3) {
        SerialWriteString("\nInserisci il nuovo tempo");

        while (!SerialAvailable()) {
          // Attendi che l'utente selezioni un'opzione
        }

        int newTime = leggiInteroSeriale();
        unsigned long tNuovoRosso=newTime*1000;

        // Verifica che il nuovo tempo sia maggiore di 0
        if (newTime > 0) {
          tAssestamento=tNuovoRosso*3/100;
          tVerde=tNuovoRosso*90/100;
          tGiallo=tNuovoRosso*7/100;
          SerialWriteString("\nTempo aggiornato con successo!");
        } else {
          SerialWriteString("\nTempo non valido. Deve essere maggiore di 0.");
        }
      } else if (subChoice == 4) {
        break;
      } else {
        SerialWriteString("\nOpzione non valida.");
      }
    }
  } else if (choice == 3) {
    while (true) {
      SerialWriteString("\n---------------------------------------------------");
      SerialWriteString("\nAvvia subito ciclo giallo-rosso su uno di questi semafori:");
      SerialWriteString("\n1: Semaforo S1(sopra)");
      SerialWriteString("\n2: Semaforo S2(sotto)");
      SerialWriteString("\n3: Semaforo S3(destro)");
      SerialWriteString("\n4: Semaforo S4(sinistro)");
      SerialWriteString("\n5: Esci");
      while (!SerialAvailable()) {
        // Attendi che l'utente selezioni un'opzione
      }
      int subChoice = leggiInteroSeriale();
      if (subChoice == 1) {
        if (statoSemaforo == 2 || statoSemaforo == 9 || statoSemaforo == 10) {
          chiamaCicloGRS1 = true;
        } else {
          SerialWriteString("\nNon è possibile avviare il ciclo giallo-rosso per questo semaforo poichè non è verde");
        }
      } else if (subChoice == 2) {
        if (statoSemaforo == 2 || statoSemaforo == 6 || statoSemaforo == 7) {
          chiamaCicloGRS2 = true;
        } else {
          SerialWriteString("\nNon è possibile avviare il ciclo giallo-rosso per questo semaforo poichè non è verde");
        }
      } else if (subChoice == 3) {
        if (statoSemaforo == 4 || statoSemaforo == 15 || statoSemaforo == 16) {
          chiamaCicloGRS3 = true;
        } else {
          SerialWriteString("\nNon è possibile avviare il ciclo giallo-rosso per questo semaforo poichè non è verde");
        }
      } else if (subChoice == 4) {
        if (statoSemaforo == 4 || statoSemaforo == 12 || statoSemaforo == 13) {
          chiamaCicloGRS4 = true;
        } else {
          SerialWriteString("\nNon è possibile avviare il ciclo giallo-rosso per questo semaforo poichè non è verde");
        }
      } else if (subChoice == 5) {
        break;
      } else {
        SerialWriteString("\nOpzione non valida.");
      }
    }
  } else if (choice == 4) {
    attivaGialloLampeggiante = 1;
  } else if (choice == 5) {
    attivaGialloLampeggiante = 0;
    statoSemaforo = 1;
    TempoTrascorso = 0;
  } else if (choice == 6) {
    while (true) {
      SerialWriteString("\n---------------------------------------------------");
      SerialWriteString("\n1. Blocca traffico");
      SerialWriteString("\n2. SBlocca traffico");
      SerialWriteString("\n3. Esci");
      while (!SerialAvailable()) {
        // Attendi che l'utente selezioni un'opzione
      }
      int subChoice = leggiInteroSeriale();
      if (subChoice == 1) {
        while (true) {
          SerialWriteString("\n---------------------------------------------------");
          SerialWriteString("\nQuale semaforo vuoi bloccare?:");
          SerialWriteString("\n1: Nord");
          SerialWriteString("\n2: Sud");
          SerialWriteString("\n3: Est");
          SerialWriteString("\n4: Ovest");
          SerialWriteString("\n5: Esci");
          while (!SerialAvailable()) {
            // Attendi che l'utente selezioni un'opzione
          }
          int subSubChoice = leggiInteroSeriale();
          if (subSubChoice == 1) {
            if(TrafficoEst || TrafficoOvest){
              SerialWriteString("\nNon è possibile bloccare il semaforo Nord essendo che anche uno tra est e ovest è bloccato");
            }else if(TrafficoNord == true){
              SerialWriteString("\nE' già bloccato il semaforo a nord");
            }else{
              TrafficoNord = true;
              SerialWriteString("\ntraffico a sud bloccato");
            }
          } else if (subSubChoice == 2) {
            if(TrafficoEst || TrafficoOvest){
              SerialWriteString("\nNon è possibile bloccare il semaforo Nord essendo che anche uno tra est e ovest è bloccato");
            }else if(TrafficoSud == true){
              SerialWriteString("\nE' già bloccato il semaforo a sud");
            }else{
              TrafficoSud = true;
              SerialWriteString("\ntraffico a sud bloccato");
            }
          } else if (subSubChoice == 3) {
            if(TrafficoNord || TrafficoSud){
              SerialWriteString("\nNon è possibile bloccare il semaforo est essendo che anche uno tra nord e sud è bloccato");
            }else if(TrafficoEst == true){
              SerialWriteString("\nE' già bloccato il semaforo a est");
            }else{
              TrafficoEst = true;
              SerialWriteString("\ntraffico a est bloccato");
            }
          } else if (subSubChoice == 4) {
            if(TrafficoNord || TrafficoSud){
              SerialWriteString("\nNon è possibile bloccare il semaforo est essendo che anche uno tra nord e sud è bloccato");
            }else if(TrafficoOvest == true){
              SerialWriteString("\nE' già bloccato il semaforo a ovest");
            }else{
              TrafficoOvest = true;
              SerialWriteString("\ntraffico a ovest bloccato");
            }
          } else if (subSubChoice == 5) {
            break;
          } else {
            SerialWriteString("\nOpzione non valida.");
          }
        }
      }else if(subChoice == 2){
        while (true) {
          SerialWriteString("\n---------------------------------------------------");
          SerialWriteString("\nQuale semaforo vuoi sbloccare?:");
          SerialWriteString("\n1: Nord");
          SerialWriteString("\n2: Sud");
          SerialWriteString("\n3: Est");
          SerialWriteString("\n4: Ovest");
          SerialWriteString("\n5: Esci");
          while (!SerialAvailable()) {
            // Attendi che l'utente selezioni un'opzione
          }
          int subSubChoice = leggiInteroSeriale();
          if (subSubChoice == 1) {
            if(TrafficoNord == false){
              SerialWriteString("\nNon c'è traffico a nord");
            }else{
              TrafficoNord = false;
              SerialWriteString("\ntraffico sbloccato corettamente");
            }
          } else if (subSubChoice == 2) {
            if(TrafficoSud == false){
              SerialWriteString("\nNon c'è a traffico a sud");
            }else{
              TrafficoSud = false;
              SerialWriteString("\ntraffico sbloccato corettamente");
            }
          } else if (subSubChoice == 3) {
            if(TrafficoEst == false){
              SerialWriteString("\nNon c'è a traffico a est");
            }else{
              TrafficoEst = false;
              SerialWriteString("\ntraffico sbloccato corettamente");
            }
          } else if (subSubChoice == 4) {
            if(TrafficoOvest == false){
              SerialWriteString("\nNon c'è a traffico a ovest");
            }else{
              TrafficoOvest = false;
              SerialWriteString("\ntraffico sbloccato corettamente");
            }
          } else if (subSubChoice == 5) {
            break;
          } else {
            SerialWriteString("\nOpzione non valida.");
          }
        }
      }else if(subChoice == 3){
        break;
      }else{
        SerialWriteString("\nOpzione non valida.");
      }
    }
  } else {
    SerialWriteString("\nOpzione non valida. Riprova.");
  }

}