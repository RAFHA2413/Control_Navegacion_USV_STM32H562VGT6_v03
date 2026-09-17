/*
 * imu_bno085_i2c.c
 *
 * Driver minimo BNO085/BNO080 por I2C para STM32 HAL.
 *
 * Implementa:
 * - deteccion del IMU en 0x4A / 0x4B
 * - paquetes SHTP
 * - Rotation Vector (0x05)
 * - cuaternion Q14
 * - Roll, Pitch y Yaw en grados
 */

#include "imu_bno085_i2c.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

extern I2C_HandleTypeDef hi2c1;


/* ===============================================================
 * CONFIGURACION
 * ===============================================================
 */

#define BNO085_ADDR_4A_7BIT              0x4AU
#define BNO085_ADDR_4B_7BIT              0x4BU

/* STM32 HAL usa la direccion I2C desplazada un bit a la izquierda */
#define BNO085_ADDR_HAL(addr7)           ((uint16_t)((addr7) << 1U))

#define SHTP_CHANNEL_EXECUTABLE          1U
#define SHTP_CHANNEL_CONTROL             2U
#define SHTP_CHANNEL_REPORTS             3U

#define SHTP_REPORT_PRODUCT_ID_RESPONSE  0xF8U
#define SHTP_REPORT_PRODUCT_ID_REQUEST   0xF9U
#define SHTP_REPORT_BASE_TIMESTAMP       0xFBU
#define SHTP_REPORT_SET_FEATURE_COMMAND  0xFDU

#define REPORT_ID_ROTATION_VECTOR        0x05U

/* 50 000 us = 50 ms = 20 Hz */
#define IMU_REPORT_INTERVAL_US           50000UL

#define IMU_I2C_TIMEOUT_MS               200U

/*
 * Cada lectura I2C del BNO085 vuelve a entregar 4 bytes
 * de cabecera SHTP. Se leen bloques de 28 bytes de datos.
 */
#define IMU_I2C_DATA_CHUNK               28U

#define IMU_PAYLOAD_BUFFER_SIZE          128U

/* Proteccion contra una cabecera corrupta */
#define IMU_MAX_PACKET_LENGTH            1024U

#define IMU_MAX_PACKETS_PER_READ         6U


/* ===============================================================
 * VARIABLES INTERNAS
 * ===============================================================
 */

static uint16_t bno085_addr_hal = 0U;

static uint8_t shtp_sequence[6] =
{
    0U, 0U, 0U, 0U, 0U, 0U
};

static uint8_t shtp_payload[IMU_PAYLOAD_BUFFER_SIZE];

static uint8_t imu_initialized = 0U;


/* ===============================================================
 * FUNCIONES INTERNAS
 * ===============================================================
 */

/**
 * @brief Envia un paquete SHTP.
 */
static uint8_t IMU_SendPacket(
        uint8_t channel,
        const uint8_t *payload,
        uint16_t payload_length)
{
    uint8_t tx_buffer[4U + 32U];
    uint16_t packet_length;

    if ((bno085_addr_hal == 0U) ||
        (channel >= 6U) ||
        (payload == NULL) ||
        (payload_length > 32U))
    {
        return 0U;
    }

    packet_length = payload_length + 4U;

    tx_buffer[0] = (uint8_t)(packet_length & 0xFFU);
    tx_buffer[1] = (uint8_t)((packet_length >> 8U) & 0x7FU);
    tx_buffer[2] = channel;
    tx_buffer[3] = shtp_sequence[channel]++;

    memcpy(
        &tx_buffer[4],
        payload,
        payload_length);

    if (HAL_I2C_Master_Transmit(
            &hi2c1,
            bno085_addr_hal,
            tx_buffer,
            packet_length,
            IMU_I2C_TIMEOUT_MS) != HAL_OK)
    {
        return 0U;
    }

    return 1U;
}


/**
 * @brief Recibe un paquete SHTP completo.
 *
 * Primero lee la cabecera SHTP de 4 bytes.
 * Despues lee el payload por bloques.
 *
 * En cada nueva lectura I2C, el BNO085 repite los 4 bytes
 * de cabecera; por eso esos 4 bytes se descartan.
 */
static uint8_t IMU_ReceivePacket(
        uint8_t *channel,
        uint8_t *payload,
        uint16_t capacity,
        uint16_t *payload_length)
{
    uint8_t header[4];
    uint8_t rx_block[4U + IMU_I2C_DATA_CHUNK];

    uint16_t packet_length;
    uint16_t total_payload_length;
    uint16_t remaining;
    uint16_t chunk;
    uint16_t stored = 0U;
    uint16_t copy_length;

    if ((bno085_addr_hal == 0U) ||
        (channel == NULL) ||
        (payload_length == NULL))
    {
        return 0U;
    }

    if (HAL_I2C_Master_Receive(
            &hi2c1,
            bno085_addr_hal,
            header,
            sizeof(header),
            IMU_I2C_TIMEOUT_MS) != HAL_OK)
    {
        return 0U;
    }

    packet_length =
        ((uint16_t)header[1] << 8U) |
        (uint16_t)header[0];

    /*
     * Bit 15 indica continuacion de paquete.
     * Para esta aplicacion se limpia el bit.
     */
    packet_length &= 0x7FFFU;

    if ((packet_length < 4U) ||
        (packet_length > IMU_MAX_PACKET_LENGTH))
    {
        return 0U;
    }

    total_payload_length = packet_length - 4U;

    *channel = header[2];
    *payload_length = total_payload_length;

    if (total_payload_length == 0U)
    {
        return 1U;
    }

    remaining = total_payload_length;

    while (remaining > 0U)
    {
        chunk = remaining;

        if (chunk > IMU_I2C_DATA_CHUNK)
        {
            chunk = IMU_I2C_DATA_CHUNK;
        }

        if (HAL_I2C_Master_Receive(
                &hi2c1,
                bno085_addr_hal,
                rx_block,
                chunk + 4U,
                IMU_I2C_TIMEOUT_MS) != HAL_OK)
        {
            return 0U;
        }

        /*
         * rx_block[0..3] contiene la cabecera repetida.
         * El payload comienza en rx_block[4].
         */
        if ((payload != NULL) && (stored < capacity))
        {
            copy_length = chunk;

            if ((stored + copy_length) > capacity)
            {
                copy_length = capacity - stored;
            }

            memcpy(
                &payload[stored],
                &rx_block[4],
                copy_length);

            stored += copy_length;
        }

        remaining -= chunk;
    }

    return 1U;
}


/**
 * @brief Descarta paquetes pendientes.
 */
static void IMU_FlushPendingPackets(
        uint8_t max_packets)
{
    uint8_t channel;
    uint16_t payload_length;
    uint8_t counter;

    for (counter = 0U;
         counter < max_packets;
         counter++)
    {
        if (IMU_ReceivePacket(
                &channel,
                shtp_payload,
                sizeof(shtp_payload),
                &payload_length) == 0U)
        {
            break;
        }
    }
}


/**
 * @brief Soft reset por canal Executable.
 */
static uint8_t IMU_SoftReset(void)
{
    const uint8_t reset_command[1] =
    {
        0x01U
    };

    if (IMU_SendPacket(
            SHTP_CHANNEL_EXECUTABLE,
            reset_command,
            sizeof(reset_command)) == 0U)
    {
        return 0U;
    }

    /*
     * Despues del reset, el BNO08x genera varios
     * paquetes de arranque. Se vacian antes de seguir.
     */
    HAL_Delay(100U);
    IMU_FlushPendingPackets(12U);

    HAL_Delay(50U);
    IMU_FlushPendingPackets(12U);

    return 1U;
}


/**
 * @brief Solicita Product ID para verificar SHTP.
 */
static uint8_t IMU_CheckProductID(void)
{
    const uint8_t request[2] =
    {
        SHTP_REPORT_PRODUCT_ID_REQUEST,
        0x00U
    };

    uint8_t channel;
    uint16_t payload_length;
    uint8_t attempt;

    if (IMU_SendPacket(
            SHTP_CHANNEL_CONTROL,
            request,
            sizeof(request)) == 0U)
    {
        return 0U;
    }

    for (attempt = 0U;
         attempt < 20U;
         attempt++)
    {
        HAL_Delay(5U);

        if (IMU_ReceivePacket(
                &channel,
                shtp_payload,
                sizeof(shtp_payload),
                &payload_length) != 0U)
        {
            if ((channel == SHTP_CHANNEL_CONTROL) &&
                (payload_length >= 1U) &&
                (shtp_payload[0] ==
                 SHTP_REPORT_PRODUCT_ID_RESPONSE))
            {
                return 1U;
            }
        }
    }

    return 0U;
}


/**
 * @brief Habilita Rotation Vector a 20 Hz.
 */
static uint8_t IMU_EnableRotationVector(void)
{
    uint8_t payload[17];
    uint32_t interval_us =
        IMU_REPORT_INTERVAL_US;

    memset(
        payload,
        0,
        sizeof(payload));

    payload[0] =
        SHTP_REPORT_SET_FEATURE_COMMAND;

    payload[1] =
        REPORT_ID_ROTATION_VECTOR;

    /*
     * payload[2] = Feature flags = 0
     * payload[3..4] = Change sensitivity = 0
     */

    /*
     * Report interval en microsegundos,
     * little-endian.
     */
    payload[5] =
        (uint8_t)(
            interval_us & 0xFFU);

    payload[6] =
        (uint8_t)(
            (interval_us >> 8U) &
            0xFFU);

    payload[7] =
        (uint8_t)(
            (interval_us >> 16U) &
            0xFFU);

    payload[8] =
        (uint8_t)(
            (interval_us >> 24U) &
            0xFFU);

    /*
     * payload[9..12]  = Batch interval = 0
     * payload[13..16] = Sensor-specific config = 0
     */

    return IMU_SendPacket(
        SHTP_CHANNEL_CONTROL,
        payload,
        sizeof(payload));
}


/**
 * @brief Convierte un entero little-endian de 16 bits.
 */
static int16_t IMU_ReadInt16LE(
        const uint8_t *data)
{
    uint16_t value;

    value =
        (uint16_t)data[0] |
        ((uint16_t)data[1] << 8U);

    return (int16_t)value;
}


/**
 * @brief Cuaternion -> Roll, Pitch, Yaw en grados.
 */
static uint8_t IMU_QuaternionToEuler(
        float qi,
        float qj,
        float qk,
        float qr,
        float *roll,
        float *pitch,
        float *yaw)
{
    float norm;
    float sin_pitch;

    if ((roll == NULL) ||
        (pitch == NULL) ||
        (yaw == NULL))
    {
        return 0U;
    }

    norm = sqrtf(
        (qi * qi) +
        (qj * qj) +
        (qk * qk) +
        (qr * qr));

    if (norm < 0.0001f)
    {
        return 0U;
    }

    qi /= norm;
    qj /= norm;
    qk /= norm;
    qr /= norm;

    *roll = atan2f(
        2.0f *
        ((qr * qi) + (qj * qk)),
        1.0f -
        2.0f *
        ((qi * qi) + (qj * qj)));

    sin_pitch =
        2.0f *
        ((qr * qj) - (qk * qi));

    if (sin_pitch > 1.0f)
    {
        sin_pitch = 1.0f;
    }
    else if (sin_pitch < -1.0f)
    {
        sin_pitch = -1.0f;
    }

    *pitch =
        asinf(sin_pitch);

    *yaw = atan2f(
        2.0f *
        ((qr * qk) + (qi * qj)),
        1.0f -
        2.0f *
        ((qj * qj) + (qk * qk)));

    /* Radianes -> grados */
    *roll  *= 57.2957795f;
    *pitch *= 57.2957795f;
    *yaw   *= 57.2957795f;

    return 1U;
}


/* ===============================================================
 * API PUBLICA
 * ===============================================================
 */

uint8_t IMU_Init(void)
{
    imu_initialized = 0U;
    bno085_addr_hal = 0U;

    memset(
        shtp_sequence,
        0,
        sizeof(shtp_sequence));

    /*
     * Detecta automaticamente 0x4A o 0x4B.
     */
    if (HAL_I2C_IsDeviceReady(
            &hi2c1,
            BNO085_ADDR_HAL(
                BNO085_ADDR_4A_7BIT),
            3U,
            100U) == HAL_OK)
    {
        bno085_addr_hal =
            BNO085_ADDR_HAL(
                BNO085_ADDR_4A_7BIT);
    }
    else if (HAL_I2C_IsDeviceReady(
                 &hi2c1,
                 BNO085_ADDR_HAL(
                     BNO085_ADDR_4B_7BIT),
                 3U,
                 100U) == HAL_OK)
    {
        bno085_addr_hal =
            BNO085_ADDR_HAL(
                BNO085_ADDR_4B_7BIT);
    }
    else
    {
        return 0U;
    }

    if (IMU_SoftReset() == 0U)
    {
        return 0U;
    }

    if (IMU_CheckProductID() == 0U)
    {
        return 0U;
    }

    if (IMU_EnableRotationVector() == 0U)
    {
        return 0U;
    }

    HAL_Delay(100U);

    imu_initialized = 1U;

    return 1U;
}


uint8_t IMU_ReadEuler(
        float *roll,
        float *pitch,
        float *yaw)
{
    uint8_t channel;
    uint16_t payload_length;
    uint8_t packet_attempt;

    int16_t raw_qi;
    int16_t raw_qj;
    int16_t raw_qk;
    int16_t raw_qr;

    float qi;
    float qj;
    float qk;
    float qr;

    const float q14_scale =
        1.0f / 16384.0f;

    if ((imu_initialized == 0U) ||
        (roll == NULL) ||
        (pitch == NULL) ||
        (yaw == NULL))
    {
        return 0U;
    }

    /*
     * Puede haber respuestas de control pendientes.
     * Se revisan varios paquetes hasta encontrar
     * Rotation Vector.
     */
    for (packet_attempt = 0U;
         packet_attempt <
             IMU_MAX_PACKETS_PER_READ;
         packet_attempt++)
    {
        if (IMU_ReceivePacket(
                &channel,
                shtp_payload,
                sizeof(shtp_payload),
                &payload_length) == 0U)
        {
            return 0U;
        }

        if (channel !=
            SHTP_CHANNEL_REPORTS)
        {
            continue;
        }

        /*
         * Para qReal se necesitan bytes
         * payload[0..16].
         */
        if ((payload_length < 17U) ||
            (payload_length >
             sizeof(shtp_payload)))
        {
            continue;
        }

        /*
         * Estructura:
         *
         * payload[0]     = 0xFB Base Timestamp
         * payload[1..4]  = timestamp
         * payload[5]     = 0x05 Rotation Vector
         * payload[6]     = sequence
         * payload[7]     = status
         * payload[8]     = delay
         * payload[9..10] = Quaternion I
         * payload[11..12]= Quaternion J
         * payload[13..14]= Quaternion K
         * payload[15..16]= Quaternion Real
         */
        if ((shtp_payload[0] !=
             SHTP_REPORT_BASE_TIMESTAMP) ||
            (shtp_payload[5] !=
             REPORT_ID_ROTATION_VECTOR))
        {
            continue;
        }

        raw_qi =
            IMU_ReadInt16LE(
                &shtp_payload[9]);

        raw_qj =
            IMU_ReadInt16LE(
                &shtp_payload[11]);

        raw_qk =
            IMU_ReadInt16LE(
                &shtp_payload[13]);

        raw_qr =
            IMU_ReadInt16LE(
                &shtp_payload[15]);

        /*
         * Rotation Vector utiliza Q14.
         */
        qi =
            (float)raw_qi *
            q14_scale;

        qj =
            (float)raw_qj *
            q14_scale;

        qk =
            (float)raw_qk *
            q14_scale;

        qr =
            (float)raw_qr *
            q14_scale;

        return IMU_QuaternionToEuler(
            qi,
            qj,
            qk,
            qr,
            roll,
            pitch,
            yaw);
    }

    return 0U;
}


uint8_t IMU_GetAddress7bit(void)
{
    if (bno085_addr_hal == 0U)
    {
        return 0U;
    }

    return (uint8_t)(
        bno085_addr_hal >> 1U);
}
