package gg.lunarunlocker.event;

import java.lang.reflect.Method;


public final class EventBus {
    private EventBus() {
    }

    public static Method getFireMethod(Class<?> eventClass) {
        throw new UnsupportedOperationException(
                "EventBus is not available in the unlocker build");
    }

    public static Method getHasListenersMethod() {
        throw new UnsupportedOperationException(
                "EventBus is not available in the unlocker build");
    }

    public static Object findEventListenersAccessor(Class<?> eventClass) {
        throw new UnsupportedOperationException(
                "EventBus is not available in the unlocker build");
    }
}
