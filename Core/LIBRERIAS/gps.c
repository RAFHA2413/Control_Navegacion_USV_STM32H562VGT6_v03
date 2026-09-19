/*
 * GPS.c
 *
 *  Created on: Apr 19, 2025
 *      Author: ALCIDES_RAMOS
 */


#include "GPS.h"
#include "UARTRX.h"

char GPS_buffer[600];  //tamaño buffer para  caprota d edatos gps
#define  hor_utc -5  // define la hora de colombia utc -5


//#define trama_gps  GPS_UARTRX  //define la variable asociada al puero serial
#define trama_gps GPS_UARTRX

double latitud, longitud,velocidad;
uint8_t min_gps,seg_gps,dia_gps,mes_gps,an_gps;
int8_t hor_gps;//
float gps_vel_nudos,gps_vel_kph,gps_rumbo,gps_desv_mag;
float ANG_SERVO;

//gga
int8_t gps_modo,gps_satelites;
float gps_hor_dilu,gps_altura;



uint8_t GPS_RMC()
{
	uint8_t captura[100];
	uint8_t info[20];
	float grados,minutos;
	//punteros para detectar inicia y fin d ela trama RMC
	const char *start;
    const char *end;
	int8_t diamas=0;// ajusta el dia al UTC
//             limpia buffer
         	 memset(GPS_buffer,0,sizeof(GPS_buffer)); // ESTO ES PARA LIMPIAR EL BUFFER ANTES DE CAPTURAR LA TRAMA RMC

		        size_t length; // ESTO ES PARA CALCULAR LA LONGITUD DE LA TRAMA RMC
		           // Encontrar el primer "$GPRMC"
		           start = strstr(trama_gps.trama_rx, "$GPRMC");
		           if (start != NULL) // ESTO ES PARA VERIFICAR SI SE ENCONTRO LA TRAMA RMC
		           {
		               // el enter o final d ela trama RMC
		               end = strstr(start, "\r"); // ESTO FUNCIONA PARA ENCONTRAR EL FINAL DE LA TRAMA RMC POR EJMPLO: $GPRMC,123456.00,A,1234.5678,N,12345.6789,W,1.23,4.56,010203,,,A*1A\r
		               // el enter o final d ela trama RMC
		               end = strstr(start, "\r");
						   if (end != NULL)
						   {
							   // Calcular la longitud de la sentencia
							   length = end - start;
							   // Copiar la sentencia completa al buffer de salida
							   strncpy(GPS_buffer, start, length);
							   GPS_buffer[length] = '\0'; // Añadir  cero al final PARA QUE NO HAYA PROBLEMAS CON LA FUNCION STRTOK
				           }
		           }
		        //busca primero si es valido el dato PARA VERIFICAR SI EL DATO ES VALIDO SE USA LA TERCERA COMA DE LA TRAMA RMC
		           strcpy(captura, strtok(GPS_buffer, ","));  //inicia captura de tokens
		           strcpy(captura, strtok(0, ","));  //captura
		           strcpy(captura, strtok(0, ","));  //captura
		           if (captura[0]!=65) return(0);//  si el dato no es valido sale

		       		//si es valido vuelve a capurar
		          //arma el buffer de nuevo
		          strncpy(GPS_buffer, start, length); // ESTO PARA QUE SE PUEDA USAR LA FUNCION STRTOK PARA CAPTURAR LOS DATOS DE LA TRAMA RMC
        		   GPS_buffer[length] = '\0'; // Añadir  cero al final 

        		   strcpy(captura, strtok(GPS_buffer, ","));  //inicia captura de tokens
	     		 strcpy(captura, strtok(0, ","));  //captura

		     		 //comienza la decodificacion
		       		strncpy(info,captura,2);//captura la hora 2 ES PARA CAPTURAR LA HORA EN FORMATO HHMMSS.SS
 	                 hor_gps=atoi(info);
		                hor_gps = hor_gps + hor_utc;

		               if (hor_gps < 0) {
		            	   diamas=-1;  //  es un dia antes al UTC
		            	   hor_gps += 24;// si es negatriva sumo 24
		               } else if (hor_gps >= 24) // si pasa de 24 le retso 24
		               {
		            	   diamas=+1;//  es el dia siguiente
		            	   hor_gps -= 24;
		               }

		            strncpy(info,&captura[2],2);//captura min
    		        min_gps=atoi(info);
    		        strncpy(info,&captura[4],2);//captura seg
    		        seg_gps=atoi(info);

    		        // ya se sabe que es valido solo  que toca capturar de nuevo
    		        strcpy(captura, strtok(0, ","));  //captura hasta 3 coma

    		        //      captura la latitud
    		            strcpy(captura, strtok(0, ","));  //captura hasta 4 coma
		     		    memset(info,0,sizeof(info));//limpía los grados
    		               strncpy(info,captura,2);//captura los grados strncpy(info,captura,2) ESTA ESTRUCTURA ES PARA CAPTURAR LOS DOS PRIMEROS CARACTERES DE LA CADENA DE LA LATITUD QUE SON LOS GRADOS

    		               grados=atof(info);      // pasa de alfanumerico o cadena a flotante

    		               // apunta a los minutos &captura[2] ESTO ES PARA CAPTURAR LOS MINUTOS DE LA LATITUD QUE SON LOS CARACTERES 3 Y 4 DE LA CADENA DE LA LATITUD
    		                 minutos=atoff(&captura[2])/60.0;  //lo pasa a grados PARA QUE LOS MINUTOS SEAN CONVERTIDOS A GRADOS SE DIVIDE ENTRE 60.0

    		                 latitud=grados+minutos;
    		                 strcpy(captura, strtok(0, ","));  //captura siguiente coma  //orientacion o signo de la lat
    		                 if (captura[0]=='S') latitud=-latitud;


    		                 //captura longitud
    		                 strcpy(captura, strtok(0, ","));  //captura siguiente coma la longitud
    		                  memset(info,0,sizeof(info));//limpía los grados
    		                   strncpy(info,captura,3);//captura los grados  3 posiciones
    		                    grados=atof(info);
    		                     // apunta a los minutos
    		                      minutos=atof(&captura[3])/60.0;  //lo pasa a grados
    		                       longitud=grados+minutos;
    		                        strcpy(captura, strtok(0, ","));  //captura siguiente coma  //orientacion o signo de la long
    		                        if (captura[0]=='W') longitud=-longitud;

    		           //captura velocidad

    		             strcpy(captura, strtok(0, ","));  //captura siguenti coma
    		             gps_vel_nudos=atof(captura);
    		             gps_vel_kph = gps_vel_nudos * 1.852;

    		             //captura rumbo
    		             strcpy(captura, strtok(0, ","));  //captura siguenti coma
    		             gps_rumbo=atof(captura);


    		           //captura dia mes año
    		            memset(info,0,sizeof(info));//limpía el  buffer
    		            strcpy(captura, strtok(0, ","));  //
                       strncpy(info,captura,2);//captura la dia
                      dia_gps=atoi(info)+diamas;
                     strncpy(info,&captura[2],2);//captura mes
                      mes_gps=atoi(info);
                      strncpy(info,&captura[4],2);//captura año
                      an_gps=atoi(info);

                      //captura deviacion magnetica
     		             strcpy(captura, strtok(0, ","));  //captura siguenti coma
     		            gps_desv_mag=atof(captura);

                         strcpy(captura, strtok(0, ","));  //captura siguiente coma  //orientacion o signo de la long
                        if (captura[0]=='W')  gps_desv_mag=- gps_desv_mag;




                     return(1);


}

uint8_t GPS_GGA()
{
	uint8_t captura[100];
	uint8_t info[20];
	float grados,minutos;
	//punteros para detectar inicia y fin d ela trama RMC
		const char *start;
	    const char *end;
		int8_t diamas=0;// ajusta el dia al UTC
	//             limpia buffer
	         	 memset(GPS_buffer,0,sizeof(GPS_buffer));

			        size_t length;
			           // Encontrar el primer "$GPGGA"
			           start = strstr(trama_gps.trama_rx, "$GPGGA");
			           if (start != NULL)
			           {
			               // el enter o final d ela trama RMC
			               end = strstr(start, "\r");
							   if (end != NULL)
							   {
								   // Calcular la longitud de la sentencia
								   length = end - start;
								   // Copiar la sentencia completa al buffer de salida
								   strncpy(GPS_buffer, start, length);
								   GPS_buffer[length] = '\0'; // Añadir  cero al final

							   }
			           }

                      //PROCESA LA TRAMA GGA

			           //busca primero si es valido el dato
			          strcpy(captura, strtok(GPS_buffer, ","));  //inicia captura de tokens
			          strcpy(captura, strtok(0, ","));  //captura
			          strcpy(captura, strtok(0, ","));  //captura
			          strcpy(captura, strtok(0, ","));  //captura
			      	  strcpy(captura, strtok(0, ","));  //captura
			      	  strcpy(captura, strtok(0, ","));
			      	  strcpy(captura, strtok(0, ","));
			          if (captura[0]=='0') return(0);//  si el dato no es valido sale
                       gps_modo=atoi(captura);

			          //si es valido vuelve a capurar
        		          //arma el buffer de nuevo
        		          strncpy(GPS_buffer, start, length);
                 		   GPS_buffer[length] = '\0'; // Añadir  cero al final
                        //   uartx_write_text(&huart2, GPS_buffer);

                           //inica captora datos validos

                           //hora
                		   strcpy(captura, strtok(GPS_buffer, ","));  //inicia captura de tokens
        	     		    strcpy(captura, strtok(0, ","));  //captura

        		     		 //comienza la decodificacion
        		       		strncpy(info,captura,2);//captura la hora
         	                 hor_gps=atoi(info);
        		                hor_gps = hor_gps + hor_utc;

        		               if (hor_gps < 0) {
        		            	   diamas=-1;  //  es un dia antes al UTC
        		            	   hor_gps += 24;// si es negatriva sumo 24
        		               } else if (hor_gps >= 24) // si pasa de 24 le retso 24
        		               {
        		            	   diamas=+1;//  es el dia siguiente
        		            	   hor_gps -= 24;
        		               }

        		            strncpy(info,&captura[2],2);//captura min
            		        min_gps=atoi(info);
            		        strncpy(info,&captura[4],2);//captura seg
            		        seg_gps=atoi(info);

            		        //      captura la latitud
            		            strcpy(captura, strtok(0, ","));  //captura hasta 4 coma
        		     		    memset(info,0,sizeof(info));//limpía los grados
            		               strncpy(info,captura,2);//captura los grados
                          //   uartx_write_text(&huart2, info);
            		               grados=atof(info);      // pasa de alfanumerico o cadena a flotante

            		               // apunta a los minutos
            		                 minutos=atoff(&captura[2])/60.0;  //lo pasa a grados

            		                 latitud=grados+minutos;
            		                 strcpy(captura, strtok(0, ","));  //captura siguiente coma  //orientacion o signo de la lat
            		                 if (captura[0]=='S') latitud=-latitud;


            		                 //captura longitud
            		                 strcpy(captura, strtok(0, ","));  //captura siguiente coma la longitud
            		                  memset(info,0,sizeof(info));//limpía los grados
            		                   strncpy(info,captura,3);//captura los grados  3 posiciones
            		                    grados=atof(info);
            		                     // apunta a los minutos
            		                      minutos=atof(&captura[3])/60.0;  //lo pasa a grados
            		                       longitud=grados+minutos;
            		                        strcpy(captura, strtok(0, ","));  //captura siguiente coma  //orientacion o signo de la long
            		                        if (captura[0]=='W') longitud=-longitud;

            		                        strcpy(captura, strtok(0, ","));
            		                         gps_modo=atoi(captura);// calidas gps

             		                        strcpy(captura, strtok(0, ","));
             		                         gps_satelites=atoi(captura);// calidas gps

              		                        strcpy(captura, strtok(0, ","));
              		                         gps_hor_dilu=atof(captura);// calidas gps

               		                        strcpy(captura, strtok(0, ","));
               		                         gps_altura=atof(captura);// calidas gps
                		                      //si la unidad es Kilometros
               		                         strcpy(captura, strtok(0, ","));  //captura siguiente coma  //orientacion o signo de la long

               		                         if (captura[0]=='K')  gps_altura=1000*gps_altura;


                           return(1);
}

uint8_t PUSVU()
{
	uint8_t captura[100];
	uint8_t info[20];
	
	//punteros para detectar inicia y fin d ela trama PUSVU
	const char *start;
    const char *end;
	int8_t diamas=0;// ajusta el dia al UTC
//             limpia buffer
         	 memset(GPS_buffer,0,sizeof(GPS_buffer)); // ESTO ES PARA LIMPIAR EL BUFFER ANTES DE CAPTURAR LA TRAMA RMC

		        size_t length; // ESTO ES PARA CALCULAR LA LONGITUD DE LA TRAMA PUSVU
		           // Encontrar el primer "$PUSVU"
		           start = strstr(trama_gps.trama_rx, "$PUSVU");
		           if (start != NULL) // ESTO ES PARA VERIFICAR SI SE ENCONTRO LA TRAMA PUSVU
		           {
		               // el enter o final d ela trama PUSVU
		               end = strstr(start, "\r"); // ESTO FUNCIONA PARA ENCONTRAR EL FINAL DE LA TRAMA PUSVU POR EJMPLO: $PUSVU,123456.00,A,1234.5678,N,12345.6789,W,1.23,4.56,010203,,,A*1A\r
		               // el enter o final d ela trama PUSVU
		               //end = strstr(start, "\r");
						   if (end != NULL)
						   {
							   // Calcular la longitud de la sentencia
							   length = end - start;
							   // Copiar la sentencia completa al buffer de salida
							   strncpy(GPS_buffer, start, length);
							   GPS_buffer[length] = '\0'; // Añadir  cero al final PARA QUE NO HAYA PROBLEMAS CON LA FUNCION STRTOK
				           }
		           }
				   // $PUSVU,558,2,-89,0,2,-87,0,0,0,0,0,0,0,1,0,0*AB
		        //busca primero si es valido el dato PARA VERIFICAR SI EL DATO ES VALIDO SE USA LA TERCERA COMA DE LA TRAMA PUSVU
		           strcpy(captura, strtok(GPS_buffer, ","));  //inicia captura de tokens
		           strcpy(captura, strtok(0, ","));  //captura
				   	luces=atoi(captura);
					
		           strcpy(captura, strtok(0, ","));  //captura
				   strcpy(captura, strtok(0, ","));  //captura
				   strcpy(captura, strtok(0, ","));  //captura
				   strcpy(captura, strtok(0, ","));  //captura
				   strcpy(captura, strtok(0, ","));  //captura
						ANG_SERVO=atof(captura);
				   
		           /* if (captura[0]!=65) return(0);//  si el dato no es valido sale

		       		//si es valido vuelve a capurar
		          //arma el buffer de nuevo
		          strncpy(GPS_buffer, start, length); // ESTO PARA QUE SE PUEDA USAR LA FUNCION STRTOK PARA CAPTURAR LOS DATOS DE LA TRAMA RMC
        		   GPS_buffer[length] = '\0'; // Añadir  cero al final 

        		   strcpy(captura, strtok(GPS_buffer, ","));  //inicia captura de tokens
	     		 strcpy(captura, strtok(0, ","));  //captura

		     		 //comienza la decodificacion
		       		strncpy(info,captura,2);//captura la hora 2 ES PARA CAPTURAR LA HORA EN FORMATO HHMMSS.SS
 	                 hor_gps=atoi(info);
		                hor_gps = hor_gps + hor_utc;

		               if (hor_gps < 0) {
		            	   diamas=-1;  //  es un dia antes al UTC
		            	   hor_gps += 24;// si es negatriva sumo 24
		               } else if (hor_gps >= 24) // si pasa de 24 le retso 24
		               {
		            	   diamas=+1;//  es el dia siguiente
		            	   hor_gps -= 24;
		               }

		            strncpy(info,&captura[2],2);//captura min
    		        min_gps=atoi(info);
    		        strncpy(info,&captura[4],2);//captura seg
    		        seg_gps=atoi(info);

    		        // ya se sabe que es valido solo  que toca capturar de nuevo
    		        strcpy(captura, strtok(0, ","));  //captura hasta 3 coma

    		        //      captura la latitud
    		            strcpy(captura, strtok(0, ","));  //captura hasta 4 coma
		     		    memset(info,0,sizeof(info));//limpía los grados
    		               strncpy(info,captura,2);//captura los grados strncpy(info,captura,2) ESTA ESTRUCTURA ES PARA CAPTURAR LOS DOS PRIMEROS CARACTERES DE LA CADENA DE LA LATITUD QUE SON LOS GRADOS

    		               grados=atof(info);      // pasa de alfanumerico o cadena a flotante

    		               // apunta a los minutos &captura[2] ESTO ES PARA CAPTURAR LOS MINUTOS DE LA LATITUD QUE SON LOS CARACTERES 3 Y 4 DE LA CADENA DE LA LATITUD
    		                 minutos=atoff(&captura[2])/60.0;  //lo pasa a grados PARA QUE LOS MINUTOS SEAN CONVERTIDOS A GRADOS SE DIVIDE ENTRE 60.0

    		                 latitud=grados+minutos;
    		                 strcpy(captura, strtok(0, ","));  //captura siguiente coma  //orientacion o signo de la lat
    		                 if (captura[0]=='S') latitud=-latitud;


    		                 //captura longitud
    		                 strcpy(captura, strtok(0, ","));  //captura siguiente coma la longitud
    		                  memset(info,0,sizeof(info));//limpía los grados
    		                   strncpy(info,captura,3);//captura los grados  3 posiciones
    		                    grados=atof(info);
    		                     // apunta a los minutos
    		                      minutos=atof(&captura[3])/60.0;  //lo pasa a grados
    		                       longitud=grados+minutos;
    		                        strcpy(captura, strtok(0, ","));  //captura siguiente coma  //orientacion o signo de la long
    		                        if (captura[0]=='W') longitud=-longitud;

    		           //captura velocidad

    		             strcpy(captura, strtok(0, ","));  //captura siguenti coma
    		             gps_vel_nudos=atof(captura);
    		             gps_vel_kph = gps_vel_nudos * 1.852;

    		             //captura rumbo
    		             strcpy(captura, strtok(0, ","));  //captura siguenti coma
    		             gps_rumbo=atof(captura);


    		           //captura dia mes año
    		            memset(info,0,sizeof(info));//limpía el  buffer
    		            strcpy(captura, strtok(0, ","));  //
                       strncpy(info,captura,2);//captura la dia
                      dia_gps=atoi(info)+diamas;
                     strncpy(info,&captura[2],2);//captura mes
                      mes_gps=atoi(info);
                      strncpy(info,&captura[4],2);//captura año
                      an_gps=atoi(info);

                      //captura deviacion magnetica
     		             strcpy(captura, strtok(0, ","));  //captura siguenti coma
     		            gps_desv_mag=atof(captura);

                         strcpy(captura, strtok(0, ","));  //captura siguiente coma  //orientacion o signo de la long
                        if (captura[0]=='W')  gps_desv_mag=- gps_desv_mag;
 */



                     return(1);


}




void GPS_Interrupt( UART_HandleTypeDef *huart,uint16_t sizex)
{
	if ((GPS_UARTRX .flag_rx==0)&& (huart->Instance == GPS_UARTRX .usart_instance))//si es el uart de datos
			{
			HAL_UART_DMAStop(GPS_UARTRX .huart);  //para la recepcion temporarmente
			GPS_UARTRX .num_datos=sizex;
			GPS_UARTRX .flag_rx=1;
			 }


}
