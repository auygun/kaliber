package com.kaliber.base;

import android.app.NativeActivity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.StrictMode;
import android.util.Log;
import android.view.WindowManager;
import android.widget.Toast;

import androidx.core.content.FileProvider;

import com.google.android.gms.ads.AdListener;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.InterstitialAd;
import com.kaliber.demo.R;

import java.io.File;

public class KaliberActivity extends NativeActivity {
    private static final Handler sHandler = new Handler(Looper.getMainLooper());

    static {
        // Get the native Java methods bound to exported functions.
        System.loadLibrary("kaliber");
    }

    private InterstitialAd mInterstitialAd;

    public static native void onShowAdResult(boolean succeeded);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mInterstitialAd = newInterstitialAd();
        loadInterstitialAd();
    }

    public void setKeepScreenOn(final boolean keepScreenOn) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (keepScreenOn) {
                    getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
                } else {
                    getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
                }
            }
        });
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

    public void shareFile(final String fileName) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    StrictMode.VmPolicy.Builder builder = new StrictMode.VmPolicy.Builder();
                    StrictMode.setVmPolicy(builder.build());

                    File dir = getExternalFilesDir(null);
                    File file = new File(dir, fileName);
                    Uri uri = FileProvider.getUriForFile(KaliberActivity.this,
                            "com.codepath.fileprovider", file);

                    Intent emailIntent = new Intent();
                    emailIntent.setAction(Intent.ACTION_SEND);
                    emailIntent.setType("text/plain");
                    emailIntent.putExtra(Intent.EXTRA_STREAM, uri);
                    startActivity(Intent.createChooser(emailIntent, "Send to..."));
                } catch (Throwable t) {
                    Toast.makeText(KaliberActivity.this, "Request failed: " + t.toString(),
                            Toast.LENGTH_LONG).show();
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
}
