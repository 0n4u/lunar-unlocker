package gg.lunarunlocker.ui.click.component;


public final class GuiComponent {
    private static GuiComponent[] legacyComponentState;

    private GuiComponent() {
    }

    public static GuiComponent[] getLegacyComponentState() {
        return legacyComponentState;
    }

    public static void setLegacyComponentState(GuiComponent[] state) {
        legacyComponentState = state;
    }
}
