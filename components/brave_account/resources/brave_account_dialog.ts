/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'

import { getCss } from './brave_account_dialog.css.js'
import { getHtml } from './brave_account_dialog.html.js'

export class BraveAccountDialogElement extends CrLitElement {
  static get is() {
    return 'brave-account-dialog'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      alertMessage: { type: String },
      dialogDescription: { type: String },
      dialogTitle: { type: String },
      showBackButton: { type: Boolean },
    }
  }

  protected accessor alertMessage: string = ''
  protected accessor dialogDescription: string = ''
  protected accessor dialogTitle: string = ''
  protected accessor showBackButton: boolean = false
}

declare global {
  interface HTMLElementTagNameMap {
    'brave-account-dialog': BraveAccountDialogElement
  }
}

customElements.define(BraveAccountDialogElement.is, BraveAccountDialogElement)
