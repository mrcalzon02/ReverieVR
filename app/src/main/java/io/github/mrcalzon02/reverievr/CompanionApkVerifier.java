package io.github.mrcalzon02.reverievr;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.Signature;
import android.os.Build;
import java.io.File;
import java.security.MessageDigest;

/**
 * Android-native executable updates require normal APK signatures, not
 * content-pack validation. Verifies an immutable signed-catalog release,
 * exact package name, publisher cert and monotonic versionCode.
 */
final class CompanionApkVerifier {
    static final class Result {
        final boolean valid;final String reason;
        Result(boolean good,String why){valid=good;reason=why;}
    }
    private CompanionApkVerifier(){}

    static Result verify(Context context,File file,
                         GameModuleCatalog.Release release){
        try{
            if(release==null||!"companion-apk".equals(release.kind))
                return new Result(false,"Not a companion APK release");
            CompanionGameRegistry.Descriptor expected=
                CompanionGameRegistry.find(release.id);
            if(expected==null)return new Result(false,"Unapproved native game");
            if(!GameModuleCatalog.approvedReleaseUrl(release.url,release.kind))
                return new Result(false,"Untrusted native release URL");
            if(file==null||!file.isFile()||file.length()!=release.byteCount)
                return new Result(false,"Incomplete native APK");
            if(!release.sha256.equals(RvPackInstaller.digest(file)))
                return new Result(false,"Native APK digest mismatch");
            PackageManager pm=context.getPackageManager();
            int flags=Build.VERSION.SDK_INT>=28
                ?PackageManager.GET_SIGNING_CERTIFICATES|PackageManager.GET_ACTIVITIES:
                 PackageManager.GET_SIGNATURES|PackageManager.GET_ACTIVITIES;
            PackageInfo apk=pm.getPackageArchiveInfo(file.getAbsolutePath(),flags);
            PackageInfo hub=pm.getPackageInfo(context.getPackageName(),flags);
            if(apk==null||!expected.packageName.equals(apk.packageName))
                return new Result(false,"APK package identity mismatch");
            Signature[] a=signers(hub),b=signers(apk);
            if(a==null||b==null||a.length!=1||b.length!=1||
               !MessageDigest.isEqual(a[0].toByteArray(),b[0].toByteArray()))
                return new Result(false,"Publisher certificate mismatch");
            long code=Build.VERSION.SDK_INT>=28?apk.getLongVersionCode():apk.versionCode;
            if(code<=0||code!=release.revision)
                return new Result(false,"Native game versionCode mismatch");
            // Refuse downgraded installed Android game packages even if the
            // publisher accidentally includes an older signed release.
            try {
                PackageInfo installed=pm.getPackageInfo(expected.packageName,0);
                long old=Build.VERSION.SDK_INT>=28?
                    installed.getLongVersionCode():installed.versionCode;
                if(code<=old)return new Result(false,"Not a newer native game");
            }catch(PackageManager.NameNotFoundException notInstalled){
                // First user-approved installation is allowed.
            }
            boolean hasEntry=false;
            if(apk.activities!=null)
                for(android.content.pm.ActivityInfo info:apk.activities)
                    if(expected.activityName().equals(info.name) &&
                       info.exported && info.enabled)hasEntry=true;
            if(!hasEntry)return new Result(false,"Missing companion game entry Activity");
            return new Result(true,"Verified companion APK");
        }catch(Exception e){
            return new Result(false,"APK verification failed: "+
                e.getClass().getSimpleName());
        }
    }
    private static Signature[] signers(PackageInfo info){
        if(Build.VERSION.SDK_INT>=28)
            return info.signingInfo==null?null:info.signingInfo.getApkContentsSigners();
        return info.signatures;
    }
}
