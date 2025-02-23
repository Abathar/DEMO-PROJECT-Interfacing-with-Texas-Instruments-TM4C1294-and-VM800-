# DEMO-PROJECT-Interfacing-with-Texas-Instruments-TM4C1294-and-VM800-

### This is a project to get a deepply understanding of the microcontroler TM4C1294 from Texas Instruments with the digital screen VM800.

Se plantea el siguiente problema: Resolver el conflictivo flujo de tráfico de un canal marítimo en el que sólo se pueda cruzar en un sentido y no haya espacio para que las embarcaciones crucen en ambos sentidos a la vez, ante lo que se propone como solución la implantación de dos terminales al inicio  y final del estrechamiento que registre el paso de vehículos y muestre en la pantalla VM800 las embarcaciones que esperan a pasar, bien sea que se encuentren en el canal o se encuentren fuera pero hayan reservado con la compañía un viaje a una hora determinada.

Para explicar el proyecto de manera clara se ha decidido subdividir el mismo en cuarto partes principales que abarcan funciones dentro del propio proyecto, manejo de la pantalla VM800, manejo de la pila circular, manejo de las comunicaciones vía UART y manejo de la EEPROM. Se anexa un diagrama de flujo para saber los estados del código y archivos de las diferentes partes.

------------------------------------------------------------------------------------------------------------------------------------------

The following problem is posed: Resolve the conflicting traffic flow of a maritime canal where passage is only possible in one direction and there is no space for vessels to cross in both directions simultaneously. The proposed solution is the implementation of two terminals at the beginning and end of the narrowing, which will record the passage of vehicles and display on the VM800 screen the vessels waiting to pass, whether they are in the canal or are outside but have booked a trip with the company at a specific time.

To explain the project clearly, it has been decided to subdivide it into four main parts that cover functions within the project itself: handling the VM800 screen, handling the circular buffer, handling UART communications, and handling EEPROM. A flow diagram is attached to show the code states and files of the different parts.
