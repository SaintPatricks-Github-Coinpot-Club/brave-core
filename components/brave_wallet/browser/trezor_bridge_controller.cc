/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/trezor_bridge_controller.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/gtest_prod_util.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/trezor_bridge/browser/mojo_trezor_web_ui_controller.h"
#include "brave/components/trezor_bridge/browser/trezor_bridge_content_observer.h"
#include "brave/components/trezor_bridge/browser/trezor_content_proxy.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

namespace brave_wallet {

TrezorBridgeController::TrezorBridgeController(
    std::unique_ptr<TrezorContentProxy> content_proxy)
    : content_proxy_(std::move(content_proxy)) {
  content_proxy_->SetObserver(this);
}

TrezorBridgeController::~TrezorBridgeController() = default;

mojo::PendingRemote<mojom::TrezorBridgeController>
TrezorBridgeController::MakeRemote() {
  mojo::PendingRemote<mojom::TrezorBridgeController> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void TrezorBridgeController::Bind(
    mojo::PendingReceiver<mojom::TrezorBridgeController> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void TrezorBridgeController::BridgeFail() {
  unlocked_ = false;
  if (unlock_callback_)
    std::move(unlock_callback_).Run(false, "todo add brige fail message");
}

void TrezorBridgeController::BridgeReady() {
  library_controller_ =
      content_proxy_->ConnectWithWebUIBridge(weak_ptr_factory_.GetWeakPtr());
  DCHECK(library_controller_);
  if (!library_controller_) {
    LOG(ERROR) << "LibraryController has not been created yet, disconnecting";
    BridgeFail();
    return;
  }
  library_controller_->Unlock();
}

bool TrezorBridgeController::IsUnlocked() const {
  return unlocked_;
}

void TrezorBridgeController::Unlock(UnlockCallback callback) {
  if (unlock_callback_) {
    LOG(ERROR) << "Unlock is already in progress";
    std::move(callback).Run(false, "add unlock is already in progress");
    return;
  }
  if (IsUnlocked()) {
    std::move(callback).Run(true, std::string());
    return;
  }
  unlock_callback_ = std::move(callback);

  if (!content_proxy_->IsReady()) {
    content_proxy_->InitWebContents();
    // Unlock will be called in BridgeReady/BridgeFail
    // when the content will be ready for messages
    return;
  }

  library_controller_->Unlock();
}

void TrezorBridgeController::GetTrezorAccounts(
    const std::vector<std::string>& paths,
    GetTrezorAccountsCallback callback) {
  if (!IsUnlocked() || !content_proxy_->IsReady() || !library_controller_ ||
      get_trezor_accounts_callback_) {
    std::move(callback).Run(false, {},
                            "to do add messagse to unlock or initialize");
    return;
  }

  get_trezor_accounts_callback_ = std::move(callback);
  library_controller_->RequestAddresses(std::move(paths));
}

void TrezorBridgeController::OnAddressesReceived(
    bool success,
    std::vector<trezor_bridge::mojom::HardwareWalletAccountPtr> accounts,
    const std::string& error) {
  std::vector<mojom::HardwareWalletAccountPtr> results;
  for (const auto& it : accounts) {
    results.push_back(mojom::HardwareWalletAccount::New(
        it->address, it->derivation_path, it->name, it->hardware_vendor));
  }
  DLOG(INFO) << "OnAddressesReceived:" << success << " error:" << error;
  std::move(get_trezor_accounts_callback_)
      .Run(success, std::move(results), error);
}

void TrezorBridgeController::OnUnlocked(bool success,
                                        const std::string& error) {
  DLOG(INFO) << "OnUnlocked:" << success << " error:" << error;
  unlocked_ = success;
  if (unlock_callback_)
    std::move(unlock_callback_).Run(success, error);
}

}  // namespace brave_wallet
