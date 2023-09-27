package com.kaliber.base;

import android.app.NativeActivity;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.StrictMode;
import android.util.Log;
import android.view.WindowManager;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.core.content.FileProvider;

import com.google.android.gms.ads.AdError;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.FullScreenContentCallback;
import com.google.android.gms.ads.LoadAdError;
import com.google.android.gms.ads.MobileAds;
import com.google.android.gms.ads.initialization.InitializationStatus;
import com.google.android.gms.ads.initialization.OnInitializationCompleteListener;
import com.google.android.gms.ads.interstitial.InterstitialAd;
import com.google.android.gms.ads.interstitial.InterstitialAdLoadCallback;

import java.io.File;

public class KaliberActivity extends NativeActivity {
    private static final Handler sHandler = new Handler(Looper.getMainLooper());

    static {
        // Get the native Java methods bound to exported functions.
        System.loadLibrary(BuildConfig.NATIVE_LIBRARY);
    }

    private InterstitialAd mInterstitialAd;

    boolean mIsDebuggable = false;

    public static native void onShowAdResult(boolean succeeded);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        ApplicationInfo appInfo = getApplicationContext().getApplicationInfo();
        mIsDebuggable = (appInfo.flags & ApplicationInfo.FLAG_DEBUGGABLE) != 0;

        if (!mIsDebuggable) {
            MobileAds.initialize(this, new OnInitializationCompleteListener() {
                @Override
                public void onInitializationComplete(InitializationStatus initializationStatus) {
                    Log.d("kaliber", "MobileAds initialization complete.");
                }
            });
            loadInterstitialAd();
        }
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

    public void showInterstitialAd() {
        if (mIsDebuggable) return;

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mInterstitialAd != null) {
                    mInterstitialAd.show(KaliberActivity.this);
                } else {
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
                            "com.woom.game.fileprovider", file);

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
        AdRequest adRequest = new AdRequest.Builder().build();
        InterstitialAd.load(this, getString(R.string.interstitial_ad_unit_id), adRequest,
                new InterstitialAdLoadCallback() {
                    @Override
                    public void onAdLoaded(@NonNull InterstitialAd interstitialAd) {
                        Log.d("kaliber", "Ad loaded.");
                        mInterstitialAd = interstitialAd;
                        interstitialAd.setFullScreenContentCallback(
                                new FullScreenContentCallback() {
                                    @Override
                                    public void onAdDismissedFullScreenContent() {
                                        Log.d("kaliber", "The ad was dismissed.");
                                        mInterstitialAd = null;
                                        loadInterstitialAd();
                                        onShowAdResult(true);
                                    }

                                    @Override
                                    public void onAdFailedToShowFullScreenContent(AdError adError) {
                                        Log.d("kaliber", "The ad failed to show.");
                                        mInterstitialAd = null;
                                        loadInterstitialAd();
                                        onShowAdResult(false);
                                    }

                                    @Override
                                    public void onAdShowedFullScreenContent() {
                                        Log.d("kaliber", "The ad was shown.");
                                    }
                                });
                    }

                    @Override
                    public void onAdFailedToLoad(@NonNull LoadAdError loadAdError) {
                        Log.d("kaliber", "Ad failed to load. errorCode: " + loadAdError);
                        mInterstitialAd = null;
                        sHandler.postDelayed(new Runnable() {
                            @Override
                            public void run() {
                                loadInterstitialAd();
                            }
                        }, 1000 * 10);
                    }
                });
    }
}
