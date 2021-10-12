/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/brave_vpn_service_state_machine.h"

#include "base/logging.h"
#include "base/notreached.h"

BraveVPNServiceStateMachine::BraveVPNServiceStateMachine() = default;
BraveVPNServiceStateMachine::~BraveVPNServiceStateMachine() = default;

bool BraveVPNServiceStateMachine::CanCreateConnection() const {
  return is_purchased_state() && has_hostname_ && has_selected_region_ &&
         has_connection_;
}

bool BraveVPNServiceStateMachine::CanConnect() const {
  return is_purchased_state() && has_hostname_ && has_selected_region_ &&
         has_connection_;
}
