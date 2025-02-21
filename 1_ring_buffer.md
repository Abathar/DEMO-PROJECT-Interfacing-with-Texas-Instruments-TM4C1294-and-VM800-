<h2>Manejo de la pila circular.</h2>
2.1. Manejo de la pila circular.
La creación de la pila circular nace como respuesta a la necesidad de guardar los datos relevantes de los vehículos como la matrícula, el tiempo de cita,
si la cita está confirmada o no y si el trayecto realizado será de subida o bajada.
Para ello se ha creado una estructura de doscientos componentes que pueda alojar doscientos posibles tránsitos a lo largo del día, aunque ya veremos que son más de los necesarios.

```c
struct navio{                           //Estructura de los barcos
    char matricula[7];
    int tcita;
    int updown;
    int greentick;
} barco[200] = {0};                     //Inicializamos lo que será nuestra pila circular
```
Para ordenar la pila de información se utiliza la función rankstack(), la cual se llama cada minuto en la función de manejo de interrupción del TIMER0, Timer0Handler(), el cual se utiliza como reloj, actualizando las variables tsec y treal, correspondientes con los segundos y minutos del reloj.
Rankstack() es una función vacía que modifica la posición de cada componente de la estructura navío según su tiempo de cita. Para ello mira si el tiempo de cita de una estructura que esté en el puesto N de la pila es menor que el tiempo de cita de su homólogo inmediatamente inferior, si es así lo que hace es intercambiar los datos de las dos estructuras, subiendo de posición la que tiene mayor tiempo de cita y bajando la que tiene menor tiempo de cita. De esta manera las estructuras con menor tiempo de cita siempre están en la parte inferior de la pila y las que tienen mayor tiempo de cita en la superior.
Esta función es crucial para poder mostrar en la pantalla VM800 las citas en orden cronológico. Al inicio del programa se disponen las estructuras desordenadas para mostrar cómo actúa esta función.

Después de llamar a rankstack(), la función de manejo del Timer 0 mira si la estructura que está en la parte baja de la pila circular tiene un tiempo menor que el tiempo actual del programa. Si esto ocurre es porque la cita que estaba prevista ya ha debido pasar, con lo cual ya no es necesario mostrarla en pantalla, por lo tanto se aumenta la parte baja de la pila circular en 1, aumentando la variable bot.
Como apunte, cabe a destacar que la ventana visible que muestra la pantalla VM800 son las siete primeras estructuras empezando por la que está en la posición bot.

Una función también relevante para el manejo de la pila es pushstack(), una función vacía que añade una estructura a la parte superior de la pila para que más tarde sea ordenada por la función rankstack(). Pushstack() pone la estructura que se quiera añadir a la pila en la estructura número num variable que indica dónde está la cima de la pila.
----------------------- --------------------------------- -----------
<h2>Circular Buffer Handling</h2>

The creation of the circular buffer arises as a response to the need to store relevant vehicle data such as license plate, appointment time, whether the appointment is confirmed or not, and whether the journey will be uphill or downhill. For this purpose, a structure of two hundred components has been created to accommodate two hundred possible transits throughout the day, although we will see that they are more than necessary.

```C

struct navio{                           //Structure of the ships
    char matricula[7];
    int tcita;
    int updown;
    int greentick;
} barco[200] = {0};                     //We initialize what will be our circular buffer
```
To order the information stack, the rankstack() function is used, which is called every minute in the TIMER0 interrupt handling function, Timer0Handler(), which is used as a clock, updating the tsec and treal variables, corresponding to the seconds and minutes of the clock.

Rankstack() is an empty function that modifies the position of each component of the navio structure according to its appointment time. To do this, it checks if the appointment time of a structure that is in position N of the stack is less than the appointment time of its immediately lower counterpart, if so, it exchanges the data of the two structures, moving up the one with the higher appointment time and moving down the one with the lower appointment time. In this way, the structures with the shortest appointment time are always at the bottom of the stack and those with the longest appointment time at the top.

This function is crucial to be able to display the appointments on the VM800 screen in chronological order. At the beginning of the program, the structures are arranged in disorder to show how this function acts.

After calling rankstack(), the Timer 0 handling function checks if the structure that is at the bottom of the circular buffer has a time less than the current time of the program. If this happens, it is because the scheduled appointment should have already passed, so it is no longer necessary to display it on the screen, therefore the bottom of the circular buffer is increased by 1, increasing the bot variable.

As a note, it is worth noting that the visible window shown by the VM800 screen are the first seven structures starting from the one that is in the bot position.

A relevant function for stack management is also pushstack(), an empty function that adds a structure to the top of the stack so that it can later be ordered by the rankstack() function. Pushstack() puts the structure to be added to the stack in the structure number num variable that indicates where the top of the stack is.
