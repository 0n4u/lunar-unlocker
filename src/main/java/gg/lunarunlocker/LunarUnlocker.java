package gg.lunarunlocker;

import gg.lunarunlocker.config.LocalConfigStore;
import gg.lunarunlocker.lifecycle.ClientDirectoryCleanupCallback;
import gg.lunarunlocker.mapping.MappedClasses;
import gg.lunarunlocker.mapping.MappingProfileSnapshotRegistry;
import gg.lunarunlocker.mapping.runtime.RuntimeNameMappingRegistry;
import gg.lunarunlocker.reflect.LunarMappings;
import gg.lunarunlocker.runtime.NativeBridge;
import gg.lunarunlocker.ui.click.component.GuiComponent;
import gg.lunarunlocker.wrapper.impl.ForgeVersion;
import java.io.File;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.Date;


public class LunarUnlocker {
    private static int opaqueState;
    public static LunarUnlocker INSTANCE;
    public static boolean mappingsLoaded;
    public boolean enabled;
    public static boolean renderReady;
    public static final String VERSION = "4.21-unlocker";

    private LunarOnlyMappingTaskSet primaryMappingTaskSet;
        private Object directoryCleanupCallback;
                private boolean forgeAbsent;
                private boolean mappingsRemapped;

    public LunarUnlocker() {
        INSTANCE = this;
        this.forgeAbsent = NativeBridge.isForgeAbsent();
        this.directoryCleanupCallback = new ClientDirectoryCleanupCallback();
    }

    

    public static void debugLog(String message) {
            String normalizedMessage = message == null ? "<null>" : message;
            try {
                NativeBridge.sce("DEBUG " + normalizedMessage);
            }
            catch (Throwable ignored) {
            }
        }

    public static void logThrowable(Throwable error) {
        StringWriter sw = new StringWriter();
        error.printStackTrace(new PrintWriter(sw));
        debugLog(sw.toString());
    }

    public static void logError(String message) {
        debugLog("ERROR " + message);
    }

    public static void notifyNativeStackTrace() {
        
    }

    public static String formatThrowable(Throwable error) {
        StringWriter sw = new StringWriter();
        error.printStackTrace(new PrintWriter(sw));
        return sw.toString();
    }

    public static int opaquePredicate() {
        return opaqueState;
    }

    public static void setOpaqueState(int value) {
        opaqueState = value;
    }

    public static int getOpaqueState() {
        return opaqueState;
    }

    public static byte[] readResource(String resourceName) {
        
        return null;
    }

    

    public boolean isVanillaMinecraftPresent() {
        return true; 
    }

    public boolean isForgeAbsent() {
        return this.forgeAbsent;
    }

    public boolean isForgeRemapInactive() {
        return true;
    }

    public boolean isMappingsRemapped() {
        return this.mappingsRemapped;
    }

    public boolean isFabricMinecraftPresent() {
        return false;
    }

    public boolean isForgeRemapActive() {
        return false;
    }

    public boolean isLabyModPresent() {
        return false;
    }

    public boolean isUnclassifiedFlag463Set() {
        return false;
    }

    public boolean isHealthPredictionEnabled() {
        return false;
    }

    public boolean inputEnabled() {
        return false;
    }

    public boolean isFeatureDisabled() {
        return false;
    }

    public Object getNotificationManager() {
        return null;
    }

    public Object getFontSelector() {
        return null;
    }

    public Object getMappings() {
        return null;
    }

    public Object getMappingsMapperCompat() {
        return null;
    }

    

    public void loadMappings() {
        MappedClasses.p();
        mappingsLoaded = true;
        if (this.forgeAbsent && ForgeVersion.MC_26_1.v()) {
            NativeBridge.fs();
            MappedClasses.p();
            NativeBridge.rsc();
        }
        MappingProfileSnapshotRegistry.X();
        RuntimeNameMappingRegistry.initializeRegistry();
        
        
        
        MappingProfileSnapshotRegistry.y();
        this.mappingsRemapped = false;
        MappingProfileSnapshotRegistry.h();
        NativeBridge.su("Unlocker");
        if (GuiComponent.getLegacyComponentState() == null) {
            setOpaqueState(++opaqueState);
        }
    }

    public boolean initAccountInfo() {
        
        return false;
    }

    public void initializeManagers() {
        
        
        initPrimaryMappingTasks();
    }

    private void initPrimaryMappingTasks() {
        this.primaryMappingTaskSet = new LunarOnlyMappingTaskSet();
        
        
        
        
        
        final long deadline = System.currentTimeMillis() + 300000L;
        final LunarOnlyMappingTaskSet taskSet = this.primaryMappingTaskSet;
        Thread waiter = new Thread(() -> {
            while (!LunarMappings.isRuntimePresent()) {
                if (System.currentTimeMillis() > deadline) {
                    LunarUnlocker.debugLog("LUNAR unlocker: runtime not detected "
                            + "within 5 minutes; giving up");
                    return;
                }
                try {
                    Thread.sleep(1000L);
                }
                catch (InterruptedException interrupted) {
                    return;
                }
            }
            try {
                taskSet.registerAndRun();
                LunarUnlocker.debugLog("LUNAR unlocker: transforms registered and run");
            }
            catch (Throwable error) {
                LunarUnlocker.logThrowable(error);
            }
            finally {
                try {
                    int status = NativeBridge.dch();
                    NativeBridge.sce("DEBUG LUNAR unlocker: dch status=" + status);
                }
                catch (Throwable error) {
                    LunarUnlocker.debugLog("LUNAR unlocker: dch failed: " + error);
                }
            }
        }, "LunarUnlockerInit");
        waiter.setDaemon(true);
        waiter.start();
    }

    public void initialize() {
        
    }
}