#ifndef PCB_BANCO_H_
#define PCB_BANCO_H_
#include "main.h"
/* Modo exclusivamente de banco. No fusionar con firmware de navegacion. */
#define PCB_BANCO_ACTIVO 1U
void PCB_Banco_Iniciar(void);
void PCB_Banco_Tarea(void);
uint8_t PCB_Banco_Armado(void);
uint8_t PCB_Banco_MotorActivo(void);
uint8_t PCB_Banco_BombaActiva(void);
uint8_t PCB_Banco_SalidaActiva(void);
#endif
