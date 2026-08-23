package gg.lunarunlocker.lifecycle;

import gg.lunarunlocker.config.LocalConfigStore;
import java.io.File;

public class ClientDirectoryCleanupCallback
implements ClientLifecycleCallback {
    @Override
    public void log(String message) {
    }

    public ClientDirectoryCleanupCallback() {
        File clientDirectory = LocalConfigStore.baseDirectory();
        if (clientDirectory.exists()) {
            File[] children = clientDirectory.listFiles();
            if (children != null) {
                for (File child : children) {
                    String name = child.getName();
                    if (name.equals("cache")
                            || name.equals("config.json")
                            || name.equals("log")
                            || name.equals("lunar")
                            || name.equals("lunarunlocker-service.json")) {
                        continue;
                    }
                    gg.lunarunlocker.LunarUnlocker.debugLog("LUNAR storage: cleanup removing "
                            + child.getAbsolutePath());
                    deleteRecursively(child);
                }
            }
        }
    }

    
    
    private static void deleteRecursively(File file) {
        if (file.isDirectory()) {
            File[] children = file.listFiles();
            if (children != null) {
                for (File child : children) {
                    deleteRecursively(child);
                }
            }
        }
        file.delete();
    }


    @Override
    public void close() {
    }
}
