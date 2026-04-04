/**
 * @file drv8704_regs_typedefs.h
 * @brief Typed register views for the DRV8704 register set.
 */

#ifndef DRV8704_REGS_TYPEDEFS_H
#define DRV8704_REGS_TYPEDEFS_H

#include <stdint.h>

/**
 * @brief CTRL register (0x00) view.
 */
typedef union {
  uint16_t all;
  struct {
    uint16_t enbl : 1;
    uint16_t reserved0 : 7;
    uint16_t isgain : 2;
    uint16_t dtime : 2;
    uint16_t reserved1 : 4;
  } bit;
} drv8704_ctrl_reg_t;

/**
 * @brief TORQUE register (0x01) view.
 */
typedef union {
  uint16_t all;
  struct {
    uint16_t torque : 8;
    uint16_t reserved : 8;
  } bit;
} drv8704_torque_reg_t;

/**
 * @brief OFF register (0x02) view.
 */
typedef union {
  uint16_t all;
  struct {
    uint16_t toff : 8;
    uint16_t pwmmode : 1;
    uint16_t reserved : 7;
  } bit;
} drv8704_off_reg_t;

/**
 * @brief BLANK register (0x03) view.
 */
typedef union {
  uint16_t all;
  struct {
    uint16_t tblank : 8;
    uint16_t reserved : 8;
  } bit;
} drv8704_blank_reg_t;

/**
 * @brief DECAY register (0x04) view.
 */
typedef union {
  uint16_t all;
  struct {
    uint16_t tdecay : 8;
    uint16_t decmod : 3;
    uint16_t reserved : 5;
  } bit;
} drv8704_decay_reg_t;

/**
 * @brief DRIVE register (0x06) view.
 */
typedef union {
  uint16_t all;
  struct {
    uint16_t ocpth : 2;
    uint16_t ocpdeg : 2;
    uint16_t tdriven : 2;
    uint16_t tdrivep : 2;
    uint16_t idriven : 2;
    uint16_t idrivep : 2;
    uint16_t reserved : 4;
  } bit;
} drv8704_drive_reg_t;

/**
 * @brief STATUS register (0x07) view.
 */
typedef union {
  uint16_t all;
  struct {
    uint16_t ots : 1;
    uint16_t aocp : 1;
    uint16_t bocp : 1;
    uint16_t apdf : 1;
    uint16_t bpdf : 1;
    uint16_t uvlo : 1;
    uint16_t reserved : 10;
  } bit;
} drv8704_status_reg_t;

#endif // DRV8704_REGS_TYPEDEFS_H

