/*
 * File:   newmain.c
 * Author: Igor Bastos
 *
 */

#include <xc.h>          //inclusão da biblioteca do compilador
#include <pic16f877a.h>  //inclusão da biblioteca do chip específico
#include <stdio.h>       //inclusão da biblioteca standard padrão "C"


#define _XTAL_FREQ 4000000 //Defini a frequência do clock, 4MHz neste caso

//********************  congiguration bits  ***********************************
#pragma config WDTE = ON     //habilita o uso do WDT
#pragma config FOSC = XT     //define uso do clock externo EM 4 OU 20mHZ
#pragma config PWRTE = ON    //habilita reset ao ligar
#pragma config BOREN = ON    //Habilita o reset por Brown-out 

//********************  configuração LCD  ************************************

//*** define pinos referentes a interface com LCD
#define RS RD2
#define EN RD3
#define D4 RD4
#define D5 RD5
#define D6 RD6
#define D7 RD7


#include "lcd.h"         //incluSÃO Da biblioteca do LCD 


//********************  configuração Saidas/Entradas portB e portC ***********

//*** ENTRADAS
#define LIGARALARME PORTBbits.RB1
#define SM          PORTBbits.RB0
#define LIGAINT     PORTBbits.RB2

//*** SAIDAS
#define EMERGENCIA  PORTBbits.RB7
#define LUZ         PORTBbits.RB6
#define SPK         PORTBbits.RB5
#define NORMALled   PORTBbits.RB4
#define INTRUSAOled PORTBbits.RB3
#define INCENDIOled PORTCbits.RC3
#define ALARME      PORTCbits.RC0
#define LIGAINTled  PORTCbits.RC2
#define EXAUSTAO    PORTCbits.RC1
#define FUMACAled   PORTCbits.RC4

//*** VARIÁVEIS GLOBAIS
int contaTimer = 0;
int ContaExterna = 0;
int valorST, valorSF, prevST = -1, prevSF = -1, INTacionado = 0, INCacionado = 0, FUacionado = 0;  
__bit LCDligado = 0, atualizarSensores = 0, ALARMEacionado = 0;
char buffer[20];

//*** Função que controla a exibição da mensagem no LCD
void LCDContro() {
    
    if(INCacionado == 0 && FUacionado == 0 &&INTacionado == 0){
    
        if (valorST != prevST || valorSF != prevSF) {  
            
            Lcd_Set_Cursor(1, 1);
            Lcd_Write_String("                ");  
            sprintf(buffer, "Temp: %d\xDF""C", valorST);  
            Lcd_Set_Cursor(1, 1);
            Lcd_Write_String(buffer);
            prevST = valorST;  
        
            Lcd_Set_Cursor(2, 1);
            Lcd_Write_String("                ");  
            sprintf(buffer, "Fum : %d %%", valorSF);  
            Lcd_Set_Cursor(2, 1);
            Lcd_Write_String(buffer);
            prevSF = valorSF;  
        }
        
    }
    
    if(FUacionado == 1) {
        
        Lcd_Clear();                    
        Lcd_Set_Cursor(1,1);
        sprintf(buffer, "FUMACA ALTA %d %%", valorSF);  
        Lcd_Write_String(buffer);
    }
    
    if(INCacionado == 1) {
        
        Lcd_Clear();                    
        Lcd_Set_Cursor(1,2);
        Lcd_Write_String("ALERTA INCENDIO");
        Lcd_Set_Cursor(2,7);
        sprintf(buffer, "%d\xDF""C", valorST);  
        Lcd_Write_String(buffer);
        
    }
    
    if(INTacionado == 1) {
        
        Lcd_Clear();                    
        Lcd_Set_Cursor(1,2);
        Lcd_Write_String("ALERTA INTRUSO");
    }
 }

///*** Função que controla os LEDs de Estado
void LEDsControl() {
    
    
    
    if(INCacionado == 0 && FUacionado == 0 && INTacionado == 0) {
        
        NORMALled = 1;
        FUMACAled = 0;
        INCENDIOled = 0;
        
    }
    
    if(FUacionado == 1 && INCacionado == 0) {
        
        FUMACAled = 1;
        NORMALled = 0;
        INCENDIOled = 0;
        
    }
    
    if(INCacionado == 1 && FUacionado == 0) {
        
        INCENDIOled = 1;
        NORMALled = 0;
        FUMACAled = 0;
        
    }
    if(INCacionado == 1 && FUacionado == 1) {
        
        INCENDIOled = 1;
        FUMACAled = 1;
        NORMALled = 0;
        
    }
    
    
    if(INTacionado == 1) {
        
        INTRUSAOled = 1;
        NORMALled = 0;
        FUMACAled = 0;
        
    }
    
    
}
        
///*** Função que le o valor da temperatura marcado no sensor
void LerTemperatura() {
     
    
    // Leitura Temp - AN0

    ADCON0bits.CHS0 = 0;  // Seleciona AN0
    ADCON0bits.CHS1 = 0;
    ADCON0bits.CHS2 = 0;
    __delay_us(20);       // Aguarde estabilização antes de iniciar a conversão

    ADCON0bits.GO = 1;    // Inicia a conversão
    while (ADCON0bits.GO); // Aguarda a conclusão
    
    valorST = (int)(((float)ADRESH / 255.0) * 5.0 * 100);  // Convertido para int
}

///*** Função que le o valor da fumaça marcado no sensor
void LerFumaca() {
    
    ADCON0bits.CHS0 = 1;  // Seleciona AN1
    ADCON0bits.CHS1 = 0;
    ADCON0bits.CHS2 = 0;
    
    __delay_us(20);       // Aguarde estabilização antes de iniciar a conversão

    ADCON0bits.GO = 1;    // Inicia a conversão
    while (ADCON0bits.GO); // Aguarda a conclusão
    
    valorSF = (int)((ADRESH / 255.0) * 100.0);  // Convertido para int
}


///*** Função que inicializa estado do sistema
void Inicializacao() {
    
    atualizarSensores = 1; // DEFAULT PARA ATUALIZAR OS SENSORES QUANDO LIGAR
    Lcd_Clear();           // LIMPO O DISPLAY
    T1CONbits.TMR1ON = 0;  // DESLIGO O TIMER CASO ESTEJA LIGADO
    
   

    // DESLIGO TODOS LEDS E PERIFÉRICOS
    NORMALled = 0;
    INTRUSAOled = 0;
    INCENDIOled = 0;
    FUMACAled = 0;
    LIGAINTled = 0;
    ALARME = 0; 
    LUZ = 0;   
    EXAUSTAO = 0;
    EMERGENCIA = 0;
    SPK = 0;

    // REINICIO VARIÁVEIS NECESSARIAS
    prevST = -1; 
    prevSF = -1;
    INCacionado = 0;
    FUacionado = 0;

    TMR1L = 0xDC;          //carga do valor inicial no contador (65536-62500)
    TMR1H = 0x0B;          //carga do valor inicial no contador
}

///*** Funções que ativam e desativam dispositivos
void AtivarControlIncendio() {
   
    ALARME = 1;
    EMERGENCIA = 1;
    SPK = 1;

}

void DesativarControIncendio() {
   
    ALARME = 0;
    EMERGENCIA = 0;
    SPK = 0;
    
}

void AtivarControlFumaca() {
    
    EXAUSTAO = 1; 

}

void DesativarControlFumaca() {
    
    EXAUSTAO = 0; 
}



void VerificaIntrusao() {
    
    if(LIGAINT == 0) {
        
        LIGAINTled = 1;
        
    } else {
        LIGAINTled = 0;
        // DESATIVA PERIFÉRICOS CASO A INTRUSÃO ESTEJA ATIVA E FOI DESATIVADA
        if(INTacionado == 1) {
            
            Inicializacao();
            INTacionado = 0;
        }
    }
}



void __interrupt() TrataInt(void)
{ 
    // Utilização da interrupção externa no RBO
    
    if (INTF) {  // Verifica o flag de interrupção externa (RB0)
        
        INTF = 0;  // Limpa o flag de interrupção para permitir novas interrupções
      
        if(LIGARALARME == 0 && LIGAINT == 0 && INTacionado == 0) {
          
            ContaExterna++;

            if(ContaExterna == 10) {

                ALARME = 1; // Liga o alarme
                LUZ = 1;    // Liga a luz Geral
                
                INTacionado = 1;  // flag de acionamento da intrusão
                ContaExterna = 0; // reseta contagem
                
                LCDContro();   // Atualiza informações no LCD
                LEDsControl(); // Atualiza controle dos leds
                
                
            } 
        }
    }
    
    // Executado a cada 500ms
    if (TMR1IF) {  // Verifica o flag de TIMER1
        
        PIR1bits.TMR1IF = 0; //reseta o flag da interrupção
        TMR1L = 0xDC;        //reinicia contagem com 3036
        TMR1H = 0x0B; 
        
        contaTimer++;

        // executa a cada 3s
        if(contaTimer == 6) {
            
            atualizarSensores = 1;
            contaTimer = 0;
        }
    }
  
  
  return;
}

int main()
{
    
  
    
    
  /// *******************  Configura A/D  ************************************
    
  
  
  //*** Habilitar AN0 e AN1 Como Analogico
  
  ADCON1bits.PCFG0 = 0;
  ADCON1bits.PCFG1 = 0;
  ADCON1bits.PCFG2 = 0;
  ADCON1bits.PCFG3 = 0;
  
  ADCON1bits.ADFM = 0;       // Default 8 bits
  ADRESH = 0x00;             // Inicializa com 0 o valor análogo
  ADCON0bits.ADON = 1;       // Liga A/D
  
  __delay_ms(20);            // Aguarda a estabilização do ADC
  
  
  /// *******************  Configura PORTB  E PORTC **************************
  
  OPTION_REG = 0b00001111;   // Habilita resistores de pull-up internos e configura pre-escala WDT
  
  TRISB = 0b00000111;        // OUTPUT RB3-RB7 INPUT RB0 RB1 RB2
  TRISC = 0b00000000;        // OUTPUT RB1 E RB2 INPUT as OUTRAS de C.
  PORTB = 0x00;              // Inicia todas saidas com 0;
  PORTC = 0x00;              // Inicia todas saidas com 0;
  
  /// *******************  Configura PORTD para LCD e incia LCD *************
  
  TRISD = 0x00;  //configura PORTD como saída 
  
  
  Lcd_Init();
  
  /// *******************  interrupções *************
  
  INTCONbits.GIE = 1;    // Habilita inr global
  INTCONbits.PEIE = 1;   // Habilita a int dos periféricos
  INTCONbits.INTE = 1;   // Habilita a int externa RB0
  PIE1bits.TMR1IE = 1;   // Habilita int do timer 1
  
  /// *******************  configura o timer 1 *************
  
  T1CONbits.TMR1CS = 0;  // Define timer 1 como temporizador
  T1CONbits.T1CKPS0 = 1; // Pre-escaler 1:8
  T1CONbits.T1CKPS1 = 1; // Pre-escaler 1:8
  
  TMR1L = 0xDC;          //carga do valor inicial no contador (65536-62500)
  TMR1H = 0x0B;          //carga do valor inicial no contador
  
  
  

  while(1)
  {
      
    CLRWDT();
    
    // VERIFICO SE O ALARME ESTÁ LIGADO E FAÇO CONF PARA GARANTIR QUE ELE FIQUE NO MODO DEFAULT QUANDO LIGAR
    if(LIGARALARME == 1) {
        Inicializacao();
        while(LIGARALARME == 1){
          CLRWDT();   
        }; 
    }
    
    // Verifico se está habilitado a leitura dos sensores
    if(atualizarSensores == 1) {
        
        atualizarSensores = 0;
        
        // Verifica se o Timer1 está ligado, caso não esteja, ligo ele
        if (T1CONbits.TMR1ON == 0) {
            T1CONbits.TMR1ON = 1;  // Liga o timer
        }
        
        // Faço leitura dos sensores
        LerTemperatura();
        LerFumaca();
        
        
        // VERIFICA SE SENSOR DE TEMPERATURA SINALIZA INCENDIO E LIGA PERIFÉRICOS
        if(valorST > 60 && INCacionado == 0) {
            
            INCacionado = 1;
            AtivarControlIncendio();
        }
        
    
        // VERIFICA SE O INCENDIO FOI CONTIDO E DESLIGA PERIFÉRICOS
        if(valorST < 30 && INCacionado == 1) {
            
            INCacionado = 0;
            DesativarControIncendio();
            
        }
        
        // VERIFICA SE SENSOR DE FUMAÇA SINALIZA NIVEL ALTO
        if(valorSF > 60 && FUacionado == 0) {
            
            FUacionado = 1;
            AtivarControlFumaca();
        }
        
        // VERIFICA SE NIVEL ALTO DE FUMAÇA FOI CONTIDO
        if(valorSF < 30 && FUacionado == 1) {
            
            FUacionado = 0;
            DesativarControlFumaca();
        }
        
        // MOSTRO AS MENSAGENS DO ESTADO NO LCD E NOS LEDS DE ESTADO
        LCDContro();
        LEDsControl();
        
        CLRWDT();
        
    }
    
    // VERIFICA SE A INTRUSÃO ESTÁ ACIONADO
    VerificaIntrusao();
    
  }
  
  
}