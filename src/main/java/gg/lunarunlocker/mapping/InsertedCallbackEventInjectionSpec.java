package gg.lunarunlocker.mapping;

import gg.lunarunlocker.mapping.EventInjectionSpec;
import gg.lunarunlocker.mapping.InsertedCallbackMarker;
import gg.lunarunlocker.mapping.MappingMethod;

public class InsertedCallbackEventInjectionSpec
extends EventInjectionSpec {
    private static final String h;

    @Override
    public String buildInjectionCode() {
        String callbackCode = this.getEventClass().getName() + h;
        return callbackCode;
    }

    public InsertedCallbackEventInjectionSpec(MappingMethod mappingMethod, Class<? extends InsertedCallbackMarker> clazz) {
        super(mappingMethod, clazz);
    }

    static {
        try {
            h = "#call();";
        }
        catch (Exception exception) {
            throw new ExceptionInInitializerError(exception);
        }
    }
}
