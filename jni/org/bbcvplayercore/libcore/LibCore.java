package org.bbcvplayercore.libcore;

//import android.os.Build;
//import android.util.Log;

import java.util.ArrayList;

/**
 * Created by yyd on 16-8-11.
 */
public class LibCore {
    private static final String TAG = "LibCore";
    private static boolean sLoaded = false;

    public LibCore(){
        loadLibraries();

    }
    // get version
    public native String version();

    public native boolean nativeNew(ArrayList<String> options);

    public native int startPlay();

    public native void stopPlay();

    public native int pausePlay();

    static synchronized void loadLibraries() {
        if (sLoaded)
            return;
        sLoaded = true;

        try {
            System.loadLibrary("ffmpeg");
            System.loadLibrary("bbcvplayercore");
            System.loadLibrary("SDL2");
        } catch (UnsatisfiedLinkError ule) {
 //           Log.e(TAG, "Can't load playcore library: " + ule);
            /// FIXME Alert user
            System.exit(1);
        } catch (SecurityException se) {
   //         Log.e(TAG, "Encountered a security issue when loading playcore library: " + se);
            /// FIXME Alert user
            System.exit(1);
        }
    }

}
