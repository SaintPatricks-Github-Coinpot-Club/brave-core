/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/check.h"
#include "brave/browser/ai_chat/ai_chat_urls.h"
#include "brave/browser/ui/webui/ai_chat/ai_chat_ui.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/side_panel/side_panel_registry.h"
#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "components/grit/brave_components_strings.h"
#include "components/user_prefs/user_prefs.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"

using SidePanelWebUIViewT_AIChatUI = SidePanelWebUIViewT<AIChatUI>;
BEGIN_TEMPLATE_METADATA(SidePanelWebUIViewT_AIChatUI, SidePanelWebUIViewT)
END_METADATA

namespace {

std::unique_ptr<views::View> CreateAIChatSidePanelWebView(
    base::WeakPtr<Profile> profile,
    SidePanelEntryScope& scope) {
  CHECK(profile);

  auto web_view = std::make_unique<SidePanelWebUIViewT<AIChatUI>>(
      scope, base::RepeatingClosure(), base::RepeatingClosure(),
      std::make_unique<WebUIContentsWrapperT<AIChatUI>>(
          ai_chat::TabAssociatedConversationUrl(), profile.get(),
          IDS_SIDEBAR_CHAT_SUMMARIZER_ITEM_TITLE,
          /*esc_closes_ui=*/false));
  web_view->ShowUI();
  return web_view;
}

}  // namespace

namespace brave {

// Register here for an entry that is used for all tabs and its life time is
// tied with tab. If it has specific life time, use separated manager for
// registering it.
void RegisterContextualSidePanel(SidePanelRegistry* registry,
                                 content::WebContents* web_contents) {
  content::BrowserContext* context = web_contents->GetBrowserContext();
  if (ai_chat::IsAIChatEnabled(user_prefs::UserPrefs::Get(context)) &&
      Profile::FromBrowserContext(context)->IsRegularProfile()) {
    // If |registry| already has it, it's no-op.
    registry->Register(std::make_unique<SidePanelEntry>(
        SidePanelEntry::Key(SidePanelEntry::Id::kChatUI),
        base::BindRepeating(&CreateAIChatSidePanelWebView,
                            Profile::FromBrowserContext(context)->GetWeakPtr()),
        SidePanelEntry::kSidePanelDefaultContentWidth));
  }
}

}  // namespace brave
