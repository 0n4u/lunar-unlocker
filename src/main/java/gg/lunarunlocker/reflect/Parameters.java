package gg.lunarunlocker.reflect;

import gg.lunarunlocker.reflect.ParameterResolver;

public class Parameters {
    public static boolean checkParameterTypes(Class<?>[] parameterTypes, Class<?> returnType, String descriptor) {
        return ParameterResolver.matchesDescriptor(parameterTypes, returnType, descriptor);
    }
}
