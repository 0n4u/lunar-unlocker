package gg.lunarunlocker.utils.render;


public final class RenderBatchManager {
    private static final RenderBatchManager INSTANCE = new RenderBatchManager();

    private RenderBatchManager() {
    }

    public static RenderBatchManager getInstance() {
        return INSTANCE;
    }

    public void refreshTargetFramebuffer() {
        
    }
}
