package gg.lunarunlocker.wrapper.impl;

import gg.lunarunlocker.wrapper.Wrapper;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;


public class LaunchClassLoader
extends Wrapper {
    public static LaunchClassLoader getLaunchClassLoader() {
        return new LaunchClassLoader(new Object());
    }

    public LaunchClassLoader(Object object) {
        super(object);
    }

    public Set getClassLoaderExceptions() {
        return new HashSet();
    }

    public Map cachedClasses() {
        return new HashMap();
    }

    public boolean supportsLegacyClassCache() {
        return false;
    }

}
