/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/simulation_request_helper.h"

#include <optional>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/check.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/browser/solana_transaction.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/solana_utils.h"

namespace brave_wallet {

namespace {

base::Value::Dict GetMetadata(const mojom::OriginInfoPtr& origin_info) {
  base::Value::Dict metadata_object;

  if (origin_info && origin_info->origin_spec != "chrome://wallet" &&
      origin_info->origin_spec != "brave://wallet") {
    metadata_object.Set("origin", origin_info->origin_spec);
  } else {
    // TODO(onyb): We use "https://brave.com" as the default origin for now.
    //  This is because Blowfish doesn't support "chrome://wallet" and
    //  "brave://wallet" as origins yet. We'll update this once Blowfish
    //  supports them.
    metadata_object.Set("origin", "https://brave.com");
  }

  return metadata_object;
}

}  // namespace

namespace evm {

std::optional<std::string> EncodeScanTransactionParams(
    const mojom::TransactionInfo& tx_info,
    const std::string& from_address) {
  base::Value::Dict tx_object;
  tx_object.Set("from", from_address);

  if (tx_info.tx_data_union->is_eth_tx_data_1559()) {
    const auto& tx_data = tx_info.tx_data_union->get_eth_tx_data_1559();

    // TODO(onyb): Convert hex to wei string
    //
    // Ethereum JSON-RPC API requires value to be in wei formatted as a string.
    // Using a 0x-prefixed hex string also works, but is neither documented in
    // the ETH JSON-RPC spec nor in Blowfish docs. Hex strings are widely used
    // in our codebase, so we'll use them here for consistency. This comment
    // applies to all other places where we use hex strings.
    //
    // Here's an unoptimised implementation for future reference:
    //
    // std::optional<std::string> HexToWei(const std::string& value) {
    //   uint256_t uint256_value;
    //   if (!HexValueToUint256(value, &uint256_value)) {
    //     return std::nullopt;
    //   }
    //
    //   std::string result;
    //   while (uint256_value > 0) {
    //     uint64_t digit = uint256_value % 10;
    //     result = base::NumberToString(digit) + result;
    //     uint256_value /= 10;
    //   }
    //
    //   return result;
    // }
    tx_object.Set("value", tx_data->base_data->value);

    // Set to address to null value in case of contract deployment.
    if (tx_data->base_data->to == "0x") {
      tx_object.Set("to", base::Value());
    } else {
      tx_object.Set("to", tx_data->base_data->to);
    }

    if (tx_data->base_data->data.empty()) {
      tx_object.Set("data", "0x");
    } else {
      tx_object.Set("data", ToHex(tx_data->base_data->data));
    }
  } else if (tx_info.tx_data_union->is_eth_tx_data()) {
    const auto& tx_data = tx_info.tx_data_union->get_eth_tx_data();

    tx_object.Set("value", tx_data->value);

    // Set to address to null value in case of contract deployment.
    if (tx_data->to == "0x") {
      tx_object.Set("to", base::Value());
    } else {
      tx_object.Set("to", tx_data->to);
    }

    if (tx_data->data.empty()) {
      tx_object.Set("data", "0x");
    } else {
      tx_object.Set("data", ToHex(tx_data->data));
    }
  } else {
    return std::nullopt;
  }

  base::Value::Dict params;

  base::Value::List tx_objects;
  tx_objects.Append(std::move(tx_object));
  params.Set("txObjects", std::move(tx_objects));
  params.Set("metadata", GetMetadata(tx_info.origin_info));
  params.Set("userAccount", from_address);

  return GetJSON(base::Value(std::move(params)));
}

}  // namespace evm

namespace solana {

namespace {

std::optional<std::string> GetBase64TransactionFromSolanaTxData(
    mojom::SolanaTxDataPtr solana_tx_data) {
  auto tx = SolanaTransaction::FromSolanaTxData(std::move(solana_tx_data));

  auto message_signers_pair = tx->GetSerializedMessage();
  if (!message_signers_pair) {
    return std::nullopt;
  }

  auto& message_bytes = message_signers_pair->first;
  auto& signers = message_signers_pair->second;

  // The Solana runtime verifies that the number of signatures matches the
  // number in the first 8 bits of the message header. We therefore check
  // if the number of signers is a valid unsigned 8-bit integer.
  if (signers.size() > UINT8_MAX) {
    return std::nullopt;
  }

  // transaction_bytes is a compact-array of signatures, followed by the
  // message bytes. Since the transactions are not signed yet, we use 64
  // empty bytes for each signature.
  std::vector<uint8_t> transaction_bytes;
  CompactU16Encode(signers.size(), &transaction_bytes);
  transaction_bytes.insert(transaction_bytes.end(),
                           kSolanaSignatureSize * signers.size(), 0);
  transaction_bytes.insert(transaction_bytes.end(), message_bytes.begin(),
                           message_bytes.end());

  return base::Base64Encode(transaction_bytes);
}

}  // namespace

bool HasEmptyRecentBlockhash(
    const mojom::SignSolTransactionsRequest& sign_sol_transactions_request) {
  for (auto& solana_tx_data : sign_sol_transactions_request.tx_datas) {
    if (solana_tx_data->recent_blockhash.empty()) {
      return true;
    }
  }

  return false;
}

bool HasEmptyRecentBlockhash(const mojom::TransactionInfo& tx_info) {
  CHECK(tx_info.tx_data_union->is_solana_tx_data());

  return tx_info.tx_data_union->get_solana_tx_data()->recent_blockhash.empty();
}

void PopulateRecentBlockhash(
    mojom::SignSolTransactionsRequest& sign_sol_transactions_request,
    const std::string& recent_blockhash) {
  for (auto& solana_tx_data : sign_sol_transactions_request.tx_datas) {
    if (solana_tx_data->recent_blockhash.empty()) {
      solana_tx_data->recent_blockhash = recent_blockhash;
    }
  }
}

void PopulateRecentBlockhash(mojom::TransactionInfo& tx_info,
                             const std::string& recent_blockhash) {
  CHECK(tx_info.tx_data_union->is_solana_tx_data());
  if (tx_info.tx_data_union->get_solana_tx_data()->recent_blockhash.empty()) {
    tx_info.tx_data_union->get_solana_tx_data()->recent_blockhash =
        recent_blockhash;
  }
}

std::optional<std::string> EncodeScanTransactionParams(
    const mojom::SignSolTransactionsRequest& sign_sol_transactions_request,
    const std::string& from_address) {
  base::Value::List transactions;
  for (auto& tx_data : sign_sol_transactions_request.tx_datas) {
    auto serialized_tx = GetBase64TransactionFromSolanaTxData(tx_data.Clone());
    if (!serialized_tx) {
      return std::nullopt;
    }

    transactions.Append(*serialized_tx);
  }

  base::Value::Dict params;
  params.Set("transactions", std::move(transactions));
  params.Set("metadata",
             GetMetadata(std::move(sign_sol_transactions_request.origin_info)));
  params.Set("userAccount", from_address);

  return GetJSON(base::Value(std::move(params)));
}

std::optional<std::string> EncodeScanTransactionParams(
    const mojom::TransactionInfo& tx_info,
    const std::string& from_address) {
  if (!tx_info.tx_data_union->is_solana_tx_data()) {
    return std::nullopt;
  }
  auto serialized_tx = GetBase64TransactionFromSolanaTxData(
      std::move(tx_info.tx_data_union->get_solana_tx_data()));
  if (!serialized_tx) {
    return std::nullopt;
  }

  base::Value::List transactions;
  transactions.Append(*serialized_tx);

  base::Value::Dict params;
  params.Set("transactions", std::move(transactions));
  params.Set("metadata", GetMetadata(tx_info.origin_info));
  params.Set("userAccount", from_address);

  return GetJSON(base::Value(std::move(params)));
}

}  // namespace solana

}  // namespace brave_wallet
