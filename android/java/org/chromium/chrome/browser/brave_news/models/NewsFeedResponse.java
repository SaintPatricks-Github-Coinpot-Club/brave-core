/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_news.models;

public abstract class NewsFeedResponse<T> {
    private NewsFeedResponse() {}

    public static final class ResponseSuccess<T> extends NewsFeedResponse<T> {
        public T data;

        public ResponseSuccess(T data) {
            this.data = data;
        }
    }

    public static final class ResponseError<T> extends NewsFeedResponse<T> {
        public Exception exception;

        public ResponseError(Exception exception) {
            this.exception = exception;
        }
    }
}
