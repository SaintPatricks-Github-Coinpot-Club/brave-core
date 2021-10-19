/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

 import { BraveWalletBrowserProxyImpl } from './brave_wallet_browser_proxy.m.js';

(function() {
  'use strict';
  
/**
* @fileoverview
* 'settings-sync-subpage' is the settings page content
*/
Polymer({
  is: 'settings-wallet-networks-subpage',

  behaviors: [
    I18nBehavior,
    WebUIListenerBehavior
  ],

  properties: {
    /**
     * Array of sites to display in the widget.
     * @type {!Array<SiteException>}
     */
    networks: {
      type: Array,
      value() {
        return [];
      },
    },
    addNetworkError_: {
      type: Boolean,
      value: false,
    },
    showAddWalletNetworkDialog_: {
      type: Boolean,
      value: false,
    }
  },

  browserProxy_: null,
  actionItemName : String,
  /** @override */
  created: function() {
    this.browserProxy_ = BraveWalletBrowserProxyImpl.getInstance();
    window.addEventListener('load', this.onLoad_.bind(this));
  },
  onLoad_: function() {
    this.updateNetworks();
  },
  notifyKeylist: function() {
    const keysList =
    /** @type {IronListElement} */ (this.$$('#networksList'));
    if (keysList) {
      keysList.notifyResize();
    }
  },
  toggleUILayout: function(launched) {
    if (launched) {
      this.localNodeLaunchError_ = false
    } else {
      this.showAddWalletNetworkDialog_ = false
    }
  },

  onServiceLaunched: function(success) {
    this.toggleUILayout(success)
    if (success) {
      this.updateNetworks();
    }
  },

  /*++++++
  * @override */
  ready: function() {
  },

  getIconForKey: function(name) {
    return name == 'self' ? 'icon-button-self' : 'icon-button'
  },
  

  onAddNetworkTap_: function(item) {
    this.showAddWalletNetworkDialog_ = true
  },

  updateNetworks: function() {
/*
    this.browserProxy_.getIpnsKeysList().then(keys => {
      if (!keys)
        return;
      this.keys_ = JSON.parse(keys);
      this.toggleUILayout(true)
      this.notifyKeylist();
    });
    */
  },

  onAddNetworkDialogClosed_: function() {
    this.showAddWalletNetworkDialog_ = false
    this.actionItemName = ""
    this.updateNetworks();
  }
});
})();
