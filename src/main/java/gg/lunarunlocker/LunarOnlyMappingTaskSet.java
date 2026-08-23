package gg.lunarunlocker;

import gg.lunarunlocker.mapping.LunarReloginTask;
import gg.lunarunlocker.mapping.LunarStubTransform;
import gg.lunarunlocker.mapping.MappingTaskSet;
import gg.lunarunlocker.lunar.LunarServicePlan;
import gg.lunarunlocker.lunar.LunarUnlockSettings;
import gg.lunarunlocker.reflect.LunarMappings;

import java.util.List;


public class LunarOnlyMappingTaskSet extends MappingTaskSet {

    public LunarOnlyMappingTaskSet() {
        
    }

    
    public void registerAndRun() {
        List<LunarServicePlan.ServiceEntry> enabled =
                LunarServicePlan.enabled(LunarUnlockSettings.current());
        for (LunarServicePlan.ServiceEntry entry : enabled) {
            this.D.add(new LunarStubTransform(
                    entry.targetClassName(), entry.transformMethodNames(),
                    entry.argumentSlots(), entry.hookMethodNames()));
        }
        
        this.D.add(new LunarReloginTask());

        this.d();
    }
}