package com.woom.game;

import android.app.NativeActivity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import com.google.android.gms.ads.AdListener;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.InterstitialAd;
import com.woom.game.R;

public class KaliberActivity extends NativeActivity {
     static {
         // Get the native Java methods bound to exported functions.
         System.loadLibrary("kaliber");
     }

    private static final Handler sHandler = new Handler(Looper.getMainLooper());

    private InterstitialAd mInterstitialAd;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mInterstitialAd = newInterstitialAd();
        loadInterstitialAd();
    }

    private InterstitialAd newInterstitialAd() {
        InterstitialAd interstitialAd = new InterstitialAd(this);
        interstitialAd.setAdUnitId(getString(R.string.interstitial_ad_unit_id));
        interstitialAd.setAdListener(new AdListener() {
            @Override
            public void onAdLoaded() {
                Log.w("kaliber", "Ad loaded.");
            }

            @Override
            public void onAdFailedToLoad(int errorCode) {
                Log.w("kaliber", "Ad failed to load. errorCode: " + errorCode);
                sHandler.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        if (!mInterstitialAd.isLoaded())
                            loadInterstitialAd();
                    }
                }, 1000 * 10);
            }

            @Override
            public void onAdClosed() {
                loadInterstitialAd();
                onShowAdResult(true);
            }
        });
        return interstitialAd;
    }

    public void showInterstitialAd() {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mInterstitialAd.isLoaded()) {
                    mInterstitialAd.show();
                } else {
                    loadInterstitialAd();
                    onShowAdResult(false);
                }
            }
        });
    }

    private void loadInterstitialAd() {
        if (!mInterstitialAd.isLoading()) {
            AdRequest adRequest = new AdRequest.Builder()
                    .setRequestAgent("android_studio:ad_template").build();
            mInterstitialAd.loadAd(adRequest);
        }
    }

    public static native void onShowAdResult(boolean succeeded);
}
