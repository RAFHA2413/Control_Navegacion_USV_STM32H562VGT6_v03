/*
 * USV 0067 A193: banco de pruebas integrado por USART6 (PC6 TX, PC7 RX).
 * No sustituye un interruptor fisico de emergencia para los ESC.
 */
#include "pcb_banco.h"
#include "servos.h"
#include "usv_motores.h"
#include "uart.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define FIFO_SIZE   128U
#define LINE_SIZE    64U
#define ARM_MS    30000U
#define MOTOR_MS    700U
#define PUMP_MS     200U
#define DO_MS       200U
extern UART_HandleTypeDef huart6;
extern TIM_HandleTypeDef htim3;
extern SERVOS SERVO1;
extern uint16_t motor_babor_pwm_us, motor_estribor_pwm_us;
extern int16_t motor_babor_aplicado_pct, motor_estribor_aplicado_pct;
extern uint8_t motores_pwm_activos, bomba_test_estado;
extern float servo_test_angulo;

static uint8_t rx_byte;
static volatile uint8_t fifo[FIFO_SIZE];
static volatile uint16_t head, tail;
static volatile uint8_t rx_fault;
static char line[LINE_SIZE];
static uint8_t line_len;
static uint8_t armed, motor_on, pump_on, do_on;
static uint32_t arm_deadline, motor_deadline, pump_deadline, do_deadline;

static uint8_t Expired(uint32_t now, uint32_t deadline)
{ return (int32_t)(now - deadline) >= 0 ? 1U : 0U; }

static void Msg(const char *msg)
{ uartx_write_text(&huart6, (char *)msg); }

static void MotorsOff(void)
{
    motor_babor_pwm_us = PWM_NEUTRO;
    motor_estribor_pwm_us = PWM_NEUTRO;
    motor_babor_aplicado_pct = 0;
    motor_estribor_aplicado_pct = 0;
    motor_on = 0U;
    if (motores_pwm_activos)
    {
        USV_Motor_Set(&htim3, TIM_CHANNEL_1, PWM_NEUTRO);
        USV_Motor_Set(&htim3, TIM_CHANNEL_4, PWM_NEUTRO);
    }
}
static void PumpOff(void)
{
    HAL_GPIO_WritePin(ACHIQUE_CTRL_GPIO_Port, ACHIQUE_CTRL_Pin, GPIO_PIN_SET);
    bomba_test_estado = 0U;
    pump_on = 0U;
}
static void DoOff(void)
{
    HAL_GPIO_WritePin(GPIOB, DO1_PB12_Pin | DO2_PB13_Pin |
                     DO3_PB14_Pin | DO4_PB15_Pin, GPIO_PIN_RESET);
    do_on = 0U;
}
static void Stop(void)
{
    MotorsOff();
    PumpOff();
    DoOff();
    SERVO_ANG(&SERVO1, 0.0f);
    servo_test_angulo = 0.0f;
    armed = 0U;
}
static void Reject(const char *msg) { Stop(); Msg(msg); }

static void Command(char *s)
{
    int n = 0, channel = 0;
    char side[12] = {0}, excess = 0;
    uint32_t now = HAL_GetTick();
    if (strcmp(s, "PCB STOP") == 0)
    { Stop(); Msg("PCB STOP OK\r\n"); return; }
    if (strcmp(s, "PCB HELP") == 0)
    {
        Msg("PCB ARM | PCB STOP | PCB STATUS\r\n"
            "PCB SERVO -30..30 | PCB MOTOR B/E/BOTH -15..15\r\n"
            "PCB PUMP 1..200 | PCB DO1..DO4 1..200\r\n");
        return;
    }
    if (strcmp(s, "PCB STATUS") == 0)
    {
        char reply[120];
        (void)snprintf(reply, sizeof(reply),
            "PCB arm=%u motor=%u bomba=%u do=%u\r\n",
            (unsigned int)armed, (unsigned int)motor_on,
            (unsigned int)pump_on, (unsigned int)do_on);
        Msg(reply);
        return;
    }
    if (strcmp(s, "PCB ARM") == 0)
    {
        Stop(); /* Nunca prolonga un pulso anterior. */
        armed = 1U;
        arm_deadline = now + ARM_MS;
        Msg("PCB ARM OK, caduca en 30s.\r\n");
        return;
    }
    if (!armed)
    { Reject("PCB ERROR: primero PCB ARM.\r\n"); return; }

    if (sscanf(s, "PCB SERVO %d %c", &n, &excess) == 1)
    {
        if (n < -30 || n > 30)
        { Reject("PCB SERVO fuera de -30..30.\r\n"); return; }
        servo_test_angulo = (float)n;
        SERVO_ANG(&SERVO1, servo_test_angulo);
        Msg("PCB SERVO OK\r\n");
        return;
    }
    if (sscanf(s, "PCB MOTOR %11s %d %c", side, &n, &excess) == 2)
    {
        uint8_t b = (strcmp(side, "B") == 0 || strcmp(side, "BOTH") == 0);
        uint8_t e = (strcmp(side, "E") == 0 || strcmp(side, "BOTH") == 0);
        uint16_t pwm;
        if ((!b && !e) || n < -15 || n > 15 || !motores_pwm_activos)
        { Reject("PCB MOTOR invalido. B/E/BOTH -15..15.\r\n"); return; }
        MotorsOff();
        pwm = (uint16_t)(PWM_NEUTRO + n * 5);
        if (b)
        {
            motor_babor_pwm_us = pwm;
            motor_babor_aplicado_pct = (int16_t)n;
            USV_Motor_Set(&htim3, TIM_CHANNEL_1, pwm);
        }
        if (e)
        {
            motor_estribor_pwm_us = pwm;
            motor_estribor_aplicado_pct = (int16_t)n;
            USV_Motor_Set(&htim3, TIM_CHANNEL_4, pwm);
        }
        motor_on = 1U;
        motor_deadline = HAL_GetTick() + MOTOR_MS;
        Msg("PCB MOTOR pulso <=700 ms, retorno NEUTRO.\r\n");
        return;
    }
    if (sscanf(s, "PCB PUMP %d %c", &n, &excess) == 1)
    {
        if (n < 1 || n > (int)PUMP_MS)
        { Reject("PCB PUMP: 1..200ms.\r\n"); return; }
        PumpOff();
        HAL_GPIO_WritePin(ACHIQUE_CTRL_GPIO_Port, ACHIQUE_CTRL_Pin,
                          GPIO_PIN_RESET);
        bomba_test_estado = 1U;
        pump_on = 1U;
        pump_deadline = HAL_GetTick() + (uint32_t)n;
        Msg("PCB PUMP pulso activo; solo sumergida.\r\n");
        return;
    }
    if (sscanf(s, "PCB DO%d %d %c", &channel, &n, &excess) == 2)
    {
        uint16_t pin;
        if (channel < 1 || channel > 4 || n < 1 || n > (int)DO_MS)
        { Reject("PCB DO1..DO4: 1..200ms.\r\n"); return; }
        pin = channel == 1 ? DO1_PB12_Pin :
              channel == 2 ? DO2_PB13_Pin :
              channel == 3 ? DO3_PB14_Pin : DO4_PB15_Pin;
        DoOff();
        HAL_GPIO_WritePin(GPIOB, pin, GPIO_PIN_SET);
        do_on = (uint8_t)channel;
        do_deadline = HAL_GetTick() + (uint32_t)n;
        Msg("PCB DO pulso HIGH.\r\n");
        return;
    }
    Reject("PCB COMANDO INVALIDO: DESARMADO. PCB HELP.\r\n");
}

void PCB_Banco_Iniciar(void)
{
    Stop();
    head = tail = 0U;
    rx_fault = line_len = 0U;
    if (HAL_UART_Receive_IT(&huart6, &rx_byte, 1U) == HAL_OK)
        Msg("PCB BANCO LISTO: PCB HELP.\r\n");
    else
        Msg("PCB ERROR: USART6 RX no inicio.\r\n");
}
void PCB_Banco_Tarea(void)
{
    uint32_t now = HAL_GetTick();
    if (rx_fault)
    {
        rx_fault = 0U;
        tail = head;
        line_len = 0U;
        Reject("PCB UART RX ERROR: DESARMADO.\r\n");
    }
    if (armed && Expired(now, arm_deadline))
    { Stop(); Msg("PCB ARM TIMEOUT: DESARMADO.\r\n"); }
    if (motor_on && Expired(now, motor_deadline)) MotorsOff();
    if (pump_on && Expired(now, pump_deadline)) PumpOff();
    if (do_on && Expired(now, do_deadline)) DoOff();

    while (tail != head)
    {
        uint8_t ch = fifo[tail];
        tail = (uint16_t)((tail + 1U) % FIFO_SIZE);
        if (ch == '\r' || ch == '\n')
        {
            if (line_len)
            {
                line[line_len] = '\0';
                Command(line);
                line_len = 0U;
            }
        }
        else if (ch >= 32U && ch <= 126U)
        {
            if (line_len < LINE_SIZE - 1U) line[line_len++] = (char)ch;
            else { line_len = 0U; Reject("PCB LINE TOO LONG: DESARMADO.\r\n"); }
        }
    }
}
/* USART6 usa RX IT sin DMA; USART1/2 conservan su DMA previo. */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart6)
    {
        uint16_t next = (uint16_t)((head + 1U) % FIFO_SIZE);
        if (next == tail) rx_fault = 1U;
        else { fifo[head] = rx_byte; head = next; }
        if (HAL_UART_Receive_IT(&huart6, &rx_byte, 1U) != HAL_OK)
            rx_fault = 1U;
    }
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart6)
    {
        rx_fault = 1U;
        (void)HAL_UART_Receive_IT(&huart6, &rx_byte, 1U);
    }
}
uint8_t PCB_Banco_Armado(void) { return armed; }
uint8_t PCB_Banco_MotorActivo(void) { return motor_on; }
uint8_t PCB_Banco_BombaActiva(void) { return pump_on; }
uint8_t PCB_Banco_SalidaActiva(void) { return do_on; }
