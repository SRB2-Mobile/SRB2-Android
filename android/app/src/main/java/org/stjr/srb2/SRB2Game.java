package org.stjr.srb2;

import org.libsdl.app.SDLActivity;

import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;

public class SRB2Game extends SDLActivity {
	public static boolean checkPermission(String permission) {
		if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
			return true;
		}
		Activity activity = (Activity)getContext();
		if (activity.checkSelfPermission(permission) == PackageManager.PERMISSION_GRANTED)
			return true;
		return false;
	}

	public static boolean inMultiWindowMode() {
		if (Build.VERSION.SDK_INT >= 24) {
			if (SRB2Game.mSingleton.isInMultiWindowMode())
				return true;
		}
		return false;
	}
}
