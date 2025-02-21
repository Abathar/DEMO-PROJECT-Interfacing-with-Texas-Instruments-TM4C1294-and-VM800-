<h2>Manejo de las comunicaciones vía UART.</h2>
Este apartado se subdivide en dos; una parte relacionada con el envío de datos por parte del microcontrolador y otra con la recepción de datos por parte del archivo de MATLAB.

Con respecto al envío de datos desde el microcontrolador se resume en la configuración de la UART en el main:
```c
sprintf(string,"%s %d\n",barco[189].matricula, barco[189].tcita);
UARTprintf(string);
```
Que rellena una cadena de caracteres con los datos que queremos transmitir (en este caso la matrícula y el tiempo de cita) y los envía a la UART.

Con respecto a la recepción del paquete de datos se ha creado un pequeño programa de Matlab que lee constantemente por UART y grafica los datos que va recibiendo además de los datos con los que se inicia la pila circular.
Como leyenda pone la matrícula y dispone la cita en un gráfico de barras en una disposición lineal a lo largo de las 24 horas del día.
Se dispone a continuación el código del archivo de Matlab ya que es muy autoexplicativo:
---------------------------- ----------------------------------------------- ---------------------------------------------------------------------------------------------------------------
<h2>Handling UART Communications</h2>
This section is subdivided into two parts; one related to sending data by the microcontroller and another with receiving data by the MATLAB file.

Regarding sending data from the microcontroller, it is summarized in the configuration of the UART in the main:
```C
sprintf(string,"%s %d\n",barco[189].matricula, barco[189].tcita);
UARTprintf(string);
```
That fills a string with the data we want to transmit (in this case the license plate and the appointment time) and sends it to the UART.

Regarding the reception of the data packet, a small Matlab program has been created that constantly reads by UART and graphs the data it is receiving in addition to the data with which the circular buffer is initialized.

As a legend it puts the license plate and arranges the appointment in a bar graph in a linear arrangement along the 24 hours of the day.
The code of the Matlab file is shown below since it is very self-explanatory:
```matlab
%   Archivo de Matlab que lee los datos por UART, los muestra en una 
%   gráfica y los guarda en un archivo de texto.
 
puerto=serial('COM5');
puerto.BaudRate=115200;
contador=0;
pause('on');
%>>>>>>>>>>>>>> LECTURA DE DATOS YA EXISTENTES EN DATOS.TXT <<<<<<<<<<<<<<<<
fileID = fopen('Datos.txt','r');
Readformat = ['%d' '%c' '%c' '%c' '%d']; %Formato de lectura: Número de Matrícula/Letra/Letra/Letra/tiempo de cita
sizeA=[5 Inf];
A = fscanf(fileID,Readformat,sizeA);
fclose(fileID);
nmat=7;                        %Número inicial de matrículas.
 
%>>>>>>>>>>>>>> PREPLOT DE DATOS YA EXISTENTE EN DATOS.TXT <<<<<<<<<<<<<
nN = 72;
N = 1:nN;
tcita = zeros(1,73);
t = 0:72;
 
 
 for i = 1:nmat
      numeromat{i} = num2str(N(i),'N=%-d');
 end
  for i = 1:nmat
      C=[A(2,i) A(3,i) A(4,i)];
      letras=char(C);                                       %Construye una cadena con las Letras de la matrícula
      numeromat{i} = cellstr(num2str(A(1,i),'%-d'));     %Pasa a string los números de la matrícula
      final(i)=strcat(numeromat{i},letras);              %Forma la matrícula al completo
      tcita(floor(A(5,i)/20))=10;
      bar(t,tcita,'stacked');
      legend(final);
      hold on
      pause(0.5)
  end
% bar(t,tcita,'stacked');
% legend(final);
title('Citas de Matrículas');
set(gca,'XTick',[0 3 6 9 12 15 18 21 24 27 30 33 36 39 42 45 48 51 54 57 60 63 66 69 72]);
set(gca,'XTickLabel',str2mat('00:00','01:00','02:00','03:00','04:00','05:00','06:00','07:00','08:00','09:00','10:00','11:00','12:00','13:00','14:00','15:00','16:00','17:00','18:00','19:00','20:00','21:00','22:00','23:00','24:00'));
xlim([-2 76]);
ylim([0 20]);
ylabel('Citas confirmadas')
drawnow
%Bucle de lectura de datos
 
 
fopen(puerto);
viejodato=[A(1,1); A(2,1); A(3,1); A(4,1); A(5,1)];
datovacio=[];
while contador<=72              %Las citas sólo se pueden hacer cada 20 minutos:24*60/20=72 citas posibles en total
    Readformat = ['%d' '%c' '%c' '%c' '%d'];
    nuevodato=fscanf(puerto,Readformat)'; %Toma el valor recibido por el puerto y lo guarda en la variable
    if isequal(datovacio,nuevodato) == 1 
        pause(1);
    elseif isequal(viejodato,nuevodato) == 0  %Si lo leido vía UART es diferente a lo anteriormente leído
         C=[nuevodato(1,2) nuevodato(1,3) nuevodato(1,4)];
         letras=char(C);
         nmatricula = cellstr(num2str(nuevodato(1,1),'%-d'));
         final(contador+7)=strcat(nmatricula,letras);
         tcita(floor(nuevodato(1,5)/20))=10;
         hold on
         bar(t,tcita,'r');
         legend(final);
         
         %>>>>>Parte del bucle que escribe las matrículas en el TxT.<<<<<
         fileID = fopen('Datos.txt','a');
         fprintf(fileID,'%d%c%c%c %d\r\n', nuevodato(1,1),nuevodato(1,2), nuevodato(1,3), nuevodato(1,4), nuevodato(1,5));
         fclose(fileID);
         
         
         %>>>>Parte del bucle en el que se actualizan las variables.<<<<<
         contador=contador+1;
         viejodato=nuevodato;
         
     end
    
   pause(1);
```
end
