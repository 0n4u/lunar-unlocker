package gg.lunarunlocker.mapping;

import gg.lunarunlocker.LunarUnlocker;
import gg.lunarunlocker.lunar.LunarRelogin;


public final class LunarReloginTask implements MappingTask {
    private boolean applied;

    @Override
    public void transform() {
        LunarUnlocker.debugLog("LUNAR relogin: triggering websocket login replay");
        LunarRelogin.trigger();
    }

    @Override
    public void rollback() {
    }

    @Override
    public Class getTargetClass() {
        return null;
    }

    @Override
    public void prepare() {
    }

    @Override
    public boolean isApplied() {
        return this.applied;
    }

    @Override
    public int commit() {
        this.applied = true;
        return 0;
    }

    @Override
    public void serialize() {
    }
}