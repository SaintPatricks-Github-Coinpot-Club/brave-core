/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.brave.browser.quick_search_engines.views;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.brave.browser.quick_search_engines.R;
import org.chromium.brave.browser.quick_search_engines.settings.QuickSearchEnginesCallback;
import org.chromium.brave.browser.quick_search_engines.settings.QuickSearchEnginesModel;
import org.chromium.brave.browser.quick_search_engines.utils.QuickSearchEnginesUtil;

import java.util.List;

public class QuickSearchEnginesViewAdapter
        extends RecyclerView.Adapter<QuickSearchEnginesViewHolder> {
    private final List<QuickSearchEnginesModel> mSearchEngines;
    private final QuickSearchEnginesCallback mQuickSearchEnginesCallback;

    public QuickSearchEnginesViewAdapter(
            Context context,
            List<QuickSearchEnginesModel> searchEngines,
            QuickSearchEnginesCallback quickSearchEnginesCallback) {
        mSearchEngines = searchEngines;
        mQuickSearchEnginesCallback = quickSearchEnginesCallback;
    }

    @Override
    public void onBindViewHolder(
            @NonNull QuickSearchEnginesViewHolder quickSearchViewHolder, int position) {
        QuickSearchEnginesModel quickSearchEnginesModel = mSearchEngines.get(position);
        int adapterPosition = quickSearchViewHolder.getAdapterPosition();
        String keyword = quickSearchEnginesModel.getKeyword();

        if (adapterPosition == 0
                && quickSearchEnginesModel.getType()
                        == QuickSearchEnginesModel.QuickSearchEnginesModelType.AI_ASSISTANT) {
            quickSearchViewHolder.mSearchEngineLogo.setImageResource(R.drawable.ic_leo_icon);
        } else if (QuickSearchEnginesUtil.YOUTUBE_SEARCH_ENGINE_KEYWORD.equals(keyword)) {
            quickSearchViewHolder.mSearchEngineLogo.setImageResource(R.drawable.ic_social_youtube);
        } else if (QuickSearchEnginesUtil.BING_SEARCH_ENGINE_KEYWORD.equals(keyword)) {
            quickSearchViewHolder.mSearchEngineLogo.setImageResource(R.drawable.ic_microsoft_color);
        } else if (QuickSearchEnginesUtil.STARTPAGE_SEARCH_ENGINE_KEYWORD.equals(keyword)) {
            quickSearchViewHolder.mSearchEngineLogo.setImageResource(R.drawable.ic_startpage_color);
        } else {
            mQuickSearchEnginesCallback.loadSearchEngineLogo(
                    quickSearchViewHolder.mSearchEngineLogo, quickSearchEnginesModel);
        }

        quickSearchViewHolder.mSearchEngineLogo.setOnClickListener(
                v ->
                        mQuickSearchEnginesCallback.onSearchEngineClick(
                                adapterPosition, quickSearchEnginesModel));
    }

    @NonNull
    @Override
    public QuickSearchEnginesViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view =
                LayoutInflater.from(parent.getContext())
                        .inflate(R.layout.quick_search_engines_view_item, parent, false);
        return new QuickSearchEnginesViewHolder(view);
    }

    @Override
    public int getItemCount() {
        return mSearchEngines.size();
    }

    public List<QuickSearchEnginesModel> getSearchEngines() {
        return mSearchEngines;
    }
}
