package io.github.mrcalzon02.reverievr;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.Signature;
import android.os.Build;
import java.security.MessageDigest;

/**
 * Admission for separately installed native game APKs. Only known package
 * identities signed by the same release signer as the installed ReverieVR
 * hub are eligible. No untrusted .so loading or shared app-data assumptions.
 */
final class CompanionGameRegistry {
    static final class Descriptor {
        final String gameId,packageName;
        Descriptor(String id,String pkg){gameId=id;packageName=pkg;}
        String activityName(){return packageName+".GameVrActivity";}
    }
    private CompanionGameRegistry(){}

    static Descriptor find(String gameId){
        GameModuleRegistry.Entry entry=GameModuleRegistry.find(gameId);
        return entry==null?null:
            new Descriptor(entry.id,entry.companionPackage);
    }

    static Descriptor installed(Context context,String gameId){
        Descriptor entry=find(gameId);
        if(entry==null||context==null)return null;
        PackageManager pm=context.getPackageManager();
        try{
            int flags=Build.VERSION.SDK_INT>=28
                ? PackageManager.GET_SIGNING_CERTIFICATES:
                  PackageManager.GET_SIGNATURES;
            PackageInfo hub=pm.getPackageInfo(context.getPackageName(),flags);
            PackageInfo game=pm.getPackageInfo(entry.packageName,flags);
            if(!matchingSigners(hub,game))
                return null;
            long code=Build.VERSION.SDK_INT>=28?game.getLongVersionCode():game.versionCode;
            if(code<=0)return null;
            ActivityInfo info=pm.getActivityInfo(
                new ComponentName(entry.packageName,entry.activityName()),0);
            if(info==null||!info.exported||!info.enabled)
                return null;
            if(!entry.packageName.equals(info.packageName))
                return null;
            // This verifies the expected component, not a generic launcher
            // activity that could belong to a different app.
            return entry;
        }catch(PackageManager.NameNotFoundException | SecurityException e){
            return null;
        }
    }

    static long installedVersion(Context context,String gameId){
        Descriptor accepted=installed(context,gameId);
        if(accepted==null)return 0L;
        try{
            PackageInfo info=context.getPackageManager().getPackageInfo(
                accepted.packageName,0);
            return Build.VERSION.SDK_INT>=28?
                info.getLongVersionCode():info.versionCode;
        }catch(PackageManager.NameNotFoundException | SecurityException e){
            return 0L;
        }
    }

    static boolean launch(Activity hub,String gameId){
        Descriptor selected=installed(hub,gameId);
        if(selected==null)return false;
        Intent intent=new Intent();
        intent.setComponent(new ComponentName(
            selected.packageName,selected.activityName()));
        intent.putExtra("reverie_protocol",1);
        intent.putExtra("reverie_game_id",selected.gameId);
        try{
            hub.startActivity(intent);
            return true;
        }catch(RuntimeException e){
            ReverieLog.incident("GAME_COMPANION",
                "Verified companion Activity could not start: "+selected.gameId);
            return false;
        }
    }

    private static boolean matchingSigners(PackageInfo hub,PackageInfo game){
        Signature[] a=signatures(hub),b=signatures(game);
        if(a==null||b==null||a.length!=1||b.length!=1)return false;
        // Certificate-byte equality, not merely a matching package label
        // or user-supplied reported certificate fingerprint.
        return MessageDigest.isEqual(a[0].toByteArray(),b[0].toByteArray());
    }
    private static Signature[] signatures(PackageInfo pkg){
        if(Build.VERSION.SDK_INT>=28)
            return pkg.signingInfo==null?null:pkg.signingInfo.getApkContentsSigners();
        return pkg.signatures;
    }
}
