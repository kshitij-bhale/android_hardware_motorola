/*
 * Copyright (c) 2024 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.lineageos.settings.device;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.os.ServiceManager;
import android.os.RemoteException;
import android.util.Log;
import android.view.IWindowManager;

import org.lineageos.settings.device.actions.TorchAction;
import org.lineageos.settings.device.actions.UpdatedStateNotifier;

public class FpGesture implements UpdatedStateNotifier {
    private static final String TAG = "MotoActions-FpGesture";

    private final MotoActionsSettings mMotoActionsSettings;
    private final Context mContext;

    private boolean mIsEnabled;

    public FpGesture(MotoActionsSettings motoActionsSettings, Context context) {
        mMotoActionsSettings = motoActionsSettings;
        mContext = context;
        nativeInit(this);
    }

    static {
        System.loadLibrary("fp_gesture");
    }

    @Override
    public synchronized void updateState() {
        if (mMotoActionsSettings.isFpGestureEnabled() && !mIsEnabled) {
            Log.d(TAG, "Enabling");
            nativeEnable(true);
            mIsEnabled = true;
        } else if (!mMotoActionsSettings.isFpGestureEnabled() && mIsEnabled) {
            Log.d(TAG, "Disabling");
            nativeEnable(false);
            mIsEnabled = false;
        }
    }

    private void onFpGesture() {
        Log.d(TAG, "Fingerprint gesture triggered");
        String action = mMotoActionsSettings.getFpGestureAction();
        if (action.equals("screenshot")) {
            takeScreenshot();
        } else if (action.equals("flashlight")) {
            new TorchAction(mContext).action();
        }
    }

    private void takeScreenshot() {
        IWindowManager windowManager = IWindowManager.Stub.asInterface(
                ServiceManager.getService(Context.WINDOW_SERVICE));
        try {
            windowManager.takeScreenshot(0, true);
        } catch (RemoteException e) {
            Log.e(TAG, "Error taking screenshot", e);
        }
    }

    private static native void nativeEnable(boolean enable);
    private native void nativeInit(FpGesture fpGesture);
}