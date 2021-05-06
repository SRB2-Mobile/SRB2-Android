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
}
