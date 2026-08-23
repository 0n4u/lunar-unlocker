package gg.lunarunlocker.asm.transform;

import gg.lunarunlocker.LunarUnlocker;
import gg.lunarunlocker.asm.ITramsformNode;
import gg.lunarunlocker.asm.helper.EventBuilder;
import gg.lunarunlocker.asm.helper.MethodInfo;
import gg.lunarunlocker.mapping.MappingMethod;
import gg.lunarunlocker.mapping.MappingTask;
import gg.lunarunlocker.runtime.ClassBytecodeCache;
import gg.lunarunlocker.wrapper.impl.ForgeVersion;
import gg.lunarunlocker.wrapper.impl.LaunchClassLoader;
import java.io.File;
import java.io.FileOutputStream;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.tree.ClassNode;

public abstract class ClassTransformer
implements MappingTask {
    protected byte[] originalBytecode;
    private static int transformerState;
    protected Class targetClass;
    private boolean commitBlocked;
    protected ClassNode classNode;
    private boolean applied;
    protected byte[] transformedBytecode;

    public static int opaquePredicate() {
        int state = ClassTransformer.getTransformerState();
        if (state == 0) {
            return 11;
        }
        return 0;
    }

    public static void setTransformerState(int state) {
        transformerState = state;
    }

    public void injectEventAtEntry(MappingMethod mappingMethod, Class eventClass, ITramsformNode ... transformNodes) {
        EventBuilder eventBuilder = new EventBuilder(0, eventClass, this.classNode,
                new MethodInfo(mappingMethod.getResolvedName(), mappingMethod.getDescriptor()), mappingMethod.isStaticMethod(), transformNodes);
        eventBuilder.buildEventDispatchInstructions();
        eventBuilder.inject();
    }

    @Override
    public abstract void transform();

    public void writeBytesToFile(String filePath, byte[] bytecode) {
        int opaqueBranch = ClassTransformer.opaquePredicate();
        try {
            File file = new File(filePath);
            boolean parentExists = file.getParentFile().exists();
            if (opaqueBranch != 0 && !parentExists) {
                parentExists = file.getParentFile().mkdirs();
            }
            FileOutputStream output = new FileOutputStream(file);
            output.write(bytecode);
            output.close();
        }
        catch (Exception exception) {
            LunarUnlocker.logThrowable(exception);
        }
    }

    @Override
    public void serialize() {
    }

    @Override
    public Class getTargetClass() {
        return this.targetClass;
    }

    public ClassTransformer(Class targetClass) {
        if (!LunarUnlocker.INSTANCE.isForgeAbsent()) {
            LaunchClassLoader launchClassLoader = LaunchClassLoader.getLaunchClassLoader();
            if (launchClassLoader.supportsLegacyClassCache()) {
                launchClassLoader.cachedClasses().put(targetClass.getName(), targetClass);
            }
        }
        this.targetClass = targetClass;
    }

    public static int getTransformerState() {
        return transformerState;
    }

    private static Exception preserveException(Exception error) {
        return error;
    }

    @Override
    public boolean isApplied() {
        return this.applied;
    }

    @Override
    public int commit() {
        if (this.commitBlocked) {
            return -3;
        }
        this.serializeClassNode();
        int result = ClassBytecodeCache.setClassBytecode(this.targetClass, this.transformedBytecode);
        if (result == 0) {
            this.applied = true;
        }
        return result;
    }

    public void injectEventAtExit(MappingMethod mappingMethod, Class eventClass, ITramsformNode ... transformNodes) {
        EventBuilder eventBuilder = new EventBuilder(-1, eventClass, this.classNode,
                new MethodInfo(mappingMethod.getResolvedName(), mappingMethod.getDescriptor()), mappingMethod.isStaticMethod(), transformNodes);
        eventBuilder.buildEventDispatchInstructions();
        eventBuilder.inject();
    }

    @Override
    public void prepare() {
        this.originalBytecode = ClassBytecodeCache.getClassBytecode(this.targetClass, true);
        this.transformedBytecode = new byte[this.originalBytecode.length];
        System.arraycopy(this.originalBytecode, 0, this.transformedBytecode, 0, this.originalBytecode.length);
        ClassReader classReader = new ClassReader(this.originalBytecode);
        this.classNode = new ClassNode();
        classReader.accept(this.classNode, 0);
    }

    static {
        if (ClassTransformer.opaquePredicate() == 0) {
            ClassTransformer.setTransformerState(92);
        }
    }

    @Override
    public void rollback() {
        ClassBytecodeCache.setClassBytecode(this.targetClass, this.originalBytecode);
    }

    public void serializeClassNode() {
        int writerFlags = 3;
        if (ForgeVersion.MC_1_12_2.d()) {
            writerFlags = 2;
        }
        TransformClassWriter classWriter = new TransformClassWriter(writerFlags);
        this.classNode.accept(classWriter);
        this.transformedBytecode = classWriter.toByteArray();
    }
}
