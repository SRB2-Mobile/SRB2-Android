package org.stjr.srb2;

import org.libsdl.app.SDLActivity; 

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.provider.Settings;
import android.net.Uri;

public class SRB2Game extends SDLActivity {
	public static boolean checkPermission(String permission) {
		Activity activity = (Activity)getContext();
		if (activity.checkSelfPermission(permission) == PackageManager.PERMISSION_GRANTED)
			return true;
		return false;
	}

	public static void appSettingsIntent() {
		Intent intent = new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
		Uri uri = Uri.fromParts("package", mSingleton.getPackageName(), null);
		intent.setData(uri);
		mSingleton.startActivity(intent);
	}
}
