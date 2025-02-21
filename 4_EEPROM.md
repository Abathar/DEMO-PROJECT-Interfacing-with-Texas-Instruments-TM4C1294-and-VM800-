<h2>Manejo de la EEPROM.</h2>

Para este apartado se ha configurado la EEPROM tal y cómo se indican en guías sobre el manejo de esta y se ha escrito en una dirección de memoria disponible, que se va actualizando con forme al tamaño del dato guardado previamente, sumando la longitud del dato a la dirección en la que se escribirá en la EEPROM, de tal manera que no se borren los datos que ya se han escrito. 
Para ello primero creamos una estructura que tiene la forma de los datos que queremos transmitir, en este caso siete caracteres y un número entero:
```C
struct E2PROM                           //Creamos una estructura en la que irán los datos que enviaremos a la EEPROM
{
    uint8_t matricula[8];
    uint16_t tcita;
} escribir = {"1111AAA",1000};          //rellenamos con valores iniciales
```
Una vez tenemos la estructura de los datos que se van a escribir configuramos la EEPROM en el main
```C   
    SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0); // Primero activamos la EEPROM
    EEPROMInit(); // Iniciamos la EEPROM
    uint32_t e2size,e2block,ADDRESS=0x0000,tcitahex=0x0000;                //Variables que nos servirán a continuación
    e2size = EEPROMSizeGet();               // Obtenemos el tamaño de la EEPROM
    e2block = EEPROMBlockCountGet();        // Determino el número de bloques de la EEPROM
```
Y finalmente, en el estado 9 en el que ya hemos confirmado que vamos a guardar la matrícula y el tiempo de cita en la EEPROM, guardamos y actualizamos la 
dirección en la que escribiremos la próxima vez.
```C            
            barco[189].tcita=escribir.tcita;
            EEPROMProgram((uint32_t *)&escribir, ADDRESS, sizeof(escribir)); //Escribimos en la EEPROM
            tcitahex=sizeof(escribir) & 0xFFFF;
            ADDRESS=(ADDRESS+tcitahex) & 0xFFFF;        //Actualizamos el valor de la dirección donde estcribirá el próximo dato
```
------------------------------------------------- ----------------------------------------------------------------------- --------------------------------------------------------
<h2>EEPROM Handling</h2>
For this section, the EEPROM has been configured as indicated in the guides on its handling and has been written to an available memory address, which is updated according to the size of the previously stored data, adding the length of the data to the address where it will be written in the EEPROM, in such a way that the data that has already been written is not erased.

For this, we first create a structure that has the shape of the data we want to transmit, in this case seven characters and an integer number:

```C

struct E2PROM                           //We create a structure in which the data that we will send to the EEPROM will go
{
    uint8_t matricula[8];
    uint16_t tcita;
} escribir = {"1111AAA",1000};          //we fill with initial values
```
Once we have the structure of the data to be written, we configure the EEPROM in the main

```C

    SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0); // First we activate the EEPROM
    EEPROMInit(); // We initialize the EEPROM
    uint32_t e2size,e2block,ADDRESS=0x0000,tcitahex=0x0000;                //Variables that will be useful to us below
    e2size = EEPROMSizeGet();               // We get the size of the EEPROM
    e2block = EEPROMBlockCountGet();        // I determine the number of blocks of the EEPROM
```
And finally, in state 9 in which we have already confirmed that we are going to save the license plate and the appointment time in the EEPROM, we save and update the address in which we will write the next time.

```C

            barco[189].tcita=escribir.tcita;
            EEPROMProgram((uint32_t *)&escribir, ADDRESS, sizeof(escribir)); //We write in the EEPROM
            tcitahex=sizeof(escribir) & 0xFFFF;
            ADDRESS=(ADDRESS+tcitahex) & 0xFFFF;        //We update the value of the address where the next data will be written
```
