package gg.lunarunlocker.runtime.model;

import gg.lunarunlocker.runtime.model.DetachedStringTreeEntry;
import gg.lunarunlocker.runtime.model.DetachedStringTreeNode;
import java.util.List;

class DetachedStringTreeListEntry
extends DetachedStringTreeEntry {
    final DetachedStringTreeNode listOwnerNode;
    List<String> values;

    DetachedStringTreeListEntry(DetachedStringTreeNode ownerNode) {
        super(ownerNode);
        this.listOwnerNode = ownerNode;
    }
}
