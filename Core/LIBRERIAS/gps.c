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



/*
 * Copia una sentencia NMEA completa desde el buffer del L76K.
 * Acepta talker GN (multi-GNSS) y GP (GPS) sin afectar $PUSVU,
 * que continua usando GPS_UARTRX / USART1.
 */
static uint8_t GPS_CopiarSentenciaGNSS(
        const char *tipo_gn,
        const char *tipo_gp)
{
    const char *start;
    const char *end;
    size_t length;

    if (GNSS_UARTRX.trama_rx == NULL)
    {
        return 0U;
    }

    start = strstr((const char *)GNSS_UARTRX.trama_rx, tipo_gn);
    if (start == NULL)
    {
        start = strstr((const char *)GNSS_UARTRX.trama_rx, tipo_gp);
    }

    if (start == NULL)
    {
        return 0U;
    }

    end = strpbrk(start, "\r\n");
    if (end == NULL)
    {
        return 0U;
    }

    length = (size_t)(end - start);
    if ((length == 0U) || (length >= sizeof(GPS_buffer)))
    {
        return 0U;
    }

    memcpy(GPS_buffer, start, length);
    GPS_buffer[length] = '\0';

    return 1U;
}

/*
 * Separa campos NMEA conservando campos vacios.
 * strtok() no es adecuado aqui porque colapsa comas consecutivas.
 */
static uint8_t GPS_SepararCampos(
        char *trama,
        char **campos,
        uint8_t capacidad)
{
    uint8_t cantidad = 0U;
    char *p;

    if ((trama == NULL) || (campos == NULL) || (capacidad == 0U))
    {
        return 0U;
    }

    campos[cantidad++] = trama;

    for (p = trama; (*p != '\0') && (cantidad < capacidad); p++)
    {
        if (*p == ',')
        {
            *p = '\0';
            campos[cantidad++] = p + 1;
        }
    }

    return cantidad;
}

static uint8_t GPS_LeerDosDigitos(
        const char *texto,
        uint8_t posicion,
        uint8_t *valor)
{
    char dato[3] = {0};

    if ((texto == NULL) ||
        (valor == NULL) ||
        (strlen(texto) < ((size_t)posicion + 2U)))
    {
        return 0U;
    }

    dato[0] = texto[posicion];
    dato[1] = texto[posicion + 1U];
    *valor = (uint8_t)atoi(dato);

    return 1U;
}

static uint8_t GPS_ConvertirCoordenada(
        const char *campo,
        char hemisferio,
        double *resultado)
{
    double valor;
    int grados;
    double minutos;

    if ((campo == NULL) || (resultado == NULL) || (campo[0] == '\0'))
    {
        return 0U;
    }

    valor = atof(campo);
    grados = (int)(valor / 100.0);
    minutos = valor - ((double)grados * 100.0);

    *resultado = (double)grados + (minutos / 60.0);

    if ((hemisferio == 'S') || (hemisferio == 'W'))
    {
        *resultado = -*resultado;
    }

    return 1U;
}

static void GPS_ActualizarHoraLocal(const char *utc, int8_t *ajuste_dia)
{
    uint8_t hora_utc = 0U;
    uint8_t minuto = 0U;
    uint8_t segundo = 0U;
    int16_t hora_local;

    if (ajuste_dia != NULL)
    {
        *ajuste_dia = 0;
    }

    if ((GPS_LeerDosDigitos(utc, 0U, &hora_utc) == 0U) ||
        (GPS_LeerDosDigitos(utc, 2U, &minuto) == 0U) ||
        (GPS_LeerDosDigitos(utc, 4U, &segundo) == 0U))
    {
        return;
    }

    hora_local = (int16_t)hora_utc + hor_utc;

    if (hora_local < 0)
    {
        hora_local += 24;
        if (ajuste_dia != NULL)
        {
            *ajuste_dia = -1;
        }
    }
    else if (hora_local >= 24)
    {
        hora_local -= 24;
        if (ajuste_dia != NULL)
        {
            *ajuste_dia = 1;
        }
    }

    hor_gps = (int8_t)hora_local;
    min_gps = minuto;
    seg_gps = segundo;
}

uint8_t GPS_RMC()
{
    char *campos[16];
    uint8_t cantidad;
    int8_t ajuste_dia = 0;
    uint8_t dia = 0U;
    uint8_t mes = 0U;
    uint8_t anio = 0U;
    double latitud_nueva;
    double longitud_nueva;

    memset(GPS_buffer, 0, sizeof(GPS_buffer));

    if (GPS_CopiarSentenciaGNSS("$GNRMC", "$GPRMC") == 0U)
    {
        return 0U;
    }

    cantidad = GPS_SepararCampos(GPS_buffer, campos, 16U);

    /* RMC: tipo, UTC, estado, lat, N/S, lon, E/W, vel, rumbo, fecha... */
    if ((cantidad < 10U) || (campos[2][0] != 'A'))
    {
        return 0U;
    }

    if ((GPS_ConvertirCoordenada(campos[3], campos[4][0], &latitud_nueva) == 0U) ||
        (GPS_ConvertirCoordenada(campos[5], campos[6][0], &longitud_nueva) == 0U))
    {
        return 0U;
    }

    GPS_ActualizarHoraLocal(campos[1], &ajuste_dia);

    latitud = latitud_nueva;
    longitud = longitud_nueva;

    gps_vel_nudos = (float)atof(campos[7]);
    gps_vel_kph = gps_vel_nudos * 1.852f;
    gps_rumbo = (float)atof(campos[8]);

    if ((GPS_LeerDosDigitos(campos[9], 0U, &dia) != 0U) &&
        (GPS_LeerDosDigitos(campos[9], 2U, &mes) != 0U) &&
        (GPS_LeerDosDigitos(campos[9], 4U, &anio) != 0U))
    {
        dia_gps = (uint8_t)((int16_t)dia + ajuste_dia);
        mes_gps = mes;
        an_gps = anio;
    }

    gps_desv_mag = 0.0f;
    if ((cantidad > 10U) && (campos[10][0] != '\0'))
    {
        gps_desv_mag = (float)atof(campos[10]);

        if ((cantidad > 11U) && (campos[11][0] == 'W'))
        {
            gps_desv_mag = -gps_desv_mag;
        }
    }

    return 1U;
}

uint8_t GPS_GGA()
{
    char *campos[16];
    uint8_t cantidad;
    int8_t ajuste_dia = 0;
    double latitud_nueva;
    double longitud_nueva;

    memset(GPS_buffer, 0, sizeof(GPS_buffer));

    if (GPS_CopiarSentenciaGNSS("$GNGGA", "$GPGGA") == 0U)
    {
        return 0U;
    }

    cantidad = GPS_SepararCampos(GPS_buffer, campos, 16U);

    /* GGA: tipo, UTC, lat, N/S, lon, E/W, calidad, sat, HDOP, altitud... */
    if (cantidad < 10U)
    {
        return 0U;
    }

    GPS_ActualizarHoraLocal(campos[1], &ajuste_dia);
    (void)ajuste_dia;

    gps_modo = (int8_t)atoi(campos[6]);
    gps_satelites = (int8_t)atoi(campos[7]);
    gps_hor_dilu = (float)atof(campos[8]);
    gps_altura = (float)atof(campos[9]);

    if ((cantidad > 10U) && (campos[10][0] == 'K'))
    {
        gps_altura *= 1000.0f;
    }

    /* Calidad 0 = sin solucion de posicion. Se conservan satelites/HDOP. */
    if (gps_modo == 0)
    {
        return 0U;
    }

    if ((GPS_ConvertirCoordenada(campos[2], campos[3][0], &latitud_nueva) == 0U) ||
        (GPS_ConvertirCoordenada(campos[4], campos[5][0], &longitud_nueva) == 0U))
    {
        return 0U;
    }

    latitud = latitud_nueva;
    longitud = longitud_nueva;

    return 1U;
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
    		                 minutos=atof(&captura[2])/60.0;  //lo pasa a grados PARA QUE LOS MINUTOS SEAN CONVERTIDOS A GRADOS SE DIVIDE ENTRE 60.0

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




void GPS_Interrupt(UART_HandleTypeDef *huart, uint16_t sizex)
{
    /* USART1 / XBee / trama $PUSVU desde tierra. */
    if ((GPS_UARTRX.flag_rx == 0) &&
        (huart->Instance == GPS_UARTRX.usart_instance))
    {
        HAL_UART_DMAStop(GPS_UARTRX.huart);
        GPS_UARTRX.num_datos = sizex;
        GPS_UARTRX.flag_rx = 1;
    }

    /* USART2 / L76K / sentencias NMEA. */
    if ((GNSS_UARTRX.flag_rx == 0) &&
        (huart->Instance == GNSS_UARTRX.usart_instance))
    {
        HAL_UART_DMAStop(GNSS_UARTRX.huart);
        GNSS_UARTRX.num_datos = sizex;
        GNSS_UARTRX.flag_rx = 1;
    }
}
