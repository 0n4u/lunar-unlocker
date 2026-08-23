package gg.lunarunlocker.mapping;

import gg.lunarunlocker.mapping.JavassistMappingTask;
import gg.lunarunlocker.mapping.MappedClasses;

@Deprecated
public class DeprecatedNoopMappingTask
extends JavassistMappingTask {
    @Override
    public void transform() {
    }

    public DeprecatedNoopMappingTask() {
        super(MappedClasses.DC);
    }
}
