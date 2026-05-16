#ifndef RECEIVER_BOARD_PINS_H
#define RECEIVER_BOARD_PINS_H

#include "stc8h_sfr.h"

#define TOY_REMOTE_NRF24_CSN_MASK 0x04u
#define TOY_REMOTE_NRF24_CE_MASK 0x40u
#define TOY_REMOTE_NRF24_IRQ_MASK 0x04u

#define DRV_NRF24L01_CSN_HIGH() do { P1 |= TOY_REMOTE_NRF24_CSN_MASK; } while (0)
#define DRV_NRF24L01_CSN_LOW() do { P1 &= (stc8h_u8)~TOY_REMOTE_NRF24_CSN_MASK; } while (0)
#define DRV_NRF24L01_CE_HIGH() do { P1 |= TOY_REMOTE_NRF24_CE_MASK; } while (0)
#define DRV_NRF24L01_CE_LOW() do { P1 &= (stc8h_u8)~TOY_REMOTE_NRF24_CE_MASK; } while (0)
#define DRV_NRF24L01_IRQ_READ() ((P3 & TOY_REMOTE_NRF24_IRQ_MASK) ? 1u : 0u)

#endif
