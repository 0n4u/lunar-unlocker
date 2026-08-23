package gg.lunarunlocker.lifecycle;

public interface ClientLifecycleCallback {
    void log(String message);

    void close();
}
