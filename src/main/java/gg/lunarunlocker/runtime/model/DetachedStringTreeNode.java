package gg.lunarunlocker.runtime.model;

import gg.lunarunlocker.runtime.model.DetachedStringTree;
import gg.lunarunlocker.runtime.model.DetachedStringTreeEntry;
import java.util.List;

class DetachedStringTreeNode {
    List<DetachedStringTreeEntry> entries;
    final DetachedStringTree ownerTree;
    String primaryValue;
    String secondaryValue;

    DetachedStringTreeNode(DetachedStringTree ownerTree) {
        this.ownerTree = ownerTree;
    }
}
