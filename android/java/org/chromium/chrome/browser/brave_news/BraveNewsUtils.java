package org.chromium.chrome.browser.brave_news;

import android.content.Context;
import android.os.Build;

import androidx.annotation.RequiresApi;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.Log;
import org.chromium.brave_news.mojom.Article;
import org.chromium.brave_news.mojom.CardType;
import org.chromium.brave_news.mojom.Deal;
import org.chromium.brave_news.mojom.FeedItem;
import org.chromium.brave_news.mojom.FeedItemMetadata;
import org.chromium.brave_news.mojom.FeedPage;
import org.chromium.brave_news.mojom.FeedPageItem;
import org.chromium.brave_news.mojom.Image;
import org.chromium.brave_news.mojom.PromotedArticle;
import org.chromium.brave_news.mojom.Publisher;
import org.chromium.chrome.browser.brave_news.models.FeedItemCard;
import org.chromium.chrome.browser.brave_news.models.FeedItemsCard;
import org.chromium.chrome.browser.brave_news.models.NewsItem;

import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.Iterator;
import java.util.concurrent.CopyOnWriteArrayList;

public class BraveNewsUtils {
    private Context mContext;

    public BraveNewsUtils(Context context) {
        mContext = context;
    }

    public CopyOnWriteArrayList<NewsItem> parseJson(CopyOnWriteArrayList<NewsItem> newsItems) {
        JSONArray feedObjects;
        String jsonString = loadJSONFromAsset(mContext);
        //        CopyOnWriteArrayList<NewsItem> newsItems = new CopyOnWriteArrayList<NewsItem>();
        try {
            feedObjects = new JSONArray(jsonString);
            JSONObject uniObject = null;

            //            for (int i = 0; i < feedObjects.length(); i++) {
            for (int i = 0; i < 100; i++) {
                JSONObject feedObject = feedObjects.getJSONObject(i);
                //                Log.d("Test", "feedObject:"+feedObject);
                //                Log.d("Test", "feedObject:"+feedObject.toString());
                Iterator<String> iterator = feedObject.keys();
                NewsItem newsItem = new NewsItem();

                while (iterator.hasNext()) {
                    String key = iterator.next();
                    String value = feedObject.getString(key);
                    //                    Log.d("test", " getting - " + key + " : " + value);
                    newsItem.setItemValue(key, value);
                }
                newsItems.add(newsItem);
                //                Log.d("test", newsItem.getCategory() + ": " +
                //                newsItem.toString()); Log.d("test", " ----------- ");
            }

        } catch (JSONException e) {
            e.printStackTrace();
        }

        return newsItems;
    }

    public CopyOnWriteArrayList<NewsItem> createFeed() {
        CopyOnWriteArrayList<NewsItem> newsItems = new CopyOnWriteArrayList<NewsItem>();

        return newsItems;
    }

    public CopyOnWriteArrayList<NewsItem> parseJson() {
        JSONArray feedObjects;
        String jsonString = loadJSONFromAsset(mContext);
        CopyOnWriteArrayList<NewsItem> newsItems = new CopyOnWriteArrayList<NewsItem>();
        try {
            feedObjects = new JSONArray(jsonString);
            JSONObject uniObject = null;

            //            for (int i = 0; i < feedObjects.length(); i++) {
            for (int i = 0; i < 100; i++) {
                JSONObject feedObject = feedObjects.getJSONObject(i);
                //                Log.d("Test", "feedObject:"+feedObject);
                //                Log.d("Test", "feedObject:"+feedObject.toString());
                Iterator<String> iterator = feedObject.keys();
                NewsItem newsItem = new NewsItem();

                while (iterator.hasNext()) {
                    String key = iterator.next();
                    String value = feedObject.getString(key);
                    //                    Log.d("test", " getting - " + key + " : " + value);
                    newsItem.setItemValue(key, value);
                }
                newsItems.add(newsItem);
                //                Log.d("test", newsItem.getCategory() + ": " +
                //                newsItem.toString()); Log.d("test", " ----------- ");
            }

        } catch (JSONException e) {
            e.printStackTrace();
        }

        return newsItems;
    }


    private String loadJSONFromAsset(Context context) {
        listAssetFiles("", context);
        String json = null;
        try {
            InputStream is = context.getAssets().open("feed.json");
            int size = is.available();
            byte[] buffer = new byte[size];
            is.read(buffer);
            is.close();
            json = new String(buffer, StandardCharsets.UTF_8);
        } catch (IOException ex) {
            ex.printStackTrace();
            return null;
        }
        return json;
    }

    private boolean listAssetFiles(String path, Context context) {
        Log.d("Test", "file listAssetFiles:" + path);
        String[] list;
        try {
            list = context.getAssets().list(path);
            if (list.length > 0) {
                // This is a folder
                for (String file : list) {
                    if (!listAssetFiles(path + "/" + file, context))
                        return false;
                    else {
                        // This is a file
                        // TODO: add file name to an array list
                        Log.d("Test", "file assets:" + file);
                    }
                }
            }
        } catch (IOException e) {
            Log.d("Test", "file IOException:" + e);
            return false;
        }

        return true;
    }

    private Publisher getPublisher(){
        Publisher publisher = null;

        return publisher;

    }

    public String getPromotionIdItem(FeedItemsCard items) {
        Log.d("bn", " type " + items.getCardType() + " size: " + items.getFeedItems().size());
        String creativeInstanceId = "null";
        for (FeedItemCard itemCard : items.getFeedItems()) {
            FeedItem item = itemCard.getFeedItem();
            FeedItemMetadata itemMetaData = new FeedItemMetadata();
            switch (item.which()) {
                case FeedItem.Tag.PromotedArticle:
                    PromotedArticle promotedArticle = item.getPromotedArticle();
                    FeedItemMetadata promotedArticleData = promotedArticle.data;
                    creativeInstanceId = promotedArticle.creativeInstanceId;
                    // braveNewsItems.add(item.getPromotedArticle());
                    // braveNewsItems.add(promotedArticle.data);
                    itemMetaData = promotedArticle.data;
                    Log.d("bn", " PromotedArticle: " + promotedArticleData.title);
                    // Log.d("bn", id+" PromotedArticle categoryName: " +
                    // promotedArticleData.categoryName); Log.d("bn", "getfeed feed pages item type
                    // PromotedArticle creativeInstanceId: " + creativeInstanceId);
                    break;
            }
        }

        return creativeInstanceId;
    }

    public void logFeedItem(FeedItemsCard items, String id) {
        Log.d("bn", id + " type " + items.getCardType() + " size: " + items.getFeedItems().size());

        for (FeedItemCard itemCard : items.getFeedItems()) {
            FeedItem item = itemCard.getFeedItem();

            FeedItemMetadata itemMetaData = new FeedItemMetadata();
            switch (item.which()) {
                case FeedItem.Tag.Article:

                    Article article = item.getArticle();
                    FeedItemMetadata articleData = article.data;
                    itemMetaData = article.data;

                    // braveNewsItems.add(article.data);

                    Log.d("bn", id + " articleData: " + articleData.title);
                    // Log.d("bn", id + " articleData categoryName: " + articleData.categoryName);
                    break;
                case FeedItem.Tag.PromotedArticle:
                    PromotedArticle promotedArticle = item.getPromotedArticle();
                    FeedItemMetadata promotedArticleData = promotedArticle.data;
                    String creativeInstanceId = promotedArticle.creativeInstanceId;
                    // braveNewsItems.add(item.getPromotedArticle());
                    // braveNewsItems.add(promotedArticle.data);
                    itemMetaData = promotedArticle.data;
                    Log.d("bn", id + " PromotedArticle: " + promotedArticleData.title);
                    // Log.d("bn", id+" PromotedArticle categoryName: " +
                    // promotedArticleData.categoryName); Log.d("bn", "getfeed feed pages item type
                    // PromotedArticle creativeInstanceId: " + creativeInstanceId);
                    break;
                case FeedItem.Tag.Deal:
                    Deal deal = item.getDeal();
                    FeedItemMetadata dealData = deal.data;
                    String offersCategory = deal.offersCategory;
                    // braveNewsItems.add(item.getDeal());
                    // braveNewsItems.add(deal.data);
                    itemMetaData = deal.data;
                    Log.d("bn", id + " Deal: " + dealData.title);
                    // Log.d("bn", id+" Deal categoryName: " + dealData.categoryName);
                    // Log.d("bn", "getfeed feed pages item type Deal offersCategory: " +
                    // offersCategory);
                    break;
            }
        }
    }
}
