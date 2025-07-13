/******************************************************************************
 * @file    network_manager.h
 * @author  Thomas Zoldowski
 * @date    June 4, 2025
 * @brief   Declaration of Wi-Fi initialization and periodic sensor upload.
 ******************************************************************************/
#pragma once

#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

// Call once in setup() to initialize Wi-Fi and start the periodic upload.
void network_init();

void network_update();

#endif /* NETWORK_MANAGER_H */

