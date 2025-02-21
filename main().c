/*****************************************************************************/
/* MAIN.C                                                                    */
/*                                                                           */
/* Autor: Andrés Delgado Domínguez                                           */
/* http://www.ti.com/                                                        */
/*                                                                           */
/*  La redistribución y uso con o sin modificación, están totalmente         */
/*  permitidas siempre que se agradezca al autor de este material            */
/*  via email, mensajería instantánea, o de manera directa su utilización    */
/*                                                                           */
/*****************************************************************************/


/* ===========================================================
 * ======================   LIBRERÍAS   ======================
 * ===========================================================
 * STDINT       ==> Librería para usar variables enteras.
 * STDBOOL      ==> Librería para usar variables booleanas.
 * DRIVERLIB2   ==> Librería creada por Manuel Ángel Perales, incluye una gran variedad de librerías.
 * FT800_TIVA   ==> Librería para el manejo de la pantalla táctil.
 * STRING       ==> Librería para poder usar strings.
 * UARTSTUDIO   ==> Librería para comunicaciones UART.
 */

#include <stdint.h>
#include <stdbool.h>
#include "driverlib2.h"
#include "FT800_TIVA.h"
#include "string.h"
#include "uartstdio.h"


/* ===========================================================
 * ==============   DEFINICIONES Y VARIABLES   ===============
 * ===========================================================
 */

#define MSEC 40000
#define B1_ON           !(GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_0))
#define B2_ON           !(GPIOPinRead(GPIO_PORTJ_BASE,GPIO_PIN_1))
#define NUM_SSI_DATA    3
#define AZULCLARO       0x00b4fa                                //Colores RGB (0x00,0xb4,0xfa)
#define AZULOSCURO      0x001e64                                //Colores RGB (0x00,0x1e,0x64)

char                chipid = 0;                                // Holds value of Chip ID read from the FT800
unsigned long       cmdBufferRd = 0x00000000;         // Store the value read from the REG_CMD_READ register
unsigned long       cmdBufferWr = 0x00000000;         // Store the value read from the REG_CMD_WRITE register
unsigned int        t=0;
unsigned long       POSX, POSY, BufferXY;
unsigned long       POSYANT=0;
unsigned int        CMD_Offset = 0;
unsigned long       REG_TT[6];
const unsigned long REG_CAL[6]={21959,177,4294145463,14,4294950369,16094853};
uint32_t            RELOJ=0;

struct navio{                       //Estructura de los barcos
    char matricula[7];
    int tcita;
    int updown;
    int greentick;
} barco[200] = {0};
volatile int estado=0;              //Estado actual de la máquina de estados
volatile int treal=538;              //Variable que guarda el tiempo en minutos
volatile int tsec=0;
volatile int no=0;
volatile int aux =0;
volatile int num=0;                 //Numero de barcos en lista
volatile int bot=0;
volatile int pin=0;
volatile int closedoor=1, opendoor=0;
volatile int closetap=1, opentap=0;
int PeriodoPWM;
volatile int notatime;
/* ===========================================================
 * ======================   PRUEBAS   ========================
 * ===========================================================
 */
#define PosMin 750
#define PosMax 1000
#define XpMax 286
#define XpMin 224
#define YpMax 186
#define YpMin 54
unsigned int Yp=120, Xp=245;
//============================================================


/* ===========================================================
 * ======================   FUNCIONES   ======================
 * ===========================================================
 */

uint8_t pulsado1(void){             //DETECTA CUANDO SE PULSA EL BOTON 1
    uint8_t resul=0;                //EVITA EL DEBOUNCING DEL BOTÓN
    if(B1_ON){                      //Y DEVUELVE SI ESTÁ PULSADO O NO
        SysCtlDelay(20*MSEC);
        resul=1;
    }
    return resul;
}

uint8_t pulsado2(void){	            //DETECTA CUANDO SE PULSA EL BOTON 2
    uint8_t resul=0;
    if(B2_ON){
        SysCtlDelay(20*MSEC);
        resul=1;
    }
    return resul;
}

void pushstack(void){               //Añade una nueva estructura a la pila de estructuras en el top (num).
    int i;
    i=0;
    barco[num].matricula[i]=barco[189].matricula[i];
    i=1;
    barco[num].matricula[i]=barco[189].matricula[i];
    i=2;
    barco[num].matricula[i]=barco[189].matricula[i];
    i=3;
    barco[num].matricula[i]=barco[189].matricula[i];
    i=4;
    barco[num].matricula[i]=barco[189].matricula[i];
    i=5;
    barco[num].matricula[i]=barco[189].matricula[i];
    i=6;
    barco[num].matricula[i]=barco[189].matricula[i];
    barco[num].tcita=barco[189].tcita;
    barco[num].updown=barco[189].updown;
    barco[num].greentick=1;
    num++;


}

void rankstack(void){
    int i,k;
    char cmatricula[7];
    int ctcita;
    int cupdown;
    int cgreentick;
    i=1;
    for(i=num-1;i>bot;i--){
        if(barco[i].tcita<barco[i-1].tcita){ //Si el tiempo de cita de una estructura que esté en un puesto N de la pila
                                             //es menor que el tiempo de cita de uno que esté inmediatamente inferior
                                             //intercambia los datos de las estructuras, así las de menor tiempo siempre está
                                             //abajo en la pila que las de mayor tiempo.

            //Actualiza las matrículas
            for(k=0;k<7;k++){
                cmatricula[k]=barco[i-1].matricula[k];
                barco[i-1].matricula[k]=barco[i].matricula[k];
                barco[i].matricula[k]=cmatricula[k];
            }
            // Actualiza los tiempos de cita
            ctcita=barco[i-1].tcita;
            barco[i-1].tcita=barco[i].tcita;
            barco[i].tcita=ctcita;

            //Actualiza updown

            cupdown=barco[i-1].updown;
            barco[i-1].updown=barco[i].updown;
            barco[i].updown=cupdown;

            //Actualiza greentick
            cgreentick=barco[i-1].greentick;
            barco[i-1].greentick=barco[i].greentick;
            barco[i].greentick=cgreentick;
            i=num;
        }
    }
}

void intlist(void){             	//Inicializa algunso Structs de Barco

    /* Las letras permitidas para las matrículas son:         *
     * B, C, D, F, G, H, J, K, L, M,                          *
     * N, P, R, S, T, V, W, X, Y y Z                          *
     * */

    strcpy( barco[0].matricula, "0152LMB" );
    barco[0].tcita=560;
    barco[0].updown=1;

    strcpy( barco[1].matricula, "5665PLB" );
    barco[1].tcita=540;
    barco[1].updown=0;

    strcpy( barco[2].matricula, "7666XLC" );
    barco[2].tcita=1000;
    barco[2].updown=1;

    strcpy( barco[3].matricula, "5712FSL" );
    barco[3].tcita=550;
    barco[3].updown=0;
    barco[3].greentick=1;

    strcpy( barco[4].matricula, "6769TLB" );
    barco[4].tcita=900;
    barco[4].updown=0;

    strcpy( barco[5].matricula, "1743XPS" );
    barco[5].tcita=800;
    barco[5].updown=0;

    strcpy( barco[6].matricula, "4183XPS" );
    barco[6].tcita=620;
    barco[6].updown=1;
    barco[6].greentick=1;
    bot=0;
    num=7;
}

void transcreen(void){          	//Función para crear una pantalla de transición
    /*
     * ===== Animaciones en pantalla: =====
     * Para hacer una animación efectiva es necesario usar al menos 24 o 25
     * fotogramas por segundo, de ahí que los bucles for utilizados tengan
     * 25 iteraciones y en total se tarde 25*20MSEC en recorrer cada for,
     * medio segundo.
     */
    int i;

    for(i=0;i<6;i++)    Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);

        for(i=0;i<24;i++)
        {
            Nueva_pantalla(0x10,0x10,0x10);
            ComGradient(0, 0, AZULOSCURO, 320, 240,AZULCLARO-1542*i);
            SysCtlDelay(20*MSEC);
            Dibuja();
        }
        for(i=0;i<24;i++)
        {
            Nueva_pantalla(0x10,0x10,0x10);
            ComGradient(0, 0, AZULOSCURO, 320, 240,AZULOSCURO+1542*i);
            SysCtlDelay(20*MSEC);
            Dibuja();
        }

}

void pintalista(void){              //Función para pintar la lista de espera de las embarcaciones.

    int i;
    for(i=0;i<6;i++)  Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);   //CALIBRACIÓN
    while(!(POSX>5 && POSX<310 && POSY>170 && POSY<220)){

        Lee_pantalla();
        Nueva_pantalla(0x10,0x10,0x10);
        int u=0;
        // Contador para ver cuántos barcos hay en reserva
        char string[50];                                                //TRADUCTORA INT/CHAR
        ComGradient(0,0, AZULOSCURO, 320, 240, AZULCLARO);              //Gradiente de azul oscuro a azul claro

        /*
         * ============== RECUADRO EXTERIOR PANAMAX ==============
         */
        //<<<<<<<<<<<<<<FONDO AZUL OSCURO>>>>>>>>>>>>>>//
        ComLineWidth(1);
        ComColor(0, 52, 193);
        Comando(CMD_BEGIN_RECTS);
        ComVertex2ff(5,10);
        ComVertex2ff(310,30);
        Comando(CMD_END);

        //<<<<<<<<<<<<<<BORDE EXTERIOR NEGRO>>>>>>>>>>>>>>//
        ComColor(0,0,0);
        ComLineWidth(1);
        Comando(CMD_BEGIN_LINES);
        ComVertex2ff(5,10);
        ComVertex2ff(310,10);
        ComVertex2ff(310,10);
        ComVertex2ff(310,30);
        ComVertex2ff(310,30);
        ComVertex2ff(5,30);
        ComVertex2ff(5,30);
        ComVertex2ff(5,10);
        Comando(CMD_END);

        //<<<<<<<<<<<<<<LETRERO Y TIEMPO>>>>>>>>>>>>>>//
        ComColor(0xff,0xff,0xff);
        ComTXT(160, 10, 22,  OPT_CENTERX, "PANAMAX");
        sprintf(string,"%d",treal/60);
        ComTXT(263,10, 22, OPT_RIGHTX, string);
        sprintf(string,"%d",treal%60);
        ComTXT(270,10, 22, OPT_RIGHTX, ":");
        ComTXT(290,10, 22, OPT_RIGHTX, string);
        //======================================================//




        u=0;
        if(barco[bot+u].matricula[4]!=0){
            //<<<<<<<<<<<<<<LETRERO Y TIEMPO>>>>>>>>>>>>>>//
            ComColor(0xff,0xff,0xff);
            if(barco[bot+u].greentick==1) ComColor(0,220,32);
            ComTXT(50,29+u*20, 22, OPT_CENTERX, barco[bot+u].matricula);
            ComColor(0xff,0xff,0xff);
            if(barco[bot+u].updown==0){
                ComTXT(160,34+u*20, 20, OPT_CENTERX, "BAJADA");
            }
            else{
                ComTXT(160,34+u*20, 20, OPT_CENTERX, "SUBIDA");
            }
            if(barco[bot+u].tcita-treal<6 && treal<barco[bot+u].tcita) ComColor(255, 0, 0);
            sprintf(string,"%d",barco[bot+u].tcita/60);
            ComTXT(263,29+u*20, 22, OPT_RIGHTX, string);
            sprintf(string,"%d",barco[bot+u].tcita%60);
            ComTXT(270,29+u*20, 22, OPT_RIGHTX, ":");
            ComTXT(290,29+u*20, 22, OPT_RIGHTX, string);
            //<<<<<<<<<<<<<<BORDE EXTERIOR NEGRO>>>>>>>>>>>>>>//
            ComColor(0,0,0);
            Comando(CMD_BEGIN_LINES);
            ComVertex2ff(5,30+u*20);
            ComVertex2ff(310,30+u*20);
            ComVertex2ff(310,30+u*20);
            ComVertex2ff(310,50+u*20);
            ComVertex2ff(310,50+u*20);
            ComVertex2ff(5,50+u*20);
            ComVertex2ff(5,50+u*20);
            ComVertex2ff(5,30+u*20);
            Comando(CMD_END);
        }
        u=1;
        if(barco[bot+u].matricula[4]!=0){
            //<<<<<<<<<<<<<<LETRERO Y TIEMPO>>>>>>>>>>>>>>//
            ComColor(0xff,0xff,0xff);
            if(barco[bot+u].greentick==1) ComColor(0,220,32);
            ComTXT(50,29+u*20, 22, OPT_CENTERX, barco[bot+u].matricula);
            ComColor(0xff,0xff,0xff);
            if(barco[bot+u].updown==0){
                ComTXT(160,34+u*20, 20, OPT_CENTERX, "BAJADA");
            }
            else{
                ComTXT(160,34+u*20, 20, OPT_CENTERX, "SUBIDA");
            }
            if(barco[bot+u].tcita-treal<6 && treal<barco[bot+u].tcita) ComColor(255, 0, 0);
            sprintf(string,"%d",barco[bot+u].tcita/60);
            ComTXT(263,29+u*20, 22, OPT_RIGHTX, string);
            sprintf(string,"%d",barco[bot+u].tcita%60);
            ComTXT(270,29+u*20, 22, OPT_RIGHTX, ":");
            ComTXT(290,29+u*20, 22, OPT_RIGHTX, string);
            //<<<<<<<<<<<<<<BORDE EXTERIOR NEGRO>>>>>>>>>>>>>>//
            ComColor(0,0,0);
            Comando(CMD_BEGIN_LINES);
            ComVertex2ff(5,30+u*20);
            ComVertex2ff(310,30+u*20);
            ComVertex2ff(310,30+u*20);
            ComVertex2ff(310,50+u*20);
            ComVertex2ff(310,50+u*20);
            ComVertex2ff(5,50+u*20);
            ComVertex2ff(5,50+u*20);
            ComVertex2ff(5,30+u*20);
            Comando(CMD_END);
        }
        u=2;
        if(barco[bot+u].matricula[4]!=0){
            //<<<<<<<<<<<<<<LETRERO Y TIEMPO>>>>>>>>>>>>>>//
            ComColor(0xff,0xff,0xff);
            if(barco[bot+u].greentick==1) ComColor(0,220,32);
            ComTXT(50,29+u*20, 22, OPT_CENTERX, barco[bot+u].matricula);
            ComColor(0xff,0xff,0xff);
            if(barco[bot+u].updown==0){
                ComTXT(160,34+u*20, 20, OPT_CENTERX, "BAJADA");
            }
            else{
                ComTXT(160,34+u*20, 20, OPT_CENTERX, "SUBIDA");
            }
            if(barco[bot+u].tcita-treal<6 && treal<barco[bot+u].tcita) ComColor(255, 0, 0);
            sprintf(string,"%d",barco[bot+u].tcita/60);
            ComTXT(263,29+u*20, 22, OPT_RIGHTX, string);
            sprintf(string,"%d",barco[bot+u].tcita%60);
            ComTXT(270,29+u*20, 22, OPT_RIGHTX, ":");
            ComTXT(290,29+u*20, 22, OPT_RIGHTX, string);
            //<<<<<<<<<<<<<<BORDE EXTERIOR NEGRO>>>>>>>>>>>>>>//
            ComColor(0,0,0);
            Comando(CMD_BEGIN_LINES);
            ComVertex2ff(5,30+u*20);
            ComVertex2ff(310,30+u*20);
            ComVertex2ff(310,30+u*20);
            ComVertex2ff(310,50+u*20);
            ComVertex2ff(310,50+u*20);
            ComVertex2ff(5,50+u*20);
            ComVertex2ff(5,50+u*20);
            ComVertex2ff(5,30+u*20);
            Comando(CMD_END);
        }
        u=3;
        if(barco[bot+u].matricula[4]!=0){
            //<<<<<<<<<<<<<<LETRERO Y TIEMPO>>>>>>>>>>>>>>//
            ComColor(0xff,0xff,0xff);
            if(barco[bot+u].greentick==1) ComColor(0,220,32);
            ComTXT(50,29+u*20, 22, OPT_CENTERX, barco[bot+u].matricula);
            ComColor(0xff,0xff,0xff);
            if(barco[bot+u].updown==0){
                ComTXT(160,34+u*20, 20, OPT_CENTERX, "BAJADA");
            }
            else{
                ComTXT(160,34+u*20, 20, OPT_CENTERX, "SUBIDA");
            }
            if(barco[bot+u].tcita-treal<6 && treal<barco[bot+u].tcita) ComColor(255, 0, 0);
            sprintf(string,"%d",barco[bot+u].tcita/60);
            ComTXT(263,29+u*20, 22, OPT_RIGHTX, string);
            sprintf(string,"%d",barco[bot+u].tcita%60);
            ComTXT(270,29+u*20, 22, OPT_RIGHTX, ":");
            ComTXT(290,29+u*20, 22, OPT_RIGHTX, string);
            //<<<<<<<<<<<<<<BORDE EXTERIOR NEGRO>>>>>>>>>>>>>>//
            ComColor(0,0,0);
            Comando(CMD_BEGIN_LINES);
            ComVertex2ff(5,30+u*20);
            ComVertex2ff(310,30+u*20);
            ComVertex2ff(310,30+u*20);
            ComVertex2ff(310,50+u*20);
            ComVertex2ff(310,50+u*20);
            ComVertex2ff(5,50+u*20);
            ComVertex2ff(5,50+u*20);
            ComVertex2ff(5,30+u*20);
            Comando(CMD_END);
        }
        u=4;
        if(barco[bot+u].matricula[4]!=0){
            //<<<<<<<<<<<<<<LETRERO Y TIEMPO>>>>>>>>>>>>>>//
            ComColor(0xff,0xff,0xff);
            if(barco[bot+u].greentick==1) ComColor(0,220,32);
            ComTXT(50,29+u*20, 22, OPT_CENTERX, barco[bot+u].matricula);
            ComColor(0xff,0xff,0xff);
            if(barco[bot+u].updown==0){
                ComTXT(160,34+u*20, 20, OPT_CENTERX, "BAJADA");
            }
            else{
                ComTXT(160,34+u*20, 20, OPT_CENTERX, "SUBIDA");
            }
            if(barco[bot+u].tcita-treal<6 && treal<barco[bot+u].tcita) ComColor(255, 0, 0);
            sprintf(string,"%d",barco[bot+u].tcita/60);
            ComTXT(263,29+u*20, 22, OPT_RIGHTX, string);
            sprintf(string,"%d",barco[bot+u].tcita%60);
            ComTXT(270,29+u*20, 22, OPT_RIGHTX, ":");
            ComTXT(290,29+u*20, 22, OPT_RIGHTX, string);
            //<<<<<<<<<<<<<<BORDE EXTERIOR NEGRO>>>>>>>>>>>>>>//
            ComColor(0,0,0);
            Comando(CMD_BEGIN_LINES);
            ComVertex2ff(5,30+u*20);
            ComVertex2ff(310,30+u*20);
            ComVertex2ff(310,30+u*20);
            ComVertex2ff(310,50+u*20);
            ComVertex2ff(310,50+u*20);
            ComVertex2ff(5,50+u*20);
            ComVertex2ff(5,50+u*20);
            ComVertex2ff(5,30+u*20);
            Comando(CMD_END);
        }
        u=5;
        if(barco[bot+u].matricula[4]!=0){
            //<<<<<<<<<<<<<<LETRERO Y TIEMPO>>>>>>>>>>>>>>//
            ComColor(0xff,0xff,0xff);
            if(barco[bot+u].greentick==1) ComColor(0,220,32);
            ComTXT(50,29+u*20, 22, OPT_CENTERX, barco[bot+u].matricula);
            ComColor(0xff,0xff,0xff);
            if(barco[bot+u].updown==0){
                ComTXT(160,34+u*20, 20, OPT_CENTERX, "BAJADA");
            }
            else{
                ComTXT(160,34+u*20, 20, OPT_CENTERX, "SUBIDA");
            }
            if(barco[bot+u].tcita-treal<6 && treal<barco[bot+u].tcita) ComColor(255, 0, 0);
            sprintf(string,"%d",barco[bot+u].tcita/60);
            ComTXT(263,29+u*20, 22, OPT_RIGHTX, string);
            sprintf(string,"%d",barco[bot+u].tcita%60);
            ComTXT(270,29+u*20, 22, OPT_RIGHTX, ":");
            ComTXT(290,29+u*20, 22, OPT_RIGHTX, string);
            //<<<<<<<<<<<<<<BORDE EXTERIOR NEGRO>>>>>>>>>>>>>>//
            ComColor(0,0,0);
            Comando(CMD_BEGIN_LINES);
            ComVertex2ff(5,30+u*20);
            ComVertex2ff(310,30+u*20);
            ComVertex2ff(310,30+u*20);
            ComVertex2ff(310,50+u*20);
            ComVertex2ff(310,50+u*20);
            ComVertex2ff(5,50+u*20);
            ComVertex2ff(5,50+u*20);
            ComVertex2ff(5,30+u*20);
            Comando(CMD_END);
        }
        u=6;
        if(barco[bot+u].matricula[4]!=0){
            //<<<<<<<<<<<<<<LETRERO Y TIEMPO>>>>>>>>>>>>>>//
            ComColor(0xff,0xff,0xff);
            if(barco[bot+u].greentick==1) ComColor(0,220,32);
            ComTXT(50,29+u*20, 22, OPT_CENTERX, barco[bot+u].matricula);
            ComColor(0xff,0xff,0xff);
            if(barco[bot+u].updown==0){
                ComTXT(160,34+u*20, 20, OPT_CENTERX, "BAJADA");
            }
            else{
                ComTXT(160,34+u*20, 20, OPT_CENTERX, "SUBIDA");
            }
            if(barco[bot+u].tcita-treal<6 && treal<barco[bot+u].tcita) ComColor(255, 0, 0);
            sprintf(string,"%d",barco[bot+u].tcita/60);
            ComTXT(263,29+u*20, 22, OPT_RIGHTX, string);
            sprintf(string,"%d",barco[bot+u].tcita%60);
            ComTXT(270,29+u*20, 22, OPT_RIGHTX, ":");
            ComTXT(290,29+u*20, 22, OPT_RIGHTX, string);
            //<<<<<<<<<<<<<<BORDE EXTERIOR NEGRO>>>>>>>>>>>>>>//
            ComColor(0,0,0);
            Comando(CMD_BEGIN_LINES);
            ComVertex2ff(5,30+u*20);
            ComVertex2ff(310,30+u*20);
            ComVertex2ff(310,30+u*20);
            ComVertex2ff(310,50+u*20);
            ComVertex2ff(310,50+u*20);
            ComVertex2ff(5,50+u*20);
            ComVertex2ff(5,50+u*20);
            ComVertex2ff(5,30+u*20);
            Comando(CMD_END);
        }
        u=7;
        if(barco[bot+u].matricula[4]!=0){
            //<<<<<<<<<<<<<<LETRERO Y TIEMPO>>>>>>>>>>>>>>//
            ComColor(0xff,0xff,0xff);
            if(barco[bot+u].greentick==1) ComColor(0,220,32);
            ComTXT(50,29+u*20, 22, OPT_CENTERX, barco[bot+u].matricula);
            ComColor(0xff,0xff,0xff);
            if(barco[bot+u].updown==0){
                ComTXT(160,34+u*20, 20, OPT_CENTERX, "BAJADA");
            }
            else{
                ComTXT(160,34+u*20, 20, OPT_CENTERX, "SUBIDA");
            }
            if(barco[bot+u].tcita-treal<6 && treal<barco[bot+u].tcita) ComColor(255, 0, 0);
            sprintf(string,"%d",barco[bot+u].tcita/60);
            ComTXT(263,29+u*20, 22, OPT_RIGHTX, string);
            sprintf(string,"%d",barco[bot+u].tcita%60);
            ComTXT(270,29+u*20, 22, OPT_RIGHTX, ":");
            ComTXT(290,29+u*20, 22, OPT_RIGHTX, string);
            //<<<<<<<<<<<<<<BORDE EXTERIOR NEGRO>>>>>>>>>>>>>>//
            ComColor(0,0,0);
            Comando(CMD_BEGIN_LINES);
            ComVertex2ff(5,30+u*20);
            ComVertex2ff(310,30+u*20);
            ComVertex2ff(310,30+u*20);
            ComVertex2ff(310,50+u*20);
            ComVertex2ff(310,50+u*20);
            ComVertex2ff(5,50+u*20);
            ComVertex2ff(5,50+u*20);
            ComVertex2ff(5,30+u*20);
            Comando(CMD_END);
        }
        //<<<<<<<<<<<<<<BOTON PULSE PARA CONTINUAR>>>>>>>>>>>>>>//
        ComColor(0xff,0xff,0xff);
        if(POSX>5 && POSX<310 && POSY>170 && POSY<220)  ComButton(5,170,305,50,28,256,"Pulse para continuar");

        else                                            ComButton(5,170,305,50,28,0,"Pulse para continuar");

        Dibuja();
        SysCtlDelay(300*MSEC);
    }
}

void preguntacita(void){            //Función que pregunta si el usuario tenía cita previa
    int i, timeout=0, timeover=0, eye=1;
    for(i=0;i<6;i++)  Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);   //CALIBRACIÓN
    char string[50];

    //Mientras no se pulse SI ni NO sigue funcionando, si se pulsan cambian el estado
    while(!(POSX>40 && POSX<90 && POSY>120 && POSY<170) && !(POSX>230 && POSX<280 && POSY>120 && POSY<170) && timeover==0){
        if (eye==1){
            timeout=tsec+15;
            eye=0;
        }
        Lee_pantalla();
        Nueva_pantalla(0x10,0x10,0x10);
        ComGradient(0,0, AZULOSCURO, 320, 240, AZULCLARO);              //Gradiente de azul oscuro a azul claro

        //<<<<<<<<<<<<<<FONDO AZUL OSCURO>>>>>>>>>>>>>>//
        ComLineWidth(1);
        ComColor(0, 52, 193);
        Comando(CMD_BEGIN_RECTS);
        ComVertex2ff(5,10);
        ComVertex2ff(310,30);
        Comando(CMD_END);

        //<<<<<<<<<<<<<BORDE EXTERIOR NEGRO>>>>>>>>>>>>//
        ComColor(0,0,0);
        ComLineWidth(1);
        Comando(CMD_BEGIN_LINES);
        ComVertex2ff(5,10);
        ComVertex2ff(310,10);
        ComVertex2ff(310,10);
        ComVertex2ff(310,30);
        ComVertex2ff(310,30);
        ComVertex2ff(5,30);
        ComVertex2ff(5,30);
        ComVertex2ff(5,10);
        Comando(CMD_END);
        ComColor(0xff,0xff,0xff);
        ComTXT(160, 10, 22,  OPT_CENTERX, "PANAMAX");
        sprintf(string,"%d",treal/60);
        ComTXT(263,10, 22, OPT_RIGHTX, string);
        sprintf(string,"%d",treal%60);
        ComTXT(270,10, 22, OPT_RIGHTX, ":");
        ComTXT(290,10, 22, OPT_RIGHTX, string);

        ComTXT(150,50, 22, OPT_CENTERX, "HA RESERVADO CON PANAMAX:");
        //<<<<<<<<<<<<<<BOTONES SI Y NO >>>>>>>>>>>>>>//
        if(POSX>40 && POSX<90 && POSY>120 && POSY<170){
            ComButton(40,120,50,50,28,256,"SI");
            eye=1;
        }

        else                                                ComButton(40,120,50,50,28,0,"SI");


        if(POSX>230 && POSX<280 && POSY>120 && POSY<170){
                                                            ComButton(230,120,50,50,28,256,"NO");
                                                            no=1;    //DETONADOR PARA CAMBIO AL ESTADO 3
                                                            eye=1;

        }

        else{
                                                            ComButton(230,120,50,50,28,0,"NO");
        }

        Dibuja();
        SysCtlDelay(300*MSEC);
        if(tsec>=timeout) timeover=1;

    }
    if(no==1)       estado=3;
    else            estado=2;
    if(timeover==1) estado=0;

}

void registro(void){                //Función que registra una matrícula.

    char string[50];
    int i;
    for(i=0;i<6;i++)  Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);   //CALIBRACIÓN
        int n=-1;
        int incompleto=0;
        int eye=1;
        int back=0;
        int enter=0;
        int letra=3;
        int fil=0;
        int col=0;
        int s;
        char matrivoid[7]={"!!!!!!!"};
        int timeout=0, timeover=0;
        timeout=tsec+15;
        for(i=0;i<=6;i++){
            barco[189].matricula[i]=' ';
        }
        while(enter==0 && back==0 && timeover==0){ // enter==0 || back==0


            Lee_pantalla();
            Nueva_pantalla(0x10,0x10,0x10);
            ComGradient(0,0, AZULOSCURO, 320, 240, AZULCLARO);              //Gradiente de azul oscuro a azul claro

            //<<<<<<<<<<<<<<FONDO AZUL OSCURO>>>>>>>>>>>>>>//
            ComLineWidth(1);
            ComColor(0, 52, 193);
            Comando(CMD_BEGIN_RECTS);
            ComVertex2ff(5,10);
            ComVertex2ff(310,30);
            Comando(CMD_END);

            //<<<<<<<<<<<<<<BORDE EXTERIOR NEGRO>>>>>>>>>>>>>>//
            ComColor(0,0,0);
            ComLineWidth(1);
            Comando(CMD_BEGIN_LINES);
            ComVertex2ff(5,10);
            ComVertex2ff(310,10);
            ComVertex2ff(310,10);
            ComVertex2ff(310,30);
            ComVertex2ff(310,30);
            ComVertex2ff(5,30);
            ComVertex2ff(5,30);
            ComVertex2ff(5,10);
            Comando(CMD_END);
            ComColor(0xff,0xff,0xff);
            ComTXT(160, 10, 22,  OPT_CENTERX, "PANAMAX");
            sprintf(string,"%d",treal/60);
            ComTXT(263,10, 22, OPT_RIGHTX, string);
            sprintf(string,"%d",treal%60);
            ComTXT(270,10, 22, OPT_RIGHTX, ":");
            ComTXT(290,10, 22, OPT_RIGHTX, string);



//<<<<<<<<<<<<<<ATRÁS Y CONTINUAR >>>>>>>>>>>>>>//

            if(POSX>5 && POSX<85 && POSY>210 && POSY<230){
                                                                ComButton(5,210,85,20,20,256,"ATRAS");
                                                                back=1;    //DETONADOR PARA CAMBIO AL ESTADO 1, ¿TIENE RESERVA?
                                                                eye=1;
            }

            else                                                ComButton(5,210,85,20,20,0,"ATRAS");


            if(POSX>225 && POSX<310 && POSY>210 && POSY<230){
                                                                 s=0;
                                                                ComButton(225,210,85,20,20,256,"CONTINUAR");
//                                                                  if(strcmp(matrivoid,barco[189].matricula)== 0){ //Compara la cadena con otra que está a 0000AAA
                                                                      for(i=0;i<=6;i++){
                                                                          if(matrivoid[i]>barco[189].matricula[i]){
                                                                              s++;
                                                                               i=7;
                                                                          }
                                                                      }

                                                                      if(s==0){
                                                                          incompleto=0;
                                                                           enter=1;    //DETONADOR PARA CAMBIO AL ESTADO 6, HORAS DISPONIBLES
                                                                           eye=1;
                                                                      }
                                                                      else {
                                                                          incompleto=1;
                                                                          enter=0;
                                                                          eye=1;
                                                                      }




            }

            else                                                ComButton(225,210,85,20,20,0,"CONTINUAR");


            /*
             * =========== TECLADO ==========
             *  [1][2][3]   [B][C][D][F][G]
             *  [4][5][6]   [H][J][K][L][M]
             *  [7][8][9]   [N][P][R][S][T]
             *  [S][0]      [V][W][X][Y][Z]
             *
             *   [ATRAS]          [ACEPTAR]
             */

            fil=0;
            col=0;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                n++;
                if (n>3) n=3;
                ComButton(10+25*fil,100+25*col,25,25,22,256,"1");
                barco[189].matricula[n]='1';
                eye=1;
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"1");

            fil=1;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                n++;
                if (n>3) n=3;
                ComButton(10+25*fil,100+25*col,25,25,22,256,"2");
                barco[189].matricula[n]='2';
                eye=1;
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"2");

            fil=2;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                n++;
                if (n>3) n=3;
                ComButton(10+25*fil,100+25*col,25,25,22,256,"3");
                barco[189].matricula[n]='3';
                eye=1;
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"3");

            col=1; fil=0;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                n++;
                if (n>3) n=3;
                ComButton(10+25*fil,100+25*col,25,25,22,256,"4");
                barco[189].matricula[n]='4';
                eye=1;
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"4");

            fil=1;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                n++;
                if (n>3) n=3;
                ComButton(10+25*fil,100+25*col,25,25,22,256,"5");
                barco[189].matricula[n]='5';
                eye=1;
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"5");

            fil=2;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                n++;
                if (n>3) n=3;
                ComButton(10+25*fil,100+25*col,25,25,22,256,"6");
                barco[189].matricula[n]='6';
                eye=1;
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"6");

            col=2; fil=0;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                n++;
                if (n>3) n=3;
                ComButton(10+25*fil,100+25*col,25,25,22,256,"7");
                barco[189].matricula[n]='7';
                eye=1;
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"7");

            fil=1;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                n++;
                if (n>3) n=3;
                ComButton(10+25*fil,100+25*col,25,25,22,256,"8");
                barco[189].matricula[n]='8';
                eye=1;
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"8");

            fil=2;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                n++;
                if (n>3) n=3;
                ComButton(10+25*fil,100+25*col,25,25,22,256,"9");
                barco[189].matricula[n]='9';
                eye=1;

            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"9");

            fil=0; col=3;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                ComButton(10+25*fil,100+25*col,25,25,20,256,"Supr");

                if(letra>3){
                    barco[189].matricula[letra]=' ';
                    letra--;
                    eye=1;
                    if (letra<3) letra=3;
                }
                else{
//                    if(n==3){
//                        barco[189].matricula[n]=' ';
//                        eye=1;
//                        n--;
//                    }
                    barco[189].matricula[n]=' ';
                        n--;
                        eye=1;
                        if (n<-1) n=-1;


                }
            }

            else ComButton(10+25*fil,100+25*col,25,25,20,0,"Supr");

            fil=1; col=3;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                n++;
                if (n>3) n=3;
                ComButton(10+25*fil,100+25*col,25,25,22,256,"0");
                barco[189].matricula[n]='0';
                eye=1;
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"0");


            fil=6; col=0;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                ComButton(10+25*fil,100+25*col,25,25,22,256,"B");
                if(n>2){
                    letra++;
                    if (letra>6) letra=6;
                    barco[189].matricula[letra]='B';
                    eye=1;
                }

            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"B");


            fil=7;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                ComButton(10+25*fil,100+25*col,25,25,22,256,"C");
                if(n>2){
                    letra++;
                    if (letra>6) letra=6;
                    barco[189].matricula[letra]='C';
                    eye=1;
                }
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"C");


            fil=8;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                ComButton(10+25*fil,100+25*col,25,25,22,256,"D");
                if(n>2){
                    letra++;
                    if (letra>6) letra=6;
                    barco[189].matricula[letra]='D';
                    eye=1;
                }
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"D");

            fil=9;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                ComButton(10+25*fil,100+25*col,25,25,22,256,"F");
                if(n>2){
                    letra++;
                    if (letra>6) letra=6;
                    barco[189].matricula[letra]='F';
                    eye=1;
                }
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"F");

            fil=10;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                ComButton(10+25*fil,100+25*col,25,25,22,256,"G");
                if(n>2){
                    letra++;
                    if (letra>6) letra=6;
                    barco[189].matricula[letra]='G';
                    eye=1;
                }
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"G");

            fil=6; col=1;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                ComButton(10+25*fil,100+25*col,25,25,22,256,"H");
                if(n>2){
                    letra++;
                    if (letra>6) letra=6;
                    barco[189].matricula[letra]='H';
                    eye=1;
                }
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"H");

            fil=7;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                ComButton(10+25*fil,100+25*col,25,25,22,256,"J");
                if(n>2){
                    letra++;
                    if (letra>6) letra=6;
                    barco[189].matricula[letra]='J';
                    eye=1;
                }
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"J");

            fil=8;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                ComButton(10+25*fil,100+25*col,25,25,22,256,"K");
                if(n>2){
                    letra++;
                    if (letra>6) letra=6;
                    barco[189].matricula[letra]='K';
                    eye=1;
                }
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"K");

            fil=9;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                ComButton(10+25*fil,100+25*col,25,25,22,256,"L");
                if(n>2){
                    letra++;
                    if (letra>6) letra=6;
                    barco[189].matricula[letra]='L';
                    eye=1;
                }
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"L");

            fil=10;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                ComButton(10+25*fil,100+25*col,25,25,22,256,"M");
                if(n>2){
                    letra++;
                    if (letra>6) letra=6;
                    barco[189].matricula[letra]='M';
                    eye=1;
                }
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"M");

            //Cuarta fila
            fil=6; col=2;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                ComButton(10+25*fil,100+25*col,25,25,22,256,"N");
                if(n>2){
                    letra++;
                    if (letra>6) letra=6;
                    barco[189].matricula[letra]='N';
                    eye=1;
                }
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"N");

            fil=7;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                ComButton(10+25*fil,100+25*col,25,25,22,256,"P");
                if(n>2){
                    letra++;
                    if (letra>6) letra=6;
                    barco[189].matricula[letra]='P';
                    eye=1;
                }
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"P");

            fil=8;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                ComButton(10+25*fil,100+25*col,25,25,22,256,"R");
                if(n>2){
                    letra++;
                    if (letra>6) letra=6;
                    barco[189].matricula[letra]='R';
                    eye=1;
                }
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"R");

            fil=9;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                ComButton(10+25*fil,100+25*col,25,25,22,256,"S");
                if(n>2){
                    letra++;
                    if (letra>6) letra=6;
                    barco[189].matricula[letra]='S';
                    eye=1;
                }
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"S");

            fil=10;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                ComButton(10+25*fil,100+25*col,25,25,22,256,"T");
                if(n>2){
                    letra++;
                    if (letra>6) letra=6;
                    barco[189].matricula[letra]='T';
                    eye=1;
                }
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"T");

            //Quinta fila
            fil=6; col=3;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                ComButton(10+25*fil,100+25*col,25,25,22,256,"V");
                if(n>2){
                    letra++;
                    if (letra>6) letra=6;
                    barco[189].matricula[letra]='V';
                    eye=1;
                }
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"V");

            fil=7;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                ComButton(10+25*fil,100+25*col,25,25,22,256,"W");
                if(n>2){
                    letra++;
                    if (letra>6) letra=6;
                    barco[189].matricula[letra]='W';
                    eye=1;
                }
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"W");

            fil=8;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                ComButton(10+25*fil,100+25*col,25,25,22,256,"X");
                if(n>2){
                    letra++;
                    if (letra>6) letra=6;
                    barco[189].matricula[letra]='X';
                    eye=1;
                }
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"X");

            fil=9;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                ComButton(10+25*fil,100+25*col,25,25,22,256,"Y");
                if(n>2){
                    letra++;
                    if (letra>6) letra=6;
                    barco[189].matricula[letra]='Y';
                    eye=1;
                }
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"Y");

            fil=10;
            if(POSX>10+25*fil && POSX<35+25*fil && POSY>100+25*col && POSY<125+25*col){
                ComButton(10+25*fil,100+25*col,25,25,22,256,"Z");
                if(n>2){
                    letra++;
                    if (letra>6) letra=6;
                    barco[189].matricula[letra]='Z';
                    eye=1;
                }
            }

            else
                ComButton(10+25*fil,100+25*col,25,25,22,0,"Z");





            if (incompleto){
                ComColor(255,0,0);
                ComTXT(100,50, 22, OPT_CENTERX, "Introduzca MATRICULA:");
            }
            else{
                ComColor(0xff,0xff,0xff);
                ComTXT(100,50, 22, OPT_CENTERX, "Introduzca su matricula:");
            }
            ComColor(0xff,0xff,0xff);
            ComTXT(260,50, 22, OPT_CENTERX, barco[189].matricula);
            Dibuja();
            if(eye){
                VolNota('0');
                TocaNota(0x00,N_DO);
                VolNota('2');
                TocaNota(0x57,N_DO);
                SysCtlDelay(300*MSEC);
                timeout=tsec+15;
                eye=0;
            }

            if(tsec>=timeout) timeover=1;
        }
        if (timeover==1)    estado=0;
        if (enter==1 && estado==3)       estado=7;
        if (enter==1 && estado==2)       estado=4;
        if (back==1)        estado=1;



}

void confirmar(void){               //Función Para confirmar si su número de matrícula es el que ha seleccionado si ya se tenía
                                    //                              cita previa y se ha encontrado la cita en la pila de datos.
        int i, timeout=0, timeover=0, eye=1,confirmar=0;
        for(i=0;i<6;i++)  Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);   //CALIBRACIÓN
        char string[50];

        //Mientras no se pulse SI ni NO sigue funcionando, si se pulsan cambian el estado
        while(!(POSX>40 && POSX<90 && POSY>120 && POSY<170) && !(POSX>230 && POSX<280 && POSY>120 && POSY<170) && timeover==0){
            if (eye==1){
                timeout=tsec+15;
                eye=0;
            }
            Lee_pantalla();
            Nueva_pantalla(0x10,0x10,0x10);
            ComGradient(0,0, AZULOSCURO, 320, 240, AZULCLARO);              //Gradiente de azul oscuro a azul claro

            //<<<<<<<<<<<<<<FONDO AZUL OSCURO>>>>>>>>>>>>>>//
            ComLineWidth(1);
            ComColor(0, 52, 193);
            Comando(CMD_BEGIN_RECTS);
            ComVertex2ff(5,10);
            ComVertex2ff(310,30);
            Comando(CMD_END);

            //<<<<<<<<<<<<<BORDE EXTERIOR NEGRO>>>>>>>>>>>>//
            ComColor(0,0,0);
            ComLineWidth(1);
            Comando(CMD_BEGIN_LINES);
            ComVertex2ff(5,10);
            ComVertex2ff(310,10);
            ComVertex2ff(310,10);
            ComVertex2ff(310,30);
            ComVertex2ff(310,30);
            ComVertex2ff(5,30);
            ComVertex2ff(5,30);
            ComVertex2ff(5,10);
            Comando(CMD_END);
            ComColor(0xff,0xff,0xff);
            ComTXT(160, 10, 22,  OPT_CENTERX, "PANAMAX");
            sprintf(string,"%d",treal/60);
            ComTXT(263,10, 22, OPT_RIGHTX, string);
            sprintf(string,"%d",treal%60);
            ComTXT(270,10, 22, OPT_RIGHTX, ":");
            ComTXT(290,10, 22, OPT_RIGHTX, string);



            ComColor(0xff,0xff,0xff);
            ComTXT(150,45, 22, OPT_CENTERX, "Indique si es esta su reserva:");
            ComTXT(90,75, 22, OPT_CENTERX, barco[aux].matricula);
            if(barco[aux].updown==0){
                ComTXT(160,78, 20, OPT_CENTERX, "BAJADA");
            }
            else{
                ComTXT(160,78, 20, OPT_CENTERX, "SUBIDA");
            }
            if(barco[aux].tcita-treal<6 && treal<barco[aux].tcita) ComColor(255, 0, 0);
            sprintf(string,"%d",barco[aux].tcita/60);
            ComTXT(210, 75, 22, OPT_RIGHTX, string);
            sprintf(string,"%d",barco[aux].tcita%60);
            ComTXT(217, 75, 22, OPT_RIGHTX, ":");         //+7
            ComTXT(237, 75, 22, OPT_RIGHTX, string);     //+27
            ComColor(0xff,0xff,0xff);
            //<<<<<<<<<<<<<<BOTONES SI Y NO >>>>>>>>>>>>>>//
            if(POSX>40 && POSX<90 && POSY>120 && POSY<170){
                ComButton(40,120,50,50,28,256,"SI");
                confirmar=1;
                eye=1;


            }

            else                                                ComButton(40,120,50,50,28,0,"SI");


            if(POSX>230 && POSX<280 && POSY>120 && POSY<170){
                                                                ComButton(230,120,50,50,28,256,"NO");
                                                                no=1;    //DETONADOR PARA CAMBIO AL ESTADO 3
                                                                eye=1;

            }

            else{
                                                                ComButton(230,120,50,50,28,0,"NO");
            }

            Dibuja();
            SysCtlDelay(300*MSEC);
            if(tsec>=timeout) timeover=1;

        }
        if(no==1)       estado=6;
        if(confirmar==1){
            estado=8;
            barco[aux].greentick=1;
        }
        if(timeover==1) estado=0;

    }

void gracias(void){                 //Función para agradecer el registro
    int i, a, b, c;

        for(i=0;i<6;i++)    Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);

            for(i=0;i<24;i++)
            {
                Nueva_pantalla(0x10,0x10,0x10);
                ComGradient(0, 0, AZULOSCURO, 320, 240,AZULCLARO-1542*i);
                SysCtlDelay(40*MSEC);
                Dibuja();
            }
            for(i=0;i<100;i++)
            {
                a=0+2*i;
                b=30+2*i; //30
                c=100+2*i;   //100
                if(a>255) a=255;
                if(b>255) b=255;
                if(c>255) c=255;
                Nueva_pantalla(0x10,0x10,0x10);
                ComGradient(0, 0, AZULOSCURO, 320, 240, AZULOSCURO);
                ComColor(a,b,c); //(0,30,100)-->(255,255,255)
                ComTXT(160, 110, 23,  OPT_CENTERX, "Gracias por confiar en Panamax,");
                ComTXT(160, 130, 23,  OPT_CENTERX, "su consulta ha sido registrada.");
                SysCtlDelay(40*MSEC);
                Dibuja();
            }
            //(0x00,0xb4,0xfa)
            for(i=0;i<24;i++)
            {
                Nueva_pantalla(0x10,0x10,0x10);
                ComGradient(0, 0, AZULOSCURO, 320, 240,AZULOSCURO+1542*i);
                ComColor(200,255,255);
                ComTXT(160, 110, 23,  OPT_CENTERX, "Gracias por confiar en Panamax,");
                ComTXT(160, 130, 23,  OPT_CENTERX, "su consulta ha sido registrada.");

                SysCtlDelay(40*MSEC);
                Dibuja();
            }
            estado=0;
    }

void Timer0Handler(void){           //Función de interrupción del TIMER 0

    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    tsec++;
    if (tsec==60){
        treal++;
        tsec=0;
        rankstack();
        if(barco[bot].tcita<treal) bot++;
    }
}

void horasdisponibles (void){
    char string[50];
    int i,k=1,l=1,exit=0, discard=0,target;
    for(i=0;i<6;i++)  Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);   //CALIBRACIÓN
    int incompleto=0;
    int eye=1;
    int back=0;
    int enter=0;
    int fil=0;
    int col=0;
    int t[8]={0};
    int timeout=0, timeover=0;
    timeout=tsec+15;
    int tselect=0;

    while(exit==0){

        discard=0;
        target=barco[bot].tcita +l*20;
        for(i=bot;i<(num);i++){
            if(target==barco[i].tcita){
                discard=1;
            }

        }

        if(t[k]==0 && discard==0){
            t[k]=target;
            k++;

        }
        l++;
        if(t[7]!=0) exit=1;
    }
    while(enter==0 && back==0 && timeover==0){ // enter==0 || back==0


        Lee_pantalla();
        Nueva_pantalla(0x10,0x10,0x10);
        ComGradient(0,0, AZULOSCURO, 320, 240, AZULCLARO);              //Gradiente de azul oscuro a azul claro

        //<<<<<<<<<<<<<<FONDO AZUL OSCURO>>>>>>>>>>>>>>//
        ComLineWidth(1);
        ComColor(0, 52, 193);
        Comando(CMD_BEGIN_RECTS);
        ComVertex2ff(5,10);
        ComVertex2ff(310,30);
        Comando(CMD_END);

        //<<<<<<<<<<<<<<BORDE EXTERIOR NEGRO>>>>>>>>>>>>>>//
        ComColor(0,0,0);
        ComLineWidth(1);
        Comando(CMD_BEGIN_LINES);
        ComVertex2ff(5,10);
        ComVertex2ff(310,10);
        ComVertex2ff(310,10);
        ComVertex2ff(310,30);
        ComVertex2ff(310,30);
        ComVertex2ff(5,30);
        ComVertex2ff(5,30);
        ComVertex2ff(5,10);
        Comando(CMD_END);
        ComColor(0xff,0xff,0xff);
        ComTXT(160, 10, 22,  OPT_CENTERX, "PANAMAX");
        sprintf(string,"%d",treal/60);
        ComTXT(263,10, 22, OPT_RIGHTX, string);
        sprintf(string,"%d",treal%60);
        ComTXT(270,10, 22, OPT_RIGHTX, ":");
        ComTXT(290,10, 22, OPT_RIGHTX, string);





        //<<<<<<<<<<<<<<ATRÁS Y CONTINUAR >>>>>>>>>>>>>>//

        if(POSX>5 && POSX<85 && POSY>210 && POSY<230){
            ComButton(5,210,85,20,20,256,"ATRAS");
            back=1;    //DETONADOR PARA CAMBIO AL ESTADO 1, ¿TIENE RESERVA?
            eye=1;
        }

        else                                                ComButton(5,210,85,20,20,0,"ATRAS");


        if(POSX>225 && POSX<310 && POSY>210 && POSY<230){

            ComButton(225,210,85,20,20,256,"CONTINUAR");
            if(tselect==0){
                incompleto=1;
                enter=0;
                eye=1;
            }
            else {
                incompleto=0;
                enter=1;
                eye=1;
            }
        }
        else                                                ComButton(225,210,85,20,20,0,"CONTINUAR");


        /*
         * =========== TIEMPOS DISPONIBLES ==========
         *  [T1]  [T2]  [T3]  [T4]
         *     [T5]  [T6]  [T7]
         *
         * [ATRAS]  [TSELECT]  [ACEPTAR]
         */
        fil=0;
        col=0;
        //TIEMPO 1
        if(POSX>10+50*fil && POSX<60+50*fil && POSY>100+25*col && POSY<125+25*col){
            ComButton(10+50*fil,100+25*col,50,25,22,256,"");
            sprintf(string,"%d",t[1]/60);
            ComTXT(10+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);
            sprintf(string,"%d",t[1]%60);
            ComTXT(17+15+50*fil,100+25*col, 22, OPT_RIGHTX, ":");
            ComTXT(37+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);

            barco[189].tcita=t[1];
            tselect=t[1];
            eye=1;
        }

        else{
            sprintf(string,"%d",t[1]);
            ComButton(10+50*fil,100+25*col,50,25,22,0,"");
            sprintf(string,"%d",t[1]/60);
            ComTXT(10+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);
            sprintf(string,"%d",t[1]%60);
            ComTXT(17+15+50*fil,100+25*col, 22, OPT_RIGHTX, ":");
            ComTXT(37+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);
        }


        fil=1;
        //TIEMPO 2
        if(POSX>10+50*fil && POSX<60+50*fil && POSY>100+25*col && POSY<125+25*col){
            ComButton(10+50*fil,100+25*col,50,25,22,256,"");
            sprintf(string,"%d",t[2]/60);
            ComTXT(10+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);
            sprintf(string,"%d",t[2]%60);
            ComTXT(17+15+50*fil,100+25*col, 22, OPT_RIGHTX, ":");
            ComTXT(37+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);

            barco[189].tcita=t[2];
            tselect=t[2];
            eye=1;
        }

        else{
            sprintf(string,"%d",t[2]);
            ComButton(10+50*fil,100+25*col,50,25,22,0,"");
            sprintf(string,"%d",t[2]/60);
            ComTXT(10+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);
            sprintf(string,"%d",t[2]%60);
            ComTXT(17+15+50*fil,100+25*col, 22, OPT_RIGHTX, ":");
            ComTXT(37+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);
        }

        fil=2;
        //TIEMPO 3
        if(POSX>10+50*fil && POSX<60+50*fil && POSY>100+25*col && POSY<125+25*col){
            ComButton(10+50*fil,100+25*col,50,25,22,256,"");
            sprintf(string,"%d",t[3]/60);
            ComTXT(10+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);
            sprintf(string,"%d",t[3]%60);
            ComTXT(17+15+50*fil,100+25*col, 22, OPT_RIGHTX, ":");
            ComTXT(37+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);

            barco[189].tcita=t[3];
            tselect=t[3];
            eye=1;
        }

        else{
            sprintf(string,"%d",t[3]);
            ComButton(10+50*fil,100+25*col,50,25,22,0,"");
            sprintf(string,"%d",t[3]/60);
            ComTXT(10+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);
            sprintf(string,"%d",t[3]%60);
            ComTXT(17+15+50*fil,100+25*col, 22, OPT_RIGHTX, ":");
            ComTXT(37+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);
        }


        fil=3;
        //TIEMPO 4
        if(POSX>10+50*fil && POSX<60+50*fil && POSY>100+25*col && POSY<125+25*col){
            ComButton(10+50*fil,100+25*col,50,25,22,256,"");
            sprintf(string,"%d",t[4]/60);
            ComTXT(10+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);
            sprintf(string,"%d",t[4]%60);
            ComTXT(17+15+50*fil,100+25*col, 22, OPT_RIGHTX, ":");
            ComTXT(37+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);

            barco[189].tcita=t[4];
            tselect=t[4];
            eye=1;
        }

        else{
            sprintf(string,"%d",t[4]);
            ComButton(10+50*fil,100+25*col,50,25,22,0,"");
            sprintf(string,"%d",t[4]/60);
            ComTXT(10+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);
            sprintf(string,"%d",t[4]%60);
            ComTXT(17+15+50*fil,100+25*col, 22, OPT_RIGHTX, ":");
            ComTXT(37+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);
        }


        col=2, fil=1;
        //TIEMPO 5
        if(POSX>10+50*fil && POSX<60+50*fil && POSY>100+25*col && POSY<125+25*col){
            ComButton(10+50*fil,100+25*col,50,25,22,256,"");
            sprintf(string,"%d",t[5]/60);
            ComTXT(10+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);
            sprintf(string,"%d",t[5]%60);
            ComTXT(17+15+50*fil,100+25*col, 22, OPT_RIGHTX, ":");
            ComTXT(37+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);

            barco[189].tcita=t[5];
            tselect=t[5];
            eye=1;
        }

        else{
            sprintf(string,"%d",t[5]);
            ComButton(10+50*fil,100+25*col,50,25,22,0,"");
            sprintf(string,"%d",t[5]/60);
            ComTXT(10+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);
            sprintf(string,"%d",t[5]%60);
            ComTXT(17+15+50*fil,100+25*col, 22, OPT_RIGHTX, ":");
            ComTXT(37+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);
        }


        fil=2;
        //TIEMPO 6
        if(POSX>10+50*fil && POSX<60+50*fil && POSY>100+25*col && POSY<125+25*col){
            ComButton(10+50*fil,100+25*col,50,25,22,256,"");
            sprintf(string,"%d",t[6]/60);
            ComTXT(10+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);
            sprintf(string,"%d",t[6]%60);
            ComTXT(17+15+50*fil,100+25*col, 22, OPT_RIGHTX, ":");
            ComTXT(37+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);

            barco[189].tcita=t[6];
            tselect=t[6];
            eye=1;
        }

        else{
            sprintf(string,"%d",t[6]);
            ComButton(10+50*fil,100+25*col,50,25,22,0,"");
            sprintf(string,"%d",t[6]/60);
            ComTXT(10+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);
            sprintf(string,"%d",t[6]%60);
            ComTXT(17+15+50*fil,100+25*col, 22, OPT_RIGHTX, ":");
            ComTXT(37+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);
        }


        fil=3;
        //TIEMPO 7
        if(POSX>10+50*fil && POSX<60+50*fil && POSY>100+25*col && POSY<125+25*col){
            ComButton(10+50*fil,100+25*col,50,25,22,256,"");
            sprintf(string,"%d",t[7]/60);
            ComTXT(10+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);
            sprintf(string,"%d",t[7]%60);
            ComTXT(17+15+50*fil,100+25*col, 22, OPT_RIGHTX, ":");
            ComTXT(37+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);

            barco[189].tcita=t[7];
            tselect=t[7];
            eye=1;
        }

        else{
            sprintf(string,"%d",t[7]);
            ComButton(10+50*fil,100+25*col,50,25,22,0,"");
            sprintf(string,"%d",t[7]/60);
            ComTXT(10+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);
            sprintf(string,"%d",t[7]%60);
            ComTXT(17+15+50*fil,100+25*col, 22, OPT_RIGHTX, ":");
            ComTXT(37+15+50*fil,100+25*col, 22, OPT_RIGHTX, string);
        }



        sprintf(string,"%d",tselect/60);
        ComTXT(160,200, 22, OPT_RIGHTX, string);
        sprintf(string,"%d",tselect%60);
        ComTXT(167,200, 22, OPT_RIGHTX, ":");
        ComTXT(197,200, 22, OPT_RIGHTX, string);


        if (incompleto){
            ComColor(255,0,0);
            ComTXT(150,50, 22, OPT_CENTERX, "Seleccione el TIEMPO de cita:");
        }
        else{
            ComColor(0xff,0xff,0xff);
            ComTXT(150,50, 22, OPT_CENTERX, "Seleccione el tiempo de cita:");
        }
        ComColor(0xff,0xff,0xff);
        Dibuja();
        if(eye){
            VolNota('0');
            TocaNota(0x00,N_DO);
            VolNota('2');
            TocaNota(0x41,N_SOL);
            SysCtlDelay(300*MSEC);
            timeout=tsec+15;
            eye=0;
        }

        if(tsec>=timeout) timeover=1;
        if (timeover==1)    estado=0;
        if (enter==1){
            pushstack();
            estado=8;
        }
        if (back==1)        estado=3;
    }
}

void matnotfound (void){            //Función que se activa en el estado 6 si la matrícula no está en la base de datos
int i, timeout=0,timeover=0, eye=1, continuar=0;
for(i=0;i<6;i++)  Esc_Reg(REG_TOUCH_TRANSFORM_A+4*i, REG_CAL[i]);   //CALIBRACIÓN
char string[50];

//Mientras no se pulse SI ni NO sigue funcionando, si se pulsan cambian el estado
while(!(POSX>90 && POSX<190 && POSY>120 && POSY<170) ){
 if (eye==1){
     timeout=tsec+15;
     eye=0;
 }
 Lee_pantalla();
 Nueva_pantalla(0x10,0x10,0x10);
 ComGradient(0,0, AZULOSCURO, 320, 240, AZULCLARO);              //Gradiente de azul oscuro a azul claro

 //<<<<<<<<<<<<<<FONDO AZUL OSCURO>>>>>>>>>>>>>>//
 ComLineWidth(1);
 ComColor(0, 52, 193);
 Comando(CMD_BEGIN_RECTS);
 ComVertex2ff(5,10);
 ComVertex2ff(310,30);
 Comando(CMD_END);

 //<<<<<<<<<<<<<BORDE EXTERIOR NEGRO>>>>>>>>>>>>//
 ComColor(0,0,0);
 ComLineWidth(1);
 Comando(CMD_BEGIN_LINES);
 ComVertex2ff(5,10);
 ComVertex2ff(310,10);
 ComVertex2ff(310,10);
 ComVertex2ff(310,30);
 ComVertex2ff(310,30);
 ComVertex2ff(5,30);
 ComVertex2ff(5,30);
 ComVertex2ff(5,10);
 Comando(CMD_END);
 ComColor(0xff,0xff,0xff);
 ComTXT(160, 10, 22,  OPT_CENTERX, "PANAMAX");
 sprintf(string,"%d",treal/60);
 ComTXT(263,10, 22, OPT_RIGHTX, string);
 sprintf(string,"%d",treal%60);
 ComTXT(270,10, 22, OPT_RIGHTX, ":");
 ComTXT(290,10, 22, OPT_RIGHTX, string);

 ComTXT(150,50, 22, OPT_CENTERX, "Su matricula no se encuentra registrada");
 ComTXT(155,70, 22, OPT_CENTERX, "pruebe de nuevo o contacte con el ");
 ComTXT(160,90, 22, OPT_CENTERX, "servicio tecnico");
 //<<<<<<<<<<<<<<BOTONES SI Y NO >>>>>>>>>>>>>>//
 if(POSX>90 && POSX<190 && POSY>120 && POSY<170){
     ComButton(90,120,100,50,28,256,"continuar");
     eye=1;
     continuar=1;

 }

 else                                                ComButton(90,120,100,50,28,0,"continuar");




 Dibuja();
 if(tsec>=timeout) timeover=1;
 SysCtlDelay(300*MSEC);
 if(continuar==1) estado=0;
 if(timeover==1) estado=0;



}
    estado=0;

}

void abrirgrifo(void){
    int tsecnow=tsec;
    while(tsecnow+20>tsec){
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, 2900);
    }
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, 0);
    opentap=0;

}

void cerrargrifo(void){
    int tsecnow=tsec;
    while(tsecnow+20>tsec){
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, 2600);
    }
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, 0);
    closetap=0;
}

void abrirpuerta(void){
    int tsecnow=tsec;
    while(tsecnow+30>tsec){
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 2800);
    }
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 0);
    opendoor=0;
}

void cerrarpuerta(void){
    int tsecnow=tsec;
    while(tsecnow+30>tsec){
        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 2500);
    }
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 0);
    closedoor=0;

}

void servohandler(void){
    if(opendoor==1){
        abrirpuerta();

    }
    if(closedoor==1){
        cerrarpuerta();
    }
    if(opentap==1){
        abrirgrifo();

    }
    if(closetap==1){
        cerrargrifo();
    }

}


/* ==============================================================================================
 * ======================================   {{{ MAIN }}}   ======================================
 * ==============================================================================================
 */
int main(void) {

    /*
     * ===== CONFIGURACIÓN DE BOTONES Y LEDS DE LA PLACA =====
     * 1º HABILITAR PUERTOS
     * F0 y F4: salidas
     * N0 y N1: salidas
     * J0 y J1: entradas
     * GPIOPadConfigSet Pullup en J0 y J1
     */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);


    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0 |GPIO_PIN_4);
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 |GPIO_PIN_1);
    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0|GPIO_PIN_1);
    GPIOPadConfigSet(GPIO_PORTJ_BASE,GPIO_PIN_0|GPIO_PIN_1,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0 |GPIO_PIN_1);
    /* ======================================================*/


//    GPIOIntEnable(GPIO_PORTJ_BASE, GPIO_PIN_0|GPIO_PIN_1);
    //GPIOIntRegister(GPIO_PORTJ_BASE, PulsaBoton);
//    IntEnable(INT_GPIOJ);
//    IntMasterEnable();

    SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);


    PWMClockSet(PWM0_BASE,PWM_SYSCLK_DIV_64);   // al PWM le llega un reloj de 1.875MHz

    GPIOPinConfigure(GPIO_PF1_M0PWM1);          //Configurar el pin a PWM
    GPIOPinConfigure(GPIO_PF2_M0PWM2);          //Configurar el otro pin al PWM

    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_1);
    GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_2);

    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0|GPIO_PIN_1);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);
    GPIOPadConfigSet(GPIO_PORTJ_BASE,GPIO_PIN_0|GPIO_PIN_1,GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD_WPU);

    //Configurar el pwm0, contador descendente y sin sincronización (actualización automática)
    PWMGenConfigure(PWM0_BASE, PWM_GEN_0, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
    PWMGenConfigure(PWM0_BASE, PWM_GEN_1, PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);

    PeriodoPWM=37499; // 50Hz  a 1.875MHz
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_0, PeriodoPWM);      //frec:50Hz
    PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1, PeriodoPWM);      //frec 50Hz
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_1, 0);              //Inicialmente, 0
    PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2, 0);              //Inicialmente, 0
    PWMGenEnable(PWM0_BASE, PWM_GEN_0);                     //Habilita el generador 0
    PWMGenEnable(PWM0_BASE, PWM_GEN_1);                     //Habilita el generador 1
    PWMOutputState(PWM0_BASE, PWM_OUT_1_BIT , true);        //Habilita la salida 1
    PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT , true);        //Habilita la salida 2



    //Habilitar y configurar botones con interrupción

    //PWM config


     // 50Hz  a 1.875MHz
    //Pos_0=1875; Pos_90=2812; Pos_180=3750;


    /*
     * ======================= RELOJ =========================
     */
    RELOJ=SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000);

    /*
     * ===== CONFIGURACIÓN DE TIMER 0 /// RELOJ QUE PROPORCIONA EL TIEMPO. =====
     * 1º Habilitar TIMER 0
     * 2º Establecer la frecuencia de t0 a 120mz
     * 3º Configurar T0 como periódico
     * 4º Establecer el PERIODO (1s)
     * 5º Declarar qué función se llamará durante la interrupción
     * 6º Habilitar interrupciones globales de los timers
     * 7º Habilitar las interrupciones de timeout
     * 8º Habilitar interrupciones
     */
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    TimerClockSourceSet(TIMER0_BASE, TIMER_CLOCK_SYSTEM);
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    TimerLoadSet(TIMER0_BASE, TIMER_A, RELOJ-1);
    TimerIntRegister(TIMER0_BASE,TIMER_A, Timer0Handler);
    IntEnable(INT_TIMER0A);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntMasterEnable();
    TimerEnable(TIMER0_BASE, TIMER_A);                      //Habilitar Timer0

    HAL_Init_SPI(2, RELOJ);                                             //BOOSTERPACK a usar y velocidad del MC

    Inicia_pantalla();
    SysCtlDelay(RELOJ/3);
    intlist();
    int exit;
    while(1){
        switch (estado){

        /*
         * ============== LISTA DE ESPERA ============== ESTADO 0
         */
        case 0:

            VolNota('0');
            transcreen();
//                TocaNota(0x57,N_DO);
                 VolNota('2');
//                TocaNota(0x32,N_DO);
//                notatime=tsec+1;
//                while(notatime>tsec)
//                TocaNota(0x32,N_SI);
//                notatime=tsec+1;
//                while(notatime>tsec)
//                TocaNota(0x32,N_SI);
//                notatime=tsec+1;
//                while(notatime>tsec)
//                TocaNota(0x32,N_SOL);
//                notatime=tsec+1;
//                while(notatime>tsec)
//                TocaNota(0x32,N_DO);
//                notatime=tsec+1;
//                while(notatime>tsec)
                //TocaNota(0x02,N_DO);
                TocaNota(0x41,N_DO);
                TocaNota(0x41,N_RE);
                TocaNota(0x41,N_MI);
                TocaNota(0x41,N_FA);
                TocaNota(0x41,N_SOL);

            FinNota();

            pintalista();
            estado=1;
        break;

        /*
         * =========== PANTALLA DE PREGUNTA ========== ESTADO 1
         */
        case 1:
            transcreen();
            preguntacita();
        break;

        /*
         * =========== PANTALLA DE REGISTRO EN CASO DE RESPONDER NO ========== ESTADO 2
         */
        case 2:
            transcreen();
            registro();
        break;

        /*
         * =========== PANTALLA DE REGISTRO EN CASO DE RESPONDER SI ========== ESTADO 3
         */
        case 3:
            transcreen();
            registro();
        break;

        /*
         * =========== PANTALLA DE COMPROBRACIÓN PARA DETECTAR SI ========== ESTADO 4
         * LA MATRÍCULA INTRODUCIDA SE ENCUENTRA O NO EN LISTA
         */
        case 4:
            exit=0;
            for(aux=0;exit!=1;aux++){
                if(strcmp(barco[189].matricula,barco[aux].matricula)== 0){ //Compara las cadenas y devuelve 0 si son iguales.
                    exit=1;
                    estado=5;
                }
                if(aux>num){
                    estado=6;
                    exit=1;
                }
            }
            aux--;

        break;

        /*
         * =========== PANTALLA PARA CONFIRMAR LA CITA ========== ESTADO 5
         */
        case 5:
            transcreen();
            confirmar();

        break;

        /*
         * =========== PANTALLA EN CASO DE QUE NO SE ENCUENTRE ========== ESTADO 6
         * CORRESPONDENCIA ENTRE MATRICULAS EN EL ESTADO 4
         */
        case 6:
            transcreen();
            matnotfound();

        break;


        case 7:
            horasdisponibles();
        break;
        /*
         * =========== PANTALLA DE AGRADECIMIENTOS ========== ESTADO 5
         */
        case 8:
            gracias();
        break;




        }
    }
}

