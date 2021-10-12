/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_news;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.LayerDrawable;
import android.graphics.drawable.ShapeDrawable;
import android.graphics.drawable.shapes.RoundRectShape;
import android.net.Uri;
import android.os.Build;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.recyclerview.widget.RecyclerView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.brave_news.models.NewsItem;
import org.chromium.chrome.browser.brave_news.models.FeedItemCard;
import org.chromium.chrome.browser.ntp.BraveNewTabPageLayout;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;
import org.chromium.brave_news.mojom.FeedItem;

public class BraveNewsAdapterFeed extends RecyclerView.Adapter<BraveNewsAdapterFeed.ViewHolder> {

    private LayoutInflater mInflater;
    public RecycleItemClickListener mClickListener;
    private int mType;
    private Context mContext;
    private Activity mActivity;
    private View view;
    private LinearLayout linearLayout;
    private LinearLayout.LayoutParams linearLayoutParams;

	private CopyOnWriteArrayList<FeedItem> mNewsItems;
    private CopyOnWriteArrayList<FeedItemCard> mNewsItemsCard;
	private ViewHolder mHolder;
    private final String TAG = "BN";


    public BraveNewsAdapterFeed(Activity activity, CopyOnWriteArrayList<FeedItem> newsItems) {
        this.mInflater = LayoutInflater.from(activity);
        this.mActivity = activity;
        this.mNewsItems = newsItems;
    }


    // public BraveNewsAdapterFeed(Activity activity, CopyOnWriteArrayList<FeedItemCard> newsItemsCard) {
    //     this.mInflater = LayoutInflater.from(activity);
    //     this.mActivity = activity;
    //     this.mNewsItemsCard = newsItemsCard;
    // }

    @NonNull
    @Override
    public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        Log.d("bn", "onCreateViewHolder viewType:" + viewType);
        view = mInflater.inflate(R.layout.brave_news_row, parent, false);
        mHolder = new ViewHolder(view);

        return mHolder;
    }

    @Override
    public void onBindViewHolder(@NonNull BraveNewsAdapterFeed.ViewHolder holder, int position) {
        LinearLayout.LayoutParams params1;
        if (mNewsItems != null){

            Log.d("bn", "createfeed onBindViewHolder position:" + position);
            Log.d("bn", "createfeed onBindViewHolder mNewsItem:" + mNewsItems.get(position));
            Log.d("bn", "createfeed onBindViewHolder mNewsItem which:" + mNewsItems.get(position).which());
            // Log.d("BN", "onBindViewHolder getItemViewType:" + holder.getItemViewType());

            // if (position >= getItemCount() - 10) {
            //     Log.d(TAG, "end");
            //     return;
            // }

            // new CardBuilderFeed(mHolder.linearLayout, mActivity, position, mNewsItems.get(position),mNewsItems.get(position).which());
        } 
        // else if (mNewsItemsCard != null){
        //     Log.d("bn", "createfeed onBindViewHolder position:" + position);
        //     Log.d("bn", "createfeed onBindViewHolder mNewsItem:" + mNewsItemsCard.get(position));
        //     Log.d("bn", "createfeed onBindViewHolder mNewsItem which:" + mNewsItemsCard.get(position).which());
        //     // Log.d("BN", "onBindViewHolder getItemViewType:" + holder.getItemViewType());

        //     // if (position >= getItemCount() - 10) {
        //     //     Log.d(TAG, "end");
        //     //     return;
        //     // }

        //     new CardBuilderFeed(mHolder.linearLayout, mActivity, position, mNewsItemsCard.get(position),mNewsItemsCard.get(position).which()); 
        // }
    }

    @Override
    public int getItemCount() {

            return mNewsItems.size();

        // return mNewsItemsCard.size();
    }


    // public void setClickListener(BraveNewTabPageLayout recyclerMain) {
    //     this.mClickListener = recyclerMain;
    // }

    public class ViewHolderWidget extends RecyclerView.ViewHolder {
        ViewHolderWidget(View itemView) {
            super(itemView);
        }
    }

    public class ViewHolder extends RecyclerView.ViewHolder {//implements View.OnClickListener {
        LinearLayout layoutRight;
        LinearLayout layoutLeft;
        TextView testTextView;
        LinearLayout testLinear;

        TableRow rowTop;
        TextView topText;

        ImageView image;
        ImageView logo;
        TextView title;
        TextView source;
        TextView desc;
        LinearLayout layou1;

        TableRow layoutRowPhotos;
        TableRow layoutRow;

        TableRow row1;
        TableRow row2;
        TableRow row3;

        LinearLayout layout1;
        LinearLayout layout2;
        LinearLayout layout3;

        LinearLayout layout;

        TableLayout tableLayoutTopNews;

        LinearLayout linearLayout;

        ViewHolder(View itemView) {
            super(itemView);
            testTextView = new TextView(mActivity);
            testLinear = new LinearLayout(mActivity);

            layou1 = new LinearLayout(mActivity);
            title = new TextView(mActivity);
            desc = new TextView(mActivity);

            image = new ImageView(mActivity);
            logo = new ImageView(mActivity);
            source = new TextView(mActivity);
            layoutRowPhotos = new TableRow(mActivity);
            layoutRow = new TableRow(mActivity);
            row1 = new TableRow(mActivity);
            row2 = new TableRow(mActivity);
            row3 = new TableRow(mActivity);
            layout1 = new LinearLayout(mActivity);
            layout2 = new LinearLayout(mActivity);
            layout3 = new LinearLayout(mActivity);
            layout = new LinearLayout(mActivity);

            tableLayoutTopNews = new TableLayout(mActivity);

            rowTop = new TableRow(mActivity);
            topText = new TextView(mActivity);

            // itemView.setOnClickListener(this);
            linearLayout = itemView.findViewById(R.id.card_layout);

            linearLayoutParams = new LinearLayout.LayoutParams(
                    LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
            linearLayoutParams.setMargins(0, 40, 0, 40);
        }

        // @Override
        // public void onClick(View view) {
        //     if (mClickListener != null) mClickListener.onItemClick(view, getAdapterPosition());
        // }
    }

    public FeedItem getItem(int id) {
            return mNewsItems.get(id);

        // return mNewsItemsCard.get(id);
    }

    public interface RecycleItemClickListener {
        void onItemClick(View view, int position);

        void onOptInClick(View view);

        void onCloseClick(View view);
    }
}
