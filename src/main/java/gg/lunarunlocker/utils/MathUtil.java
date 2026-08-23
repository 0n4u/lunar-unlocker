package gg.lunarunlocker.utils;


public final class MathUtil {
    private MathUtil() {
    }

    public static int floor(double value) {
        int truncated = (int)value;
        return value < (double)truncated ? truncated - 1 : truncated;
    }
}
