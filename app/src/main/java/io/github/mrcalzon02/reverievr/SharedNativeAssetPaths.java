package io.github.mrcalzon02.reverievr;

/** Exact shared-art allowlist for legacy data-only .rvpack fixtures.
 * The single-APK baseline is the authoritative, nonduplicated delivery.
 * Keep names synchronized with shared-assets/catalog-v1.json and native scopes.
 */
final class SharedNativeAssetPaths {
    private static final String MATERIALS="assets/native/shared/v1/materials/coastal/";
    private static final String MODELS="assets/native/shared/v1/models/psx/";
    private static final String[] TILES={
        "shingle.png","quay_concrete.png","black_basalt.png",
        "oxidized_steel.png","sandbag_burlap.png","olive_enamel.png"
    };
    private static final String[] PROPS={
        "concrete_block_1.bwm","wooden_crate_1.bwm","ammo_box_mp_1.bwm"
    };
    private SharedNativeAssetPaths(){}
    static boolean allowed(String id,String path,boolean directory){
        if(id==null||path==null||directory)return false;
        boolean materials="breakwater-battery".equals(id)||"omega-front".equals(id)||
            "between-deliveries-red-ledger".equals(id)||
            "procedural-test-chamber".equals(id);
        if(!materials)return false;
        for(String name:TILES)
            if(path.equals(MATERIALS+name))return true;
        if(!"omega-front".equals(id))
            for(String name:PROPS)
                if(path.equals(MODELS+name))return true;
        return false;
    }
}
