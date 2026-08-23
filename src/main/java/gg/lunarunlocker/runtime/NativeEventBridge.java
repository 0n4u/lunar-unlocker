package gg.lunarunlocker.runtime;

import gg.lunarunlocker.LunarUnlocker;

public class NativeEventBridge {
    public static void reg(Class<?> eventClass, int eventId) {
    }

    public static void call(Object event) {
        if (LunarUnlocker.INSTANCE == null) {
            return;
        }
    }

}

