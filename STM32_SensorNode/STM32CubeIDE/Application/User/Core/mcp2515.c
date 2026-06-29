#include "mcp2515.h"

#define MCP_RESET       0xC0
#define MCP_READ        0x03
#define MCP_WRITE       0x02
#define MCP_BITMOD      0x05
#define MCP_RTS_TX0     0x81

#define CANCTRL         0x0F

#define CNF3            0x28
#define CNF2            0x29
#define CNF1            0x2A

#define CANINTE         0x2B
#define CANINTF         0x2C
#define EFLG            0x2D

#define TXB0CTRL        0x30
#define TXB0SIDH        0x31
#define TXB0SIDL        0x32
#define TXB0EID8        0x33
#define TXB0EID0        0x34
#define TXB0DLC         0x35
#define TXB0D0          0x36

#define RXB0CTRL        0x60
#define RXB1CTRL        0x70

void MCP2515_Reset(void)
{
    uint8_t cmd = MCP_RESET;

    MCP2515_CS_HIGH();
    HAL_Delay(1);

    MCP2515_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
    MCP2515_CS_HIGH();

    HAL_Delay(20);
}

uint8_t MCP2515_Read(uint8_t address)
{
    uint8_t tx[3] = {MCP_READ, address, 0x00};
    uint8_t rx[3] = {0};

    MCP2515_CS_LOW();
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 3, HAL_MAX_DELAY);
    MCP2515_CS_HIGH();

    return rx[2];
}

void MCP2515_Write(uint8_t address, uint8_t data)
{
    uint8_t tx[3] = {MCP_WRITE, address, data};

    MCP2515_CS_LOW();
    HAL_SPI_Transmit(&hspi1, tx, 3, HAL_MAX_DELAY);
    MCP2515_CS_HIGH();
}

void MCP2515_BitModify(uint8_t address, uint8_t mask, uint8_t data)
{
    uint8_t tx[4] = {MCP_BITMOD, address, mask, data};

    MCP2515_CS_LOW();
    HAL_SPI_Transmit(&hspi1, tx, 4, HAL_MAX_DELAY);
    MCP2515_CS_HIGH();
}

void MCP2515_Init(void)
{
    MCP2515_Reset();

    MCP2515_Write(CANCTRL, 0x80);
    HAL_Delay(10);

    /* 125 kbps @ 8 MHz */
    MCP2515_Write(CNF1, 0x01);
    MCP2515_Write(CNF2, 0xB1);
    MCP2515_Write(CNF3, 0x05);

    MCP2515_Write(RXB0CTRL, 0x60);
    MCP2515_Write(RXB1CTRL, 0x60);

    MCP2515_Write(CANINTE, 0x00);
    MCP2515_Write(CANINTF, 0x00);
    MCP2515_Write(EFLG, 0x00);

    MCP2515_Write(CANCTRL, 0x00);
    HAL_Delay(10);
}

uint8_t MCP2515_SendMessage(uint16_t id, uint8_t *data, uint8_t len)
{
    if (len > 8)
    {
        len = 8;
    }

    uint32_t timeout = HAL_GetTick();

    while (MCP2515_Read(TXB0CTRL) & 0x08)
    {
        if (HAL_GetTick() - timeout > 100)
        {
            MCP2515_BitModify(TXB0CTRL, 0x08, 0x00);
            return 0;
        }
    }

    MCP2515_Write(TXB0CTRL, 0x00);

    MCP2515_Write(TXB0SIDH, (uint8_t)(id >> 3));
    MCP2515_Write(TXB0SIDL, (uint8_t)(id << 5));
    MCP2515_Write(TXB0EID8, 0x00);
    MCP2515_Write(TXB0EID0, 0x00);

    MCP2515_Write(TXB0DLC, len);

    for (uint8_t i = 0; i < len; i++)
    {
        MCP2515_Write(TXB0D0 + i, data[i]);
    }

    uint8_t rts = MCP_RTS_TX0;

    MCP2515_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &rts, 1, HAL_MAX_DELAY);
    MCP2515_CS_HIGH();

    timeout = HAL_GetTick();

    while (MCP2515_Read(TXB0CTRL) & 0x08)
    {
        if (HAL_GetTick() - timeout > 100)
        {
            MCP2515_BitModify(TXB0CTRL, 0x08, 0x00);
            return 0;
        }
    }

    if (MCP2515_Read(TXB0CTRL) & 0x10)
    {
        MCP2515_BitModify(TXB0CTRL, 0x10, 0x00);
        return 0;
    }

    return 1;
}
