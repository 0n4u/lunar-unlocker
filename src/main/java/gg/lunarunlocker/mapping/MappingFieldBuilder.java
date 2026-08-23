package gg.lunarunlocker.mapping;

import gg.lunarunlocker.mapping.MappingField;
import gg.lunarunlocker.mapping.MappingMemberBuilder;

public class MappingFieldBuilder
extends MappingMemberBuilder<MappingFieldBuilder, MappingField> {
    private int arrayDimensions = 0;

    public MappingField buildField() {
        return MappingField.fromBuilder(this);
    }

    @Override
    public MappingField build() {
        return this.buildField();
    }

    public int getArrayDimensions() {
        return this.arrayDimensions;
    }

    public MappingFieldBuilder setArrayDimensions(int arrayDimensions) {
        this.arrayDimensions = arrayDimensions;
        return this;
    }
}
