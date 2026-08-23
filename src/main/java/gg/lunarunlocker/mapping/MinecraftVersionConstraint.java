package gg.lunarunlocker.mapping;

import gg.lunarunlocker.wrapper.impl.ForgeVersion;


public class MinecraftVersionConstraint {
    public final ForgeVersion D;
    public final MinecraftVersionComparisonOperator s;

    public MinecraftVersionConstraint(ForgeVersion forgeVersion,
                                      MinecraftVersionComparisonOperator operator) {
        this.D = forgeVersion;
        this.s = operator;
    }

    public ForgeVersion L() {
        return this.D;
    }

    public MinecraftVersionComparisonOperator l() {
        return this.s;
    }

    public boolean y() {
        return this.s.N(ForgeVersion.c(), this.D.i());
    }
}
