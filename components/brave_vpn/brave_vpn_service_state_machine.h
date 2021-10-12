/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_STATE_MACHINE_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_STATE_MACHINE_H_

#include "brave/components/brave_vpn/brave_vpn.mojom.h"

using ConnectionState = brave_vpn::mojom::ConnectionState;
using PurchasedState = brave_vpn::mojom::PurchasedState;

class BraveVPNServiceStateMachine {
 public:
  BraveVPNServiceStateMachine();
  ~BraveVPNServiceStateMachine();

  BraveVPNServiceStateMachine(const BraveVPNServiceStateMachine&) = delete;
  BraveVPNServiceStateMachine& operator=(const BraveVPNServiceStateMachine&) =
      delete;

  bool CanConnect() const;
  bool CanCreateConnection() const;

  void set_has_region_list(bool has_region_list) {
    has_region_list_ = has_region_list;
  }
  bool has_region_list() const { return has_region_list_; }
  void set_has_hostname(bool has_hostname) { has_hostname_ = has_hostname; }
  bool has_hostname() const { return has_hostname_; }

  void set_needs_connect(bool needs_connect) { needs_connect_ = needs_connect; }
  bool needs_connect() const { return needs_connect_; }

  void set_has_selected_region(bool has_selected_region) {
    has_selected_region_ = has_selected_region;
  }
  bool has_selected_region() const { return has_selected_region_; }

  void set_has_connection(bool has_connection) {
    has_connection_ = has_connection;
  }
  bool has_connection() const { return has_connection_; }

  PurchasedState purchased_state() const { return purchased_state_; }
  void set_purchased_state(PurchasedState state) { purchased_state_ = state; }
  bool is_purchased_state() const {
    return purchased_state_ == PurchasedState::PURCHASED;
  }

  void set_connection_state(ConnectionState state) {
    connection_state_ = state;
  }
  bool is_connected() const {
    return connection_state_ == ConnectionState::CONNECTED;
  }
  bool is_connecting() const {
    return connection_state_ == ConnectionState::CONNECTING;
  }
  bool is_connect_failed() const {
    return connection_state_ == ConnectionState::CONNECT_FAILED;
  }
  bool is_disconnected() const {
    return connection_state_ == ConnectionState::DISCONNECTED;
  }
  bool is_disconnecting() const {
    return connection_state_ == ConnectionState::DISCONNECTING;
  }
  bool can_disconnect() const { return is_connected() || is_connecting(); }
  ConnectionState connection_state() const { return connection_state_; }

 private:
  bool has_region_list_ = false;
  bool has_hostname_ = false;
  bool has_selected_region_ = false;
  bool needs_connect_ = false;
  bool has_connection_ = false;
  PurchasedState purchased_state_ = PurchasedState::NOT_PURCHASED;
  ConnectionState connection_state_ = ConnectionState::DISCONNECTED;
};

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_SERVICE_STATE_MACHINE_H_
